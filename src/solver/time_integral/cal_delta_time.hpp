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

#include "basic/config.hpp"
#include "basic/constant.hpp"
#include "basic/enum.hpp"
#include "integral/integral_structure.hpp"
#include "mesh/get_elem_info.hpp"
#include "mesh/mesh_structure.hpp"
#include "solver/solver_structure.hpp"
#include "solver/variable/cal_primitive_var.hpp"

namespace SubrosaDG {

template <int PolyOrder, ElemType ElemT, EquModel EquModelT, TimeDiscrete TimeDiscreteT>
inline void calDeltaTime(
    const ElemMesh<2, ElemT>& elem_mesh, const ElemIntegral<PolyOrder, ElemT>& elem_integral,
    const ThermoModel<EquModel::Euler>& thermo_model,
    const Eigen::Vector<PerElemSolver<2, PolyOrder, ElemT, EquModelT>, Eigen::Dynamic>& elem_solver,
    TimeSolver<TimeDiscreteT>& time_solver) {
  Eigen::Vector<Real, 4> conserved_var;
  Eigen::Vector<Real, 5> primitive_var;
  Real u;
  Real v;
  Real a;
  Real lambda_x;
  Real lambda_y;
  Real delta_t;
  for (Isize i = 0; i < elem_mesh.num_; i++) {
    for (Isize j = 0; j < elem_integral.kIntegralNum; j++) {
      conserved_var.noalias() = elem_solver(i).basis_fun_coeff_(1) * elem_integral.basis_fun_.row(j).transpose();
      calPrimitiveVar(thermo_model, conserved_var, primitive_var);
      u = primitive_var(1);
      v = primitive_var(2);
      a = std::sqrt(thermo_model.gamma_ * primitive_var(3) / primitive_var(0));
      lambda_x = std::fabs(u) * (1 + a / std::sqrt(u * u + v * v)) * elem_mesh.elem_(i).projection_measure_.x();
      lambda_y = std::fabs(v) * (1 + a / std::sqrt(u * u + v * v)) * elem_mesh.elem_(i).projection_measure_.y();
      delta_t = time_solver.cfl_ * (elem_mesh.elem_(i).jacobi_ / elem_integral.measure) / (lambda_x + lambda_y);
      time_solver.delta_t_ = std::ranges::min(time_solver.delta_t_, delta_t);
    }
  }
}

template <int PolyOrder, MeshType MeshT, EquModel EquModelT, TimeDiscrete TimeDiscreteT>
inline void calDeltaTime(const Mesh<2, MeshT>& mesh, const Integral<2, PolyOrder, MeshT>& integral,
                         const ThermoModel<EquModel::Euler>& thermo_model,
                         const ElemSolver<2, PolyOrder, MeshT, EquModelT>& elem_solver,
                         TimeSolver<TimeDiscreteT>& time_solver) {
  time_solver.delta_t_ = kMax;
  if constexpr (HasTri<MeshT>) {
    calDeltaTime(mesh.tri_, integral.tri_, thermo_model, elem_solver.tri_, time_solver);
  }
  if constexpr (HasQuad<MeshT>) {
    calDeltaTime(mesh.quad_, integral.quad_, thermo_model, elem_solver.quad_, time_solver);
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CAL_DELTA_TIME_HPP_
