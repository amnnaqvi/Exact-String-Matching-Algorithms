#pragma once

#include "Matcher.h"
#include <array>
#include <chrono>
#include <stdexcept>

// Boyer-Moore-Horspool (Horspool 1980, Algorithm SBM in the paper).
//
// Core idea: align pattern against text, inspect only the rightmost
// character of the current window. On mismatch, use a precomputed
// shift table to jump forward — skipping many characters at once.
//
// This is the baseline algorithm and the base class for FBAS.
// FBAS must extend this class, not rewrite it.

class BMHMatcher : public Matcher {
public:

    void preprocess(const std::string& pattern) override {
        if (pattern.empty())
            throw std::invalid_argument("Pattern cannot be empty.");

        auto t_start = std::chrono::high_resolution_clock::now();

        pattern_     = pattern;   // stored in base class for search() to use
        pattern_len_ = pattern.size();

        // Default: if a text character doesn't appear in the pattern,
        // shift by the full pattern length.
        shift_table_.fill(pattern_len_);

        // For characters that appear in the pattern (except the last position),
        // store how far they are from the right edge of the pattern.
        // This is Horspool's bad-character shift (SBM formulation).
        for (size_t i = 0; i < pattern_len_ - 1; ++i) {
            unsigned char c = static_cast<unsigned char>(pattern[i]);
            shift_table_[c] = pattern_len_ - 1 - i;
        }

        auto t_end = std::chrono::high_resolution_clock::now();
        metrics_.preprocess_ms =
            std::chrono::duration<double, std::milli>(t_end - t_start).count();
    }

    std::vector<size_t> search(const std::string& text) override {
        if (pattern_.empty())
            throw std::logic_error("Call preprocess() before search().");

        metrics_.comparisons   = 0;
        metrics_.matches_found = 0;

        std::vector<size_t> positions;

        if (text.size() < pattern_len_)
            return positions;

        auto t_start = std::chrono::high_resolution_clock::now();

        size_t n = text.size();
        size_t m = pattern_len_;
        size_t i = m - 1;   // rightmost index of the current window

        while (i < n) {
            // Walk the window right-to-left, counting every comparison.
            size_t k = i;
            size_t j = m - 1;
            bool match = true;

            while (true) {
                metrics_.comparisons++;
                if (text[k] != pattern_[j]) { match = false; break; }
                if (j == 0) break;
                --k;
                --j;
            }

            if (match) {
                positions.push_back(i - m + 1);
                metrics_.matches_found++;
            }

            // Shift using the rightmost character of the current window.
            unsigned char last = static_cast<unsigned char>(text[i]);
            i += shift_table_[last];
        }

        auto t_end = std::chrono::high_resolution_clock::now();
        metrics_.search_ms =
            std::chrono::duration<double, std::milli>(t_end - t_start).count();

        return positions;
    }

    std::string name() const override { return "BMH"; }

protected:
    // Protected so FBAS can reuse the same table without rebuilding it.
    std::array<size_t, 256> shift_table_;
    size_t pattern_len_ = 0;
};
