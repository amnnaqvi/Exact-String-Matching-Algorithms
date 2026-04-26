#pragma once

#include <string>
#include <vector>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <iostream>

// Selects reproducible patterns that actually occur in a corpus. The random
// pass gives varied samples; the systematic fallback handles small or repetitive
// corpora without silently returning duplicate patterns.

class PatternSampler {
public:

    explicit PatternSampler(unsigned int seed = 42) : rng_(seed) {}

    // Sample `count` distinct patterns of exactly `length` characters.
    // Returns fewer only if the corpus genuinely does not contain enough
    // distinct substrings of that length.
    std::vector<std::string> sample(const std::string& text,
                                    size_t length,
                                    size_t count) {
        if (length == 0 || length > text.size())
            throw std::invalid_argument(
                "Pattern length " + std::to_string(length) +
                " invalid for corpus of size " + std::to_string(text.size()));

        size_t max_start = text.size() - length;
        std::uniform_int_distribution<size_t> dist(0, max_start);

        std::unordered_set<std::string> seen;
        std::vector<std::string> patterns;
        patterns.reserve(count);

        // Phase 1: random sampling with a generous retry budget.
        size_t budget = count * 200;
        for (size_t attempt = 0; attempt < budget && patterns.size() < count; ++attempt) {
            size_t pos = dist(rng_);
            add_if_unique(text, pos, length, seen, patterns);
        }

        // Phase 2: scan shuffled start positions if random sampling collides.
        if (patterns.size() < count) {
            size_t step = std::max<size_t>(1, length / 4);
            std::vector<size_t> starts;
            starts.reserve(max_start / step + 1);
            for (size_t i = 0; i <= max_start; i += step)
                starts.push_back(i);
            std::shuffle(starts.begin(), starts.end(), rng_);

            for (size_t pos : starts) {
                if (patterns.size() >= count) break;
                add_if_unique(text, pos, length, seen, patterns);
            }
        }

        if (patterns.size() < count) {
            std::cerr << "[PatternSampler] Warning: corpus only contains "
                      << patterns.size() << " distinct substrings of length "
                      << length << " (requested " << count << ").\n";
        }

        return patterns;
    }

    // Bucket by the least-frequent character in the pattern. These thresholds
    // are an experimental grouping for analysis, not a claim from the papers.
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

    inline void add_if_unique(const std::string& text,
                               size_t pos,
                               size_t length,
                               std::unordered_set<std::string>& seen,
                               std::vector<std::string>& out) {
        std::string p = text.substr(pos, length);
        if (!seen.count(p)) {
            seen.insert(p);
            out.push_back(std::move(p));
        }
    }

    std::mt19937 rng_;
};
