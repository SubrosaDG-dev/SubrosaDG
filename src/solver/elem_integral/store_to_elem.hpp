/**
 * @file store_to_elem.hpp
 * @brief The head file to store the integral to the element.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-06-04
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_STORE_TO_ELEM_HPP_
#define SUBROSA_DG_STORE_TO_ELEM_HPP_

// clang-format off

#include <Eigen/Core>           // for DenseBase::col, Vector

#include "mesh/elem_type.hpp"   // for ElemInfo, kQuad, kTri
#include "basic/data_type.hpp"  // for Isize, Real
#include "basic/enum.hpp"       // for EquModel (ptr only), MeshType

// clang-format on

namespace SubrosaDG {

template <int Dim, int PolyOrder, MeshType MeshT, EquModel EquModelT>
struct ElemSolver;

template <int PolyOrder, MeshType MeshT, EquModel EquModelT>
inline void storeAdjacencyIntegralToElem(const Isize elem_tag, const Isize adjacency_integral_order,
                                         const Eigen::Vector<Real, 4>& adjacency_integral,
                                         ElemSolver<2, PolyOrder, MeshT, EquModelT>& elem_solver) {
  if constexpr (MeshT == MeshType::Tri) {
    elem_solver.tri_(elem_tag).adjacency_integral_.col(adjacency_integral_order) = adjacency_integral;
  } else if constexpr (MeshT == MeshType::Quad) {
    elem_solver.quad_(elem_tag).adjacency_integral_.col(adjacency_integral_order) = adjacency_integral;
  }
}

template <int PolyOrder, EquModel EquModelT>
inline void storeAdjacencyIntegralToElem(const int elem_topology, const Isize elem_tag,
                                         const Isize adjacency_integral_order,
                                         const Eigen::Vector<Real, 4>& adjacency_integral,
                                         ElemSolver<2, PolyOrder, MeshType::TriQuad, EquModelT>& elem_solver) {
  switch (elem_topology) {
  case kTri.kTopology:
    elem_solver.tri_(elem_tag).adjacency_integral_.col(adjacency_integral_order) = adjacency_integral;
    break;
  case kQuad.kTopology:
    elem_solver.quad_(elem_tag).adjacency_integral_.col(adjacency_integral_order) = adjacency_integral;
    break;
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_STORE_TO_ELEM_HPP_
