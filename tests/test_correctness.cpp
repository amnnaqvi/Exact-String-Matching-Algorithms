#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <stdexcept>
#include <unordered_set>

#include "../src/algorithms/BMHMatcher.h"
#include "../src/algorithms/FBASMatcher.h"
#include "../src/utils/FrequencyTables.h"
#include "../src/utils/PatternSampler.h"

// Simple test runner — no external libraries needed.
// Each test calls assert(); a failing test will crash with a clear message.

static int tests_run    = 0;
static int tests_passed = 0;

#define TEST(name) void name()
#define RUN(name) do { \
    std::cout << "  " << #name << " ... "; \
    try { name(); std::cout << "PASS\n"; ++tests_passed; } \
    catch (const std::exception& e) { std::cout << "FAIL: " << e.what() << "\n"; } \
    catch (...) { std::cout << "FAIL (unknown exception)\n"; } \
    ++tests_run; \
} while(0)

// Helper: run algo and return match positions
static std::vector<size_t> run(Matcher& algo,
                                const std::string& text,
                                const std::string& pattern) {
    algo.preprocess(pattern);
    return algo.search(text);
}

// ---- Individual test cases ----

TEST(no_match) {
    BMHMatcher bmh;
    auto res = run(bmh, "hello world", "xyz");
    assert(res.empty());
}

TEST(single_match) {
    BMHMatcher bmh;
    auto res = run(bmh, "hello world", "world");
    assert(res.size() == 1);
    assert(res[0] == 6);
}

TEST(multiple_matches) {
    BMHMatcher bmh;
    auto res = run(bmh, "ababab", "ab");
    assert(res.size() == 3);
    assert(res[0] == 0);
    assert(res[1] == 2);
    assert(res[2] == 4);
}

TEST(match_at_start) {
    BMHMatcher bmh;
    auto res = run(bmh, "pattern is here", "pattern");
    assert(res.size() == 1);
    assert(res[0] == 0);
}

TEST(match_at_end) {
    BMHMatcher bmh;
    auto res = run(bmh, "find the end", "end");
    assert(res.size() == 1);
    assert(res[0] == 9);
}

TEST(pattern_equals_text) {
    BMHMatcher bmh;
    auto res = run(bmh, "exact", "exact");
    assert(res.size() == 1);
    assert(res[0] == 0);
}

TEST(pattern_longer_than_text) {
    BMHMatcher bmh;
    auto res = run(bmh, "hi", "hello");
    assert(res.empty());
}

TEST(length_one_pattern) {
    BMHMatcher bmh;
    auto res = run(bmh, "banana", "a");
    assert(res.size() == 3);
}

TEST(repeated_characters) {
    BMHMatcher bmh;
    auto res = run(bmh, "aaaaaa", "aaa");
    // Matches at 0, 1, 2, 3
    assert(res.size() == 4);
}

TEST(empty_text_returns_empty) {
    BMHMatcher bmh;
    auto res = run(bmh, "", "abc");
    assert(res.empty());
}

TEST(empty_pattern_throws) {
    BMHMatcher bmh;
    bool threw = false;
    try { bmh.preprocess(""); }
    catch (const std::invalid_argument&) { threw = true; }
    assert(threw);
}

// ---- FBAS sanity: must find the same positions as BMH ----
// (Until partner completes FBASMatcher, this confirms the placeholder
//  produces correct positions. Once FBAS is fully implemented,
//  positions must still match — only comparison counts will differ.)

TEST(fbas_matches_bmh_positions) {
    auto eng = FrequencyTables::english();
    BMHMatcher  bmh;
    FBASMatcher fbas(eng);

    std::string text    = "the quick brown fox jumps over the lazy dog";
    std::string pattern = "the";

    auto bmh_res  = run(bmh,  text, pattern);
    auto fbas_res = run(fbas, text, pattern);

    assert(bmh_res == fbas_res);
}

TEST(comparisons_are_counted) {
    BMHMatcher bmh;
    bmh.preprocess("hello");
    bmh.search("hello world hello");
    assert(bmh.get_metrics().comparisons > 0);
    assert(bmh.get_metrics().matches_found == 2);
}

TEST(search_without_preprocess_throws) {
    BMHMatcher bmh;
    bool threw = false;
    try { bmh.search("some text"); }
    catch (const std::logic_error&) { threw = true; }
    assert(threw);
}

TEST(pattern_sampler_returns_unique_patterns) {
    // Use a short synthetic text with enough distinct substrings
    std::string text = "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"
                       "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz";
    PatternSampler s(42);
    auto patterns = s.sample(text, 3, 10);
    std::unordered_set<std::string> seen(patterns.begin(), patterns.end());
    assert(seen.size() == patterns.size());  // no duplicates
}

// ---- Main ----

int main() {
    std::cout << "Running correctness tests...\n\n";

    RUN(no_match);
    RUN(single_match);
    RUN(multiple_matches);
    RUN(match_at_start);
    RUN(match_at_end);
    RUN(pattern_equals_text);
    RUN(pattern_longer_than_text);
    RUN(length_one_pattern);
    RUN(repeated_characters);
    RUN(empty_text_returns_empty);
    RUN(empty_pattern_throws);
    RUN(fbas_matches_bmh_positions);
    RUN(comparisons_are_counted);
    RUN(search_without_preprocess_throws);
    RUN(pattern_sampler_returns_unique_patterns);

    std::cout << "\n" << tests_passed << " / " << tests_run << " passed.\n";
    return (tests_passed == tests_run) ? 0 : 1;
}
