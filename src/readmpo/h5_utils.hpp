// Copyright 2023 quocdang1998
#ifndef READMPO_H5_UTILS_HPP_
#define READMPO_H5_UTILS_HPP_

#include <cstdint>  // std::uint64_t
#include <utility>  // std::pair
#include <string>  // std::string
#include <vector>  // std::vector

#include "H5Group.h"  // H5::Group

#include "merlin/vector.hpp"  // merlin::intvec

namespace readmpo {

// Utils for HDF5 read
// -------------------

/** @brief Get data from an HDF dataset in form of an ``std::vector``.
 *  @returns Contiguous array of data, and the shape of data.
 */
template <typename T>
std::pair<std::vector<T>, merlin::intvec> get_dset(H5::Group * group, char const * dset_address);

// Utils for string
// ----------------

/** @brief Trim a string.*/
std::string & trim(std::string & s);

}  // namespace readmpo

#include "readmpo/h5_utils.tpp"

#endif  // READMPO_H5_UTILS_HPP_
