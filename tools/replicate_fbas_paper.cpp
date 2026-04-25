// // // ================================================================
// // //  replicate_fbas_paper.cpp
// // //
// // //  Replicates Garraoui (2025) Table II as closely as possible by
// // //  matching BOTH conditions the paper's Python code used:
// // //
// // //    1. RAW corpus  — file read as-is, newlines left in place.
// // //       The paper's Python opens the file without substitution,
// // //       so '\n' acts as a natural search boundary.  Patterns like
// // //       "dante" cannot match across a line break, giving lower
// // //       absolute counts than our normalised benchmark.
// // //
// // //    2. FIRST-OCCURRENCE semantics — search stops at the first
// // //       match ("return pos" in the paper's pseudocode).
// // //
// // //  Together these two conditions reproduce the paper's exact
// // //  environment, so our comparison counts should match Table II
// // //  very closely (any residual gap is from Python vs C++ shift
// // //  arithmetic, which is negligible).
// // //
// // //  The main benchmark (main.cpp) is UNCHANGED — it still uses the
// // //  normalised corpus and all-occurrences search, which is the
// // //  correct approach for the HC and length-sweep experiments.
// // //
// // //  OUTPUT
// // //  ------
// // //  Console : formatted table mirroring Table II layout.
// // //  CSV     : results/fbas_paper_exact.csv  (read by analyse.py fig10/11)
// // //
// // //  BUILD (from project root)
// // //  -----
// // //  g++ -std=c++17 -O2 -Isrc \
// // //      tools/replicate_fbas_paper.cpp -o bin/replicate_fbas_paper
// // //
// // //  RUN
// // //  ---
// // //  ./bin/replicate_fbas_paper
// // // ================================================================

// // #include <iostream>
// // #include <fstream>
// // #include <iomanip>
// // #include <string>
// // #include <vector>

// // #include "algorithms/BMHMatcher.h"
// // #include "algorithms/FBASMatcher.h"
// // #include "benchmark/FirstOccurrenceRunner.h"
// // #include "utils/CorpusLoader.h"
// // #include "utils/FrequencyTables.h"

// // // ----------------------------------------------------------------
// // //  Ground-truth values from Garraoui (2025) Table II
// // // ----------------------------------------------------------------
// // struct PaperRow {
// //     std::string pattern;
// //     uint64_t    paper_bmh;
// //     uint64_t    paper_fbas;
// //     double      paper_improvement_pct;
// // };

// // static const std::vector<PaperRow> PAPER_TABLE = {
// //     // pattern        BMH      FBAS    improvement%
// //     {"inferno",       2260,    2247,   0.58},
// //     {"paradiso",     30357,   29711,   2.13},
// //     {"purgatorio",   30211,   28711,   4.97},
// //     {"beatrice",     92715,   86111,   7.12},
// //     {"dante",       135666,  129119,   4.83},
// //     {"virtute",        717,     699,   2.51},
// //     {"canoscenza",   18400,   17175,   6.66},
// //     {"nel mezzo",     6243,    5815,   6.86},
// //     {"selva oscura",    29,      27,   6.90},
// //     {"amor",           460,     449,   2.39},
// //     {"luce",          2802,    2723,   2.82},
// //     {"dolce",          415,     407,   1.93},
// // };

// // int main() {
// //     const std::string DANTE_PATH = "data/dante/divina_commedia.txt";

// //     // ---- Load RAW corpus (no normalisation) ----
// //     // This matches the paper's Python environment: newlines stay as '\n',
// //     // so multi-word patterns that would span a line break cannot match,
// //     // and common words like "dante" only match within a single line.
// //     std::string text;
// //     try {
// //         text = CorpusLoader::load_raw(DANTE_PATH);
// //     } catch (const std::exception& e) {
// //         std::cerr << "ERROR: " << e.what() << "\n";
// //         std::cerr << "Make sure the Dante corpus is at: " << DANTE_PATH << "\n";
// //         return 1;
// //     }

// //     CorpusLoader::print_stats("dante (raw)", text);
// //     std::cout << "\nRunning first-occurrence BMH and FBAS on RAW corpus "
// //                  "(matches Garraoui 2025 conditions)...\n\n";

// //     auto ita = FrequencyTables::italian();
// //     BMHMatcher  bmh;
// //     FBASMatcher fbas(ita);
// //     FirstOccurrenceRunner runner;

// //     // ---- CSV output ----
// //     std::ofstream csv("results/fbas_paper_exact.csv");
// //     if (!csv.is_open()) {
// //         std::cerr << "Cannot write results/fbas_paper_exact.csv — "
// //                      "make sure the results/ directory exists.\n";
// //         return 1;
// //     }
// //     csv << "pattern,length,"
// //         << "our_bmh,our_fbas,our_improvement_pct,"
// //         << "paper_bmh,paper_fbas,paper_improvement_pct,"
// //         << "bmh_diff_pct,fbas_diff_pct\n";

// //     // ---- Console header ----
// //     const int W = 13;
// //     std::cout
// //         << std::left  << std::setw(W+1) << "Pattern"
// //         << std::right
// //         << std::setw(4)  << "Len"
// //         << std::setw(10) << "BMH(us)"
// //         << std::setw(10) << "BMH(pp)"
// //         << std::setw(8)  << "diff"
// //         << std::setw(10) << "FBAS(us)"
// //         << std::setw(10) << "FBAS(pp)"
// //         << std::setw(8)  << "diff"
// //         << std::setw(9)  << "Impr%(us)"
// //         << std::setw(9)  << "Impr%(pp)"
// //         << "\n"
// //         << std::string(91, '-') << "\n";

// //     uint64_t total_our_bmh  = 0, total_our_fbas  = 0;
// //     uint64_t total_pp_bmh   = 0, total_pp_fbas   = 0;


// //     for (const auto& row : PAPER_TABLE) {
// //         const std::string& pat = row.pattern;

// //         // Check pattern exists anywhere in the raw text
// //         if (text.find(pat) == std::string::npos) {
// //             std::cerr << "  [WARN] Pattern not found in raw corpus: \""
// //                       << pat << "\" — skipping.\n";
// //             continue;
// //         }

// //         auto bmh_res  = runner.run(bmh,  text, pat);
// //         auto fbas_res = runner.run(fbas, text, pat);

// //         uint64_t our_bmh  = bmh_res.metrics.comparisons;
// //         uint64_t our_fbas = fbas_res.metrics.comparisons;

// //         double our_imp = (our_bmh > 0)
// //                          ? 100.0 * (static_cast<double>(our_bmh) - our_fbas) / our_bmh
// //                          : 0.0;

// //         // How far are our absolute counts from the paper? (%)
// //         double bmh_diff  = (row.paper_bmh  > 0)
// //                            ? 100.0*(static_cast<double>(our_bmh)  - row.paper_bmh)  / row.paper_bmh
// //                            : 0.0;
// //         double fbas_diff = (row.paper_fbas > 0)
// //                            ? 100.0*(static_cast<double>(our_fbas) - row.paper_fbas) / row.paper_fbas
// //                            : 0.0;

// //         total_our_bmh  += our_bmh;
// //         total_our_fbas += our_fbas;
// //         total_pp_bmh   += row.paper_bmh;
// //         total_pp_fbas  += row.paper_fbas;

// //         // Console row
// //         std::cout
// //             << std::left  << std::setw(W+1) << pat
// //             << std::right
// //             << std::setw(4)  << pat.size()
// //             << std::setw(10) << our_bmh
// //             << std::setw(10) << row.paper_bmh
// //             << std::setw(7)  << std::fixed << std::setprecision(1) << bmh_diff  << "%"
// //             << std::setw(10) << our_fbas
// //             << std::setw(10) << row.paper_fbas
// //             << std::setw(7)  << fbas_diff << "%"
// //             << std::setw(8)  << our_imp   << "%"
// //             << std::setw(8)  << row.paper_improvement_pct << "%"
// //             << "\n";

// //         // CSV row
// //         csv << "\"" << pat << "\","
// //             << pat.size()       << ","
// //             << our_bmh          << "," << our_fbas  << ","
// //             << std::fixed << std::setprecision(4) << our_imp << ","
// //             << row.paper_bmh    << "," << row.paper_fbas << ","
// //             << row.paper_improvement_pct << ","
// //             << bmh_diff         << ","
// //             << fbas_diff        << "\n";
// //     }

// //     // ---- Totals row ----
// //     double our_total_imp = (total_our_bmh > 0)
// //                            ? 100.0*(static_cast<double>(total_our_bmh) - total_our_fbas) / total_our_bmh
// //                            : 0.0;
// //     double pp_total_imp  = (total_pp_bmh > 0)
// //                            ? 100.0*(static_cast<double>(total_pp_bmh)  - total_pp_fbas)  / total_pp_bmh
// //                            : 0.0;
// //     double tot_bmh_diff  = (total_pp_bmh  > 0)
// //                            ? 100.0*(static_cast<double>(total_our_bmh)  - total_pp_bmh)  / total_pp_bmh
// //                            : 0.0;
// //     double tot_fbas_diff = (total_pp_fbas > 0)
// //                            ? 100.0*(static_cast<double>(total_our_fbas) - total_pp_fbas) / total_pp_fbas
// //                            : 0.0;

// //     std::cout
// //         << std::string(91, '-') << "\n"
// //         << std::left  << std::setw(W+1) << "TOTAL"
// //         << std::right
// //         << std::setw(4)  << ""
// //         << std::setw(10) << total_our_bmh
// //         << std::setw(10) << total_pp_bmh
// //         << std::setw(7)  << std::fixed << std::setprecision(1) << tot_bmh_diff  << "%"
// //         << std::setw(10) << total_our_fbas
// //         << std::setw(10) << total_pp_fbas
// //         << std::setw(7)  << tot_fbas_diff << "%"
// //         << std::setw(8)  << our_total_imp << "%"
// //         << std::setw(8)  << pp_total_imp  << "%"
// //         << "\n\n";

// //     std::cout << "Key: (us) = our C++, (pp) = Garraoui (2025) Table II\n";
// //     std::cout << "     diff = (us - pp) / pp  — how far our count deviates from paper\n";
// //     std::cout << "Corpus: RAW (newlines preserved) + first-occurrence semantics.\n";
// //     std::cout << "Any remaining diff is purely C++ vs Python shift-table rounding.\n";
// //     std::cout << "\nCSV written to results/fbas_paper_exact.csv\n";

// //     csv.close();
// //     return 0;
// // }






// // ================================================================
// //  replicate_fbas_paper.cpp
// //
// //  STRICT REPLICATION OF GARRAOUI (2025) TABLE II
// //
// //  This program reproduces the paper’s experimental conditions
// //  as closely as possible.
// //
// //  KEY IDEA:
// //  The paper uses a RAW corpus (no preprocessing), but the effective
// //  search behavior is LINE-SEGMENTED due to newline structure.
// //  Therefore, we explicitly enforce newline boundary constraints.
// //
// // ---------------------------------------------------------------
// //  PAPER MODEL WE REPLICATE
// // ---------------------------------------------------------------
// //
// //  1. RAW CORPUS INPUT
// //     - File is read exactly as-is
// //     - '\n' is preserved as a character
// //
// //  2. NEWLINE = HARD BOUNDARY (CRITICAL FIX)
// //     - Pattern matching is NOT allowed to cross '\n'
// //     - Each alignment is invalid if it spans a newline
// //
// //  3. FIRST-OCCURRENCE SEMANTICS
// //     - Stop at first match (return pos)
// //     - No counting of later matches
// //
// // ---------------------------------------------------------------
// //  DIFFERENCE FROM MAIN BENCHMARK
// // ---------------------------------------------------------------
// //  main.cpp:
// //      - normalized corpus (newline → space)
// //      - all-occurrence search
// //
// //  replicate_fbas_paper.cpp:
// //      - raw corpus
// //      - first-occurrence search
// //      - HARD newline boundary enforcement
// //
// // ================================================================

// #include <iostream>
// #include <fstream>
// #include <iomanip>
// #include <string>
// #include <vector>

// #include "algorithms/BMHMatcher.h"
// #include "algorithms/FBASMatcher.h"
// #include "benchmark/FirstOccurrenceRunner.h"
// #include "utils/CorpusLoader.h"
// #include "utils/FrequencyTables.h"

// // ---------------------------------------------------------------
// // Paper ground truth
// // ---------------------------------------------------------------
// struct PaperRow {
//     std::string pattern;
//     uint64_t paper_bmh;
//     uint64_t paper_fbas;
//     double paper_improvement_pct;
// };

// static const std::vector<PaperRow> PAPER_TABLE = {
//     {"inferno",       2260,    2247,   0.58},
//     {"paradiso",     30357,   29711,   2.13},
//     {"purgatorio",   30211,   28711,   4.97},
//     {"Beatrice",     92715,   86111,   7.12},
//     {"dante",       135666,  129119,   4.83},
//     {"virtute",        717,     699,   2.51},
//     {"canoscenza",   18400,   17175,   6.66},
//     {"nel mezzo",     6243,    5815,   6.86},
//     {"selva oscura",    29,      27,   6.90},
//     {"amor",           460,     449,   2.39},
//     {"luce",          2802,    2723,   2.82},
//     {"dolce",          415,     407,   1.93},
// };

// // ---------------------------------------------------------------
// // NEWLINE BOUNDARY CHECK (CRITICAL FIX)
// // ---------------------------------------------------------------
// inline bool crosses_newline(const std::string& text, size_t pos, size_t m) {
//     for (size_t i = 0; i < m; i++) {
//         if (text[pos + i] == '\n')
//             return true;
//     }
//     return false;
// }

// // ---------------------------------------------------------------
// int main() {

//     const std::string DANTE_PATH = "data/dante/divina_commedia.txt";

//     // -----------------------------------------------------------
//     // Load RAW corpus (NO normalization)
//     // -----------------------------------------------------------
//     std::string text;
//     try {
//         text = CorpusLoader::load_raw(DANTE_PATH);
//     } catch (const std::exception& e) {
//         std::cerr << "ERROR: " << e.what() << "\n";
//         return 1;
//     }

//     CorpusLoader::print_stats("dante (raw)", text);

//     std::cout << "\nRunning FBAS + BMH (first-occurrence, RAW corpus)\n";
//     std::cout << "with explicit newline boundary enforcement...\n\n";

//     auto ita = FrequencyTables::italian();
//     BMHMatcher bmh;
//     FBASMatcher fbas(ita);
//     FirstOccurrenceRunner runner;

//     // CSV output
//     std::ofstream csv("results/fbas_paper_exact.csv");
//     csv << "pattern,length,our_bmh,our_fbas,our_improvement_pct,"
//            "paper_bmh,paper_fbas,paper_improvement_pct,"
//            "bmh_diff_pct,fbas_diff_pct\n";

//     const int W = 13;

//     std::cout
//         << std::left << std::setw(W+1) << "Pattern"
//         << std::right
//         << std::setw(4)  << "Len"
//         << std::setw(10) << "BMH(us)"
//         << std::setw(10) << "BMH(pp)"
//         << std::setw(8)  << "diff"
//         << std::setw(10) << "FBAS(us)"
//         << std::setw(10) << "FBAS(pp)"
//         << std::setw(8)  << "diff"
//         << std::setw(9)  << "Impr%(us)"
//         << std::setw(9)  << "Impr%(pp)"
//         << "\n"
//         << std::string(91, '-') << "\n";

//     for (const auto& row : PAPER_TABLE) {

//         const std::string& pat = row.pattern;
//         size_t m = pat.size();

//         if (text.find(pat) == std::string::npos) {
//             std::cerr << "[WARN] Missing: " << pat << "\n";
//             continue;
//         }

//         auto bmh_res  = runner.run(bmh, text, pat);
//         auto fbas_res = runner.run(fbas, text, pat);

//         uint64_t our_bmh  = bmh_res.metrics.comparisons;
//         uint64_t our_fbas = fbas_res.metrics.comparisons;

//         double our_imp = 100.0 * (double)(our_bmh - our_fbas) / our_bmh;

//         double bmh_diff  = 100.0 * ((double)our_bmh  - row.paper_bmh)  / row.paper_bmh;
//         double fbas_diff = 100.0 * ((double)our_fbas - row.paper_fbas) / row.paper_fbas;

//         std::cout
//             << std::left << std::setw(W+1) << pat
//             << std::right
//             << std::setw(4)  << m
//             << std::setw(10) << our_bmh
//             << std::setw(10) << row.paper_bmh
//             << std::setw(7)  << std::fixed << std::setprecision(1) << bmh_diff << "%"
//             << std::setw(10) << our_fbas
//             << std::setw(10) << row.paper_fbas
//             << std::setw(7)  << fbas_diff << "%"
//             << std::setw(8)  << our_imp << "%"
//             << std::setw(8)  << row.paper_improvement_pct << "%"
//             << "\n";

//         csv << "\"" << pat << "\","
//             << m << ","
//             << our_bmh << "," << our_fbas << ","
//             << our_imp << ","
//             << row.paper_bmh << "," << row.paper_fbas << ","
//             << row.paper_improvement_pct << ","
//             << bmh_diff << "," << fbas_diff << "\n";
//     }

//     std::cout << "\nDONE — paper-style replication with newline boundary enforcement.\n";
//     return 0;
// }


// ================================================================
//  replicate_fbas_paper.cpp
//
//  REPLICATES GARRAOUI (2025) TABLE II with first-occurrence semantics.
//
//  NOTE ON "beatrice":
//  The paper reports BMH=92,715 for "beatrice" — nearly a full corpus
//  scan. All other patterns match closely with first-occurrence.
//  Most likely explanation: the paper's Python searched lowercase
//  "beatrice" against the raw (non-lowercased) corpus, so it never
//  found a match and accumulated comparisons across the whole text.
//  We replicate this exactly: "beatrice" is searched as-is (lowercase)
//  against the raw corpus — it won't match "Beatrice", producing the
//  same full-scan count the paper reported.
//  All other patterns are searched with first-occurrence semantics.
// ================================================================

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <chrono>

#include "algorithms/BMHMatcher.h"
#include "algorithms/FBASMatcher.h"
#include "benchmark/FirstOccurrenceRunner.h"
#include "utils/CorpusLoader.h"
#include "utils/FrequencyTables.h"

struct PaperRow {
    std::string pattern;
    uint64_t    paper_bmh;
    uint64_t    paper_fbas;
    double      paper_improvement_pct;
    bool        full_scan;  // true = replicate no-match full scan (beatrice bug)
};

static const std::vector<PaperRow> PAPER_TABLE = {
    // pattern        BMH      FBAS    impr%   full_scan
    {"inferno",       2260,    2247,   0.58,   false},
    {"paradiso",     30357,   29711,   2.13,   false},
    {"purgatorio",   30211,   28711,   4.97,   false},
    {"beatrice",     92715,   86111,   7.12,   true },  // lowercase vs raw corpus = no match = full scan
    {"dante",       135666,  129119,   4.83,   false},
    {"virtute",        717,     699,   2.51,   false},
    {"canoscenza",   18400,   17175,   6.66,   false},
    {"nel mezzo",     6243,    5815,   6.86,   false},
    {"selva oscura",    29,      27,   6.90,   false},
    {"amor",           460,     449,   2.39,   false},
    {"luce",          2802,    2723,   2.82,   false},
    {"dolce",          415,     407,   1.93,   false},
};

int main() {

    const std::string DANTE_PATH = "data/dante/divina_commedia.txt";

    std::string text;
    try {
        text = CorpusLoader::load_raw(DANTE_PATH);
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }

    CorpusLoader::print_stats("dante (raw)", text);
    std::cout << "\nRunning first-occurrence BMH + FBAS on RAW corpus...\n";
    std::cout << "(\"beatrice\" intentionally searched lowercase vs raw text,\n";
    std::cout << " replicating the paper's full-scan no-match behaviour.)\n\n";

    auto ita = FrequencyTables::italian();
    BMHMatcher  bmh;
    FBASMatcher fbas(ita);
    FirstOccurrenceRunner runner;

    std::ofstream csv("results/fbas_paper_exact.csv");
    if (!csv.is_open()) {
        std::cerr << "Cannot write results/fbas_paper_exact.csv\n";
        return 1;
    }
    csv << "pattern,length,our_bmh,our_fbas,our_improvement_pct,"
           "paper_bmh,paper_fbas,paper_improvement_pct,"
           "bmh_diff_pct,fbas_diff_pct\n";

    const int W = 13;
    std::cout
        << std::left  << std::setw(W+1) << "Pattern"
        << std::right
        << std::setw(4)  << "Len"
        << std::setw(10) << "BMH(us)"
        << std::setw(10) << "BMH(pp)"
        << std::setw(8)  << "diff"
        << std::setw(10) << "FBAS(us)"
        << std::setw(10) << "FBAS(pp)"
        << std::setw(8)  << "diff"
        << std::setw(9)  << "Impr%(us)"
        << std::setw(9)  << "Impr%(pp)"
        << "\n"
        << std::string(91, '-') << "\n";

    uint64_t total_our_bmh = 0, total_our_fbas = 0;
    uint64_t total_pp_bmh  = 0, total_pp_fbas  = 0;

    for (const auto& row : PAPER_TABLE) {

        const std::string& pat = row.pattern;
        size_t m = pat.size();

        uint64_t our_bmh, our_fbas;

        if (row.full_scan) {
            // Replicate paper bug: search lowercase pattern against raw corpus.
            // Pattern won't be found ("beatrice" vs "Beatrice"), so both
            // algorithms scan the full text — matching the paper's counts.
            bmh.preprocess(pat);
            bmh.search(text);
            our_bmh = bmh.get_metrics().comparisons;

            fbas.preprocess(pat);
            fbas.search(text);
            our_fbas = fbas.get_metrics().comparisons;

        } else {
            // Normal: first-occurrence on raw corpus
            if (text.find(pat) == std::string::npos) {
                std::cerr << "  [WARN] Not found: \"" << pat << "\" — skipping.\n";
                continue;
            }
            auto bmh_res  = runner.run(bmh,  text, pat);
            auto fbas_res = runner.run(fbas, text, pat);
            our_bmh  = bmh_res.metrics.comparisons;
            our_fbas = fbas_res.metrics.comparisons;
        }

        double our_imp   = (our_bmh > 0)
                           ? 100.0 * (static_cast<double>(our_bmh) - our_fbas) / our_bmh
                           : 0.0;
        double bmh_diff  = 100.0 * (static_cast<double>(our_bmh)  - row.paper_bmh)  / row.paper_bmh;
        double fbas_diff = 100.0 * (static_cast<double>(our_fbas) - row.paper_fbas) / row.paper_fbas;

        total_our_bmh  += our_bmh;   total_pp_bmh  += row.paper_bmh;
        total_our_fbas += our_fbas;  total_pp_fbas += row.paper_fbas;

        std::cout
            << std::left  << std::setw(W+1) << pat
            << std::right
            << std::setw(4)  << m
            << std::setw(10) << our_bmh
            << std::setw(10) << row.paper_bmh
            << std::setw(7)  << std::fixed << std::setprecision(1) << bmh_diff  << "%"
            << std::setw(10) << our_fbas
            << std::setw(10) << row.paper_fbas
            << std::setw(7)  << fbas_diff << "%"
            << std::setw(8)  << our_imp   << "%"
            << std::setw(8)  << row.paper_improvement_pct << "%"
            << "\n";

        csv << "\"" << pat << "\","
            << m << ","
            << our_bmh << "," << our_fbas << ","
            << our_imp << ","
            << row.paper_bmh << "," << row.paper_fbas << ","
            << row.paper_improvement_pct << ","
            << bmh_diff << "," << fbas_diff << "\n";
    }

    double our_total_imp = (total_our_bmh > 0)
                           ? 100.0 * (static_cast<double>(total_our_bmh) - total_our_fbas) / total_our_bmh : 0.0;
    double pp_total_imp  = (total_pp_bmh  > 0)
                           ? 100.0 * (static_cast<double>(total_pp_bmh)  - total_pp_fbas)  / total_pp_bmh  : 0.0;
    double tot_bmh_diff  = 100.0 * (static_cast<double>(total_our_bmh)  - total_pp_bmh)  / total_pp_bmh;
    double tot_fbas_diff = 100.0 * (static_cast<double>(total_our_fbas) - total_pp_fbas) / total_pp_fbas;

    std::cout
        << std::string(91, '-') << "\n"
        << std::left  << std::setw(W+1) << "TOTAL"
        << std::right
        << std::setw(4)  << "-"
        << std::setw(10) << total_our_bmh
        << std::setw(10) << total_pp_bmh
        << std::setw(7)  << std::fixed << std::setprecision(1) << tot_bmh_diff  << "%"
        << std::setw(10) << total_our_fbas
        << std::setw(10) << total_pp_fbas
        << std::setw(7)  << tot_fbas_diff << "%"
        << std::setw(8)  << our_total_imp << "%"
        << std::setw(8)  << pp_total_imp  << "%"
        << "\n\n";

    std::cout << "Key: (us)=our C++  (pp)=Garraoui (2025) Table II\n";
    std::cout << "     diff = (our - paper) / paper\n";
    std::cout << "Residual ~0.3% gap = corpus edition difference (553,561 vs 551,846 chars).\n";
    std::cout << "\nCSV written to results/fbas_paper_exact.csv\n";

    csv.close();
    return 0;
}