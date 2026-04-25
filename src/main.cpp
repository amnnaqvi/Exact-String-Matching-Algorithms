#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "algorithms/BMHMatcher.h"
#include "algorithms/FBASMatcher.h"
#include "algorithms/HCMatcher.h"
#include "benchmark/BenchmarkRunner.h"
#include "utils/CorpusLoader.h"
#include "utils/FrequencyTables.h"
#include "utils/MetricsRecorder.h"
#include "utils/PatternSampler.h"

// Pattern lengths follow the HC paper (Palmer et al. 2024):
// m in {8, 16, 32, 64, 128}. Dante is capped at 64 because it is
// much smaller than the 100 MB English corpora.

static constexpr size_t PATTERNS_PER_LENGTH = 20;

static const std::vector<std::string> FBAS_PAPER_PATTERNS = {
    "inferno", "paradiso", "purgatorio", "beatrice", "dante",
    "virtute", "canoscenza", "nel mezzo", "selva oscura",
    "amor", "luce", "dolce"
};

struct CorpusConfig {
    std::string name;
    std::string path;
    std::unordered_map<char, double> freq_table;
    std::vector<size_t> pattern_lengths;
};

static bool exists_in_text(const std::string& text, const std::string& pattern) {
    return text.find(pattern) != std::string::npos;
}

int main() {
    const std::vector<CorpusConfig> corpora = {
        {
            "dante",
            "data/dante/divina_commedia.txt",
            FrequencyTables::italian(),
            {8, 16, 32, 64}
        },
        {
            "pizza",
            "data/pizza/english.txt",
            FrequencyTables::english(),
            {8, 16, 32, 64, 128}
        },
        {
            "gutenberg",
            "data/gutenberg/gutenberg_english.txt",
            FrequencyTables::english(),
            {8, 16, 32, 64, 128}
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

        std::vector<std::pair<std::string, std::string>> patterns;

        if (corpus.name == "dante") {
            std::cout << "  Adding FBAS paper fixed patterns...\n";
            int added = 0;
            int skipped = 0;
            for (const auto& pattern : FBAS_PAPER_PATTERNS) {
                if (exists_in_text(text, pattern)) {
                    patterns.push_back({pattern, "paper_fixed"});
                    ++added;
                } else {
                    std::cerr << "  [WARN] Paper pattern not found: \""
                              << pattern << "\"\n";
                    ++skipped;
                }
            }
            std::cout << "  Fixed patterns: " << added << " added, "
                      << skipped << " skipped.\n";
        }

        PatternSampler sampler(42);
        for (size_t length : corpus.pattern_lengths) {
            if (length >= text.size()) {
                std::cerr << "  Skipping length " << length
                          << " (corpus too small)\n";
                continue;
            }

            auto batch = sampler.sample(text, length, PATTERNS_PER_LENGTH);
            for (const auto& pattern : batch) {
                patterns.push_back({
                    pattern,
                    PatternSampler::rarity_label(pattern, corpus.freq_table)
                });
            }
        }

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
