// Copyright 2023 quocdang1998
#ifndef READMPO_QUERY_MPO_HPP_
#define READMPO_QUERY_MPO_HPP_

#include <map>     // std::map
#include <vector>  // std::vector
#include <string>  // std::string

namespace readmpo {

/** @brief Read a MPO file and return geometry names and energy mesh names.
 *  @return A map with 2 keys (``emesh`` and ``geom``), each corresponding to a list of names of energy meshes and
 *  geometries found in the MPO file.
 */
std::map<std::string, std::vector<std::string>> query_mpo(const std::string & mpofile_name);

}  // namespace readmpo

#endif  // READMPO_QUERY_MPO_HPP_
