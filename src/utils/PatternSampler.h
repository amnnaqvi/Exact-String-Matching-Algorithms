#pragma once

#include <string>
#include <vector>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <iostream>

// Extracts patterns that actually exist in the corpus text.
// All returned patterns are guaranteed to be unique substrings,
// which prevents the same pattern from inflating comparison counts.

class PatternSampler {
public:

    // seed: fixed value ensures the same patterns are drawn every run,
    // which is required for reproducible results across machines.
    explicit PatternSampler(unsigned int seed = 42) : rng_(seed) {}

    // Sample `count` distinct patterns of exactly `length` chars from the corpus.
    // Patterns containing null bytes, newlines, or carriage returns are skipped
    // (common in raw Gutenberg/Pizza files and not meaningful as search targets).
    // If `count` distinct clean patterns cannot be found after many tries,
    // returns however many were found and prints a warning.
    std::vector<std::string> sample(const std::string& text,
                                    size_t length,
                                    size_t count) {
        if (length == 0 || length > text.size())
            throw std::invalid_argument("Invalid pattern length for this corpus.");

        size_t max_start = text.size() - length;
        std::uniform_int_distribution<size_t> dist(0, max_start);

        std::unordered_set<std::string> seen;   // enforces uniqueness
        std::vector<std::string> patterns;
        patterns.reserve(count);

        size_t attempts     = 0;
        size_t max_attempts = count * 50;  // generous budget for large corpora

        while (patterns.size() < count && attempts < max_attempts) {
            ++attempts;
            size_t pos = dist(rng_);
            std::string p = text.substr(pos, length);

            // Skip dirty patterns
            bool clean = true;
            for (char c : p) {
                if (c == '\0' || c == '\n' || c == '\r') { clean = false; break; }
            }
            if (!clean) continue;

            // Skip duplicates
            if (seen.count(p)) continue;

            seen.insert(p);
            patterns.push_back(p);
        }

        if (patterns.size() < count) {
            std::cerr << "[PatternSampler] Warning: only found " << patterns.size()
                      << " distinct patterns of length " << length
                      << " (requested " << count << ")\n";
        }

        return patterns;
    }

    // Assign a rarity label based on the least-frequent character in the pattern.
    //
    // These thresholds are our experimental grouping choice (not from Garraoui 2025).
    // They are calibrated against standard English/Italian frequency distributions:
    //   rare   — least-frequent char has freq < 0.005  (e.g. 'x', 'z', 'q')
    //   medium — least-frequent char has freq < 0.040  (e.g. 'b', 'v', 'k')
    //   common — everything else                       (e.g. 'e', 'a', 't')
    // Mention this explicitly in the report when describing the rarity experiment.
    static std::string rarity_label(const std::string& pattern,
                                    const std::unordered_map<char, double>& freq_table) {
        double min_freq = 1.0;
        for (char c : pattern) {
            auto it = freq_table.find(c);
            double f = (it != freq_table.end()) ? it->second : 0.001;
            if (f < min_freq) min_freq = f;
        }

        if (min_freq < 0.005) return "rare";
        if (min_freq < 0.040) return "medium";
        return "common";
    }

private:
    std::mt19937 rng_;
};
