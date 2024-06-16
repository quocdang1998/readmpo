// Copyright 2023 quocdang1998
#include "readmpo/single_mpo.hpp"

#include <algorithm>  // std::find, std::max
#include <iostream>   // std::clog
#include <sstream>    // std::ostringstream
#include <stdexcept>  // std::invalid_argument


#include "readmpo/h5_utils.hpp"  // readmpo::check_string_in_array, readmpo::get_dset, readmpo::ndim_to_c_idx,
                                 // readmpo::stringify, readmpo::lowercase, readmpo::trim, readmpo::is_near,
                                 // readmpo::ls_groups

namespace readmpo {

// Get cross section from type
static void get_xs(std::uint64_t ngroups, std::vector<std::uint64_t> & output_index, std::int64_t address_xs,
                   NdArray & output_data, XsType type, const std::vector<float> & cross_sections,
                   const std::vector<float> & zoneflux, double iso_conc) {
    for (std::uint64_t i_group = 0; i_group < ngroups; i_group++) {
        output_index[0] = i_group;
        if (output_data[output_index] != 0.0) {
            std::clog << "Overwrite at index " << output_index << "\n";
        }
        switch (type) {
            case XsType::Micro : {
                output_data[output_index] = cross_sections[address_xs + i_group];
                break;
            }
            case XsType::Macro : {
                output_data[output_index] = iso_conc * cross_sections[address_xs + i_group];
                break;
            }
            case XsType::Flux : {
                output_data[output_index] = zoneflux[i_group];
                break;
            }
            case XsType::ReactRate : {
                output_data[output_index] = zoneflux[i_group] * iso_conc * cross_sections[address_xs + i_group];
                break;
            }
        }
    }
}

std::ostream & operator<<(std::ostream & os, const ValidSet & v) {
    os << std::get<0>(v) << " " << std::get<1>(v);
    return os;
}

// Constructor from list of MPO file names, name of homogenized geometry and name of energy mesh
SingleMpo::SingleMpo(const std::string & mpofile_name, const std::string & geometry, const std::string & energy_mesh) {
    // get file pointer
    this->fname_ = mpofile_name;
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
    this->output_name_ = stringify("output/output_", output_id);
    this->output_ = new H5::Group(this->file_->openGroup(this->output_name_.c_str()));
    // get map of isotope name to its index
    auto [isotope_names, n_isotopes] = get_dset<std::string>(this->file_, "contents/isotopes/ISOTOPENAME");
    auto [addriso, n_addrz] = get_dset<int>(this->output_, "info/ADDRISO");
    auto [i_isos, n_isos_output] = get_dset<int>(this->output_, "info/ISOTOPE");
    for (std::uint64_t zone_idx = 0; zone_idx < addriso.size() - 1; zone_idx++) {
        std::uint64_t start_idx = addriso[zone_idx], end_idx = addriso[zone_idx + 1];
        std::map<std::string, std::uint64_t> zone_map_iso;
        for (std::uint64_t i = start_idx; i < end_idx; i++) {
            zone_map_iso[trim(isotope_names[i_isos[i]])] = i - start_idx;
        }
        this->map_isotopes_.push_back(zone_map_iso);
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
std::set<std::string> SingleMpo::get_isotopes(void) {
    std::set<std::string> isotopes;
    for (auto & isotope_map_per_zone : this->map_isotopes_) {
        for (auto & [isotope_name, idx_isot] : isotope_map_per_zone) {
            isotopes.insert(isotope_name);
        }
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

// Get valid parameter set for Diffusion and Scattering reactions
void SingleMpo::get_valid_set(std::map<std::string, ValidSet> & global_valid_set, std::ofstream & logfile) {
    logfile << "Reading " << this->fname_ << ":";
    logfile.flush();
    // get addrxs and transprofile
    auto [addrxs, addrxs_shape] = get_dset<int>(this->output_, "info/ADDRXS");
    auto [transprofile, transprf_shape] = get_dset<int>(this->output_, "info/TRANSPROFILE");
    // initialize index
    std::vector<std::uint64_t> ndiffusion_idx = {0, 0, this->map_reactions_.size()};
    std::vector<std::uint64_t> ntransfer_idx = {0, 0, this->map_reactions_.size() + 1};
    std::vector<std::uint64_t> scaterring_adrr_idx = {0, 0, this->map_reactions_.size() + 2};
    // loop over each statept
    std::vector<std::string> statepts = ls_groups(this->output_, "statept_");
    for (std::string & statept_name : statepts) {
        // get statept
        logfile << " " << statept_name;
        logfile.flush();
        H5::Group statept = this->output_->openGroup(statept_name.c_str());
        // loop over each zone
        for (std::uint64_t i_zone = 0; i_zone < this->n_zones; i_zone++) {
            // open zone
            std::string zone_name = stringify("zone_", i_zone);
            H5::Group zone = statept.openGroup(zone_name.c_str());
            // get addrzx
            auto [addrzx_data, _naddrzx] = get_dset<int>(&zone, "ADDRZX");
            std::uint64_t addrzx = addrzx_data[0];
            // set zone index for each index vector
            ndiffusion_idx[0] = addrzx;
            ntransfer_idx[0] = addrzx;
            scaterring_adrr_idx[0] = addrzx;
            // loop on each isotope
            auto [addrzi_data, _naddrzi] = get_dset<int>(&zone, "ADDRZI");
            std::uint64_t addrzi = addrzi_data[0];
            std::map<std::string, std::uint64_t> & map_iso_zone = this->map_isotopes_[addrzi];
            for (auto & [isotope, valid_set] : global_valid_set) {
                // set isotope index
                if (!map_iso_zone.contains(isotope)) {
                    continue;
                }
                std::uint64_t isotope_idx = map_iso_zone[isotope];
                ndiffusion_idx[1] = isotope_idx;
                ntransfer_idx[1] = isotope_idx;
                scaterring_adrr_idx[1] = isotope_idx;
                // get max anisotropy order for Diffusion and Scattering
                int diffusion_max_order = addrxs[ndim_to_c_idx(ndiffusion_idx, addrxs_shape)];
                int scattering_max_order = addrxs[ndim_to_c_idx(ntransfer_idx, addrxs_shape)];
                if (diffusion_max_order < 0 && scattering_max_order < 0) {
                    continue;  // skip because the isotope do not present in this zone
                }
                std::get<0>(valid_set) = std::max(diffusion_max_order, static_cast<int>(std::get<0>(valid_set)));
                std::get<1>(valid_set) = std::max(scattering_max_order, static_cast<int>(std::get<1>(valid_set)));
                // get first arrival group and adr per arrival group start from TRANSPROFILE
                std::uint64_t index_in_tf = addrxs[ndim_to_c_idx(scaterring_adrr_idx, addrxs_shape)];
                std::vector<int> trans_fag(transprofile.data() + index_in_tf,
                                           transprofile.data() + index_in_tf + this->n_groups);
                std::vector<int> trans_adr(transprofile.data() + index_in_tf + this->n_groups,
                                           transprofile.data() + index_in_tf + 2 * this->n_groups + 1);
                // loop for each departure group and arrival group
                for (std::uint64_t departure_gridx = 0; departure_gridx < this->n_groups; departure_gridx++) {
                    for (std::uint64_t arrival_gridx = 0; arrival_gridx < this->n_groups; arrival_gridx++) {
                        int scale = trans_adr[departure_gridx] + static_cast<int>(arrival_gridx) -
                                    trans_fag[departure_gridx];
                        if ((trans_adr[departure_gridx] <= scale) && (scale < trans_adr[departure_gridx + 1])) {
                            std::get<2>(valid_set).insert(std::make_pair(departure_gridx, arrival_gridx));
                        }
                    }
                }
            }
        }
    }
    logfile << "\n";
    logfile.flush();
}

// Retrieve microscopic homogenized cross section of an isotope and a reaction from MPO
void SingleMpo::get_microlib(const std::vector<std::string> & isotopes, const std::vector<std::string> & reactions,
                             const std::vector<std::uint64_t> & global_skipped_dims,
                             const std::map<std::string, ValidSet> & global_valid_set,
                             std::map<std::string, std::map<std::string, NdArray>> & micro_lib, XsType type,
                             std::uint64_t max_anisop_order, std::ofstream & logfile) {
    logfile << "Rettrieving " << this->fname_ << ":";
    logfile.flush();
    // check for isotope and reaction
    std::set<std::string> mpo_isotopes = this->get_isotopes();
    for (const std::string & isotope : isotopes) {
        for (const std::string & reaction : reactions) {
            // check if isotope and reaction is in MPO file
            if (mpo_isotopes.find(isotope) == mpo_isotopes.end()) {
                std::clog << "This MPO does not contain the isotope " << isotope << ". No isotope will be retrived\n";
                return;
            }
            if (!this->map_reactions_.contains(reaction)) {
                std::clog << "This MPO does not contain the reaction " << reaction << ". \n";
                return;
            }
        }
    }
    // get addrxs (address of cross section) and transprofile
    auto [addrxs, addrxs_shape] = get_dset<int>(this->output_, "info/ADDRXS");
    auto [transprofile, transprf_shape] = get_dset<int>(this->output_, "info/TRANSPROFILE");
    // initialize memory for index
    std::vector<std::uint64_t> output_index(this->map_global_idx_.size() - global_skipped_dims.size() + 2);
    std::vector<std::uint64_t> cross_section_idx = {0, 0, 0};
    std::vector<std::uint64_t> ndiffusion_idx = {0, 0, this->map_reactions_.size()};
    std::vector<std::uint64_t> ntransfer_idx = {0, 0, this->map_reactions_.size() + 1};
    std::vector<std::uint64_t> scaterring_adrr_idx = {0, 0, this->map_reactions_.size() + 2};
    // loop on each statepoint
    std::vector<std::string> statepts = ls_groups(this->output_, "statept_");
    for (std::string & statept_name : statepts) {
        // get statept
        logfile << " " << statept_name;
        logfile.flush();
        H5::Group statept = this->output_->openGroup(statept_name.c_str());
        // get global index inside the output array
        auto [local_idx, total_ndim] = get_dset<int>(&statept, "PARAMVALUEORD");
        for (std::uint64_t idim_global = 0, write_idim = 2; idim_global < total_ndim[0]; idim_global++) {
            if (std::find(global_skipped_dims.begin(), global_skipped_dims.end(), idim_global) !=
                global_skipped_dims.end()) {
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
            // get concentration, flux, addrzx and cross sections of all isotopes and reactions
            auto [concentrations, _n_concs] = get_dset<float>(&zone, "CONCENTRATION");
            auto [zoneflux, _ng] = get_dset<float>(&zone, "ZONEFLUX");
            auto [addrzx_data, _naddrzx] = get_dset<int>(&zone, "ADDRZX");
            auto [cross_sections, cross_section_shape] = get_dset<float>(&zone, "CROSSECTION");
            // set zone index
            std::uint64_t addrzx = addrzx_data[0];
            cross_section_idx[0] = addrzx;
            ndiffusion_idx[0] = addrzx;
            ntransfer_idx[0] = addrzx;
            scaterring_adrr_idx[0] = addrzx;
            auto [addrzi_data, _naddrzi] = get_dset<int>(&zone, "ADDRZI");
            std::uint64_t addrzi = addrzi_data[0];
            // retrive for each isotope
            for (const std::string & isotope : isotopes) {
                // check if isotope present
                if (!this->map_isotopes_[addrzi].contains(isotope)) {
                    continue;
                }
                // set isotope index
                std::uint64_t isotope_idx = this->map_isotopes_[addrzi][isotope];
                cross_section_idx[1] = isotope_idx;
                ndiffusion_idx[1] = isotope_idx;
                ntransfer_idx[1] = isotope_idx;
                scaterring_adrr_idx[1] = isotope_idx;
                // get isotope concentration
                double iso_conc = concentrations[isotope_idx];
                // get first arrival group and adr per arrival group start from TRANSPROFILE
                std::uint64_t index_in_tf = addrxs[ndim_to_c_idx(scaterring_adrr_idx, addrxs_shape)];
                std::vector<int> trans_fag(transprofile.data() + index_in_tf,
                                           transprofile.data() + index_in_tf + this->n_groups);
                std::vector<int> trans_adr(transprofile.data() + index_in_tf + this->n_groups,
                                           transprofile.data() + index_in_tf + 2 * this->n_groups + 1);
                // get valid set
                const ValidSet & valid_set = global_valid_set.at(isotope);
                // retrive for each reaction
                for (const std::string & reaction : reactions) {
                    // set reaction index
                    std::uint64_t reaction_idx = this->map_reactions_[reaction];
                    cross_section_idx[2] = reaction_idx;
                    // calculate index in the cross section array
                    std::int64_t address_xs = addrxs[ndim_to_c_idx(cross_section_idx, addrxs_shape)];
                    if (address_xs < 0) {
                        // std::clog << "Cross section not found for isotope " << isotope << " reaction " << reaction
                        //         << " state point " << statept_name << " in zone " << i_zone << "\n";
                        continue;
                    }
                    // get cross section
                    if (reaction.compare("Diffusion") == 0) {
                        // get cross section for Diffusion
                        std::uint64_t max_anisop = std::min(std::get<0>(valid_set), max_anisop_order);
                        for (std::uint64_t anisop = 0; anisop < max_anisop; anisop++) {
                            NdArray & output_data = micro_lib[isotope][stringify(reaction, anisop)];
                            std::int64_t adr_xs = address_xs + anisop * this->n_groups;
                            get_xs(this->n_groups, output_index, adr_xs, output_data, type, cross_sections, zoneflux,
                                   iso_conc);
                        }
                    } else if (reaction.compare("Scattering") == 0) {
                        // get cross section for Scattering
                        std::uint64_t max_anisop = std::min(std::get<1>(valid_set), max_anisop_order);
                        for (std::uint64_t anisop = 0; anisop < max_anisop; anisop++) {
                            for (const std::pair<std::uint64_t, std::uint64_t> & p : std::get<2>(valid_set)) {
                                NdArray & output_data =
                                    micro_lib[isotope][stringify(reaction, anisop, '_', p.first, '-', p.second)];
                                int scale = trans_adr[p.first] + static_cast<int>(p.second) - trans_fag[p.first];
                                std::int64_t adr_xs = address_xs + anisop * this->n_groups + scale;
                                get_xs(1, output_index, adr_xs, output_data, type, cross_sections,
                                       zoneflux, iso_conc);
                            }
                        }
                    } else {
                        // get cross section for others reaction
                        NdArray & output_data = micro_lib[isotope][reaction];
                        get_xs(this->n_groups, output_index, address_xs, output_data, type, cross_sections, zoneflux,
                               iso_conc);
                    }
                }
            }
        }
    }
    logfile << "\n";
    logfile.flush();
}

// Retrieve concentration from MPO
void SingleMpo::get_concentration(const std::vector<std::string> & isotopes, std::uint64_t burnup_i_dim,
                                  std::map<std::string, NdArray> & output) {
    // check if isotope is in MPO file
    std::set<std::string> mpo_isotopes = this->get_isotopes();
    for (const std::string & isotope : isotopes) {
        if (mpo_isotopes.find(isotope) == mpo_isotopes.end()) {
            std::clog << "This MPO does not contain the isotope " << isotope << "\n";
            return;
        }
    }
    // loop over each statept
    std::vector<std::uint64_t> output_index(2);
    std::vector<std::string> statepts = ls_groups(this->output_, "statept_");
    for (std::string & statept_name : statepts) {
        // get statept
        H5::Group statept = this->output_->openGroup(statept_name.c_str());
        // get global index inside the output array
        auto [local_idx, total_ndim] = get_dset<int>(&statept, "PARAMVALUEORD");
        std::uint64_t idim_local = this->map_local_idim_[burnup_i_dim];
        std::uint64_t index_local = local_idx[idim_local];
        std::uint64_t index_global = this->map_global_idx_[idim_local][index_local];
        output_index[0] = index_global;
        // loop over each zone
        for (std::uint64_t i_zone = 0; i_zone < this->n_zones; i_zone++) {
            // open zone
            output_index[1] = i_zone;
            std::string zone_name = stringify("zone_", i_zone);
            H5::Group zone = statept.openGroup(zone_name.c_str());
            auto [addrzi_data, _naddrzi] = get_dset<int>(&zone, "ADDRZI");
            std::uint64_t addrzi = addrzi_data[0];
            // get isotope concentration
            auto [concentrations, _n_concs] = get_dset<float>(&zone, "CONCENTRATION");
            for (const std::string & isotope : isotopes) {
                std::map<std::string, std::uint64_t> & iso_map = this->map_isotopes_[addrzi];
                if (!iso_map.contains(isotope)) {
                    continue;
                }
                std::uint64_t isotope_idx = iso_map[isotope];
                output[isotope][output_index] = concentrations[isotope_idx];
            }
        }
    }
}

// String representation
std::string SingleMpo::str(void) const {
    std::ostringstream os;
    os << "<MpoFile \"" << this->fname_ << "\" output \"" << this->output_name_ << "\">";
    return os.str();
}

// Close file and flush memory
void SingleMpo::close(void) {
    delete this->output_;
    this->output_ = nullptr;
    delete this->file_;
    this->file_ = nullptr;
}

// Reopen file
void SingleMpo::reopen(void) {
    this->file_ = new H5::H5File(this->fname_.c_str(), H5F_ACC_RDONLY);
    this->output_ = new H5::Group(this->file_->openGroup(this->output_name_.c_str()));
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
