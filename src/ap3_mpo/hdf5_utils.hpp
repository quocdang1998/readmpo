// Copyright 2022 quocdang1998
#ifndef AP3_MPO_HDF5_UTILS_HPP_
#define AP3_MPO_HDF5_UTILS_HPP_

#include <algorithm>    // std::min, std::transform
#include <cctype>       // std::tolower
#include <cinttypes>    // SCNu64
#include <cmath>        // std::fabs
#include <cstdint>      // std::uint64_t
#include <cstdio>       // std::snprintf, std::sscanf
#include <string>       // std::string
#include <type_traits>  // std::is_floating_point
#include <utility>      // std::pair
#include <vector>       // std::vector

#include "H5Cpp.h"  // H5::Group

#include "merlin/vector.hpp"  // merlin::intvec

namespace ap3_mpo {

/** @brief Check if 2 floats are near to one another.*/
template <typename T1, typename T2>
bool is_near(const T1 & a, const T2 & b) {
    if constexpr (std::is_floating_point<T1>::value || std::is_floating_point<T2>::value) {
        return std::fabs(a-b) <= 1e-4 * std::min(a,b);
    } else {
        return a == b;
    }
}

/** @brief Trim a string.*/
std::string & trim(std::string & s);

/** @brief Get lowercased string.*/
inline std::string lowercase(const std::string & s) {
    std::string result(s);
    std::transform(s.begin(), s.end(), result.begin(), [](char c) { return std::tolower(c); });
    return result;
}

/** @brief Append index as suffix of a string.*/
inline std::string append_suffix(const std::string & pattern, int suffix) {
    std::string result(pattern.size() + 5, '\0');  // suffix up to 99999
    std::snprintf(const_cast<char *>(result.c_str()), result.size(), "%s%d", pattern.c_str(), suffix);
    return result;
}

/** @brief Get suffix of a string in form of integer.*/
inline std::uint64_t get_suffix(const std::string & pattern, const std::string & prefix) {
    std::uint64_t result;
    std::string format = prefix + "%" + SCNu64;
    std::sscanf(pattern.c_str(), format.c_str(), &result);
    return result;
}

/** @brief Check if a string (case insensitive) is found in an array of string.*/
std::uint64_t check_string_in_array(std::string element, std::vector<std::string> array);

/** @brief List all subgroups and dataset of an HDF group, if their name contains the substring.*/
std::vector<std::string> ls_groups(H5::Group * group, const char * substring = "");

/** @brief Get data from an HDF dataset in form of an ``std::vector``.*/
template <typename T>
std::pair<std::vector<T>, merlin::intvec> get_dset(H5::Group * group, char const * dset_address);

/** @brief Get index of an element in array.*/
template <typename ArrayType, typename Sample>
std::uint64_t find_element(const std::vector<ArrayType> & array, const Sample & element);

}  // namespace ap3_mpo

#include "hdf5_utils.tpp"

#endif  // AP3_MPO_HDF5_UTILS_HPP_
