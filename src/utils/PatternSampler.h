#pragma once

#include <string>
#include <vector>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <iostream>

// ================================================================
//  PatternSampler
//
//  Extracts patterns that actually exist in the corpus text.
//  All returned patterns are guaranteed to be unique substrings.
//
//  PREREQUISITE: the corpus text must have been normalised by
//  CorpusLoader::load() so that newlines/nulls are replaced with
//  spaces. After normalisation, every position is a valid pattern
//  start — no position-skipping is needed here.
//
//  Sampling strategy:
//    1. Random sampling with a fixed seed (reproducible across machines).
//    2. If random sampling cannot fill the quota after a generous
//       budget, fall back to a systematic scan of the text to collect
//       all remaining distinct patterns — guaranteeing we always
//       return exactly `count` patterns when enough unique substrings
//       exist in the corpus.
// ================================================================

class PatternSampler {
public:

    explicit PatternSampler(unsigned int seed = 42) : rng_(seed) {}

    // Sample `count` distinct patterns of exactly `length` chars.
    // Returns fewer only if the corpus genuinely does not contain
    // `count` distinct substrings of that length.
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

        // ---- Phase 1: random sampling ----
        // Budget: 200x the requested count. For large corpora this is
        // always sufficient; for small corpora we fall back to Phase 2.
        size_t budget = count * 200;
        for (size_t attempt = 0; attempt < budget && patterns.size() < count; ++attempt) {
            size_t pos = dist(rng_);
            add_if_unique(text, pos, length, seen, patterns);
        }

        // ---- Phase 2: systematic scan fallback ----
        // Walk the text in steps of length/4 (shuffled) to cover
        // distinct regions without the collision penalty of pure random.
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

    // ----------------------------------------------------------------
    //  rarity_label
    //
    //  Assigns a bucket based on the LEAST-frequent character in the
    //  pattern according to the corpus language's frequency table.
    //
    //  Thresholds (our experimental choice, not from Garraoui 2025):
    //    rare   -- min char freq < 0.005  (e.g. 'x', 'z', 'q', 'j')
    //    medium -- min char freq < 0.040  (e.g. 'b', 'v', 'k', 'f')
    //    common -- everything else        (e.g. 'e', 'a', 't', ' ')
    //
    //  Space (' ') is treated as a common character (freq ~0.13) after
    //  corpus normalisation. Patterns spanning original line boundaries
    //  will contain spaces; this is fine and expected.
    // ----------------------------------------------------------------
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