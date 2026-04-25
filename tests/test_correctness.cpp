#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <stdexcept>
#include <unordered_set>
#include <algorithm>
#include <random>

#include "../src/algorithms/BMHMatcher.h"
#include "../src/algorithms/FBASMatcher.h"
#include "../src/algorithms/HCMatcher.h"
#include "../src/utils/FrequencyTables.h"
#include "../src/utils/PatternSampler.h"

// ================================================================
//  Simple test runner — no external libraries needed.
//  Each test calls assert(); a failing test crashes with a message.
// ================================================================

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

// Helper: preprocess + search
static std::vector<size_t> run(Matcher& algo,
                                const std::string& text,
                                const std::string& pattern) {
    algo.preprocess(pattern);
    return algo.search(text);
}

// Helper: sort positions for order-independent comparison
static std::vector<size_t> sorted(std::vector<size_t> v) {
    std::sort(v.begin(), v.end());
    return v;
}

// ================================================================
//  BMH tests
// ================================================================

TEST(bmh_no_match) {
    BMHMatcher bmh;
    assert(run(bmh, "hello world", "xyz").empty());
}

TEST(bmh_single_match) {
    BMHMatcher bmh;
    auto res = run(bmh, "hello world", "world");
    assert(res.size() == 1 && res[0] == 6);
}

TEST(bmh_multiple_matches) {
    BMHMatcher bmh;
    auto res = run(bmh, "ababab", "ab");
    assert(res.size() == 3);
    assert(res[0] == 0 && res[1] == 2 && res[2] == 4);
}

TEST(bmh_match_at_start) {
    BMHMatcher bmh;
    auto res = run(bmh, "pattern is here", "pattern");
    assert(res.size() == 1 && res[0] == 0);
}

TEST(bmh_match_at_end) {
    BMHMatcher bmh;
    auto res = run(bmh, "find the end", "end");
    assert(res.size() == 1 && res[0] == 9);
}

TEST(bmh_pattern_equals_text) {
    BMHMatcher bmh;
    auto res = run(bmh, "exact", "exact");
    assert(res.size() == 1 && res[0] == 0);
}

TEST(bmh_pattern_longer_than_text) {
    BMHMatcher bmh;
    assert(run(bmh, "hi", "hello").empty());
}

TEST(bmh_length_one_pattern) {
    BMHMatcher bmh;
    auto res = run(bmh, "banana", "a");
    assert(res.size() == 3);
}

TEST(bmh_repeated_characters) {
    BMHMatcher bmh;
    auto res = run(bmh, "aaaaaa", "aaa");
    // Matches at 0, 1, 2, 3
    assert(res.size() == 4);
}

TEST(bmh_empty_text_returns_empty) {
    BMHMatcher bmh;
    assert(run(bmh, "", "abc").empty());
}

TEST(bmh_empty_pattern_throws) {
    BMHMatcher bmh;
    bool threw = false;
    try { bmh.preprocess(""); }
    catch (const std::invalid_argument&) { threw = true; }
    assert(threw);
}

TEST(bmh_comparisons_are_counted) {
    BMHMatcher bmh;
    bmh.preprocess("hello");
    bmh.search("hello world hello");
    assert(bmh.get_metrics().comparisons > 0);
    assert(bmh.get_metrics().matches_found == 2);
}

TEST(bmh_search_without_preprocess_throws) {
    BMHMatcher bmh;
    bool threw = false;
    try { bmh.search("some text"); }
    catch (const std::logic_error&) { threw = true; }
    assert(threw);
}

// ================================================================
//  FBAS tests
// ================================================================

TEST(fbas_matches_bmh_positions_simple) {
    auto eng = FrequencyTables::english();
    BMHMatcher  bmh;
    FBASMatcher fbas(eng);

    std::string text    = "the quick brown fox jumps over the lazy dog";
    std::string pattern = "the";

    auto bmh_res  = run(bmh,  text, pattern);
    auto fbas_res = run(fbas, text, pattern);

    assert(sorted(bmh_res) == sorted(fbas_res));
}

TEST(fbas_no_match) {
    auto eng = FrequencyTables::english();
    FBASMatcher fbas(eng);
    assert(run(fbas, "hello world", "xyz").empty());
}

TEST(fbas_single_match) {
    auto eng = FrequencyTables::english();
    FBASMatcher fbas(eng);
    auto res = run(fbas, "hello world", "world");
    assert(res.size() == 1 && res[0] == 6);
}

TEST(fbas_multiple_matches) {
    auto eng = FrequencyTables::english();
    FBASMatcher fbas(eng);
    auto res = sorted(run(fbas, "ababab", "ab"));
    assert(res.size() == 3);
    assert(res[0] == 0 && res[1] == 2 && res[2] == 4);
}

TEST(fbas_comparisons_fewer_than_bmh_on_rare_anchor) {
    // Pattern with a rare character (z) — FBAS should reject most windows
    // with 1 comparison at the anchor, so total comparisons <= BMH.
    auto eng = FrequencyTables::english();
    BMHMatcher  bmh;
    FBASMatcher fbas(eng);

    std::string text    = "the quick brown fox jumps over the lazy dog again and again";
    std::string pattern = "lazy";

    bmh.preprocess(pattern);
    bmh.search(text);
    uint64_t bmh_cmp = bmh.get_metrics().comparisons;

    fbas.preprocess(pattern);
    fbas.search(text);
    uint64_t fbas_cmp = fbas.get_metrics().comparisons;

    assert(fbas_cmp <= bmh_cmp);
}

TEST(fbas_italian_table) {
    auto ita = FrequencyTables::italian();
    FBASMatcher fbas(ita);
    std::string text    = "nel mezzo del cammin di nostra vita";
    std::string pattern = "cammin";
    auto res = run(fbas, text, pattern);
    assert(res.size() == 1 && res[0] == 14);
}

// ================================================================
//  HC tests
// ================================================================

TEST(hc_no_match) {
    HCMatcher hc;
    assert(run(hc, "hello world", "xyz").empty());
}

TEST(hc_single_match) {
    HCMatcher hc;
    auto res = run(hc, "hello world", "world");
    assert(res.size() == 1 && res[0] == 6);
}

TEST(hc_multiple_matches) {
    HCMatcher hc;
    auto res = sorted(run(hc, "ababab", "ab"));
    assert(res.size() == 3);
    assert(res[0] == 0 && res[1] == 2 && res[2] == 4);
}

TEST(hc_match_at_start) {
    HCMatcher hc;
    auto res = run(hc, "pattern is here", "pattern");
    assert(res.size() == 1 && res[0] == 0);
}

TEST(hc_match_at_end) {
    HCMatcher hc;
    auto res = run(hc, "find the end", "end");
    assert(res.size() == 1 && res[0] == 9);
}

TEST(hc_pattern_equals_text) {
    HCMatcher hc;
    auto res = run(hc, "exact", "exact");
    assert(res.size() == 1 && res[0] == 0);
}

TEST(hc_pattern_longer_than_text) {
    HCMatcher hc;
    assert(run(hc, "hi", "hello").empty());
}

TEST(hc_length_one_pattern) {
    HCMatcher hc(1, 8);   // q=1, alpha=8 — works for single-char patterns
    auto res = run(hc, "banana", "a");
    assert(res.size() == 3);
}

TEST(hc_repeated_characters) {
    HCMatcher hc;
    auto res = sorted(run(hc, "aaaaaa", "aaa"));
    assert(res.size() == 4);
}

TEST(hc_empty_text_returns_empty) {
    HCMatcher hc;
    assert(run(hc, "", "abc").empty());
}

TEST(hc_empty_pattern_throws) {
    HCMatcher hc;
    bool threw = false;
    try { hc.preprocess(""); }
    catch (const std::invalid_argument&) { threw = true; }
    assert(threw);
}

TEST(hc_search_without_preprocess_throws) {
    HCMatcher hc;
    bool threw = false;
    try { hc.search("some text"); }
    catch (const std::logic_error&) { threw = true; }
    assert(threw);
}

TEST(hc_matches_bmh_positions_short) {
    // HC must report exactly the same positions as BMH on short text
    BMHMatcher bmh;
    HCMatcher  hc;
    std::string text    = "the quick brown fox jumps over the lazy dog";
    std::string pattern = "the";
    assert(sorted(run(bmh, text, pattern)) == sorted(run(hc, text, pattern)));
}

TEST(hc_matches_bmh_positions_longer_pattern) {
    BMHMatcher bmh;
    HCMatcher  hc(3, 11);
    std::string text    = "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"
                          "abcdefghijklmnopqrstuvwxyz";
    std::string pattern = "ghijklmn";   // length 8
    assert(sorted(run(bmh, text, pattern)) == sorted(run(hc, text, pattern)));
}

TEST(hc_matches_bmh_multiple_occurrences) {
    BMHMatcher bmh;
    HCMatcher  hc;
    std::string text    = "hello world hello earth hello";
    std::string pattern = "hello";
    assert(sorted(run(bmh, text, pattern)) == sorted(run(hc, text, pattern)));
}

TEST(hc_comparisons_counted) {
    HCMatcher hc;
    hc.preprocess("hello");
    hc.search("hello world hello");
    // HC only counts verification comparisons — must be > 0
    assert(hc.get_metrics().comparisons > 0);
    assert(hc.get_metrics().matches_found == 2);
}

TEST(hc_different_alpha_same_positions) {
    // Different alpha values may differ in speed but must agree on positions
    BMHMatcher bmh;
    HCMatcher  hc11(3, 11);
    HCMatcher  hc12(3, 12);
    std::string text    = "to be or not to be that is the question whether tis";
    std::string pattern = "the";
    auto bmh_res = sorted(run(bmh,  text, pattern));
    assert(sorted(run(hc11, text, pattern)) == bmh_res);
    assert(sorted(run(hc12, text, pattern)) == bmh_res);
}

TEST(hc_q_larger_than_pattern_clamped) {
    // q=10 > pattern length=5 — should clamp without crashing
    HCMatcher hc(10, 11);
    auto res = run(hc, "hello world hello", "hello");
    assert(res.size() == 2);
}

// ================================================================
//  Cross-algorithm agreement test on longer synthetic text
// ================================================================

TEST(all_three_agree_on_positions) {
    auto eng = FrequencyTables::english();
    BMHMatcher  bmh;
    FBASMatcher fbas(eng);
    HCMatcher   hc(3, 11);

    std::string text =
        "the cat sat on the mat and the rat saw the bat near the hat";
    std::string pattern = "the";

    auto bmh_res  = sorted(run(bmh,  text, pattern));
    auto fbas_res = sorted(run(fbas, text, pattern));
    auto hc_res   = sorted(run(hc,   text, pattern));

    assert(bmh_res == fbas_res);
    assert(bmh_res == hc_res);
}

TEST(all_algorithms_agree_on_random_texts) {
    auto eng = FrequencyTables::english();
    std::mt19937 rng(47);
    std::uniform_int_distribution<int> char_dist(0, 4);
    const std::string alphabet = "ab cd";

    for (int case_id = 0; case_id < 100; ++case_id) {
        std::string text;
        text.reserve(250);
        for (int i = 0; i < 250; ++i)
            text.push_back(alphabet[char_dist(rng)]);

        for (size_t length : {1u, 2u, 3u, 5u, 8u, 13u}) {
            size_t start = static_cast<size_t>(case_id * 17 + length * 11)
                         % (text.size() - length);
            std::string pattern = text.substr(start, length);

            BMHMatcher bmh;
            FBASMatcher fbas(eng);
            HCMatcher hc11(3, 11);
            HCMatcher hc12(3, 12);

            auto expected = sorted(run(bmh, text, pattern));
            assert(sorted(run(fbas, text, pattern)) == expected);
            assert(sorted(run(hc11, text, pattern)) == expected);
            assert(sorted(run(hc12, text, pattern)) == expected);
        }
    }
}

// ================================================================
//  PatternSampler tests
// ================================================================

TEST(pattern_sampler_returns_unique_patterns) {
    // Simulate normalised corpus text (spaces instead of newlines)
    std::string text = "abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz "
                       "abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz";
    PatternSampler s(42);
    auto patterns = s.sample(text, 4, 15);
    std::unordered_set<std::string> seen(patterns.begin(), patterns.end());
    assert(seen.size() == patterns.size());   // all unique
    for (const auto& p : patterns)
        assert(p.size() == 4);               // correct length
}

TEST(pattern_sampler_systematic_fallback) {
    // Small corpus — systematic fallback must find distinct patterns
    std::string text = "abcde fghij klmno pqrst uvwxy zabcd efghi jklmn opqrs tuvwx";
    PatternSampler s(42);
    auto patterns = s.sample(text, 5, 10);
    std::unordered_set<std::string> seen(patterns.begin(), patterns.end());
    assert(seen.size() == patterns.size());  // all unique
    assert(patterns.size() == 10);           // got all requested
}

// ================================================================
//  Main
// ================================================================

int main() {
    std::cout << "Running correctness tests...\n\n";

    std::cout << "--- BMH ---\n";
    RUN(bmh_no_match);
    RUN(bmh_single_match);
    RUN(bmh_multiple_matches);
    RUN(bmh_match_at_start);
    RUN(bmh_match_at_end);
    RUN(bmh_pattern_equals_text);
    RUN(bmh_pattern_longer_than_text);
    RUN(bmh_length_one_pattern);
    RUN(bmh_repeated_characters);
    RUN(bmh_empty_text_returns_empty);
    RUN(bmh_empty_pattern_throws);
    RUN(bmh_comparisons_are_counted);
    RUN(bmh_search_without_preprocess_throws);

    std::cout << "\n--- FBAS ---\n";
    RUN(fbas_no_match);
    RUN(fbas_single_match);
    RUN(fbas_multiple_matches);
    RUN(fbas_matches_bmh_positions_simple);
    RUN(fbas_comparisons_fewer_than_bmh_on_rare_anchor);
    RUN(fbas_italian_table);

    std::cout << "\n--- HC ---\n";
    RUN(hc_no_match);
    RUN(hc_single_match);
    RUN(hc_multiple_matches);
    RUN(hc_match_at_start);
    RUN(hc_match_at_end);
    RUN(hc_pattern_equals_text);
    RUN(hc_pattern_longer_than_text);
    RUN(hc_length_one_pattern);
    RUN(hc_repeated_characters);
    RUN(hc_empty_text_returns_empty);
    RUN(hc_empty_pattern_throws);
    RUN(hc_search_without_preprocess_throws);
    RUN(hc_matches_bmh_positions_short);
    RUN(hc_matches_bmh_positions_longer_pattern);
    RUN(hc_matches_bmh_multiple_occurrences);
    RUN(hc_comparisons_counted);
    RUN(hc_different_alpha_same_positions);
    RUN(hc_q_larger_than_pattern_clamped);

    std::cout << "\n--- Cross-algorithm ---\n";
    RUN(all_three_agree_on_positions);
    RUN(all_algorithms_agree_on_random_texts);
    RUN(pattern_sampler_returns_unique_patterns);
    RUN(pattern_sampler_systematic_fallback);

    std::cout << "\n" << tests_passed << " / " << tests_run << " passed.\n";
    return (tests_passed == tests_run) ? 0 : 1;
}
