// Copyright 2023 quocdang1998
#include "readmpo/master_mpo.hpp"

namespace readmpo {

// Constructor from list of MPO file names, name of homogenized geometry and name of energy mesh
MasterMpo::MasterMpo(const std::vector<std::string> & mpofile_list, const std::string & geometry,
                     const std::string & energy_mesh) : geometry_(geometry), energy_mesh_(energy_mesh) {
    // open each file in the list
    for (const std::string & mpofile_name : mpofile_list) {
        H5::H5File * mpofile = new H5::H5File(filename.c_str(), H5F_ACC_RDONLY);
        H5::Group root = mpofile->openGroup("/");
        // read geometries
        std::printf("    Geometries ");
        auto [geometry_names, n_geometry] = get_dset<std::string>(&root, "geometry/GEOMETRY_NAME");
        std::printf("[%zu] :", geometry_names.size());
        for (std::string & geometry_name : geometry_names) {
            std::printf(" %s", trim(geometry_name).c_str());
        }
        std::printf("\n");
        // read energy meshes
        std::printf("    Energy meshes ");
        auto [energymesh_names, n_energymesh] = get_dset<std::string>(&root, "energymesh/ENERGYMESH_NAME");
        std::printf("[%zu] :", energymesh_names.size());
        for (int i_emesh = 0; i_emesh < energymesh_names.size(); i_emesh++) {
            std::string & energymesh_name = energymesh_names[i_emesh];
            std::printf(" %s", trim(energymesh_name).c_str());
            std::string emesh_group_name = append_suffix("energymesh/energymesh_", i_emesh);
            H5::Group emesh_group = root.openGroup(emesh_group_name.c_str());
            auto [n_group, _] = get_dset<int>(&emesh_group, "NG");
            std::printf("[%d]", n_group[0]);
        }
        std::printf("\n");
    }
}

}  // namespace readmpo
