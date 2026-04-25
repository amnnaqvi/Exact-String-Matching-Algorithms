#pragma once

#include "BMHMatcher.h"
#include <unordered_map>
#include <string>

// ================================================================
//  FBASMatcher — Frequency-Based Anchor Selection (Garraoui 2025)
//
//  Extends BMHMatcher. Only two things differ from BMH:
//    1. preprocess() also finds the "anchor" — the rarest character
//       in the pattern according to the language frequency table.
//    2. search() checks the anchor position FIRST at each window,
//       so most non-matching windows are rejected with 1 comparison.
//
//  The shift table, timing infrastructure, metrics, and CSV output
//  are all inherited unchanged from BMHMatcher.
// ================================================================

class FBASMatcher : public BMHMatcher {
public:

    explicit FBASMatcher(const std::unordered_map<char, double>& freq_table)
        : freq_table_(freq_table) {}

    // ----------------------------------------------------------------
    //  preprocess
    //  Step 1: delegate to BMH (builds shift_table_, stores pattern_,
    //          records preprocess_ms).
    //  Step 2: scan pattern_ to find the anchor — the index of the
    //          character with the lowest frequency in freq_table_.
    //          Characters absent from the table are treated as
    //          frequency 1.0 (very common, poor anchor choice).
    // ----------------------------------------------------------------
    void preprocess(const std::string& pattern) override {
        // Step 1 — full BMH preprocessing (shift table + timing).
        BMHMatcher::preprocess(pattern);

        // Step 2 — anchor selection (FBAS-specific).
        double min_freq = 1e9;   // sentinel: higher than any real freq
        anchor_pos_ = 0;

        for (size_t idx = 0; idx < pattern_.size(); ++idx) {
            char c = std::tolower(pattern_[idx]);
            auto it = freq_table_.find(c);
            double freq = (it != freq_table_.end()) ? it->second : 50.0;

            if (freq < min_freq) {
                min_freq    = freq;
                anchor_pos_ = idx;
            }
        }
    }

    // ----------------------------------------------------------------
    //  search
    //  Same outer loop and shift logic as BMH.
    //  The ONLY difference: at each window we check the anchor
    //  character FIRST before verifying the rest of the pattern.
    //
    //  Window layout (i = rightmost index of current window):
    //    window start     = i - m + 1
    //    anchor in text   = i - m + 1 + anchor_pos_
    //    shift character  = text[i]   (identical to BMH — unchanged)
    // ----------------------------------------------------------------
    std::vector<size_t> search(const std::string& text) override {
        if (pattern_.empty())
            throw std::logic_error("Call preprocess() before search().");

        metrics_.comparisons   = 0;
        metrics_.matches_found = 0;

        std::vector<size_t> positions;

        if (text.size() < pattern_len_)
            return positions;

        auto t_start = std::chrono::high_resolution_clock::now();

        size_t n           = text.size();
        size_t m           = pattern_len_;
        size_t i           = m - 1;   // rightmost index of current window

        while (i < n) {
            size_t win_start       = i - m + 1;
            size_t anchor_text_pos = win_start + anchor_pos_;

            // --- Anchor-first check (the FBAS optimisation) ---
            metrics_.comparisons++;
            if (text[anchor_text_pos] != pattern_[anchor_pos_]) {
                // Mismatch at the rarest character: reject immediately.
                // One comparison used; shift and move on.
                unsigned char last = static_cast<unsigned char>(text[i]);
                i += shift_table_[last];
                continue;
            }

            // Anchor matched — verify the rest of the pattern.
            // Check every position except anchor_pos_ (already verified).
            bool match = true;
            for (size_t j = 0; j < m; ++j) {
                if (j == anchor_pos_) continue;   // already checked
                metrics_.comparisons++;
                if (text[win_start + j] != pattern_[j]) {
                    match = false;
                    break;
                }
            }

            if (match) {
                positions.push_back(win_start);
                metrics_.matches_found++;
            }

            // Shift: always driven by the rightmost character (same as BMH).
            unsigned char last = static_cast<unsigned char>(text[i]);
            i += shift_table_[last];
        }

        auto t_end = std::chrono::high_resolution_clock::now();
        metrics_.search_ms =
            std::chrono::duration<double, std::milli>(t_end - t_start).count();

        return positions;
    }

    std::string name() const override { return "FBAS"; }

private:
    const std::unordered_map<char, double>& freq_table_;
    size_t anchor_pos_ = 0;
};