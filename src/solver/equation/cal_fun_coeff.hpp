/**
 * @file cal_fun_coeff.hpp
 * @brief The header file to calculate the function coefficients.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-06-25
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CAL_FUN_COEFF_HPP_
#define SUBROSA_DG_CAL_FUN_COEFF_HPP_

#include "basic/concept.hpp"
#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "integral/integral_structure.hpp"
#include "mesh/mesh_structure.hpp"
#include "solver/solver_structure.hpp"

namespace SubrosaDG {

template <PolyOrder P, ElemType ElemT>
inline void calElemFunCoeff(const ElemMesh<2, ElemT>& elem_mesh, const ElemIntegral<P, ElemT>& elem_integral,
                            const Real delta_t, const Real time_discrete_coeff,
                            ElemSolver<2, P, ElemT, EquModel::Euler>& elem_solver) {
  for (Isize i = 0; i < elem_mesh.num_; i++) {
    elem_solver.elem_(i).basis_fun_coeff_(1) *= 1 - time_discrete_coeff;
    elem_solver.elem_(i).basis_fun_coeff_(1).noalias() +=
        time_discrete_coeff * elem_solver.elem_(i).basis_fun_coeff_(0) +
        (1 - time_discrete_coeff) * delta_t * elem_solver.elem_(i).residual_ * elem_integral.local_mass_mat_inv_;
  }
}

template <PolyOrder P, MeshType MeshT, TimeDiscrete TimeDiscreteT>
inline void calFunCoeff(const Mesh<2, P, MeshT>& mesh, const Integral<2, P, MeshT>& integral,
                        const SolverSupplemental<2, EquModel::Euler, TimeDiscreteT>& solver_supplemental,
                        const Real time_discrete_coeff, Solver<2, P, EquModel::Euler, MeshT>& solver) {
  if constexpr (HasTri<MeshT>) {
    calElemFunCoeff(mesh.tri_, integral.tri_, solver_supplemental.delta_t_, time_discrete_coeff, solver.tri_);
  }
  if constexpr (HasQuad<MeshT>) {
    calElemFunCoeff(mesh.quad_, integral.quad_, solver_supplemental.delta_t_, time_discrete_coeff, solver.quad_);
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CAL_FUN_COEFF_HPP_
