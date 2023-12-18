// Copyright 2023 quocdang1998
#include "readmpo/master_mpo.hpp"

#include <algorithm>  // std::copy, std::find, std::set_union, std::sort, std::unique
#include <iostream>   // std::cout
#include <iterator>   // std::back_inserter
#include <set>        // std::set
#include <utility>    // std::move

#include <omp.h>  // #pragma omp

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
}

// Retrieve microscopic homogenized cross sections at some isotopes, reactions and skipped dimensions
MpoLib MasterMpo::build_microlib_xs(const std::vector<std::string> & isotopes,
                                    const std::vector<std::string> & reactions,
                                    const std::vector<std::string> & skipped_dims, XsType type,
                                    std::uint64_t anisotropy_order) {
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
        micro_lib[isotope][reaction] = NdArray(shape_lib);
        }
    }
    // retrieve data from each MPO file
    #pragma omp parallel for
    for (std::int64_t i_fmpo = 0; i_fmpo < this->mpofiles_.size(); i_fmpo++) {
        SingleMpo & mpofile = this->mpofiles_[i_fmpo];
        for (const std::string & isotope : isotopes) {
            for (const std::string & reaction : reactions) {
                std::uint64_t anisop = 0;
                if (reaction.find("Diffusion") != std::string::npos) {
                    anisop = anisotropy_order;
                }
                mpofile.retrieve_micro_xs(isotope, reaction, global_skipped_idims, micro_lib[isotope][reaction], type,
                                          anisop);
            }
        }
    }
    return micro_lib;
}

}  // namespace readmpo
