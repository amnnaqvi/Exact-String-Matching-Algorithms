#pragma once

#include "algorithms/Matcher.h"
#include "utils/FrequencyTables.h"
#include <array>
#include <chrono>
#include <cctype>
#include <stdexcept>
#include <string>
#include <unordered_map>

struct FirstOccResult {
    size_t       position;     // SIZE_MAX if not found
    MatchMetrics metrics;      // comparisons up to first match
};

class FirstOccurrenceRunner {
public:
    FirstOccResult run(Matcher& algo,
                       const std::string& text,
                       const std::string& pattern) {
        if (pattern.empty())
            throw std::invalid_argument("Empty pattern");

        algo.preprocess(pattern);

        FirstOccResult result;
        result.position = SIZE_MAX;
        result.metrics = {};

        if (text.size() < pattern.size())
            return result;

        auto t_start = std::chrono::high_resolution_clock::now();

        const std::string algo_name = algo.name();
        if (algo_name == "BMH") {
            run_bmh_first(text, pattern, result);
        } else if (algo_name == "FBAS") {
            run_fbas_first(text, pattern, FrequencyTables::italian(), result);
        } else {
            auto positions = algo.search(text);
            result.metrics = algo.get_metrics();
            if (!positions.empty())
                result.position = positions[0];
        }

        auto t_end = std::chrono::high_resolution_clock::now();
        result.metrics.search_ms =
            std::chrono::duration<double, std::milli>(t_end - t_start).count();

        return result;
    }

private:
    static std::array<size_t, 256> build_shift_table(const std::string& pattern) {
        std::array<size_t, 256> shift;
        const size_t m = pattern.size();
        shift.fill(m);
        for (size_t i = 0; i < m - 1; ++i)
            shift[static_cast<unsigned char>(pattern[i])] = m - 1 - i;
        return shift;
    }

    static void run_bmh_first(const std::string& text,
                              const std::string& pattern,
                              FirstOccResult& out) {
        const size_t n = text.size();
        const size_t m = pattern.size();
        const auto shift = build_shift_table(pattern);

        size_t i = m - 1;
        uint64_t comparisons = 0;

        while (i < n) {
            size_t k = i;
            size_t j = m - 1;
            bool match = true;

            while (true) {
                ++comparisons;
                if (text[k] != pattern[j]) {
                    match = false;
                    break;
                }
                if (j == 0) break;
                --k;
                --j;
            }

            if (match) {
                out.position = i - m + 1;
                out.metrics.comparisons = comparisons;
                out.metrics.matches_found = 1;
                return;
            }

            i += shift[static_cast<unsigned char>(text[i])];
        }

        out.metrics.comparisons = comparisons;
        out.metrics.matches_found = 0;
    }

    static size_t choose_anchor(
        const std::string& pattern,
        const std::unordered_map<char, double>& freq_table) {

        double min_freq = 1e9;
        size_t anchor_pos = 0;

        for (size_t idx = 0; idx < pattern.size(); ++idx) {
            char c = static_cast<char>(
                std::tolower(static_cast<unsigned char>(pattern[idx])));
            auto it = freq_table.find(c);
            double freq = (it != freq_table.end()) ? it->second : 1.0;
            if (freq < min_freq) {
                min_freq = freq;
                anchor_pos = idx;
            }
        }

        return anchor_pos;
    }

    static void run_fbas_first(
        const std::string& text,
        const std::string& pattern,
        const std::unordered_map<char, double>& freq_table,
        FirstOccResult& out) {

        const size_t n = text.size();
        const size_t m = pattern.size();
        const auto shift = build_shift_table(pattern);
        const size_t anchor_pos = choose_anchor(pattern, freq_table);

        size_t i = m - 1;
        uint64_t comparisons = 0;

        while (i < n) {
            size_t win_start = i - m + 1;
            size_t anchor_text_pos = win_start + anchor_pos;

            ++comparisons;
            if (text[anchor_text_pos] != pattern[anchor_pos]) {
                i += shift[static_cast<unsigned char>(text[i])];
                continue;
            }

            bool match = true;
            for (size_t j = 0; j < m; ++j) {
                if (j == anchor_pos) continue;
                ++comparisons;
                if (text[win_start + j] != pattern[j]) {
                    match = false;
                    break;
                }
            }

            if (match) {
                out.position = win_start;
                out.metrics.comparisons = comparisons;
                out.metrics.matches_found = 1;
                return;
            }

            i += shift[static_cast<unsigned char>(text[i])];
        }

        out.metrics.comparisons = comparisons;
        out.metrics.matches_found = 0;
    }
};
