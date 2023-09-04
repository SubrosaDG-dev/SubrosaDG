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

#include <array>

#include "basic/concept.hpp"
#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "mesh/mesh_structure.hpp"
#include "solver/solver_structure.hpp"

namespace SubrosaDG {

template <int Dim, PolyOrder P, ElemType ElemT, EquModel EquModelT>
inline void calElemFunCoeff(const ElemMesh<Dim, P, ElemT>& elem_mesh, const Real delta_t,
                            const std::array<Real, 3>& time_discrete_coeff,
                            ElemSolver<Dim, P, ElemT, EquModelT>& elem_solver) {
#ifdef SUBROSA_DG_WITH_OPENMP
#pragma omp parallel for default(none) schedule(auto) shared(elem_mesh, delta_t, time_discrete_coeff, elem_solver)
#endif
  for (Isize i = 0; i < elem_mesh.num_; i++) {
    elem_solver.elem_(i).basis_fun_coeff_(1).noalias() =
        time_discrete_coeff[0] * elem_solver.elem_(i).basis_fun_coeff_(0) +
        time_discrete_coeff[1] * elem_solver.elem_(i).basis_fun_coeff_(1) +
        time_discrete_coeff[2] * delta_t * elem_solver.elem_(i).residual_ * elem_mesh.elem_(i).local_mass_mat_inv_;
  }
}

template <PolyOrder P, MeshType MeshT, TimeDiscrete TimeDiscreteT, EquModel EquModelT>
inline void calFunCoeff(const Mesh<2, P, MeshT>& mesh,
                        const SolverSupplemental<2, EquModelT, TimeDiscreteT>& solver_supplemental,
                        const std::array<Real, 3>& time_discrete_coeff, Solver<2, P, MeshT, EquModelT>& solver) {
  if constexpr (HasTri<MeshT>) {
    calElemFunCoeff(mesh.tri_, solver_supplemental.delta_t_, time_discrete_coeff, solver.tri_);
  }
  if constexpr (HasQuad<MeshT>) {
    calElemFunCoeff(mesh.quad_, solver_supplemental.delta_t_, time_discrete_coeff, solver.quad_);
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CAL_FUN_COEFF_HPP_
