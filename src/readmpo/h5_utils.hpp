// Copyright 2023 quocdang1998
#ifndef READMPO_H5_UTILS_HPP_
#define READMPO_H5_UTILS_HPP_

#include <algorithm>  // std::copy, std::min
#include <cmath>      // std::abs
#include <cstdint>    // std::uint64_t
#include <iterator>   // std::ostream_iterator
#include <string>     // std::string
#include <utility>    // std::pair
#include <vector>     // std::vector

#include <H5Cpp.h>  // H5::Group

namespace readmpo {

// Common utils
// ------------

/** @brief Check if 2 values are close to each other.*/
inline bool is_near(const double & a, const double & b) {
    return (std::abs(a - b) <= 1e-6 + 1e-5 * std::abs(std::min(a, b)));
};

// Stream operator
template <typename T>
std::ostream & operator<<(std::ostream & os, const std::vector<T> & v) {
    std::ostream_iterator<T> os_it(os, ", ");
    std::copy(v.begin(), v.end(), os_it);
    return os;
}

/** @brief Print percentage of the process.*/
void print_process(const double & percent);

// Utils for HDF5 read
// -------------------

/** @brief Get data from an HDF dataset in form of an ``std::vector``.
 *  @returns Contiguous array of data, and the shape of data.
 */
template <typename T>
std::pair<std::vector<T>, std::vector<std::uint64_t>> get_dset(H5::Group * group, const char * dset_address);

/** @brief List all subgroups and dataset of an HDF group, if their name contains the substring.*/
std::vector<std::string> ls_groups(H5::Group * group, const char * substring = "");

// Utils for string
// ----------------

/** @brief Trim a string.*/
std::string & trim(std::string & s);

/** @brief Get lowercased string.*/
inline std::string lowercase(const std::string & s) {
    std::string result(s);
    std::transform(s.begin(), s.end(), result.begin(), [](char c) { return std::tolower(c); });
    return result;
}

/** @brief Check if a string (case insensitive) is found in an array of string.
 *  @return Index of found element.
 */
std::uint64_t check_string_in_array(std::string element, std::vector<std::string> array);

/** @brief Merge streamable objects into a string.*/
template <typename... Args>
std::string stringify(const Args &... args);

// Utils for multi-dimensional array
// ---------------------------------

/** @brief Convert n-dimensional index to C-contiguous index.
 *  @param index Multi-dimensional index.
 *  @param shape Shape vector.
 *  @return C-contiguous index as an ``std::uint64_t``.
 */
std::uint64_t ndim_to_c_idx(const std::vector<std::uint64_t> & index, const std::vector<std::uint64_t> & shape);

}  // namespace readmpo

#include "readmpo/h5_utils.tpp"

#endif  // READMPO_H5_UTILS_HPP_
