// Copyright 2023 quocdang1998
#ifndef READMPO_MASTER_MPO_HPP_
#define READMPO_MASTER_MPO_HPP_

#include <string>  // std::string
#include <vector>  // std::vector

#include "H5File.h"  // H5::File

namespace readmpo {

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

  protected:
    /** @brief Name of geometry.*/
    std::string geometry_;
    /** @brief Name of energy.*/
    std::string energy_mesh_;
};

}  // namespace readmpo

#endif  // READMPO_MASTER_MPO_HPP_
