// Copyright 2023 quocdang1998
#ifndef READMPO_MASTER_MPO_HPP_
#define READMPO_MASTER_MPO_HPP_

#include <map>     // std::map
#include <string>  // std::string
#include <vector>  // std::vector

#include "readmpo/nd_array.hpp"    // readmpo::NdArray
#include "readmpo/single_mpo.hpp"  // readmpo::SingleMpo, readmpo::XsType

namespace readmpo {

using MpoLib = std::map<std::string, std::map<std::string, NdArray>>;
using ConcentrationLib = std::map<std::string, NdArray>;

/** @brief Class containing merged information of all MPOs.*/
class MasterMpo {
  public:
    /// @name Constructor
    /// @{
    /** @brief Default constructor.*/
    MasterMpo(void) = default;
    /** @brief Constructor from list of MPO file names, name of homogenized geometry and name of energy mesh.*/
    MasterMpo(const std::vector<std::string> & mpofile_list, const std::string & geometry,
              const std::string & energy_mesh);
    /// @}

    /// @name Copy and move
    /// @{
    /** @brief Copy constructor.*/
    MasterMpo(const MasterMpo & src) = delete;
    /** @brief Copy assignment.*/
    MasterMpo & operator=(const MasterMpo & src) = delete;
    /// @}

    /// @name Attributes
    /// @{
    /** @brief Get merged parameter space.*/
    constexpr const std::map<std::string, std::vector<double>> & master_pspace(void) const noexcept {
        return this->master_pspace_;
    }
    /** @brief Get available isotopes.*/
    constexpr const std::vector<std::string> & get_isotopes(void) const noexcept { return this->avail_isotopes_; }
    /** @brief Get available reactions.*/
    constexpr const std::vector<std::string> & get_reactions(void) const noexcept { return this->avail_reactions_; }
    /// @}

    /// @name Retrieve data from MPO
    /// @{
    /** @brief Retrieve microscopic homogenized cross sections at some isotopes, reactions and skipped dimensions in
     *  all MPO files.
     *  @param isotopes List of isotopes.
     *  @param reactions List of reactions.
     *  @param skipped_dims List of lowercased skipped dimension.
     *  @param type Cross section type to get.
     *  @param anisotropy_order Anisotropy order to get for diffusion cross section.
     */
    MpoLib build_microlib_xs(const std::vector<std::string> & isotopes, const std::vector<std::string> & reactions,
                             const std::vector<std::string> & skipped_dims, XsType type = XsType::Micro,
                             std::uint64_t anisotropy_order = 0);
    /** @brief Retrieve concentration of some isotopes at each value of burnup in each zone.
     *  @param isotopes List of isotopes.
     *  @param burnup_name Name of parameter representing burnup.
     */
    ConcentrationLib get_concentration(const std::vector<std::string> & isotopes,
                                       const std::string & burnup_name = "burnup");
    /// @}

  protected:
    /** @brief Name of geometry.*/
    std::string geometry_;
    /** @brief Name of energy.*/
    std::string energy_mesh_;
    /** @brief Number of zone.*/
    std::uint16_t n_zone_ = 0;

    /** @brief List of MPO files.*/
    std::vector<SingleMpo> mpofiles_;

    /** @brief Merged parameter space.*/
    std::map<std::string, std::vector<double>> master_pspace_;
    /** @brief Possible isotopes.*/
    std::vector<std::string> avail_isotopes_;
    /** @brief Possible reactions.*/
    std::vector<std::string> avail_reactions_;
};

}  // namespace readmpo

#endif  // READMPO_MASTER_MPO_HPP_
