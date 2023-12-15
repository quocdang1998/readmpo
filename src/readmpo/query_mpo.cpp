// Copyright 2023 quocdang1998
#include "readmpo/query_mpo.hpp"

#include "H5Cpp.h"  // H5::H5File

#include "readmpo/h5_utils.hpp"  // readmpo::get_dset, readmpo::trim

namespace readmpo {

// Read a MPO file and return geometry names and energy mesh names
std::map<std::string, std::vector<std::string>> query_mpo(const std::string & mpofile_name) {
    // initialize
    std::map<std::string, std::vector<std::string>> result;
    result["emesh"] = std::vector<std::string>();
    result["geom"] = std::vector<std::string>();
    // open file
    H5::H5File mpofile(mpofile_name.c_str(), H5F_ACC_RDONLY);
    // get geometry names
    auto [geometry_names, n_geometry] = get_dset<std::string>(&mpofile, "geometry/GEOMETRY_NAME");
    for (std::string & geometry_name: geometry_names) {
        result["geom"].push_back(trim(geometry_name));
    }
    // get energy mesh
    auto [emesh_names, n_emesh] = get_dset<std::string>(&mpofile, "energymesh/ENERGYMESH_NAME");
    for (std::string & emesh_name : emesh_names) {
        result["emesh"].push_back(trim(emesh_name));
    }
    return result;
}

}  // namespace readmpo
