/**
 * @file environments.cpp
 * @brief The source file for SubrosaDG environments.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-04-13
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

// clang-format off

#ifdef SUBROSA_DG_WITH_OPENMP
#include <omp.h>        // for omp_set_num_threads
#endif

#include <fmt/core.h>   // for format
#include <iostream>     // for endl, cout, operator<<, ostream, basic_ostream
#include <string_view>  // for basic_string_view
#include <regex>        // for sregex_token_iterator, regex

#include "basic/environments.h"
#include "cmake.h"      // for kNumberOfPhysicalCores, kSubrosaDGVersionString

// clang-format on

namespace SubrosaDG {

namespace Internal {

void printEnvironmentInfo() {
  std::cout << "SubrosaDG Info:" << std::endl;
  std::cout << fmt::format("Version: {}", kSubrosaDGVersionString) << std::endl;
  std::cout << fmt::format("Number of physical cores: {}", kNumberOfPhysicalCores) << std::endl << std::endl;
  std::cout << "Gmsh Info:" << std::endl;
  for (const auto& line : getGmshInfo()) {
    std::cout << line << std::endl;
  }
  std::cout << std::endl;
};

std::vector<std::string> getGmshInfo() {
  std::string info;
  gmsh::option::getString("General.BuildInfo", info);
  std::regex re(";\\s*");
  std::vector<std::string> lines{std::sregex_token_iterator(info.begin(), info.end(), re, -1),
                                 std::sregex_token_iterator()};
  return lines;
}

#ifdef SUBROSA_DG_WITH_OPENMP
void setMaxThreads() {
  omp_set_num_threads(getMaxCores());
  gmsh::option::setNumber("General.NumThreads", getMaxCores());
}

int getMaxCores() { return kNumberOfPhysicalCores; }
#endif

}  // namespace Internal

EnvironmentGardian::EnvironmentGardian() { initializeEnvironment(); }
EnvironmentGardian::~EnvironmentGardian() { finalizeEnvironment(); }

}  // namespace SubrosaDG
