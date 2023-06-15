/**
 * @file step_time.hpp
 * @brief The step time header file.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-06-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_STEP_TIME_HPP_
#define SUBROSA_DG_STEP_TIME_HPP_

#include "basic/concept.hpp"
#include "basic/enum.hpp"
#include "integral/integral_structure.hpp"
#include "mesh/mesh_structure.hpp"
#include "solver/equation/cal_residual.hpp"
#include "solver/equation/update_fun_coeff.hpp"
#include "solver/solver_structure.hpp"
#include "solver/space_discrete/cal_adjacency_integral.hpp"
#include "solver/space_discrete/cal_elem_integral.hpp"

namespace SubrosaDG {

template <typename T, PolyOrder P, MeshType MeshT, TimeDiscrete TimeDiscreteT>
  requires DerivedFromSpatialDiscrete<T, EquModel::Euler>
inline void stepTime(const Mesh<2, MeshT>& mesh, const Integral<2, P, MeshT>& integral,
                     const SolverSupplemental<2, EquModel::Euler, TimeDiscreteT>& solver_supplemental,
                     Solver<2, P, EquModel::Euler, MeshT>& solver) {
  calElemIntegral(mesh, integral, solver_supplemental, solver);
  calAdjacencyElemIntegral<T>(mesh, integral, solver_supplemental, solver);
  calResidual(mesh, integral, solver);
  updateFunCoeff(mesh, integral, solver);
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_STEP_TIME_HPP_
