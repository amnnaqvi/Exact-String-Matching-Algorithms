#pragma once

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

class CorpusLoader {
public:
    static std::string load(const std::string& filepath) {
        std::string text = load_raw(filepath);
        normalise(text);
        return text;
    }

    static std::string load_raw(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open())
            throw std::runtime_error("Cannot open corpus file: " + filepath);

        std::ostringstream buf;
        buf << file.rdbuf();
        return buf.str();
    }

    static void print_stats(const std::string& name, const std::string& text) {
        size_t spaces = 0;
        size_t newlines = 0;
        for (unsigned char c : text) {
            if (c == ' ')  ++spaces;
            if (c == '\n') ++newlines;
        }

        const double n = static_cast<double>(text.size());
        std::cout << "[CorpusLoader] " << name
                  << ": " << text.size() << " chars, "
                  << spaces << " spaces (" << (100.0 * spaces / n) << "%), "
                  << newlines << " newlines (" << (100.0 * newlines / n) << "%)\n";
    }

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
