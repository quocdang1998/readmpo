// Copyright 2023 quocdang1998
#include "readmpo/master_mpo.hpp"

#include <algorithm>  // std::copy, std::find, std::set_union, std::sort, std::unique
#include <fstream>
#include <iomanip>
#include <iostream>  // std::cout
#include <iterator>  // std::back_inserter
#include <set>       // std::set
#include <sstream>   // std::ostringstream
#include <utility>   // std::move

#include "readmpo/h5_utils.hpp"    // readmpo::is_near
#include "readmpo/serializer.hpp"  // readmpo::serialize_obj, readmpo::deserialize_obj

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
        std::set<std::string> mpo_isotopes = mpofile.get_isotopes();
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
    for (SingleMpo & mpofile : this->mpofiles_) {
        mpofile.close();
    }
    // get list of valid set for each isotope
    for (std::string & isotope : this->avail_isotopes_) {
        this->valid_set_[isotope] = ValidSet();
    }
    std::ofstream logfile("log_validset.txt");
    for (SingleMpo & mpofile : this->mpofiles_) {
        mpofile.reopen();
        mpofile.get_valid_set(this->valid_set_, logfile);
        mpofile.close();
    }
    std::cout << "Anisotropy order for each isotope(\n";
    std::cout << "isotope              max-diffsion-anisop-order max-scattering-anisop-order valid-in-out-idx-group\n";
    for (auto & [isotope, iso_validset] : this->valid_set_) {
        std::cout << std::setw(20) << isotope << " ";
        std::cout << std::setw(25) << std::get<0>(iso_validset) << " " << std::setw(27) << std::get<1>(iso_validset)
                  << " ";
        for (const std::pair<std::uint64_t, std::uint64_t> & p : std::get<2>(iso_validset)) {
            std::cout << "(" << p.first << " " << p.second << "), ";
        }
        std::cout << "\n";
    }
    std::cout << ")\n";
}

// Get list of MPOfile names
std::vector<std::string> MasterMpo::get_mpo_fnames(void) const {
    std::vector<std::string> mpo_fnames;
    mpo_fnames.resize(this->mpofiles_.size());
    for (std::uint64_t i_mpo = 0; i_mpo < mpo_fnames.size(); i_mpo++) {
        mpo_fnames[i_mpo] = this->mpofiles_[i_mpo].fname();
    }
    return mpo_fnames;
}

// Retrieve microscopic homogenized cross sections at some isotopes, reactions and skipped dimensions
MpoLib MasterMpo::build_microlib_xs(const std::vector<std::string> & isotopes,
                                    const std::vector<std::string> & reactions,
                                    const std::vector<std::string> & skipped_dims, XsType type,
                                    std::uint64_t max_anisop_order, const std::string & logfile) {
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
    std::vector<std::uint64_t> scattering_shape_lib(shape_lib);
    scattering_shape_lib[0] = 1;
    // allocate data for microlib
    MpoLib micro_lib;
    for (const std::string & isotope : isotopes) {
        for (const std::string & reaction : reactions) {
            if (reaction.compare("Diffusion") == 0) {
                std::uint64_t max_anisop = std::min(std::get<0>(this->valid_set_[isotope]), max_anisop_order);
                for (std::uint64_t anisop = 0; anisop < max_anisop; anisop++) {
                    micro_lib[isotope][stringify(reaction, anisop)] = NdArray(shape_lib);
                }
            } else if (reaction.compare("Scattering") == 0) {
                std::uint64_t max_anisop = std::min(std::get<1>(this->valid_set_[isotope]), max_anisop_order);
                for (std::uint64_t anisop = 0; anisop < max_anisop; anisop++) {
                    for (const std::pair<std::uint64_t, std::uint64_t> & p : std::get<2>(this->valid_set_[isotope])) {
                        micro_lib[isotope][stringify(reaction, anisop, '_', p.first, '-', p.second)] = NdArray(
                            scattering_shape_lib);
                    }
                }
            } else {
                micro_lib[isotope][reaction] = NdArray(shape_lib);
            }
        }
    }
    // retrieve data from each MPO file
    std::printf("\n");
    std::ofstream log(logfile.c_str());
    for (std::uint64_t i_fmpo = 0; i_fmpo < this->mpofiles_.size(); i_fmpo++) {
        this->mpofiles_[i_fmpo].reopen();
        this->mpofiles_[i_fmpo].get_microlib(isotopes, reactions, global_skipped_idims, this->valid_set_, micro_lib,
                                             type, max_anisop_order, log);
        print_process(static_cast<double>(i_fmpo) / static_cast<double>(this->mpofiles_.size()));
        this->mpofiles_[i_fmpo].close();
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
        mpofile.reopen();
        mpofile.get_concentration(isotopes, bu_idx, conc_lib);
        mpofile.close();
    }
    return conc_lib;
}

// Serialize
void MasterMpo::serialize(const std::string & fname) {
    std::ofstream out(fname.c_str());
    serialize_obj(out, this->geometry_);
    serialize_obj(out, this->energy_mesh_);
    serialize_obj(out, this->n_zone_);
    std::vector<std::string> mpo_fnames = this->get_mpo_fnames();
    serialize_obj(out, mpo_fnames);
    serialize_obj(out, this->master_pspace_);
    serialize_obj(out, this->avail_isotopes_);
    serialize_obj(out, this->avail_reactions_);
    serialize_obj(out, this->valid_set_);
}

// Deserialize
void MasterMpo::deserialize(const std::string & fname) {
    std::ifstream in(fname.c_str());
    deserialize_obj(in, this->geometry_);
    deserialize_obj(in, this->energy_mesh_);
    deserialize_obj(in, this->n_zone_);
    std::vector<std::string> mpo_fnames;
    deserialize_obj(in, mpo_fnames);
    deserialize_obj(in, this->master_pspace_);
    deserialize_obj(in, this->avail_isotopes_);
    deserialize_obj(in, this->avail_reactions_);
    deserialize_obj(in, this->valid_set_);
    for (const std::string & mpofile_name : mpo_fnames) {
        this->mpofiles_.push_back(SingleMpo(mpofile_name, this->geometry_, this->energy_mesh_));
        this->mpofiles_.back().construct_global_idx_map(this->master_pspace_);
        this->mpofiles_.back().close();
    }
}

// String representation
std::string MasterMpo::str(void) const {
    std::ostringstream out;
    out << "<MasterMpo:\n";
    out << "  Geometry: " << this->geometry_ << "\n";
    out << "  Emesh: " << this->energy_mesh_ << "\n";
    out << "  n_zone: " << this->n_zone_ << "\n";
    out << "  MPO list:\n";
    for (const SingleMpo & mpofile : this->mpofiles_) {
        out << "    " << mpofile.fname() << "\n";
    }
    out << "  pspace:\n";
    for (auto & [name, value] : this->master_pspace_) {
        out << "    " << name << "(" << value.size() << ") : " << value << "\n";
    }
    out << "  isotopes (" << this->avail_isotopes_.size() << "): " << this->avail_isotopes_ << "\n";
    out << "  reactions (" << this->avail_reactions_.size() << "): " << this->avail_reactions_ << "\n";
    out << "  validset:\n";
    for (auto & [isotope, iso_validset] : this->valid_set_) {
        out << "    " << isotope << ":\n";
        out << "      max-diffusion-anisop-order:" << std::get<0>(iso_validset) << "\n";
        out << "      max-scattering-anisop-order:" << std::get<1>(iso_validset) << "\n";
        out << "      departure-arrival-group-idx:\n";
        for (const std::pair<std::uint64_t, std::uint64_t> & p : std::get<2>(iso_validset)) {
            out << "        (" << p.first << ", " << p.second << ")\n";
        }
    }
    out << ">\n";
    return out.str();
}

// Load pickled data
void MasterMpo::set_state(const std::string & geometry, const std::string & energy_mesh, std::uint64_t n_zone,
                          const std::vector<std::string> & mpo_fnames,
                          const std::map<std::string, std::vector<double>> & master_pspace,
                          const std::vector<std::string> & isotopes, const std::vector<std::string> & reactions,
                          const std::map<std::string, ValidSet> & valid_set) {
    // copy other data
    this->geometry_ = geometry;
    this->energy_mesh_ = energy_mesh;
    this->n_zone_ = n_zone;
    this->master_pspace_ = master_pspace;
    this->avail_isotopes_ = isotopes;
    this->avail_reactions_ = reactions;
    this->valid_set_ = valid_set;
    // save each mpo to vector
    this->mpofiles_.reserve(mpo_fnames.size());
    for (const std::string & mpofile_name : mpo_fnames) {
        this->mpofiles_.push_back(SingleMpo(mpofile_name, geometry, energy_mesh));
        this->mpofiles_.back().construct_global_idx_map(this->master_pspace_);
        this->mpofiles_.back().close();
    }
}

}  // namespace readmpo
