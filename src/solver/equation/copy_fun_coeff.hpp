/**
 * @file copy_fun_coeff.hpp
 * @brief The copy fun coeff header file.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-06-24
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_COPY_FUN_COEFF_HPP_
#define SUBROSA_DG_COPY_FUN_COEFF_HPP_

#include "basic/concept.hpp"
#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "mesh/mesh_structure.hpp"
#include "solver/solver_structure.hpp"

namespace SubrosaDG {

template <PolyOrder P, ElemType ElemT>
inline void copyElemFunCoeff(const ElemMesh<2, ElemT>& elem_mesh,
                             ElemSolver<2, P, ElemT, EquModel::Euler>& elem_solver) {
  for (Isize i = 0; i < elem_mesh.num_; i++) {
    elem_solver.elem_(i).basis_fun_coeff_(0).noalias() = elem_solver.elem_(i).basis_fun_coeff_(1);
  }
}

template <PolyOrder P, MeshType MeshT>
inline void copyFunCoeff(const Mesh<2, P, MeshT>& mesh, Solver<2, P, EquModel::Euler, MeshT>& solver) {
  if constexpr (HasTri<MeshT>) {
    copyElemFunCoeff(mesh.tri_, solver.tri_);
  }
  if constexpr (HasQuad<MeshT>) {
    copyElemFunCoeff(mesh.quad_, solver.quad_);
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_COPY_FUN_COEFF_HPP_