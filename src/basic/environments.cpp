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

#ifdef SUBROSA_DG_WITH_OPENMP
#include <omp.h>
#endif

#include <fmt/core.h>

#include <iostream>
#include <regex>
#include <string_view>

#include "basic/environments.h"
#include "cmake.h"

namespace SubrosaDG::Internal {

void printEnvironmentInfo() {
  std::cout << "SubrosaDG Info:" << std::endl;
  std::cout << fmt::format("Version: {}", kSubrosaDGVersionString) << std::endl << std::endl;
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
  omp_set_num_threads(getMaxThreads());
  gmsh::option::setNumber("General.NumThreads", getMaxThreads());
}

int getMaxThreads() { return omp_get_max_threads(); }
#endif

}  // namespace SubrosaDG::Internal
