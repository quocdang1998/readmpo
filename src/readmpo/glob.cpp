// Copyright 2022 quocdang1998
#include "readmpo/glob.hpp"

#include <cctype>      // std::isalnum
#include <cstddef>     // std::size_t
#include <filesystem>  // std::filesystem
#include <regex>       // std::regex, std::regex::ECMAScript, std::regex_match

namespace readmpo {

// Replace a string
static bool string_replace(std::string & str, const std::string & substring, const std::string & destination) {
    auto start_pos = str.find(substring);
    if (start_pos == std::string::npos) {
        return false;
    }
    str.replace(start_pos, substring.length(), destination);
    return true;
}

// Glob to regex
static std::string glob_to_regex(const std::string & pattern) {
    std::size_t i = 0, j, n = pattern.size();
    std::string result_string;

    while (i < n) {
        char c = pattern[i++];
        switch (c) {
            case '*' :
                result_string += ".*";
                break;
            case '?' :
                result_string += ".";
                break;
            case '[' :
                j = i;
                // skip "[]" and "[!]" (pattern with zero size)
                if (j < n && pattern[j] == '!') {
                    ++j;
                }
                if (j < n && pattern[j] == ']') {
                    ++j;
                }
                // find the closing "]"
                while (j < n && pattern[j] != ']') {
                    ++j;
                }
                if (j < n) {
                    std::string stuff = std::string(&pattern[i], j - i);
                    string_replace(stuff, "\\", "\\\\");
                    char first_char = pattern[i];
                    i = j + 1;
                    result_string += "[";
                    switch (first_char) {
                        case '!' :
                            result_string += "^" + stuff.substr(1);
                            break;
                        case '^' :
                            result_string += "\\" + stuff;
                            break;
                        default :
                            result_string += stuff;
                    }
                    result_string += "]";
                } else {
                    result_string += "\\[";
                }
                break;
            default :
                if (std::isalnum(c)) {
                    result_string += c;
                } else {
                    result_string += "\\";
                    result_string += c;
                }
        }
    }
    // assert end of string
    return "^" + result_string + "$";
}

// Check if current pathname contains wilcard characters
static bool has_magic(const std::string & pathname) {
    static const std::regex magic_check = std::regex("([*?[])");
    return std::regex_search(pathname, magic_check);
}

// Get list of files satisfying the pattern
std::vector<std::string> glob(const std::string & pattern) {
    // get base path
    std::filesystem::path base_path(pattern);
    while (has_magic(base_path.string())) {
        base_path = base_path.parent_path();
    }
    if (base_path.empty()) {
        base_path = std::filesystem::current_path();
    }
    // filter path with given pattern
    std::vector<std::string> result;
    if (!std::filesystem::is_directory(base_path)) {
        result.push_back(base_path.string());
        return result;
    }
    std::regex regex_pattern(glob_to_regex(pattern), std::regex::ECMAScript);
    for (const std::filesystem::directory_entry & d_entry : std::filesystem::recursive_directory_iterator(base_path)) {
        if (std::regex_match(d_entry.path().string(), regex_pattern)) {
            result.push_back(d_entry.path().string());
        }
    }
    return result;
}

}  // namespace readmpo
