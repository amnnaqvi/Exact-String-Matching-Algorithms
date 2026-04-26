#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <stdexcept>

// Shared interface for the algorithms under test. Keeping the metrics here
// makes the benchmark runner and CSV writer independent of algorithm details.

struct MatchMetrics {
    uint64_t comparisons   = 0;    // character comparisons made during search
    double   preprocess_ms = 0.0;  // time to build shift table or filter
    double   search_ms     = 0.0;  // wall-clock time for the search phase
    uint64_t matches_found = 0;    // total match positions found in text
};

class Matcher {
public:
    virtual ~Matcher() = default;

    virtual void preprocess(const std::string& pattern) = 0;
    virtual std::vector<size_t> search(const std::string& text) = 0;
    virtual std::string name() const = 0;

    const MatchMetrics& get_metrics() const { return metrics_; }

    // The pattern currently loaded; useful when recording benchmark rows.
    const std::string& current_pattern() const { return pattern_; }

protected:
    MatchMetrics metrics_;
    std::string  pattern_;
};
