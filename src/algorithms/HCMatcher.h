#pragma once

#include "../algorithms/Matcher.h"
#include <vector>
#include <string>
#include <chrono>
#include <stdexcept>
#include <cstdint>
#include <algorithm>

// ================================================================
//  HCMatcher — Hash Chain (Palmer, Faro, Scafiti 2024)
//  SEA 2024, LIPIcs vol. 301, Article 24.
//
//  Core idea: hash short non-overlapping q-grams of the pattern into
//  an extended Bloom filter F, where ADJACENT q-grams are linked via
//  a secondary "link-hash" stored in the same word. During search,
//  a window is rejected with a single F lookup if its last q-gram
//  hashes to an empty word; otherwise we walk left checking links
//  until the whole window passes — then do one full verification.
//  This gives average skip distances larger than character methods.
//
//  Parameters (paper's best results for English text):
//    q     — q-gram length; 3 is good for short patterns (m <= 32)
//    alpha — log2 of the filter table width; 11 or 12 is optimal
//    w     — word width = 64 bits (hardcoded)
//
//  Comparison counting: ONLY character comparisons in the final
//  verification step are counted. Hash lookups are NOT counted.
//  This is consistent with BMH and FBAS (both count only the
//  character comparisons they make during window scanning),
//  making the three metrics directly comparable.
// ================================================================

class HCMatcher : public Matcher {
public:

    // q=3, alpha=11 matches paper's best for short patterns on English/Italian text.
    explicit HCMatcher(int q = 3, int alpha = 11)
        : q_(q), alpha_(alpha) {}

    // ----------------------------------------------------------------
    //  preprocess  — implements Figure 3 of Palmer et al. (2024)
    //
    //  Builds the extended Bloom filter F:
    //   1. For each q-gram chain (processed right-to-left), link
    //      adjacent q-gram hashes: F[h(right)] |= lambda(h(left))
    //   2. For the first q q-grams (those with nothing to their left),
    //      set F[h(u)] = 1 only if that word is still zero — this
    //      minimises false positives (Optimisation 3.1 in paper).
    //   3. Return Hv = hash of leftmost q-gram of the last chain
    //      processed — used in search to gate full verification
    //      (Optimisation 3.2 in paper).
    // ----------------------------------------------------------------
    void preprocess(const std::string& pattern) override {
        if (pattern.empty())
            throw std::invalid_argument("Pattern cannot be empty.");

        auto t_start = std::chrono::high_resolution_clock::now();

        pattern_     = pattern;
        pattern_len_ = pattern.size();
        int m        = static_cast<int>(pattern_len_);

        // Clamp q to pattern length so very short patterns still work.
        effective_q_ = std::min(q_, m);
        int q        = effective_q_;

        // ---- Derived constants ----
        s_      = alpha_ / q;                    // bit-shift per character
        f_mask_ = (1ULL << alpha_) - 1ULL;       // index mask
        size_t table_size = (1ULL << alpha_);
        F_.assign(table_size, 0ULL);

        // ---- Build q-gram chains (Figure 3, lines 5-13) ----
        // num_chains = min(m - q + 1, q)
        // Each chain i starts at the rightmost character at position (m - i).
        int num_chains = std::min(m - q + 1, q);
        Hv_ = 0ULL;

        for (int i = num_chains; i >= 1; --i) {
            // Rightmost q-gram of this chain: rightmost char at (m - i)
            uint64_t v = hash_qgram(pattern, m - i, q);

            // Walk left, linking adjacent q-gram pairs
            // j = rightmost char index of the NEXT (leftward) q-gram
            int j = m - i - q;
            while (j >= q - 1) {
                uint64_t v_right = v;               // hash of right q-gram
                v = hash_qgram(pattern, j, q);      // hash of left q-gram
                // Link: right word stores a bit encoding left hash
                F_[v_right] |= link_hash(v);
                j -= q;
            }

            // Hv = leftmost q-gram hash of the LAST chain (i == 1 after loop)
            Hv_ = v;
        }

        // ---- Mark first-q q-grams (Figure 3, lines 14-17, Opt 3.1) ----
        // These q-grams have no left neighbour in the pattern, so we set
        // F[h(u)] = 1 only if the slot is still empty (to keep bits sparse).
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

    // ----------------------------------------------------------------
    //  search  — implements Figure 5 of Palmer et al. (2024)
    //
    //  j tracks the rightmost character of the current window.
    //  Outer loop:
    //    - Look up F[h(text[j-q+1..j])]. If 0 => no match possible,
    //      advance by (m - q + 1).
    //    - Otherwise walk left checking that consecutive q-gram pairs
    //      are linked in F (inner while loop).
    //    - If the whole window passes (else branch): check Hv, then
    //      do a full character-by-character verification.
    //    - Always advance j by (m - q + 1) after processing a window.
    //
    //  Metric: only the characters compared in full verification
    //  are counted (consistent with BMH/FBAS counting policy).
    // ----------------------------------------------------------------
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

        int j = m - 1;   // rightmost index of current window

        while (j < n) {
            uint64_t v = hash_qgram(text, j, q);
            uint64_t z = F_[v];

            if (z != 0ULL) {
                // Possible factor match — walk left checking links
                int i = j - m + 2 * q;   // lower boundary for inner while

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
                    // Whole window passed filter — enter else branch
                    j = i - q;

                    // Optimisation 3.2: only verify if leftmost q-gram hash matches Hv
                    if (v == Hv_) {
                        int win_start = j - q + 1;

                        if (win_start >= 0 && win_start + m <= n) {
                            // Full character-by-character verification
                            // (THIS is what we count — consistent with BMH/FBAS)
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
        // Include parameters so different HC configs appear as separate
        // rows in the CSV (e.g. "HC_q3a11" vs "HC_q3a12")
        return "HC_q" + std::to_string(q_) + "a" + std::to_string(alpha_);
    }

    // Expose parameters for reporting
    int get_q()     const { return effective_q_; }
    int get_alpha() const { return alpha_; }

private:

    // ----------------------------------------------------------------
    //  hash_qgram  (paper equation 3, Hash function in Figure 3)
    //
    //  Hashes x[pos-q+1 .. pos] right-to-left:
    //    v = 0
    //    for k = pos downto pos-q+1:  v = (v << s) + x[k]
    //    return v & f_mask
    //
    //  'pos' is the rightmost character index of the q-gram.
    // ----------------------------------------------------------------
    inline uint64_t hash_qgram(const std::string& s, int pos, int q) const {
        uint64_t v = 0;
        for (int k = pos; k > pos - q; --k) {
            v = (v << s_) + static_cast<unsigned char>(s[k]);
        }
        return v & f_mask_;
    }

    // ----------------------------------------------------------------
    //  link_hash  (paper equation 5, LinkHash function)
    //
    //  lambda(v) = 2^(v mod 64)  — one bit set in a 64-bit word.
    // ----------------------------------------------------------------
    inline uint64_t link_hash(uint64_t v) const {
        return 1ULL << (v & 63ULL);
    }

    // ---- Configuration ----
    int q_;        // requested q-gram length
    int alpha_;    // log2 of filter table size

    // ---- Derived at preprocess time ----
    int      effective_q_;     // actual q used (clamped to pattern length)
    int      s_;               // bit-shift per character in hash
    uint64_t f_mask_;          // index mask = 2^alpha - 1
    uint64_t Hv_;              // leftmost q-gram hash of last chain (Opt 3.2)

    std::vector<uint64_t> F_;  // extended Bloom filter, 2^alpha 64-bit words
    size_t pattern_len_ = 0;
};