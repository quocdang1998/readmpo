// Copyright 2023 quocdang1998
#ifndef READMPO_H5_UTILS_HPP_
#define READMPO_H5_UTILS_HPP_

#include <algorithm>  // std::find_if
#include <cctype>  // std::isspace
#include <locale>  // std::locale

namespace readmpo {

// ---------------------------------------------------------------------------------------------------------------------
// Utils for string
// ---------------------------------------------------------------------------------------------------------------------

// Left trim a string
static std::string & ltrim(std::string & s) {
    auto it = std::find_if(s.begin(), s.end(), [](char c) { return !std::isspace<char>(c, std::locale::classic()); });
    s.erase(s.begin(), it);
    return s;
}

// Right trim a string
static std::string & rtrim(std::string & s) {
    auto it = std::find_if(s.rbegin(), s.rend(), [](char c) { return !std::isspace<char>(c, std::locale::classic()); });
    s.erase(it.base(), s.end());
    return s;
}

// Trim a string.
std::string & trim(std::string & s) { return ltrim(rtrim(s)); }

}  // namespace readmpo

#include "readmpo/h5_utils.tpp"

#endif  // READMPO_H5_UTILS_HPP_
