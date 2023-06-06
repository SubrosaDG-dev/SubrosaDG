/**
 * @file get_parent_var.hpp
 * @brief The get parent variable header file.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-06-04
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_GET_PARENT_VAR_HPP_
#define SUBROSA_DG_GET_PARENT_VAR_HPP_

// clang-format off

#include <Eigen/Core>           // for DenseBase::row, Vector

#include "basic/config.hpp"     // for TimeDiscrete
#include "mesh/elem_type.hpp"   // for ElemInfo, kQuad, kTri
#include "basic/data_type.hpp"  // for Isize, Real
#include "basic/enum.hpp"       // for MeshType

// clang-format on

namespace SubrosaDG {

template <int PolyOrder, ElemInfo ElemT, MeshType MeshT>
struct AdjacencyElemIntegral;
template <int Dim, int PolyOrder, MeshType MeshT, TimeDiscrete TimeDiscreteT>
struct ElemConvectiveSolver;

template <int PolyOrder, ElemInfo ElemT, MeshType MeshT, TimeDiscrete TimeDiscreteT>
inline void getParentVar(const Isize elem_tag, const Isize adjacency_integral_order,
                         const AdjacencyElemIntegral<PolyOrder, ElemT, MeshT>& adjacency_elem_integral,
                         const ElemConvectiveSolver<2, PolyOrder, MeshT, TimeDiscreteT>& elem_solver,
                         Eigen::Vector<Real, 4>& parent_conserved_var) {
  if constexpr (MeshT == MeshType::Tri) {
    parent_conserved_var = elem_solver.tri_(elem_tag).basis_fun_coeff_(1) *
                           adjacency_elem_integral.tri_basis_fun_.row(adjacency_integral_order).transpose();
  } else if constexpr (MeshT == MeshType::Quad) {
    parent_conserved_var = elem_solver.quad_(elem_tag).basis_fun_coeff_(1) *
                           adjacency_elem_integral.quad_basis_fun_.row(adjacency_integral_order).transpose();
  }
}

template <int PolyOrder, ElemInfo ElemT, TimeDiscrete TimeDiscreteT>
inline void getParentVar(const int elem_topology, const Isize elem_tag, const Isize adjacency_integral_order,
                         const AdjacencyElemIntegral<PolyOrder, ElemT, MeshType::TriQuad>& adjacency_elem_integral,
                         const ElemConvectiveSolver<2, PolyOrder, MeshType::TriQuad, TimeDiscreteT>& elem_solver,
                         Eigen::Vector<Real, 4>& parent_conserved_var) {
  switch (elem_topology) {
  case kTri.kTopology:
    parent_conserved_var = elem_solver.tri_(elem_tag).basis_fun_coeff_(1) *
                           adjacency_elem_integral.tri_basis_fun_.row(adjacency_integral_order).transpose();
    break;
  case kQuad.kTopology:
    parent_conserved_var = elem_solver.quad_(elem_tag).basis_fun_coeff_(1) *
                           adjacency_elem_integral.quad_basis_fun_.row(adjacency_integral_order).transpose();
    break;
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_PARENT_VAR_HPP_
