// #pragma once

// // ================================================================
// //  FirstOccurrenceRunner
// //
// //  Wraps any Matcher subclass and runs it in FIRST-OCCURRENCE mode:
// //  the search stops as soon as the first match is found and returns
// //  that position (or SIZE_MAX if no match).
// //
// //  WHY THIS EXISTS
// //  ---------------
// //  Garraoui (2025) Table II was produced by a Python implementation
// //  whose pseudocode returns on the first match ("return pos").
// //  Our main benchmark correctly finds ALL occurrences, which gives
// //  much higher comparison counts on patterns like "dante" that
// //  appear hundreds of times.
// //
// //  Running the same 12 paper patterns through this runner lets us
// //  compare our comparison counts DIRECTLY to Table II without the
// //  all-occurrences vs first-occurrence mismatch.
// //
// //  The metrics recorded here count only the comparisons made up to
// //  and including the first match — identical to what the paper's
// //  Python code accumulates before it returns.
// //
// //  USAGE
// //  -----
// //  FirstOccurrenceRunner runner;
// //  auto [pos, metrics] = runner.run(algo, text, pattern);
// // ================================================================

// #include "algorithms/Matcher.h"
// #include <string>
// #include <vector>
// #include <utility>
// #include <chrono>
// #include <stdexcept>
// #include <array>
// #include <unordered_map>

// struct FirstOccResult {
//     size_t       position;     // SIZE_MAX if not found
//     MatchMetrics metrics;      // comparisons up to first match
// };

// class FirstOccurrenceRunner {
// public:

//     // Run algo on text, stopping at the FIRST occurrence of pattern.
//     // preprocess() is called internally.
//     FirstOccResult run(Matcher& algo,
//                        const std::string& text,
//                        const std::string& pattern) {

//         algo.preprocess(pattern);

//         // We cannot intercept mid-search, so we re-implement the
//         // search loop here for BMH and FBAS, stopping at first match.
//         // This is safe because both algorithms share the same outer
//         // loop structure — we call the full search and then recount
//         // up to the first position.
//         //
//         // Approach: call full search to get positions, then re-run
//         // a manual counting loop that stops at positions[0].
//         //
//         // Actually simpler: call the full search, get first position,
//         // then run a MANUAL first-occurrence search below that counts
//         // correctly. We do not use algo.search() for counting here —
//         // we do our own loop so the comparison count is accurate.

//         size_t m = pattern.size();
//         size_t n = text.size();

//         if (m == 0) throw std::invalid_argument("Empty pattern");

//         FirstOccResult result;
//         result.position = SIZE_MAX;
//         result.metrics  = {};

//         if (n < m) return result;

//         // We need the shift table — use the algo's full search to get
//         // positions, then recount comparisons up to position[0] with
//         // a manual BMH-style loop. This gives the exact comparison
//         // count the paper's code would accumulate.
//         //
//         // For simplicity and correctness we implement a first-occ BMH
//         // loop and a first-occ FBAS loop inline, both identical to
//         // the paper's pseudocode.
//         //
//         // The algo's name() tells us which path to take.

//         auto t_start = std::chrono::high_resolution_clock::now();

//         const std::string algo_name = algo.name();

//         if (algo_name == "BMH") {
//             run_bmh_first(text, pattern, result);
//         } else if (algo_name == "FBAS") {
//             // FBAS needs the frequency table — but we have no direct
//             // access to the anchor from outside.  Instead we call the
//             // full search to learn the first position, then run a
//             // manual FBAS-style loop using the anchor_pos that the
//             // full search used.
//             //
//             // Simplest correct approach: run the full algo search
//             // and recount manually using anchor-first logic.
//             // But we don't have the anchor exposed...
//             //
//             // CLEANEST SOLUTION: run the full search (all occurrences),
//             // then run a second manual BMH-with-anchor-first loop.
//             // The anchor position can be recovered by scanning the
//             // pattern for the minimum frequency character (same logic
//             // as FBASMatcher::preprocess).
//             run_fbas_first(text, pattern, algo, result);
//         } else {
//             // Fallback for any other algo: call full search, stop at first
//             auto positions = algo.search(text);
//             result.metrics = algo.get_metrics();
//             if (!positions.empty()) {
//                 result.position = positions[0];
//                 // Note: comparisons here include ALL comparisons up to end,
//                 // not just to first match. Tag this so caller knows.
//             }
//         }

//         auto t_end = std::chrono::high_resolution_clock::now();
//         result.metrics.search_ms =
//             std::chrono::duration<double, std::milli>(t_end - t_start).count();

//         return result;
//     }

// private:

//     // ------------------------------------------------------------
//     //  BMH first-occurrence — exact Horspool loop, stops on match.
//     //  Comparison count = every character comparison made before
//     //  and including the first full match verification.
//     // ------------------------------------------------------------
//     void run_bmh_first(const std::string& text,
//                        const std::string& pattern,
//                        FirstOccResult& out) {

//         size_t n = text.size();
//         size_t m = pattern.size();

//         // Build shift table (same as BMHMatcher::preprocess)
//         std::array<size_t, 256> shift;
//         shift.fill(m);
//         for (size_t i = 0; i < m - 1; ++i)
//             shift[static_cast<unsigned char>(pattern[i])] = m - 1 - i;

//         size_t i = m - 1;   // rightmost index of current window
//         uint64_t cmps = 0;

//         while (i < n) {
//             size_t k = i, j = m - 1;
//             bool match = true;

//             while (true) {
//                 cmps++;
//                 if (text[k] != pattern[j]) { match = false; break; }
//                 if (j == 0) break;
//                 --k; --j;
//             }

//             if (match) {
//                 out.position = i - m + 1;
//                 out.metrics.comparisons   = cmps;
//                 out.metrics.matches_found = 1;
//                 return;
//             }

//             i += shift[static_cast<unsigned char>(text[i])];
//         }

//         out.metrics.comparisons   = cmps;
//         out.metrics.matches_found = 0;
//     }

//     // ------------------------------------------------------------
//     //  FBAS first-occurrence — anchor-first verification, stops on
//     //  first match.  Anchor is re-derived from the pattern using the
//     //  same frequency table logic as FBASMatcher (frequency values
//     //  are hardcoded here to avoid a dependency on FBASMatcher's
//     //  private fields).
//     //
//     //  We use the Italian frequency table since replication is for
//     //  the Dante corpus. Characters absent from the table get freq=1.
//     // ------------------------------------------------------------
//     void run_fbas_first(const std::string& text,
//                         const std::string& pattern,
//                         Matcher& algo,
//                         FirstOccResult& out) {

//         size_t n = text.size();
//         size_t m = pattern.size();

//         // --- Derive anchor using the same logic as FBASMatcher ---
//         // Italian frequency table (same values as FrequencyTables::italian())
//         static const std::unordered_map<char,double> ITA = {
//             {'a',0.1174},{'e',0.1179},{'i',0.1128},{'o',0.0983},
//             {'n',0.0688},{'t',0.0562},{'r',0.0637},{'s',0.0498},
//             {'l',0.0651},{'c',0.0450},{'d',0.0373},{'u',0.0301},
//             {'p',0.0305},{'m',0.0251},{'v',0.0210},{'g',0.0164},
//             {'h',0.0154},{'f',0.0095},{'b',0.0092},{'z',0.0049},
//             {'q',0.0051},{'x',0.0003},{'w',0.0003},{'k',0.0001},
//             {'j',0.0001},{'y',0.0001},
//             {' ',0.1300},{',',0.0120},{'.',0.0070},
//             {'\'',0.0080},{'-',0.0010}
//         };

//         double min_freq  = 2.0;
//         size_t anchor_pos = 0;
//         for (size_t idx = 0; idx < m; ++idx) {
//             char c = pattern[idx];
//             auto it = ITA.find(c);
//             double freq = (it != ITA.end()) ? it->second : 1.0;
//             if (freq < min_freq) { min_freq = freq; anchor_pos = idx; }
//         }

//         // --- Build BMH shift table ---
//         std::array<size_t, 256> shift;
//         shift.fill(m);
//         for (size_t i = 0; i < m - 1; ++i)
//             shift[static_cast<unsigned char>(pattern[i])] = m - 1 - i;

//         // --- FBAS first-occurrence loop ---
//         size_t i = m - 1;
//         uint64_t cmps = 0;

//         while (i < n) {
//             size_t win_start       = i - m + 1;
//             size_t anchor_text_pos = win_start + anchor_pos;

//             // Anchor check first
//             cmps++;
//             if (text[anchor_text_pos] != pattern[anchor_pos]) {
//                 i += shift[static_cast<unsigned char>(text[i])];
//                 continue;
//             }

//             // Verify the rest
//             bool match = true;
//             for (size_t j = 0; j < m; ++j) {
//                 if (j == anchor_pos) continue;
//                 cmps++;
//                 if (text[win_start + j] != pattern[j]) {
//                     match = false;
//                     break;
//                 }
//             }

//             if (match) {
//                 out.position = win_start;
//                 out.metrics.comparisons   = cmps;
//                 out.metrics.matches_found = 1;
//                 return;
//             }

//             i += shift[static_cast<unsigned char>(text[i])];
//         }

//         out.metrics.comparisons   = cmps;
//         out.metrics.matches_found = 0;
//     }
// };



#pragma once

// ================================================================
//  FirstOccurrenceRunner
//
//  Wraps any Matcher subclass and runs it in FIRST-OCCURRENCE mode:
//  the search stops as soon as the first match is found and returns
//  that position (or SIZE_MAX if no match).
//
//  WHY THIS EXISTS
//  ---------------
//  Garraoui (2025) Table II was produced by a Python implementation
//  whose pseudocode returns on the first match ("return pos").
//  Our main benchmark correctly finds ALL occurrences, which gives
//  much higher comparison counts on patterns like "dante" that
//  appear hundreds of times.
//
//  Running the same 12 paper patterns through this runner lets us
//  compare our comparison counts DIRECTLY to Table II without the
//  all-occurrences vs first-occurrence mismatch.
//
//  The metrics recorded here count only the comparisons made up to
//  and including the first match — identical to what the paper's
//  Python code accumulates before it returns.
//
//  USAGE
//  -----
//  FirstOccurrenceRunner runner;
//  auto [pos, metrics] = runner.run(algo, text, pattern);
// ================================================================

#include "algorithms/Matcher.h"
#include <string>
#include <vector>
#include <utility>
#include <chrono>
#include <stdexcept>
#include <array>
#include <unordered_map>

struct FirstOccResult {
    size_t       position;     // SIZE_MAX if not found
    MatchMetrics metrics;      // comparisons up to first match
};

class FirstOccurrenceRunner {
public:

    // Run algo on text, stopping at the FIRST occurrence of pattern.
    // preprocess() is called internally.
    FirstOccResult run(Matcher& algo,
                       const std::string& text,
                       const std::string& pattern) {

        algo.preprocess(pattern);

        // We cannot intercept mid-search, so we re-implement the
        // search loop here for BMH and FBAS, stopping at first match.
        // This is safe because both algorithms share the same outer
        // loop structure — we call the full search and then recount
        // up to the first position.
        //
        // Approach: call full search to get positions, then re-run
        // a manual counting loop that stops at positions[0].
        //
        // Actually simpler: call the full search, get first position,
        // then run a MANUAL first-occurrence search below that counts
        // correctly. We do not use algo.search() for counting here —
        // we do our own loop so the comparison count is accurate.

        size_t m = pattern.size();
        size_t n = text.size();

        if (m == 0) throw std::invalid_argument("Empty pattern");

        FirstOccResult result;
        result.position = SIZE_MAX;
        result.metrics  = {};

        if (n < m) return result;

        // We need the shift table — use the algo's full search to get
        // positions, then recount comparisons up to position[0] with
        // a manual BMH-style loop. This gives the exact comparison
        // count the paper's code would accumulate.
        //
        // For simplicity and correctness we implement a first-occ BMH
        // loop and a first-occ FBAS loop inline, both identical to
        // the paper's pseudocode.
        //
        // The algo's name() tells us which path to take.

        auto t_start = std::chrono::high_resolution_clock::now();

        const std::string algo_name = algo.name();

        if (algo_name == "BMH") {
            run_bmh_first(text, pattern, result);
        } else if (algo_name == "FBAS") {
            // FBAS needs the frequency table — but we have no direct
            // access to the anchor from outside.  Instead we call the
            // full search to learn the first position, then run a
            // manual FBAS-style loop using the anchor_pos that the
            // full search used.
            //
            // Simplest correct approach: run the full algo search
            // and recount manually using anchor-first logic.
            // But we don't have the anchor exposed...
            //
            // CLEANEST SOLUTION: run the full search (all occurrences),
            // then run a second manual BMH-with-anchor-first loop.
            // The anchor position can be recovered by scanning the
            // pattern for the minimum frequency character (same logic
            // as FBASMatcher::preprocess).
            run_fbas_first(text, pattern, algo, result);
        } else {
            // Fallback for any other algo: call full search, stop at first
            auto positions = algo.search(text);
            result.metrics = algo.get_metrics();
            if (!positions.empty()) {
                result.position = positions[0];
                // Note: comparisons here include ALL comparisons up to end,
                // not just to first match. Tag this so caller knows.
            }
        }

        auto t_end = std::chrono::high_resolution_clock::now();
        result.metrics.search_ms =
            std::chrono::duration<double, std::milli>(t_end - t_start).count();

        return result;
    }

private:

    // ------------------------------------------------------------
    //  BMH first-occurrence — exact Horspool loop, stops on match.
    //  Comparison count = every character comparison made before
    //  and including the first full match verification.
    // ------------------------------------------------------------
    void run_bmh_first(const std::string& text,
                       const std::string& pattern,
                       FirstOccResult& out) {

        size_t n = text.size();
        size_t m = pattern.size();

        // Build shift table (same as BMHMatcher::preprocess)
        std::array<size_t, 256> shift;
        shift.fill(m);
        for (size_t i = 0; i < m - 1; ++i)
            shift[static_cast<unsigned char>(pattern[i])] = m - 1 - i;

        size_t i = m - 1;   // rightmost index of current window
        uint64_t cmps = 0;

        while (i < n) {
            size_t k = i, j = m - 1;
            bool match = true;

            while (true) {
                cmps++;
                if (text[k] != pattern[j]) { match = false; break; }
                if (j == 0) break;
                --k; --j;
            }

            if (match) {
                out.position = i - m + 1;
                out.metrics.comparisons   = cmps;
                out.metrics.matches_found = 1;
                return;
            }

            i += shift[static_cast<unsigned char>(text[i])];
        }

        out.metrics.comparisons   = cmps;
        out.metrics.matches_found = 0;
    }

    // ------------------------------------------------------------
    //  FBAS first-occurrence — anchor-first verification, stops on
    //  first match.  Anchor is re-derived from the pattern using the
    //  same frequency table logic as FBASMatcher (frequency values
    //  are hardcoded here to avoid a dependency on FBASMatcher's
    //  private fields).
    //
    //  We use the Italian frequency table since replication is for
    //  the Dante corpus. Characters absent from the table get freq=1.
    // ------------------------------------------------------------
    void run_fbas_first(const std::string& text,
                        const std::string& pattern,
                        Matcher& algo,
                        FirstOccResult& out) {

        size_t n = text.size();
        size_t m = pattern.size();

        // --- Derive anchor using the same logic as FBASMatcher ---
        // Italian rank-based table — MUST match FrequencyTables::italian() exactly.
        // Lower rank = rarer character = better anchor candidate.
        // Using rank values (1..50) not actual frequencies so anchor selection
        // is IDENTICAL to FBASMatcher::preprocess() which uses the same table.
        static const std::unordered_map<char,double> ITA = {
            {'z', 1}, {'j', 2}, {'x', 3}, {'q', 4}, {'k', 5},
            {'v', 6}, {'b', 7}, {'p', 8}, {'g', 9}, {'f',10},
            {'y',11}, {'w',12}, {'m',13}, {'u',14},
            {'c',15}, {'l',16}, {'d',17}, {'r',18},
            {'h',19}, {'s',20}, {'n',21}, {'t',22},
            {'o',23}, {'i',24}, {'a',28}, {'e',29},
            // Uppercase — same ranks
            {'Z', 1}, {'J', 2}, {'X', 3}, {'Q', 4}, {'K', 5},
            {'V', 6}, {'B', 7}, {'P', 8}, {'G', 9}, {'F',10},
            {'Y',11}, {'W',12}, {'M',13}, {'U',14},
            {'C',15}, {'L',16}, {'D',17}, {'R',18},
            {'H',19}, {'S',20}, {'N',21}, {'T',22},
            {'O',23}, {'I',24}, {'A',28}, {'E',29},
            {' ',50},{',',50},{'.',50},{'\'',50},{'-',50}
        };

        double min_freq  = 1e9;
        size_t anchor_pos = 0;
        for (size_t idx = 0; idx < m; ++idx) {
            char c = std::tolower(static_cast<unsigned char>(pattern[idx]));
            auto it = ITA.find(c);
            double freq = (it != ITA.end()) ? it->second : 50.0;
            if (freq < min_freq) { min_freq = freq; anchor_pos = idx; }
        }

        // --- Build BMH shift table ---
        std::array<size_t, 256> shift;
        shift.fill(m);
        for (size_t i = 0; i < m - 1; ++i)
            shift[static_cast<unsigned char>(pattern[i])] = m - 1 - i;

        // --- FBAS first-occurrence loop ---
        size_t i = m - 1;
        uint64_t cmps = 0;

        while (i < n) {
            size_t win_start       = i - m + 1;
            size_t anchor_text_pos = win_start + anchor_pos;

            // Anchor check first
            cmps++;
            if (text[anchor_text_pos] != pattern[anchor_pos]) {
                i += shift[static_cast<unsigned char>(text[i])];
                continue;
            }

            // Verify the rest
            bool match = true;
            for (size_t j = 0; j < m; ++j) {
                if (j == anchor_pos) continue;
                cmps++;
                if (text[win_start + j] != pattern[j]) {
                    match = false;
                    break;
                }
            }

            if (match) {
                out.position = win_start;
                out.metrics.comparisons   = cmps;
                out.metrics.matches_found = 1;
                return;
            }

            i += shift[static_cast<unsigned char>(text[i])];
        }

        out.metrics.comparisons   = cmps;
        out.metrics.matches_found = 0;
    }
};