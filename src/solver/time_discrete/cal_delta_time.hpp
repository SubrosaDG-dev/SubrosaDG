/**
 * @file cal_delta_time.hpp
 * @brief The header file to calculate the time step size.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-06-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CAL_DELTA_TIME_HPP_
#define SUBROSA_DG_CAL_DELTA_TIME_HPP_

#include <Eigen/Core>
#include <algorithm>
#include <cmath>

#include "basic/concept.hpp"
#include "basic/constant.hpp"
#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "integral/integral_structure.hpp"
#include "mesh/mesh_structure.hpp"
#include "solver/solver_structure.hpp"
#include "solver/variable/cal_primitive_var.hpp"
#include "solver/variable/get_var_num.hpp"

namespace SubrosaDG {

template <PolyOrder P, ElemType ElemT, EquModel EquModelT, TimeDiscrete TimeDiscreteT>
  requires IsExplicit<TimeDiscreteT>
inline void calElemDeltaTime(const ElemIntegral<P, ElemT>& elem_integral, const ElemMesh<2, P, ElemT>& elem_mesh,
                             const ElemSolver<2, P, ElemT, EquModelT>& elem_solver,
                             SolverSupplemental<2, EquModelT, TimeDiscreteT>& solver_supplemental) {
  Eigen::Vector<Real, getConservedVarNum<EquModelT>(2)> conserved_var;
  Eigen::Vector<Real, getPrimitiveVarNum<EquModelT>(2)> primitive_var;
  Real u;
  Real v;
  Real a;
  Real lambda_x;
  Real lambda_y;
  Real delta_t;
  for (Isize i = 0; i < elem_mesh.num_; i++) {
    for (Isize j = 0; j < elem_integral.kIntegralNum; j++) {
      conserved_var.noalias() = elem_solver.elem_(i).basis_fun_coeff_(1) * elem_integral.basis_fun_.row(j).transpose();
      calPrimitiveVar(solver_supplemental.thermo_model_, conserved_var, primitive_var);
      u = primitive_var(1);
      v = primitive_var(2);
      a = std::sqrt(solver_supplemental.thermo_model_.gamma_ * primitive_var(3) / primitive_var(0));
      lambda_x = std::fabs(u) * (1 + a / std::sqrt(u * u + v * v)) * elem_mesh.elem_(i).projection_measure_.x();
      lambda_y = std::fabs(v) * (1 + a / std::sqrt(u * u + v * v)) * elem_mesh.elem_(i).projection_measure_.y();
      delta_t = solver_supplemental.time_solver_.cfl_ * (elem_mesh.elem_(i).jacobian_det_(j) / elem_integral.measure) /
                (lambda_x + lambda_y);
      solver_supplemental.delta_t_ = std::ranges::min(solver_supplemental.delta_t_, delta_t);
    }
  }
}

template <PolyOrder P, MeshType MeshT, EquModel EquModelT, TimeDiscrete TimeDiscreteT>
  requires IsExplicit<TimeDiscreteT>
inline void calDeltaTime(const Integral<2, P, MeshT>& integral, const Mesh<2, P, MeshT>& mesh,
                         const Solver<2, P, MeshT, EquModelT>& solver,
                         SolverSupplemental<2, EquModelT, TimeDiscreteT>& solver_supplemental) {
  solver_supplemental.delta_t_ = kMax;
  if constexpr (HasTri<MeshT>) {
    calElemDeltaTime(integral.tri_, mesh.tri_, solver.tri_, solver_supplemental);
  }
  if constexpr (HasQuad<MeshT>) {
    calElemDeltaTime(integral.quad_, mesh.quad_, solver.quad_, solver_supplemental);
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CAL_DELTA_TIME_HPP_
