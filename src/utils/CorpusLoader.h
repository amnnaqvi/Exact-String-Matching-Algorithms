#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cctype>

// Loads a plain-text corpus file into a std::string.
// Also provides a helper to normalise text to lowercase ASCII,
// which is useful if you want case-insensitive experiments.

class CorpusLoader {
public:

    // Load entire file into a string. Throws if the file can't be opened.
    static std::string load(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open())
            throw std::runtime_error("Cannot open corpus file: " + filepath);

        std::ostringstream buf;
        buf << file.rdbuf();
        return buf.str();
    }

    // Optional: convert text to lowercase for case-insensitive matching.
    // Only applies to ASCII characters (safe for Latin-script corpora).
    static std::string to_lowercase(const std::string& text) {
        std::string result = text;
        std::transform(result.begin(), result.end(), result.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return result;
    }
};
