#pragma once

#include <vector>
#include <string>
#include <iostream>
#include "../algorithms/Matcher.h"
#include "../utils/MetricsRecorder.h"

// Runs each algorithm over a set of (pattern, rarity) pairs for one corpus.
// Each (algorithm, pattern) combination is searched N_RUNS times.
// Preprocess is called once per pattern — not once per run —
// which accurately isolates search cost from setup cost.

static constexpr int N_RUNS = 5;

class BenchmarkRunner {
public:

    void add_algorithm(Matcher* algo) {
        algorithms_.push_back(algo);
    }

    // patterns: vector of (pattern_string, rarity_label) pairs
    void run(const std::string& corpus_name,
             const std::string& text,
             const std::vector<std::pair<std::string, std::string>>& patterns,
             MetricsRecorder& recorder) {

        for (Matcher* algo : algorithms_) {
            for (const auto& [pattern, rarity] : patterns) {

                // Preprocess once — builds the shift table (or hash chain for HC).
                algo->preprocess(pattern);
                double preprocess_ms = algo->get_metrics().preprocess_ms;

                for (int run = 0; run < N_RUNS; ++run) {
                    // search() uses the pattern stored internally by preprocess()
                    algo->search(text);
                    const MatchMetrics& m = algo->get_metrics();

                    MatchMetrics row     = m;
                    row.preprocess_ms    = preprocess_ms;

                    recorder.record(
                        algo->name(),
                        corpus_name,
                        algo->current_pattern(),
                        algo->current_pattern().size(),
                        rarity,
                        row,
                        run + 1
                    );
                }

                std::cout << "[" << algo->name() << "] "
                          << corpus_name
                          << " | len=" << pattern.size()
                          << " | rarity=" << rarity << "\n";
            }
        }
    }

private:
    std::vector<Matcher*> algorithms_;
};
