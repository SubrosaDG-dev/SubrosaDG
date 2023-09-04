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

#include <Eigen/Core>

#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "mesh/get_elem_info.hpp"
#include "solver/solver_structure.hpp"
#include "solver/variable/get_var_num.hpp"

namespace SubrosaDG {

template <PolyOrder P, MeshType MeshT, EquModel EquModelT>
inline void storeAdjacencyIntegralToElem(
    const Isize elem_tag, const Isize adjacency_integral_order,
    const Eigen::Vector<Real, getConservedVarNum<EquModelT>(2)>& adjacency_integral,
    Solver<2, P, MeshT, EquModelT>& solver) {
  if constexpr (MeshT == MeshType::Tri) {
    solver.tri_.elem_(elem_tag).adjacency_integral_.col(adjacency_integral_order) = adjacency_integral;
  } else if constexpr (MeshT == MeshType::Quad) {
    solver.quad_.elem_(elem_tag).adjacency_integral_.col(adjacency_integral_order) = adjacency_integral;
  }
}

template <PolyOrder P, EquModel EquModelT>
inline void storeAdjacencyIntegralToElem(
    const int elem_topology, const Isize elem_tag, const Isize adjacency_integral_order,
    const Eigen::Vector<Real, getConservedVarNum<EquModelT>(2)>& adjacency_integral,
    Solver<2, P, MeshType::TriQuad, EquModelT>& solver) {
  switch (elem_topology) {
  case getTopology<ElemType::Tri>(P):
    solver.tri_.elem_(elem_tag).adjacency_integral_.col(adjacency_integral_order) = adjacency_integral;
    break;
  case getTopology<ElemType::Quad>(P):
    solver.quad_.elem_(elem_tag).adjacency_integral_.col(adjacency_integral_order) = adjacency_integral;
    break;
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_STORE_TO_ELEM_HPP_
