// #pragma once

// #include <string>
// #include <fstream>
// #include <sstream>
// #include <stdexcept>
// #include <algorithm>
// #include <cctype>
// #include <iostream>

// // ================================================================
// //  CorpusLoader
// //
// //  Loads a plain-text corpus file and normalises it so that
// //  PatternSampler can extract patterns of any length without
// //  hitting newline/null boundaries.
// //
// //  Normalisation: all newline (\n), carriage-return (\r), and
// //  null (\0) bytes are replaced with a single space ' '.
// //
// //  This is the standard approach in string-matching benchmarks,
// //  including the SMART tool used by Palmer et al. (2024) to
// //  evaluate the Hash Chain algorithm. It reflects real search
// //  scenarios where patterns span word and sentence boundaries.
// // ================================================================

// class CorpusLoader {
// public:

//     // Load the entire file and normalise whitespace to spaces.
//     // Throws if the file cannot be opened.
//     static std::string load(const std::string& filepath) {
//         std::ifstream file(filepath, std::ios::binary);
//         if (!file.is_open())
//             throw std::runtime_error("Cannot open corpus file: " + filepath);

//         std::ostringstream buf;
//         buf << file.rdbuf();
//         std::string text = buf.str();

//         normalise(text);
//         return text;
//     }

//     // Print basic corpus statistics — useful for verifying the loaded corpus.
//     static void print_stats(const std::string& name, const std::string& text) {
//         size_t n = text.size();
//         size_t spaces = 0;
//         for (unsigned char c : text) {
//             if (c == ' ') spaces++;
//         }
//         std::cout << "[CorpusLoader] " << name
//                   << ": " << n << " chars, "
//                   << spaces << " spaces ("
//                   << (100.0 * spaces / n) << "%)\n";
//     }

//     // Optional: convert text to lowercase ASCII (for case-insensitive experiments).
//     static std::string to_lowercase(const std::string& text) {
//         std::string result = text;
//         std::transform(result.begin(), result.end(), result.begin(),
//             [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
//         return result;
//     }

// private:

//     // Replace \n, \r, \0 with space so the corpus is one continuous string.
//     // Consecutive whitespace is NOT collapsed — we preserve original byte count
//     // so position offsets remain meaningful.
//     static void normalise(std::string& text) {
//         for (char& c : text) {
//             if (c == '\n' || c == '\r' || c == '\0') {
//                 c = ' ';
//             }
//         }
//     }
// };









#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <iostream>

// ================================================================
//  CorpusLoader
//
//  Loads a plain-text corpus file in one of two modes:
//
//  load()      — normalised (newlines/nulls → spaces).
//                Used by the main benchmark and PatternSampler so
//                that patterns of any length can be sampled without
//                hitting line-boundary gaps.  Standard in benchmarks
//                including the SMART tool (Palmer et al. 2024).
//
//  load_raw()  — raw bytes, no substitution whatsoever.
//                Used by replicate_fbas_paper to match the exact
//                corpus conditions of Garraoui (2025), whose Python
//                implementation reads the file as-is and therefore
//                treats newlines as search boundaries.
// ================================================================

class CorpusLoader {
public:

    // Load and normalise: \n, \r, \0 → ' '.
    // Throws if the file cannot be opened.
    static std::string load(const std::string& filepath) {
        std::string text = load_raw(filepath);
        normalise(text);
        return text;
    }

    // Load raw bytes — no substitution.
    // Throws if the file cannot be opened.
    static std::string load_raw(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open())
            throw std::runtime_error("Cannot open corpus file: " + filepath);

        std::ostringstream buf;
        buf << file.rdbuf();
        return buf.str();
    }

    // Print basic corpus statistics.
    static void print_stats(const std::string& name, const std::string& text) {
        size_t n = text.size();
        size_t spaces = 0, newlines = 0;
        for (unsigned char c : text) {
            if (c == ' ')  ++spaces;
            if (c == '\n') ++newlines;
        }
        std::cout << "[CorpusLoader] " << name
                  << ": " << n << " chars, "
                  << spaces   << " spaces ("   << (100.0 * spaces   / n) << "%), "
                  << newlines << " newlines (" << (100.0 * newlines / n) << "%)\n";
    }

    // Optional: lowercase conversion.
    static std::string to_lowercase(const std::string& text) {
        std::string result = text;
        std::transform(result.begin(), result.end(), result.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return result;
    }

private:

    static void normalise(std::string& text) {
        for (char& c : text) {
            if (c == '\n' || c == '\r' || c == '\0')
                c = ' ';
        }
    }
};