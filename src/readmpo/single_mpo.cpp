// Copyright 2023 quocdang1998
#include "readmpo/single_mpo.hpp"

#include <algorithm>  // std::find
#include <iostream>   // std::clog
#include <stdexcept>  // std::invalid_argument

#include "readmpo/h5_utils.hpp"  // readmpo::check_string_in_array, readmpo::get_dset, readmpo::ndim_to_c_idx,
                                 // readmpo::stringify, readmpo::lowercase, readmpo::trim, readmpo::is_near,
                                 // readmpo::ls_groups

namespace readmpo {

// Constructor from list of MPO file names, name of homogenized geometry and name of energy mesh
SingleMpo::SingleMpo(const std::string & mpofile_name, const std::string & geometry, const std::string & energy_mesh) {
    // get file pointer
    this->file_ = new H5::H5File(mpofile_name.c_str(), H5F_ACC_RDONLY);
    // get geometry ID and number of zones
    auto [geometry_names, n_geometry] = get_dset<std::string>(this->file_, "geometry/GEOMETRY_NAME");
    std::uint64_t geom_id = check_string_in_array(geometry, geometry_names);
    if (geom_id == UINT64_MAX) {
        throw std::invalid_argument("Geometry " + geometry + " not found in MPO file " + mpofile_name + ".\n");
    }
    auto [nzone, _nz] = get_dset<int>(this->file_, stringify("geometry/geometry_", geom_id, "/NZONE").c_str());
    this->n_zones = nzone[0];
    // get energy mesh ID and number of
    auto [emesh_names, n_emesh] = get_dset<std::string>(this->file_, "energymesh/ENERGYMESH_NAME");
    std::uint64_t emesh_id = check_string_in_array(energy_mesh, emesh_names);
    if (emesh_id == UINT64_MAX) {
        throw std::invalid_argument("Energymesh " + energy_mesh + " not found in MPO file " + mpofile_name + ".\n");
    }
    auto [ngroup, _ng] = get_dset<int>(this->file_, stringify("energymesh/energymesh_", emesh_id, "/NG").c_str());
    this->n_groups = ngroup[0];
    // get output ID
    auto [output_ids, outputid_shape] = get_dset<int>(this->file_, "output/OUPUTID");
    std::uint64_t output_idx = ndim_to_c_idx({geom_id, emesh_id}, outputid_shape);
    int output_id = output_ids[output_idx];
    if (output_id) {
        throw std::invalid_argument("The combination of energy mesh and geometry is not recorded in the MPO.\n");
    }
    // open output
    std::string output_name = stringify("output/output_", output_id);
    this->output_ = new H5::Group(this->file_->openGroup(output_name.c_str()));
    // get map of isotope name to its index
    auto [isotope_names, n_isotopes] = get_dset<std::string>(this->file_, "contents/isotopes/ISOTOPENAME");
    auto [i_isos, n_isos_output] = get_dset<int>(this->output_, "info/ISOTOPE");
    for (std::uint64_t i = 0; i < n_isos_output[0]; i++) {
        this->map_isotopes_[trim(isotope_names[i_isos[i]])] = i;
    }
    // get map of reaction name to its index
    auto [reaction_names, n_reactions] = get_dset<std::string>(this->file_, "contents/reactions/REACTIONAME");
    auto [i_reacs, n_reacs_output] = get_dset<int>(this->output_, "info/REACTION");
    for (std::uint64_t i = 0; i < n_reacs_output[0]; i++) {
        this->map_reactions_[trim(reaction_names[i_reacs[i]])] = i;
    }
}

// Get state parameters
std::map<std::string, std::vector<double>> SingleMpo::get_state_params(void) {
    // initialize result
    std::map<std::string, std::vector<double>> state_params;
    // get name of parameters
    auto [param_names, n_params] = get_dset<std::string>(this->file_, "parameters/info/PARAMNAME");
    // get value of each parameter
    for (std::uint64_t i_param = 0; i_param < n_params[0]; i_param++) {
        std::string param_id = stringify("parameters/values/PARAM_", i_param);
        auto [param_values, n_values] = get_dset<float>(this->file_, param_id.c_str());
        std::string param_name = lowercase(trim(param_names[i_param]));
        state_params[param_name] = std::vector<double>(param_values.begin(), param_values.end());
    }
    return state_params;
}

// Get parameter names
std::vector<std::string> SingleMpo::get_param_names(void) {
    auto [param_names, n_params] = get_dset<std::string>(this->file_, "parameters/info/PARAMNAME");
    for (std::string & param_name : param_names) {
        param_name = lowercase(trim(param_name));
    }
    return param_names;
}

// Get list of isotopes in MPO
std::vector<std::string> SingleMpo::get_isotopes(void) {
    std::vector<std::string> isotopes;
    for (auto & [isotope_name, idx_isot] : this->map_isotopes_) {
        isotopes.push_back(isotope_name);
    }
    return isotopes;
}

// Get list of reactions in MPO
std::vector<std::string> SingleMpo::get_reactions(void) {
    std::vector<std::string> reactions;
    for (auto & [reaction_name, idx_reac] : this->map_reactions_) {
        reactions.push_back(reaction_name);
    }
    return reactions;
}

// Construct map from local index to global index
void SingleMpo::construct_global_idx_map(const std::map<std::string, std::vector<double>> & master_pspace) {
    // get name of parameters
    auto [param_names, n_params] = get_dset<std::string>(this->file_, "parameters/info/PARAMNAME");
    this->map_global_idx_.resize(n_params[0]);
    // construct global index for each parameter
    for (std::uint64_t i_param = 0; i_param < n_params[0]; i_param++) {
        // get param name
        std::string param_name = lowercase(trim(param_names[i_param]));
        // get param value
        std::string param_id = stringify("parameters/values/PARAM_", i_param);
        auto [param_values, n_values] = get_dset<float>(this->file_, param_id.c_str());
        // initialize memory
        this->map_global_idx_[i_param].resize(n_values[0]);
        for (std::uint64_t i_value = 0; i_value < n_values[0]; i_value++) {
            double value = param_values[i_value];
            auto it = std::find_if(master_pspace.at(param_name).begin(), master_pspace.at(param_name).end(),
                                   [&value](const double & x) { return is_near(x, value); });
            this->map_global_idx_[i_param][i_value] = it - master_pspace.at(param_name).begin();
        }
    }
    // get map from local idim to global idim
    this->map_global_idim_.resize(n_params[0]);
    std::vector<std::string> local_param_names = this->get_param_names();
    std::uint64_t global_idim = 0;
    for (const auto & [param_name, param_values] : master_pspace) {
        auto it = std::find(local_param_names.begin(), local_param_names.end(), param_name);
        this->map_global_idim_[it - local_param_names.begin()] = global_idim;
        global_idim++;
    }
    // get map from global idim to local idim
    this->map_local_idim_.resize(n_params[0]);
    for (std::uint64_t i = 0; i < this->map_local_idim_.size(); i++) {
        this->map_local_idim_[this->map_global_idim_[i]] = i;
    }
}

// Retrieve microscopic homogenized cross section of an isotope and a reaction from MPO
void SingleMpo::retrieve_micro_xs(const std::string & isotope, const std::string & reaction,
                                  const std::vector<std::uint64_t> & skipped_dims, NdArray & output_data,
                                  XsType type, std::uint64_t anisotropy_order) {
    // check dimensionality
    if (output_data.ndim() - 2 != this->map_global_idx_.size() - skipped_dims.size()) {
        throw std::invalid_argument("Inconsistance of number of dimension between arguments.\n");
    }
    if ((output_data.shape()[0] != this->n_groups) || (output_data.shape()[1] != this->n_zones)) {
        throw std::invalid_argument("The first 2 dimensions of output data must be a NZONE and NG.\n");
    }
    // check if isotope and reaction is in MPO file
    if (!this->map_isotopes_.contains(isotope)) {
        std::clog << "This MPO does not contain the isotope " << isotope << "\n";
        return;
    }
    std::uint64_t isotope_idx = this->map_isotopes_[isotope];
    if (!this->map_reactions_.contains(reaction)) {
        std::clog << "This MPO does not contain the reaction " << reaction << "\n";
        return;
    }
    if (reaction.find("Scattering") != std::string::npos) {
        throw std::logic_error("Get Scattering cross section not implemented.\n");
    }
    std::uint64_t reaction_idx = this->map_reactions_[reaction];
    // if anisotropy order provided, reaction must be diffusion
    if ((anisotropy_order != 0) && (reaction.find("Diffusion") == std::string::npos)) {
        throw std::invalid_argument("Anisotropy order can only be provided with Diffusion reaction.\n");
    }
    // get addrxs (address of cross section)
    auto [addrxs, addrxs_shape] = get_dset<int>(this->output_, "info/ADDRXS");
    // loop over each statept
    std::vector<std::string> statepts = ls_groups(this->output_, "statept_");
    std::vector<std::uint64_t> output_index(output_data.ndim());
    std::vector<std::uint64_t> cross_section_idx = {0, isotope_idx, reaction_idx};
    std::vector<std::uint64_t> anisotropy_idx = {0, isotope_idx, this->map_reactions_.size()};
    for (std::string & statept_name : statepts) {
        // get statept
        H5::Group statept = this->output_->openGroup(statept_name.c_str());
        // get global index inside the output array
        auto [local_idx, total_ndim] = get_dset<int>(&statept, "PARAMVALUEORD");
        for (std::uint64_t idim_global = 0, write_idim = 2; idim_global < total_ndim[0]; idim_global++) {
            if (std::find(skipped_dims.begin(), skipped_dims.end(), idim_global) != skipped_dims.end()) {
                continue;
            }
            std::uint64_t idim_local = this->map_local_idim_[idim_global];
            std::uint64_t index_local = local_idx[idim_local];
            std::uint64_t index_global = this->map_global_idx_[idim_local][index_local];
            output_index[write_idim] = index_global;
            write_idim++;
        }
        // loop over each zone
        for (std::uint64_t i_zone = 0; i_zone < this->n_zones; i_zone++) {
            // open zone
            output_index[1] = i_zone;
            std::string zone_name = stringify("zone_", i_zone);
            H5::Group zone = statept.openGroup(zone_name.c_str());
            // get isotope concentration
            auto [concentrations, _n_concs] = get_dset<float>(&zone, "CONCENTRATION");
            double iso_conc = concentrations[isotope_idx];
            // get zone flux
            auto [zoneflux, _ng] = get_dset<float>(&zone, "ZONEFLUX");
            // get addrzx (address zone in cross section array)
            auto [addrzx_data, _naddrzx] = get_dset<int>(&zone, "ADDRZX");
            std::uint64_t addrzx = addrzx_data[0];
            cross_section_idx[0] = addrzx;
            anisotropy_idx[0] = addrzx;
            // open cross section
            auto [cross_sections, cross_section_shape] = get_dset<float>(&zone, "CROSSECTION");
            std::int64_t address_xs = addrxs[ndim_to_c_idx(cross_section_idx, addrxs_shape)];
            if (address_xs < 0) {
                std::clog << "Cross section not found for isotope " << isotope << " reaction " << reaction << "\n";
                continue;
            }
            // check for anisotropy order
            std::uint64_t max_anisotropy_order = addrxs[ndim_to_c_idx(anisotropy_idx, addrxs_shape)];
            if (anisotropy_order >= max_anisotropy_order) {
                throw std::invalid_argument("Anisotropy order bigger than max anisotropy order.\n");
            }
            address_xs += anisotropy_order * this->n_groups;
            // write cross section to data for each group
            for (std::uint64_t i_group = 0; i_group < this->n_groups; i_group++) {
                output_index[0] = i_group;
                if (output_data[output_index] != 0.0) {
                    std::clog << "Overwrite at index " << output_index << "\n";
                }
                switch (type) {
                    case XsType::Micro: {
                        output_data[output_index] = cross_sections[address_xs + i_group];
                        break;
                    }
                    case XsType::Macro: {
                        output_data[output_index] = iso_conc * cross_sections[address_xs + i_group];
                        break;
                    }
                    case XsType::Flux: {
                        output_data[output_index] = zoneflux[i_group];
                        break;
                    }
                    case XsType::ReactRate: {
                        output_data[output_index] = zoneflux[i_group] * iso_conc * cross_sections[address_xs + i_group];
                        break;
                    }
                }
            }
        }
    }
}

// Default destructor
SingleMpo::~SingleMpo(void) {
    if (this->output_ != nullptr) {
        delete this->output_;
    }
    if (this->file_ != nullptr) {
        delete this->file_;
    }
}

}  // namespace readmpo
