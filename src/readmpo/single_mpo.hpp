// Copyright 2023 quocdang1998
#ifndef READMPO_SINGLE_MPO_HPP_
#define READMPO_SINGLE_MPO_HPP_

#include <map>      // std::map
#include <string>   // std::string
#include <utility>  // std::exchange
#include <vector>   // std::vector

#include "H5Cpp.h"  // H5::H5File, H5::Group

#include "readmpo/nd_array.hpp"  // readmpo::NdArray

namespace readmpo {

/** @brief Physical quantity to calculate.*/
enum class XsType : unsigned int {
    /** @brief Microscopic cross section.*/
    Micro = 0,
    /** @brief Macroscopic cross section.*/
    Macro = 1,
    /** @brief Neutron flux.*/
    Flux = 2,
    /** @brief Reaction rate.*/
    ReactRate = 3
};

/** @brief Class representing a single output ID inside an MPO.*/
class SingleMpo {
  public:
    /// @name Constructor
    /// @{
    /** @brief Default constructor.*/
    SingleMpo(void) = default;
    /** @brief Constructor from list of MPO file names, name of homogenized geometry and name of energy mesh.*/
    SingleMpo(const std::string & mpofile_name, const std::string & geometry, const std::string & energy_mesh);
    /// @}

    /// @name Copy and move
    /// @{
    /** @brief Copy constructor.*/
    SingleMpo(const SingleMpo & src) = delete;
    /** @brief Copy assignment.*/
    SingleMpo & operator=(const SingleMpo & src) = delete;
    /** @brief Move constructor.*/
    SingleMpo(SingleMpo && src) :
    n_zones(src.n_zones),
    n_groups(src.n_groups),
    fname_(src.fname_),
    output_name_(src.output_name_),
    map_global_idx_(std::move(src.map_global_idx_)),
    map_global_idim_(std::move(src.map_global_idim_)),
    map_local_idim_(std::move(src.map_local_idim_)),
    map_isotopes_(std::move(src.map_isotopes_)),
    map_reactions_(std::move(src.map_reactions_)) {
        this->file_ = std::exchange(src.file_, nullptr);
        this->output_ = std::exchange(src.output_, nullptr);
    }
    /** @brief Move assignment.*/
    SingleMpo & operator=(SingleMpo && src) {
        this->n_zones = std::exchange(src.n_zones, 0);
        this->n_groups = std::exchange(src.n_groups, 0);
        this->fname_ = std::exchange(src.fname_, std::string());
        this->file_ = std::exchange(src.file_, nullptr);
        this->output_name_ = std::exchange(src.output_name_, std::string());
        this->output_ = std::exchange(src.output_, nullptr);
        this->map_global_idx_ = std::exchange(src.map_global_idx_, std::vector<std::vector<std::uint64_t>>());
        this->map_global_idim_ = std::exchange(src.map_global_idim_, std::vector<std::uint64_t>());
        this->map_local_idim_ = std::exchange(src.map_local_idim_, std::vector<std::uint64_t>());
        this->map_isotopes_ = std::exchange(src.map_isotopes_, std::map<std::string, std::uint64_t>());
        this->map_reactions_ = std::exchange(src.map_reactions_, std::map<std::string, std::uint64_t>());
        return *this;
    }
    /// @}

    /// @name Attributes
    /// @{
    /** @brief Get state parameters.*/
    std::map<std::string, std::vector<double>> get_state_params(void);
    /** @brief Get lowercased parameter names.*/
    std::vector<std::string> get_param_names(void);
    /** @brief Get list of isotopes in MPO.*/
    std::vector<std::string> get_isotopes(void);
    /** @brief Get list of reactions in MPO.*/
    std::vector<std::string> get_reactions(void);
    /** @brief Number of zones in the geometry.*/
    std::uint64_t n_zones;
    /** @brief Number of groups in the energy mesh.*/
    std::uint64_t n_groups;
    /// @}

    /// @name Global indexing
    /// @{
    /** @brief Construct map from local index to global index.*/
    void construct_global_idx_map(const std::map<std::string, std::vector<double>> & master_pspace);
    /// @}

    /// @name Retrieve data from MPO
    /// @{
    /** @brief Retrieve microscopic homogenized cross section of an isotope and a reaction from MPO.
     *  @param isotope Isotope to get.
     *  @param reaction Reaction to get.
     *  @param skipped_dims Dimensions (0-base indexed) to ignore. Once ignore, elements at any indexes will be rewrite
     *  at index 0.
     *  @param output_data Multi-dimensional array to write result to. Its dimensions mut be equals to the number of
     *  dimensions in the parameter space minus the number of element in the argument ``skipped_dims``.
     *  @param type Type of cross section to retrieve.
     *  @param anisotropy_order Anisotropy order to get for diffusion cross section.
     */
    void retrieve_micro_xs(const std::string & isotope, const std::string & reaction,
                           const std::vector<std::uint64_t> & skipped_dims, NdArray & output_data,
                           XsType type = XsType::Micro, std::uint64_t anisotropy_order = 0);
    /** @brief Retrieve concentration from MPO.
     *  @param isotope Isotope to get.
     *  @param burnup_i_dim Index of burnup axis.
     *  @param output Output array to write result to.
     */
    void get_concentration(const std::string & isotope, std::uint64_t burnup_i_dim, NdArray & output);
    /// @}

    /// @name Representation
    /// @{
    /** @brief String representation.*/
    std::string str(void) const;
    /// @}

    /// @name Destructor
    /// @{
    /** @brief Default destructor.*/
    ~SingleMpo(void);
    /// @}

  protected:
    /** @brief Name of the file.*/
    std::string fname_;
    /** @brief Pointer to H5 file.*/
    H5::H5File * file_ = nullptr;
    /** @brief Name of the output.*/
    std::string output_name_;
    /** @brief Pointer to the H5 group of ``output``.*/
    H5::Group * output_ = nullptr;

    /** @brief Map from local index to global index.*/
    std::vector<std::vector<std::uint64_t>> map_global_idx_;
    /** @brief Map from local i_dim to global i_dim.*/
    std::vector<std::uint64_t> map_global_idim_;
    /** @brief Map from global i_dim to local i_dim.*/
    std::vector<std::uint64_t> map_local_idim_;
    /** @brief Map from isotope name to its index.*/
    std::map<std::string, std::uint64_t> map_isotopes_;
    /** @brief Map from reaction name to its index.*/
    std::map<std::string, std::uint64_t> map_reactions_;
};

}  // namespace readmpo

#endif  // READMPO_SINGLE_MPO_HPP_
