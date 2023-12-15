// Copyright 2022 quocdang1998
#ifndef READMPO_GLOB_HPP_
#define READMPO_GLOB_HPP_

#include <string>  // std::string
#include <vector>  // std::vector

namespace readmpo {

/** @brief Get list of files matching a certain pattern.*/
std::vector<std::string> glob(const std::string & pattern);

}  // namespace readmpo

#endif  // READMPO_GLOB_HPP_
