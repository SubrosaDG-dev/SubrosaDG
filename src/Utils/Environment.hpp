/**
 * @file Environment.hpp
 * @brief The header file of SubrosaDG environment.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-12-06
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_ENVIRONMENT_HPP_
#define SUBROSA_DG_ENVIRONMENT_HPP_

#if defined(SUBROSA_DG_WITH_OPENMP) && !defined(SUBROSA_DG_DEVELOP)
#include <omp.h>
#endif  // SUBROSA_DG_WITH_OPENMP && !SUBROSA_DG_DEVELOP

#include <gmsh.h>

namespace SubrosaDG {

struct Environment {
  inline Environment();

  inline ~Environment();
};

inline Environment::Environment() {
  gmsh::initialize();
#if defined(SUBROSA_DG_WITH_OPENMP) && !defined(SUBROSA_DG_DEVELOP)
  omp_set_num_threads(omp_get_max_threads());
  gmsh::option::setNumber("General.NumThreads", omp_get_max_threads());
#endif  // SUBROSA_DG_WITH_OPENMP && !SUBROSA_DG_DEVELOP
}

inline Environment::~Environment() { gmsh::finalize(); }

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_ENVIRONMENT_HPP_
