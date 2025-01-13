/**
 * @file Environment.cpp
 * @brief The header file of SubrosaDG environment.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-12-06
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2025 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_ENVIRONMENT_CPP_
#define SUBROSA_DG_ENVIRONMENT_CPP_

#include <../opt/compiler/include/omp.h>
#include <gmsh.h>
#include <oneapi/tbb/global_control.h>

#include "Cmake.cpp"

namespace SubrosaDG {

struct Environment {
  inline Environment();

  inline ~Environment();
};

inline Environment::Environment() {
  gmsh::initialize();
#ifdef SUBROSA_DG_DEVELOP
  oneapi::tbb::global_control global_limit(oneapi::tbb::global_control::max_allowed_parallelism, 1);
  omp_set_num_threads(1);
  gmsh::option::setNumber("General.NumThreads", 1);
#else   // SUBROSA_DG_DEVELOP
  oneapi::tbb::global_control global_limit(oneapi::tbb::global_control::max_allowed_parallelism,
                                           kNumberOfPhysicalCores - 1);
  omp_set_num_threads(kNumberOfPhysicalCores - 1);
  gmsh::option::setNumber("General.NumThreads", kNumberOfPhysicalCores - 1);
#endif  // SUBROSA_DG_DEVELOP
}

inline Environment::~Environment() { gmsh::finalize(); }

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_ENVIRONMENT_CPP_
