/**
 * @file cal_residual.hpp
 * @brief The head file to calculate the residual.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-06-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CAL_RESIDUAL_HPP_
#define SUBROSA_DG_CAL_RESIDUAL_HPP_

#include "basic/concept.hpp"
#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "integral/integral_structure.hpp"
#include "mesh/mesh_structure.hpp"
#include "solver/solver_structure.hpp"

namespace SubrosaDG {

template <int Dim, PolyOrder P, ElemType ElemT, EquModel EquModelT>
inline void calElemResidual(const ElemIntegral<P, ElemT>& elem_integral,
                            const ElemAdjacencyIntegral<P, ElemT>& elem_adjacency_integral,
                            const ElemMesh<Dim, P, ElemT>& elem_mesh,
                            ElemSolver<Dim, P, ElemT, EquModelT>& elem_solver) {
#ifdef SUBROSA_DG_WITH_OPENMP
#pragma omp parallel for default(none) schedule(auto) \
    shared(elem_mesh, elem_integral, elem_adjacency_integral, elem_solver)
#endif
  for (Isize i = 0; i < elem_mesh.num_; i++) {
    elem_solver.elem_(i).residual_.noalias() =
        elem_solver.elem_(i).elem_integral_ * elem_integral.grad_basis_fun_ -
        elem_solver.elem_(i).adjacency_integral_ * elem_adjacency_integral.basis_fun_;
  }
}

template <PolyOrder P, MeshType MeshT, EquModel EquModelT>
inline void calResidual(const Integral<2, P, MeshT>& integral, const Mesh<2, P, MeshT>& mesh,
                        Solver<2, P, MeshT, EquModelT>& solver) {
  if constexpr (HasTri<MeshT>) {
    calElemResidual(integral.tri_, integral.line_.tri_, mesh.tri_, solver.tri_);
  }
  if constexpr (HasQuad<MeshT>) {
    calElemResidual(integral.quad_, integral.line_.quad_, mesh.quad_, solver.quad_);
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CAL_RESIDUAL_HPP_
