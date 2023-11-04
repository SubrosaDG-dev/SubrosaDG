/**
 * @file environment.hpp
 * @brief The header file for SubrosaDG environment.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-04-13
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_ENVIRONMENT_HPP_
#define SUBROSA_DG_ENVIRONMENT_HPP_

#ifdef SUBROSA_DG_WITH_OPENMP
#include <omp.h>
#endif

#include <fmt/core.h>
#include <gmsh.h>

#include <iostream>
#include <regex>
#include <string>
#include <string_view>
#include <vector>

#include "cmake.hpp"

namespace SubrosaDG {

inline std::vector<std::string> getGmshInfo() {
  std::string info;
  gmsh::option::getString("General.BuildInfo", info);
  std::regex re(";\\s*");
  std::vector<std::string> lines{std::sregex_token_iterator(info.begin(), info.end(), re, -1),
                                 std::sregex_token_iterator()};
  return lines;
}

inline void printEnvInfo() {
  std::cout << "SubrosaDG Info:" << std::endl;
  std::cout << fmt::format("Version: {}", kSubrosaDGVersionString) << std::endl;
#ifdef SUBROSA_DG_DEVELOP
  std::cout << "Build type: Debug" << std::endl;
#else
  std::cout << "Build type: Release" << std::endl;
#endif
  std::cout << fmt::format("Number of physical cores: {}", kNumberOfPhysicalCores) << std::endl << std::endl;
  std::cout << "Gmsh Info:" << std::endl;
  for (const auto& line : getGmshInfo()) {
    std::cout << line << std::endl;
  };
}

#ifdef SUBROSA_DG_WITH_OPENMP
inline int getMaxCoreNum() { return kNumberOfPhysicalCores; }

inline void setMaxThreadNum() {
  omp_set_num_threads(getMaxCoreNum());
  gmsh::option::setNumber("General.NumThreads", getMaxCoreNum());
}
#endif

class EnvGardian {
 public:
  inline EnvGardian() { initEnv(); }
  inline ~EnvGardian() { finalizeEnv(); }

 private:
  inline static void initEnv() {
    gmsh::initialize();
#ifndef SUBROSA_DG_DEVELOP
    gmsh::option::setNumber("Mesh.Binary", 1);
#endif
    printEnvInfo();
#ifdef SUBROSA_DG_WITH_OPENMP
    setMaxThreadNum();
#endif
  }
  inline static void finalizeEnv() { gmsh::finalize(); }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_ENVIRONMENT_HPP_
