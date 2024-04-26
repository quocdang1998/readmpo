// Copyright 2023 quocdang1998
#include "readmpo/master_mpo.hpp"

#include <algorithm>  // std::copy, std::find, std::set_union, std::sort, std::unique
#include <iostream>   // std::cout
#include <iomanip>
#include <iterator>   // std::back_inserter
#include <set>        // std::set
#include <utility>    // std::move

#include "readmpo/h5_utils.hpp"  // readmpo::is_near

namespace readmpo {

// Constructor from list of MPO file names, name of homogenized geometry and name of energy mesh
MasterMpo::MasterMpo(const std::vector<std::string> & mpofile_list, const std::string & geometry,
                     const std::string & energy_mesh) :
geometry_(geometry), energy_mesh_(energy_mesh) {
    // check for non empty geometry and isotope
    if (geometry.empty()) {
        throw std::invalid_argument("Empty geometry provided.\n");
    }
    if (energy_mesh.empty()) {
        throw std::invalid_argument("Empty energymesh provided.\n");
    }
    // reserve memory for child MPO
    if (mpofile_list.size() == 0) {
        throw std::invalid_argument("Empty MPO file list.\n");
    }
    this->mpofiles_.reserve(mpofile_list.size());
    // save each mpo to vector
    for (const std::string & mpofile_name : mpofile_list) {
        // save each mpo
        this->mpofiles_.push_back(SingleMpo(mpofile_name, geometry, energy_mesh));
        if (this->n_zone_ == 0) {
            this->n_zone_ = this->mpofiles_.back().n_zones;
        } else {
            if (this->n_zone_ != this->mpofiles_.back().n_zones) {
                throw std::invalid_argument("Inconsistent geometry across MPOs.\n");
            }
        }
    }
    // construct master parameter space
    for (SingleMpo & mpofile : this->mpofiles_) {
        // get pspace of each file
        std::map<std::string, std::vector<double>> file_pspace = mpofile.get_state_params();
        // merge each parameter to the master parameter
        for (auto & [pname, pvalues] : file_pspace) {
            // add name of parameter of not added
            if (!this->master_pspace_.contains(pname)) {
                this->master_pspace_[pname] = std::vector<double>();
            }
            // merge 2 ranges
            std::copy(pvalues.begin(), pvalues.end(), std::back_inserter(this->master_pspace_[pname]));
            std::sort(this->master_pspace_[pname].begin(), this->master_pspace_[pname].end());
            auto last = std::unique(this->master_pspace_[pname].begin(), this->master_pspace_[pname].end(), is_near);
            this->master_pspace_[pname].erase(last, this->master_pspace_[pname].end());
        }
    }
    for (auto & [name, value] : this->master_pspace_) {
        std::cout << name << "(" << value.size() << ") : " << value << "\n";
    }
    // calculate global index from local index
    for (SingleMpo & mpofile : this->mpofiles_) {
        mpofile.construct_global_idx_map(this->master_pspace_);
    }
    // get list of available isotopes
    std::set<std::string> set_isotopes;
    for (SingleMpo & mpofile : this->mpofiles_) {
        std::vector<std::string> mpo_isotopes = mpofile.get_isotopes();
        set_isotopes.insert(mpo_isotopes.begin(), mpo_isotopes.end());
    }
    std::copy(set_isotopes.begin(), set_isotopes.end(), std::back_inserter(this->avail_isotopes_));
    std::cout << "Avail isotopes (" << this->avail_isotopes_.size() << "): " << this->avail_isotopes_ << "\n";
    // get list of available reactions
    std::set<std::string> set_reactions;
    for (SingleMpo & mpofile : this->mpofiles_) {
        std::vector<std::string> mpo_reactions = mpofile.get_reactions();
        set_reactions.insert(mpo_reactions.begin(), mpo_reactions.end());
    }
    std::copy(set_reactions.begin(), set_reactions.end(), std::back_inserter(this->avail_reactions_));
    std::cout << "Avail reactions (" << this->avail_reactions_.size() << "): " << this->avail_reactions_ << "\n";
    // get list of valid set for each isotope
    std::cout << "Anisotropy order for each isotope(\n";
    std::cout << "isotope              max-diffsion-anisop-order max-scattering-anisop-order valid-in-out-idx-group\n";
    for (std::string & isotope : this->avail_isotopes_) {
        this->valid_set_[isotope] = ValidSet();
        ValidSet & iso_validset = this->valid_set_[isotope];
        std::cout << std::setw(20) << isotope << " ";
        for (SingleMpo & mpofile : this->mpofiles_) {
            ValidSet mpo_validset = mpofile.get_valid_set(isotope);
            std::get<0>(iso_validset) = std::max(std::get<0>(iso_validset), std::get<0>(mpo_validset));
            std::get<1>(iso_validset) = std::max(std::get<1>(iso_validset), std::get<1>(mpo_validset));
            std::get<2>(iso_validset).merge(std::get<2>(mpo_validset));
        }
        std::cout << std::setw(25) << std::get<0>(iso_validset) << " "
                  << std::setw(27) << std::get<1>(iso_validset) << " ";
        for (const std::pair<std::uint64_t, std::uint64_t> & p : std::get<2>(iso_validset)) {
            std::cout << "(" << p.first << " " << p.second << "), ";
        }
        std::cout << "\n";
    }
    std::cout << ")\n";
}

// Retrieve microscopic homogenized cross sections at some isotopes, reactions and skipped dimensions
MpoLib MasterMpo::build_microlib_xs(const std::vector<std::string> & isotopes,
                                    const std::vector<std::string> & reactions,
                                    const std::vector<std::string> & skipped_dims, XsType type) {
    // check isotope and reaction
    for (const std::string & isotope : isotopes) {
        auto it = std::find(this->avail_isotopes_.begin(), this->avail_isotopes_.end(), isotope);
        if (it == this->avail_isotopes_.end()) {
            throw std::invalid_argument(stringify("Isotope ", isotope, " not found.\n"));
        }
    }
    for (const std::string & reaction : reactions) {
        auto it = std::find(this->avail_reactions_.begin(), this->avail_reactions_.end(), reaction);
        if (it == this->avail_reactions_.end()) {
            throw std::invalid_argument(stringify("Reaction ", reaction, " not found.\n"));
        }
    }
    // get shape of each microlib
    std::vector<std::uint64_t> global_skipped_idims;
    std::vector<std::uint64_t> shape_lib;
    shape_lib.push_back(this->mpofiles_[0].n_groups);
    shape_lib.push_back(this->mpofiles_[0].n_zones);
    std::uint64_t idx_param = 0;
    for (auto & [param_name, param_values] : this->master_pspace_) {
        auto it = std::find(skipped_dims.begin(), skipped_dims.end(), param_name);
        if (it == skipped_dims.end()) {
            shape_lib.push_back(param_values.size());
        } else {
            global_skipped_idims.push_back(idx_param);
        }
        idx_param++;
    }
    // allocate data for microlib
    MpoLib micro_lib;
    for (const std::string & isotope : isotopes) {
        for (const std::string & reaction : reactions) {
            if (reaction.compare("Diffusion") == 0) {
                std::uint64_t max_anisop = std::get<0>(this->valid_set_[isotope]);
                for (std::uint64_t anisop = 0; anisop < max_anisop; anisop++) {
                    micro_lib[isotope][stringify(reaction, anisop)] = NdArray(shape_lib);
                }
            }
            else if (reaction.compare("Scattering") == 0) {
                std::uint64_t max_anisop = std::get<1>(this->valid_set_[isotope]);
                for (std::uint64_t anisop = 0; anisop < max_anisop; anisop++) {
                    for (const std::pair<std::uint64_t, std::uint64_t> & p : std::get<2>(this->valid_set_[isotope])) {
                        micro_lib[isotope][stringify(reaction, anisop, '_', p.first, '-', p.second)] = NdArray(shape_lib);
                    }
                }
            } else {
                micro_lib[isotope][reaction] = NdArray(shape_lib);
            }
        }
    }
    // retrieve data from each MPO file
    std::printf("\n");
    for (std::uint64_t i_fmpo = 0; i_fmpo < this->mpofiles_.size(); i_fmpo++) {
        this->mpofiles_[i_fmpo].get_microlib(isotopes, reactions, global_skipped_idims, this->valid_set_,
                                             micro_lib, type);
        print_process(static_cast<double>(i_fmpo) / static_cast<double>(this->mpofiles_.size()));
    }
    return micro_lib;
}

// Retrieve concentration of some isotopes at each value of burnup in each zone
ConcentrationLib MasterMpo::get_concentration(const std::vector<std::string> & isotopes,
                                              const std::string & burnup_name) {
    // check isotope
    for (const std::string & isotope : isotopes) {
        auto it = std::find(this->avail_isotopes_.begin(), this->avail_isotopes_.end(), isotope);
        if (it == this->avail_isotopes_.end()) {
            throw std::invalid_argument(stringify("Isotope ", isotope, " not found.\n"));
        }
    }
    // allocate data for concentration lib
    ConcentrationLib conc_lib;
    for (const std::string & isotope : isotopes) {
        conc_lib[isotope] = NdArray({this->master_pspace_.at(burnup_name).size(), this->n_zone_});
    }
    // get concentration of isotope from each MPO
    std::uint64_t bu_idx = std::distance(this->master_pspace_.begin(), this->master_pspace_.find(burnup_name));
    for (std::uint64_t i_fmpo = 0; i_fmpo < this->mpofiles_.size(); i_fmpo++) {
        SingleMpo & mpofile = this->mpofiles_[i_fmpo];
        for (const std::string & isotope : isotopes) {
            mpofile.get_concentration(isotope, bu_idx, conc_lib[isotope]);
        }
    }
    return conc_lib;
}

}  // namespace readmpo
