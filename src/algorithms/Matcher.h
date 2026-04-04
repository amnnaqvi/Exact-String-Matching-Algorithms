#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <stdexcept>

// Every algorithm (BMH, FBAS, HC) inherits from this.
// This guarantees they all produce metrics in the same format,
// which is critical for fair benchmarking.

struct MatchMetrics {
    uint64_t comparisons   = 0;    // character comparisons made during search
    double   preprocess_ms = 0.0;  // time to build shift table (or equivalent)
    double   search_ms     = 0.0;  // wall-clock time for the search phase
    uint64_t matches_found = 0;    // total match positions found in text
};

class Matcher {
public:
    virtual ~Matcher() = default;

    // Build internal tables from the pattern.
    // Must be called before search(). Stores the pattern internally
    // so search() can never accidentally use a different pattern.
    virtual void preprocess(const std::string& pattern) = 0;

    // Find all occurrences of the preprocessed pattern in text.
    // Throws if preprocess() has not been called yet.
    // Returns 0-based start positions of every match.
    virtual std::vector<size_t> search(const std::string& text) = 0;

    // Label used in CSV output and plot legends.
    virtual std::string name() const = 0;

    const MatchMetrics& get_metrics() const { return metrics_; }

    // The pattern currently loaded — useful for the benchmark runner
    // to pass the same string to MetricsRecorder without storing it twice.
    const std::string& current_pattern() const { return pattern_; }

protected:
    MatchMetrics metrics_;
    std::string  pattern_;   // set by preprocess(), used by search()
};
