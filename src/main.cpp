#include <iostream>
#include <vector>
#include <string>

#include "algorithms/BMHMatcher.h"
#include "algorithms/FBASMatcher.h"
#include "benchmark/BenchmarkRunner.h"
#include "utils/CorpusLoader.h"
#include "utils/PatternSampler.h"
#include "utils/FrequencyTables.h"
#include "utils/MetricsRecorder.h"

// Pattern lengths and sample count per length, matching the proposal's
// experiment plan (Section 4). For checkpoint 2 we use a subset.
static const std::vector<size_t> PATTERN_LENGTHS = {8, 16, 32};
static constexpr size_t PATTERNS_PER_LENGTH = 20;

// ---------------------------------------------------------------
// Corpora config — edit paths to point at your actual data files.
// Download instructions are in data/README.txt.
// ---------------------------------------------------------------
struct CorpusConfig {
    std::string name;
    std::string path;
    std::unordered_map<char, double> freq_table;
};

int main() {
    // --- Set up corpora ---
    std::vector<CorpusConfig> corpora = {
        { "dante",    "data/dante/divina_commedia.txt",  FrequencyTables::italian() },
        { "pizza",    "data/pizza/english.txt",          FrequencyTables::english() },
    };

    // --- Set up algorithms ---
    // FBAS is constructed with the corpus-appropriate freq table.
    // We create one FBAS instance per corpus inside the loop below.
    BMHMatcher bmh;

    // --- Results CSV ---
    MetricsRecorder recorder("results/results.csv");

    BenchmarkRunner runner;
    runner.add_algorithm(&bmh);
    // FBAS is added per corpus (different freq tables), see loop below.

    // --- Run experiments ---
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

        // Build the full pattern list: all lengths + rarity labels
        PatternSampler sampler(42);  // fixed seed = reproducible
        std::vector<std::pair<std::string, std::string>> patterns;

        for (size_t len : PATTERN_LENGTHS) {
            auto batch = sampler.sample(text, len, PATTERNS_PER_LENGTH);
            for (const auto& p : batch) {
                std::string rarity = PatternSampler::rarity_label(p, corpus.freq_table);
                patterns.push_back({p, rarity});
            }
        }

        // FBAS needs a freq table matching this corpus's language
        FBASMatcher fbas(corpus.freq_table);

                // Run BMH + FBAS on this corpus
        BenchmarkRunner corp_runner;
        corp_runner.add_algorithm(&bmh);
        corp_runner.add_algorithm(&fbas);
        corp_runner.run(corpus.name, text, patterns, recorder);
    }

    std::cout << "\nDone. Results written to results/results.csv\n";
    return 0;
}
