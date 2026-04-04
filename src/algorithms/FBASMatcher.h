#pragma once

#include "BMHMatcher.h"
#include <unordered_map>
#include <string>

// ================================================================
//  FBASMatcher — FOR YOUR PARTNER TO COMPLETE
//
//  Frequency-Based Anchor Selection (Garraoui 2025).
//  Extends BMHMatcher. The shift table, timing, and search loop
//  structure are all inherited — do not rewrite them.
//
//  The ONLY two things that change vs BMH:
//    1. preprocess() additionally identifies the "anchor" —
//       the rarest pattern character per the language freq table.
//    2. search() checks the anchor position first at each window
//       instead of always starting from the right.
//
//  Everything else (shift table, comparison counter, metrics,
//  CSV output) is inherited. DO NOT rewrite any of that.
// ================================================================

class FBASMatcher : public BMHMatcher {
public:

    explicit FBASMatcher(const std::unordered_map<char, double>& freq_table)
        : freq_table_(freq_table) {}

    void preprocess(const std::string& pattern) override {
        // Step 1: Run full BMH preprocessing (builds shift table, stores pattern_).
        BMHMatcher::preprocess(pattern);

        // Step 2 (FBAS-specific): Find the anchor.
        // The anchor is the pattern character with the lowest frequency
        // in the language. Store its index in pattern as anchor_pos_.
        //
        // TODO (Partner): implement this.
        // Iterate over pattern_, look each char up in freq_table_.
        // If a char is missing from the table, treat its frequency as 1.0.
        // Set anchor_pos_ to the index of the minimum-frequency character.
        //
        // Example:
        //   pattern_ = "extra"
        //   freq_table_['e']=0.127, ['x']=0.003, ['t']=0.091, ...
        //   anchor_pos_ = 1   (index of 'x', the rarest)

        anchor_pos_ = 0;  // placeholder — replace with real logic
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
            // TODO (Partner): anchor-first check.
            //
            // The anchor character in the pattern is at index anchor_pos_.
            // At window position i (rightmost), the window starts at i - m + 1.
            // So the anchor aligns with text at:
            //   size_t anchor_text_pos = i - m + 1 + anchor_pos_;
            //
            // Check text[anchor_text_pos] vs pattern_[anchor_pos_] FIRST:
            //   - If they differ:  metrics_.comparisons++, shift, continue.
            //   - If they match:   metrics_.comparisons++, then verify the
            //                      full pattern (any order), counting each
            //                      comparison in metrics_.comparisons.
            //
            // Shift always uses: shift_table_[text[i]]  (same as BMH).

            // --- Placeholder: plain BMH loop (partner replaces this block) ---
            size_t k = i;
            size_t j = m - 1;
            bool match = true;
            while (true) {
                metrics_.comparisons++;
                if (text[k] != pattern_[j]) { match = false; break; }
                if (j == 0) break;
                --k; --j;
            }
            // -----------------------------------------------------------------

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

    std::string name() const override { return "FBAS"; }

private:
    const std::unordered_map<char, double>& freq_table_;
    size_t anchor_pos_ = 0;
};
