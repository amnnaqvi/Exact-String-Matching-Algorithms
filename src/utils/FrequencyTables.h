#pragma once

#include <unordered_map>
#include <string>

// Character frequency tables used by FBAS to select the anchor.
// Source: standard corpus-derived letter frequencies.
// Italian table matches the language used in Garraoui (2025) — Dante corpus.
// Values are approximate relative frequencies (0.0 to 1.0).
// Lower value = rarer character = better anchor candidate.

namespace FrequencyTables {

    // English letter frequencies (lowercase).
    // Source: standard English corpus statistics.
    inline std::unordered_map<char, double> english() {
        return {
            {'e', 0.1270}, {'t', 0.0906}, {'a', 0.0817}, {'o', 0.0751},
            {'i', 0.0697}, {'n', 0.0675}, {'s', 0.0633}, {'h', 0.0609},
            {'r', 0.0599}, {'d', 0.0425}, {'l', 0.0403}, {'c', 0.0278},
            {'u', 0.0276}, {'m', 0.0241}, {'w', 0.0234}, {'f', 0.0223},
            {'g', 0.0202}, {'y', 0.0197}, {'p', 0.0193}, {'b', 0.0149},
            {'v', 0.0098}, {'k', 0.0077}, {'j', 0.0015}, {'x', 0.0015},
            {'q', 0.0010}, {'z', 0.0007},
            // Uppercase — same relative frequencies
            {'E', 0.1270}, {'T', 0.0906}, {'A', 0.0817}, {'O', 0.0751},
            {'I', 0.0697}, {'N', 0.0675}, {'S', 0.0633}, {'H', 0.0609},
            {'R', 0.0599}, {'D', 0.0425}, {'L', 0.0403}, {'C', 0.0278},
            {'U', 0.0276}, {'M', 0.0241}, {'W', 0.0234}, {'F', 0.0223},
            {'G', 0.0202}, {'Y', 0.0197}, {'P', 0.0193}, {'B', 0.0149},
            {'V', 0.0098}, {'K', 0.0077}, {'J', 0.0015}, {'X', 0.0015},
            {'Q', 0.0010}, {'Z', 0.0007},
            // Space and common punctuation
            {' ', 0.1300}, {',', 0.0100}, {'.', 0.0065},
            {'\'', 0.0024}, {'-', 0.0015}
        };
    }

    // Italian letter frequencies (lowercase).
    // Italian has high 'a', 'e', 'i' and very low 'j', 'k', 'w', 'x', 'y'.
    // Used for Dante's Divina Commedia corpus.
    inline std::unordered_map<char, double> italian() {
        return {
            {'a', 0.1174}, {'e', 0.1179}, {'i', 0.1128}, {'o', 0.0983},
            {'n', 0.0688}, {'t', 0.0562}, {'r', 0.0637}, {'s', 0.0498},
            {'l', 0.0651}, {'c', 0.0450}, {'d', 0.0373}, {'u', 0.0301},
            {'p', 0.0305}, {'m', 0.0251}, {'v', 0.0210}, {'g', 0.0164},
            {'h', 0.0154}, {'f', 0.0095}, {'b', 0.0092}, {'z', 0.0049},
            {'q', 0.0051}, {'x', 0.0003}, {'w', 0.0003}, {'k', 0.0001},
            {'j', 0.0001}, {'y', 0.0001},
            // Uppercase
            {'A', 0.1174}, {'E', 0.1179}, {'I', 0.1128}, {'O', 0.0983},
            {'N', 0.0688}, {'T', 0.0562}, {'R', 0.0637}, {'S', 0.0498},
            {'L', 0.0651}, {'C', 0.0450}, {'D', 0.0373}, {'U', 0.0301},
            {'P', 0.0305}, {'M', 0.0251}, {'V', 0.0210}, {'G', 0.0164},
            {'H', 0.0154}, {'F', 0.0095}, {'B', 0.0092}, {'Z', 0.0049},
            {'Q', 0.0051}, {'X', 0.0003}, {'W', 0.0003}, {'K', 0.0001},
            {'J', 0.0001}, {'Y', 0.0001},
            {' ', 0.1300}, {',', 0.0120}, {'.', 0.0070},
            {'\'', 0.0080}, {'-', 0.0010}
        };
    }

} // namespace FrequencyTables
