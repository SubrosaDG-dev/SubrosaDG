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

#include <Eigen/Core>

#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "integral/integral_structure.hpp"
#include "mesh/get_elem_info.hpp"
#include "solver/solver_structure.hpp"

namespace SubrosaDG {

template <PolyOrder P, ElemType ElemT, MeshType MeshT, EquModel EquModelT>
inline void getParentVar(const Isize elem_tag, const Isize adjacency_integral_order,
                         const AdjacencyElemIntegral<P, ElemT, MeshT>& adjacency_elem_integral,
                         const Solver<2, P, EquModelT, MeshT>& solver, Eigen::Vector<Real, 4>& parent_conserved_var) {
  if constexpr (MeshT == MeshType::Tri) {
    parent_conserved_var.noalias() = solver.tri_.elem_(elem_tag).basis_fun_coeff_(1) *
                                     adjacency_elem_integral.tri_.basis_fun_.row(adjacency_integral_order).transpose();
  } else if constexpr (MeshT == MeshType::Quad) {
    parent_conserved_var.noalias() = solver.quad_.elem_(elem_tag).basis_fun_coeff_(1) *
                                     adjacency_elem_integral.quad_.basis_fun_.row(adjacency_integral_order).transpose();
  }
}

template <PolyOrder P, ElemType ElemT, EquModel EquModelT>
inline void getParentVar(const int elem_topology, const Isize elem_tag, const Isize adjacency_integral_order,
                         const AdjacencyElemIntegral<P, ElemT, MeshType::TriQuad>& adjacency_elem_integral,
                         const Solver<2, P, EquModelT, MeshType::TriQuad>& solver,
                         Eigen::Vector<Real, 4>& parent_conserved_var) {
  switch (elem_topology) {
  case getTopology<ElemType::Tri>():
    parent_conserved_var.noalias() = solver.tri_.elem_(elem_tag).basis_fun_coeff_(1) *
                                     adjacency_elem_integral.tri_.basis_fun_.row(adjacency_integral_order).transpose();
    break;
  case getTopology<ElemType::Quad>():
    parent_conserved_var.noalias() = solver.quad_.elem_(elem_tag).basis_fun_coeff_(1) *
                                     adjacency_elem_integral.quad_.basis_fun_.row(adjacency_integral_order).transpose();
    break;
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_PARENT_VAR_HPP_
