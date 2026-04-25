#pragma once

#include <string>
#include <fstream>
#include <stdexcept>
#include "../algorithms/Matcher.h"

// Writes one result row per search run to a CSV file.
// Each row corresponds to one (algorithm, corpus, pattern, run_id) combination.
// The CSV is the single source of truth for all plotting and analysis.

class MetricsRecorder {
public:

    // Start each benchmark execution with a fresh CSV so old rows cannot
    // contaminate means, standard deviations, or paper cross-check plots.
    explicit MetricsRecorder(const std::string& filepath) : filepath_(filepath) {
        out_.open(filepath_, std::ios::trunc);
        if (!out_.is_open())
            throw std::runtime_error("Cannot open results file: " + filepath_);

        write_header();
    }

    ~MetricsRecorder() {
        if (out_.is_open()) out_.close();
    }

    void record(const std::string& algorithm,
                const std::string& corpus,
                const std::string& pattern,
                size_t             pattern_length,
                const std::string& rarity_bucket,
                const MatchMetrics& m,
                int                run_id) {

        out_ << algorithm      << ","
             << corpus         << ","
             << escape(pattern)<< ","
             << pattern_length << ","
             << rarity_bucket  << ","
             << m.comparisons  << ","
             << m.search_ms    << ","
             << m.preprocess_ms<< ","
             << m.matches_found<< ","
             << run_id         << "\n";
        out_.flush();
    }

private:

    void write_header() {
        out_ << "algorithm,corpus,pattern,length,rarity_bucket,"
                "comparisons,runtime_ms,preprocess_ms,matches_found,run_id\n";
    }

    static std::string escape(const std::string& s) {
        std::string out = "\"";
        for (char c : s) {
            if (c == '"') out += "\"\"";
            else          out += c;
        }
        out += "\"";
        return out;
    }

    std::string   filepath_;
    std::ofstream out_;
};
