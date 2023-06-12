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

#include <Eigen/Core>

#include "basic/concept.hpp"
#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "integral/cal_basisfun_num.hpp"
#include "integral/get_integral_num.hpp"
#include "mesh/get_elem_info.hpp"

namespace SubrosaDG {

template <int Dim, ElemType ElemT>
struct ElemMesh;
template <int Dim, MeshType MeshT>
struct Mesh;
template <int PolyOrder, ElemType ElemT>
struct ElemIntegral;
template <int PolyOrder, ElemType ElemT, MeshType MeshT>
struct AdjacencyElemIntegral;
template <int Dim, int PolyOrder, MeshType MeshT>
struct Integral;
template <int Dim, int PolyOrder, ElemType ElemT, EquModel EquModelT>
struct PerElemSolver;
template <int Dim, int PolyOrder, MeshType MeshT, EquModel EquModelT>
struct ElemSolver;

template <int PolyOrder, ElemType ElemT, MeshType MeshT>
inline void calResidual(
    const ElemMesh<2, ElemT>& elem_mesh, const ElemIntegral<PolyOrder, ElemT>& elem_integral,
    const Eigen::Matrix<Real, getElemAdjacencyIntegralNum<ElemT>(PolyOrder), calBasisFunNum<ElemT>(PolyOrder),
                        Eigen::RowMajor>& parent_basis_fun,
    Eigen::Vector<PerElemSolver<2, PolyOrder, ElemT, EquModel::Euler>, Eigen::Dynamic>& elem_solver) {
  for (Isize i = 0; i < elem_mesh.num_; i++) {
    for (Isize j = 0; j < elem_integral.kIntegralNum; j++) {
      elem_solver(i).residual_.noalias() = elem_solver(i).elem_integral_ * elem_integral.grad_basis_fun_ -
                                           elem_solver(i).adjacency_integral_ * parent_basis_fun;
    }
  }
}

template <int PolyOrder, ElemType ElemT, MeshType MeshT>
inline void calResidual(const Mesh<2, MeshT>& mesh, const Integral<2, PolyOrder, MeshT>& integral,
                        ElemSolver<2, PolyOrder, MeshT, EquModel::Euler>& elem_solver) {
  if constexpr (HasTri<MeshT>) {
    calResidual(mesh.tri_, integral.tri_, integral.tri_adjacency_, elem_solver.tri_);
  }
  if constexpr (HasQuad<MeshT>) {
    calResidual(mesh.quad_, integral.quad_, integral.quad_adjacency_, elem_solver.quad_);
  }
}
}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CAL_RESIDUAL_HPP_
