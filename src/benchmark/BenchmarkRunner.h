#pragma once

#include <vector>
#include <string>
#include <iostream>
#include "../algorithms/Matcher.h"
#include "../utils/MetricsRecorder.h"

// ================================================================
//  BenchmarkRunner
//
//  Runs each registered algorithm over all (pattern, rarity) pairs
//  for one corpus. Each (algorithm, pattern) pair is searched
//  N_RUNS times. Preprocess is called ONCE per pattern (not per run)
//  to accurately isolate search cost from setup cost.
//
//  The HC algorithm's name() method includes its parameter string
//  (e.g. "HC_q3a11"), so different HC configurations appear as
//  separate rows in the CSV and can be compared in analysis.
// ================================================================

static constexpr int N_RUNS = 30;

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

                // Preprocess once per (algorithm, pattern) pair
                algo->preprocess(pattern);
                double preprocess_ms = algo->get_metrics().preprocess_ms;

                for (int run = 0; run < N_RUNS; ++run) {
                    algo->search(text);
                    const MatchMetrics& m = algo->get_metrics();

                    MatchMetrics row  = m;
                    row.preprocess_ms = preprocess_ms;

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
