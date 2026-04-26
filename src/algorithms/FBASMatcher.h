#pragma once

#include "BMHMatcher.h"
#include <cctype>
#include <unordered_map>
#include <string>

// Frequency-Based Anchor Selection (Garraoui 2025).
// FBAS keeps BMH's shift table, but checks the rarest pattern character first.
// If that anchor mismatches, the whole window is rejected after one comparison.

class FBASMatcher : public BMHMatcher {
public:

    explicit FBASMatcher(const std::unordered_map<char, double>& freq_table)
        : freq_table_(freq_table) {}

    void preprocess(const std::string& pattern) override {
        BMHMatcher::preprocess(pattern);

        double min_freq = 1e9;
        anchor_pos_ = 0;

        for (size_t idx = 0; idx < pattern_.size(); ++idx) {
            char c = static_cast<char>(
                std::tolower(static_cast<unsigned char>(pattern_[idx])));
            auto it = freq_table_.find(c);
            double freq = (it != freq_table_.end()) ? it->second : 1.0;

            if (freq < min_freq) {
                min_freq    = freq;
                anchor_pos_ = idx;
            }
        }
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
            size_t win_start       = i - m + 1;
            size_t anchor_text_pos = win_start + anchor_pos_;

            metrics_.comparisons++;
            if (text[anchor_text_pos] != pattern_[anchor_pos_]) {
                unsigned char last = static_cast<unsigned char>(text[i]);
                i += shift_table_[last];
                continue;
            }

            bool match = true;
            for (size_t j = 0; j < m; ++j) {
                if (j == anchor_pos_) continue;
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
