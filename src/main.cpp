#include <iostream>
#include <vector>
#include <string>

#include "algorithms/BMHMatcher.h"
#include "algorithms/FBASMatcher.h"
#include "algorithms/HCMatcher.h"
#include "benchmark/BenchmarkRunner.h"
#include "utils/CorpusLoader.h"
#include "utils/PatternSampler.h"
#include "utils/FrequencyTables.h"
#include "utils/MetricsRecorder.h"

// ================================================================
//  Experiment configuration
//
//  Pattern lengths follow the HC paper (Palmer et al. 2024):
//    m in {8, 16, 32, 64, 128}
//
//  Per-corpus length caps:
//    Dante (~600 KB): cap at 64. At m=128 a 600 KB corpus has
//      fewer than 20 distinct substrings that don't cross the
//      original line boundaries (which are now spaces), so there
//      isn't enough variety to fill the sample. We drop 128 for
//      Dante and note this in the report.
//    Pizza (100 MB): full range {8, 16, 32, 64, 128}.
//
//  HC parameter configurations: we run both (q=3, alpha=11) and
//  (q=3, alpha=12). analyse.py selects the faster one per corpus
//  and reports it as "HC", matching the paper's "best variant"
//  reporting style.
// ================================================================

static constexpr size_t PATTERNS_PER_LENGTH = 20;

struct CorpusConfig {
    std::string name;
    std::string path;
    std::unordered_map<char, double> freq_table;
    std::vector<size_t> pattern_lengths;   // per-corpus length sweep
};

int main() {
    std::vector<CorpusConfig> corpora = {
        {
            "dante",
            "data/dante/divina_commedia.txt",
            FrequencyTables::italian(),
            {8, 16, 32, 64}          // 128 not feasible for ~600 KB corpus
        },
        {
            "pizza",
            "data/pizza/english.txt",
            FrequencyTables::english(),
            {8, 16, 32, 64, 128}     // full sweep on 100 MB corpus
        },
    };

    MetricsRecorder recorder("results/results.csv");

    for (const auto& corpus : corpora) {
        std::string text;
        try {
            text = CorpusLoader::load(corpus.path);
        } catch (const std::exception& e) {
            std::cerr << "Skipping " << corpus.name << ": " << e.what() << "\n";
            continue;
        }

        std::cout << "\nLoaded " << corpus.name
                  << " (" << text.size() << " chars)\n";
        CorpusLoader::print_stats(corpus.name, text);

        // Build pattern list: all lengths * 20 samples, labelled by rarity
        PatternSampler sampler(42);
        std::vector<std::pair<std::string, std::string>> patterns;

        for (size_t len : corpus.pattern_lengths) {
            if (len >= text.size()) {
                std::cerr << "  Skipping length " << len
                          << " (corpus too small)\n";
                continue;
            }
            auto batch = sampler.sample(text, len, PATTERNS_PER_LENGTH);
            for (const auto& p : batch) {
                std::string rarity = PatternSampler::rarity_label(
                    p, corpus.freq_table);
                patterns.push_back({p, rarity});
            }
        }

        // Instantiate algorithms
        BMHMatcher  bmh;
        FBASMatcher fbas(corpus.freq_table);
        HCMatcher   hc_11(3, 11);
        HCMatcher   hc_12(3, 12);

        BenchmarkRunner runner;
        runner.add_algorithm(&bmh);
        runner.add_algorithm(&fbas);
        runner.add_algorithm(&hc_11);
        runner.add_algorithm(&hc_12);

        runner.run(corpus.name, text, patterns, recorder);
    }

    std::cout << "\nDone. Results written to results/results.csv\n";
    return 0;
}