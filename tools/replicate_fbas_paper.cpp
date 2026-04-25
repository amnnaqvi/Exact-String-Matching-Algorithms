#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

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
    bool        full_scan;
};

static const std::vector<PaperRow> PAPER_TABLE = {
    {"inferno",       2260,    2247,   0.58, false},
    {"paradiso",     30357,   29711,   2.13, false},
    {"purgatorio",   30211,   28711,   4.97, false},
    {"beatrice",     92715,   86111,   7.12, true },
    {"dante",       135666,  129119,   4.83, false},
    {"virtute",        717,     699,   2.51, false},
    {"canoscenza",   18400,   17175,   6.66, false},
    {"nel mezzo",     6243,    5815,   6.86, false},
    {"selva oscura",    29,      27,   6.90, false},
    {"amor",           460,     449,   2.39, false},
    {"luce",          2802,    2723,   2.82, false},
    {"dolce",          415,     407,   1.93, false},
};

static double pct_diff(uint64_t ours, uint64_t paper) {
    return paper == 0 ? 0.0
                      : 100.0 * (static_cast<double>(ours) - paper) / paper;
}

int main() {
    const std::string dante_path = "data/dante/divina_commedia.txt";

    std::string text;
    try {
        text = CorpusLoader::load_raw(dante_path);
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }

    CorpusLoader::print_stats("dante (raw)", text);
    std::cout << "\nRunning first-occurrence BMH + FBAS on raw Dante text...\n";
    std::cout << "The lowercase beatrice row intentionally uses full-scan/no-match\n";
    std::cout << "semantics, matching the paper's reported full-corpus count.\n\n";

    auto italian = FrequencyTables::italian();
    BMHMatcher bmh;
    FBASMatcher fbas(italian);
    FirstOccurrenceRunner runner;

    std::ofstream csv("results/fbas_paper_exact.csv");
    if (!csv.is_open()) {
        std::cerr << "Cannot write results/fbas_paper_exact.csv\n";
        return 1;
    }

    csv << "pattern,length,our_bmh,our_fbas,our_improvement_pct,"
           "paper_bmh,paper_fbas,paper_improvement_pct,"
           "bmh_diff_pct,fbas_diff_pct\n";

    const int w = 13;
    std::cout
        << std::left  << std::setw(w + 1) << "Pattern"
        << std::right << std::setw(4)  << "Len"
        << std::setw(10) << "BMH(us)"
        << std::setw(10) << "BMH(pp)"
        << std::setw(8)  << "diff"
        << std::setw(10) << "FBAS(us)"
        << std::setw(10) << "FBAS(pp)"
        << std::setw(8)  << "diff"
        << std::setw(10) << "Imp(us)"
        << std::setw(10) << "Imp(pp)"
        << "\n"
        << std::string(94, '-') << "\n";

    uint64_t total_our_bmh = 0;
    uint64_t total_our_fbas = 0;
    uint64_t total_paper_bmh = 0;
    uint64_t total_paper_fbas = 0;

    for (const auto& row : PAPER_TABLE) {
        uint64_t our_bmh = 0;
        uint64_t our_fbas = 0;

        if (row.full_scan) {
            bmh.preprocess(row.pattern);
            bmh.search(text);
            our_bmh = bmh.get_metrics().comparisons;

            fbas.preprocess(row.pattern);
            fbas.search(text);
            our_fbas = fbas.get_metrics().comparisons;
        } else {
            if (text.find(row.pattern) == std::string::npos) {
                std::cerr << "  [WARN] Not found: \"" << row.pattern << "\"\n";
                continue;
            }
            our_bmh = runner.run(bmh, text, row.pattern).metrics.comparisons;
            our_fbas = runner.run(fbas, text, row.pattern).metrics.comparisons;
        }

        double our_imp = our_bmh == 0 ? 0.0
            : 100.0 * (static_cast<double>(our_bmh) - our_fbas) / our_bmh;
        double bmh_diff = pct_diff(our_bmh, row.paper_bmh);
        double fbas_diff = pct_diff(our_fbas, row.paper_fbas);

        total_our_bmh += our_bmh;
        total_our_fbas += our_fbas;
        total_paper_bmh += row.paper_bmh;
        total_paper_fbas += row.paper_fbas;

        std::cout
            << std::left  << std::setw(w + 1) << row.pattern
            << std::right << std::setw(4) << row.pattern.size()
            << std::setw(10) << our_bmh
            << std::setw(10) << row.paper_bmh
            << std::setw(7) << std::fixed << std::setprecision(1) << bmh_diff << "%"
            << std::setw(10) << our_fbas
            << std::setw(10) << row.paper_fbas
            << std::setw(7) << fbas_diff << "%"
            << std::setw(9) << our_imp << "%"
            << std::setw(9) << row.paper_improvement_pct << "%"
            << "\n";

        csv << "\"" << row.pattern << "\","
            << row.pattern.size() << ","
            << our_bmh << "," << our_fbas << ","
            << std::fixed << std::setprecision(4) << our_imp << ","
            << row.paper_bmh << "," << row.paper_fbas << ","
            << row.paper_improvement_pct << ","
            << bmh_diff << "," << fbas_diff << "\n";
    }

    double our_total_imp = total_our_bmh == 0 ? 0.0
        : 100.0 * (static_cast<double>(total_our_bmh) - total_our_fbas)
          / total_our_bmh;
    double paper_total_imp = total_paper_bmh == 0 ? 0.0
        : 100.0 * (static_cast<double>(total_paper_bmh) - total_paper_fbas)
          / total_paper_bmh;

    std::cout << std::string(94, '-') << "\n";
    std::cout
        << std::left  << std::setw(w + 1) << "TOTAL"
        << std::right << std::setw(4) << "-"
        << std::setw(10) << total_our_bmh
        << std::setw(10) << total_paper_bmh
        << std::setw(7) << pct_diff(total_our_bmh, total_paper_bmh) << "%"
        << std::setw(10) << total_our_fbas
        << std::setw(10) << total_paper_fbas
        << std::setw(7) << pct_diff(total_our_fbas, total_paper_fbas) << "%"
        << std::setw(9) << our_total_imp << "%"
        << std::setw(9) << paper_total_imp << "%"
        << "\n\n";

    std::cout << "Key: us = our C++, pp = Garraoui (2025) Table II\n";
    std::cout << "Residual gaps are expected from corpus edition and line-ending differences.\n";
    std::cout << "CSV written to results/fbas_paper_exact.csv\n";
    return 0;
}
