#pragma once

#include "../algorithms/Matcher.h"
#include <vector>
#include <string>
#include <chrono>
#include <stdexcept>
#include <cstdint>
#include <algorithm>

// Hash Chain (Palmer, Faro, Scafiti 2024).
// The matcher hashes q-grams into an extended Bloom-filter style table. A text
// window is usually rejected by q-gram checks before a full character
// verification is needed.
//
// Metric note: HC counts only the character comparisons made during final
// verification. Hash operations still affect runtime_ms, so runtime is the
// fairest direct comparison against BMH and FBAS.

class HCMatcher : public Matcher {
public:

    // q=3, alpha=11 is a strong default for short English/Italian patterns.
    explicit HCMatcher(int q = 3, int alpha = 11)
        : q_(q), alpha_(alpha) {}

    void preprocess(const std::string& pattern) override {
        if (pattern.empty())
            throw std::invalid_argument("Pattern cannot be empty.");

        auto t_start = std::chrono::high_resolution_clock::now();

        pattern_     = pattern;
        pattern_len_ = pattern.size();
        int m        = static_cast<int>(pattern_len_);

        // Clamp q to the pattern length so very short patterns still work.
        effective_q_ = std::min(q_, m);
        int q        = effective_q_;

        s_      = alpha_ / q;
        f_mask_ = (1ULL << alpha_) - 1ULL;
        size_t table_size = (1ULL << alpha_);
        F_.assign(table_size, 0ULL);

        int num_chains = std::min(m - q + 1, q);
        Hv_ = 0ULL;

        for (int i = num_chains; i >= 1; --i) {
            uint64_t v = hash_qgram(pattern, m - i, q);

            int j = m - i - q;
            while (j >= q - 1) {
                uint64_t v_right = v;
                v = hash_qgram(pattern, j, q);
                F_[v_right] |= link_hash(v);
                j -= q;
            }

            Hv_ = v;
        }

        // Mark the first q-grams only when their slot is empty. This keeps the
        // filter sparse and reduces false positives.
        for (int i = q - 1; i < std::min(2 * q - 1, m); ++i) {
            uint64_t v = hash_qgram(pattern, i, q);
            if (F_[v] == 0ULL) {
                F_[v] = 1ULL;
            }
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

        int n = static_cast<int>(text.size());
        int m = static_cast<int>(pattern_len_);
        int q = effective_q_;

        if (n < m) return positions;

        auto t_start = std::chrono::high_resolution_clock::now();

        int j = m - 1;

        while (j < n) {
            uint64_t v = hash_qgram(text, j, q);
            uint64_t z = F_[v];

            if (z != 0ULL) {
                int i = j - m + 2 * q;

                bool broke = false;
                while (j >= i) {
                    j -= q;
                    v = hash_qgram(text, j, q);
                    if ((z & link_hash(v)) == 0ULL) {
                        broke = true;
                        break;
                    }
                    z = F_[v];
                }

                if (!broke) {
                    j = i - q;

                    if (v == Hv_) {
                        int win_start = j - q + 1;

                        if (win_start >= 0 && win_start + m <= n) {
                            bool match = true;
                            for (int k = 0; k < m; ++k) {
                                metrics_.comparisons++;
                                if (text[win_start + k] != pattern_[k]) {
                                    match = false;
                                    break;
                                }
                            }
                            if (match) {
                                positions.push_back(static_cast<size_t>(win_start));
                                metrics_.matches_found++;
                            }
                        }
                    }
                }
            }

            j += m - q + 1;
        }

        auto t_end = std::chrono::high_resolution_clock::now();
        metrics_.search_ms =
            std::chrono::duration<double, std::milli>(t_end - t_start).count();

        return positions;
    }

    std::string name() const override {
        // Keep parameter settings visible in CSV rows and plot legends.
        return "HC_q" + std::to_string(q_) + "a" + std::to_string(alpha_);
    }

    int get_q()     const { return effective_q_; }
    int get_alpha() const { return alpha_; }

private:

    inline uint64_t hash_qgram(const std::string& s, int pos, int q) const {
        uint64_t v = 0;
        for (int k = pos; k > pos - q; --k) {
            v = (v << s_) + static_cast<unsigned char>(s[k]);
        }
        return v & f_mask_;
    }

    inline uint64_t link_hash(uint64_t v) const {
        return 1ULL << (v & 63ULL);
    }

    int q_;
    int alpha_;

    int      effective_q_ = 0;
    int      s_ = 0;
    uint64_t f_mask_ = 0;
    uint64_t Hv_ = 0;

    std::vector<uint64_t> F_;
    size_t pattern_len_ = 0;
};
