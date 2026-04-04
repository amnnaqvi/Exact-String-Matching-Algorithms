#pragma once

#include <string>
#include <fstream>
#include <stdexcept>
#include "../algorithms/Matcher.h"

// Appends one result row per search run to a CSV file.
// Each row corresponds to one (algorithm, corpus, pattern, run_id) combination.
// The CSV is the single source of truth for all plotting and analysis.

class MetricsRecorder {
public:

    // Opens (or creates) the CSV file and writes the header row if it's new.
    explicit MetricsRecorder(const std::string& filepath) : filepath_(filepath) {
        // Check if file already exists — if so, don't rewrite the header.
        std::ifstream check(filepath_);
        bool exists = check.good();
        check.close();

        out_.open(filepath_, std::ios::app);
        if (!out_.is_open())
            throw std::runtime_error("Cannot open results file: " + filepath_);

        if (!exists)
            write_header();
    }

    ~MetricsRecorder() {
        if (out_.is_open()) out_.close();
    }

    // Record one run. Call this once per search() call.
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

    // Wrap pattern in quotes and escape any internal quotes.
    // Patterns can contain commas, so this keeps the CSV valid.
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
