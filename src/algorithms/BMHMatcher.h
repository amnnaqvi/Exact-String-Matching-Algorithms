#pragma once

#include "../algorithms/Matcher.h"
#include <array>
#include <chrono>
#include <stdexcept>

// Boyer-Moore-Horspool (Horspool 1980).
// Baseline matcher: compare a window from right to left, then shift by the
// bad-character table built during preprocessing. FBAS reuses this table and
// changes only the order in which a candidate window is checked.

class BMHMatcher : public Matcher {
public:

    void preprocess(const std::string& pattern) override {
        if (pattern.empty())
            throw std::invalid_argument("Pattern cannot be empty.");

        auto t_start = std::chrono::high_resolution_clock::now();

        pattern_     = pattern;
        pattern_len_ = pattern.size();

        // Characters absent from the pattern shift by the full pattern length.
        shift_table_.fill(pattern_len_);

        // Characters inside the pattern shift to their distance from the
        // right edge; the final pattern character keeps the default shift.
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
        size_t i = m - 1;

        while (i < n) {
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
    std::array<size_t, 256> shift_table_;
    size_t pattern_len_ = 0;
};
