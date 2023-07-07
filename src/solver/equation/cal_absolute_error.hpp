/**
 * @file cal_absolute_error.hpp
 * @brief The head file to calculate the absolute error.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-06-13
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CAL_ABSOLUTE_ERROR_HPP_
#define SUBROSA_DG_CAL_ABSOLUTE_ERROR_HPP_

#include <Eigen/Core>

#include "basic/concept.hpp"
#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "integral/integral_structure.hpp"
#include "mesh/mesh_structure.hpp"
#include "solver/solver_structure.hpp"

namespace SubrosaDG {

template <PolyOrder P, ElemType ElemT>
inline void calElemAbsoluteError(const ElemMesh<2, ElemT>& elem_mesh, const ElemIntegral<P, ElemT>& elem_integral,
                                 const ElemSolver<2, P, ElemT, EquModel::Euler>& elem_solver,
                                 Eigen::Vector<Real, 4>& absolute_error) {
  for (Isize i = 0; i < elem_mesh.num_; i++) {
    absolute_error.array() +=
        (elem_solver.elem_(i).residual_ * elem_integral.basis_fun_.transpose()).array().abs().rowwise().mean();
  }
}

template <PolyOrder P, MeshType MeshT>
inline void calAbsoluteError(const Mesh<2, P, MeshT>& mesh, const Integral<2, P, MeshT>& integral,
                             const Solver<2, P, EquModel::Euler, MeshT>& solver,
                             Eigen::Vector<Real, 4>& absolute_error) {
  absolute_error.setZero();
  if constexpr (HasTri<MeshT>) {
    calElemAbsoluteError(mesh.tri_, integral.tri_, solver.tri_, absolute_error);
  }
  if constexpr (HasQuad<MeshT>) {
    calElemAbsoluteError(mesh.quad_, integral.quad_, solver.quad_, absolute_error);
  }
  absolute_error /= static_cast<Real>(mesh.elem_num_);
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CAL_ABSOLUTE_ERROR_HPP_
