/**
 * @file get_adjacency_integral.hpp
 * @brief The head file of get adjacency integral.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-07-14
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_GET_ADJACENCY_INTEGRAL_HPP_
#define SUBROSA_DG_GET_ADJACENCY_INTEGRAL_HPP_

#include <fmt/core.h>
#include <gmsh.h>

#include <Eigen/Core>
#include <vector>

#include "basic/concept.hpp"
#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "integral/get_gauss_quad.hpp"
#include "integral/get_integral_num.hpp"
#include "integral/integral_structure.hpp"
#include "mesh/get_elem_info.hpp"

namespace SubrosaDG {

template <PolyOrder P, ElemType ElemT, ElemType ParentElemT, MeshType MeshT>
inline void getAdjacencyElemIntegralFromParent(const std::vector<double>& coords_basis_functions,
                                               ElemAdjacencyIntegral<P, ParentElemT>& elem_adjacency_integral) {
  using T = AdjacencyElemIntegral<P, ElemT, MeshT>;
  using ParentT = ElemIntegral<P, ParentElemT>;
  Eigen::Matrix<double, 3, T::kIntegralNum * getElemAdjacencyNum<ParentElemT>()> parent_coords =
      Eigen::Matrix<double, 3, T::kIntegralNum * getElemAdjacencyNum<ParentElemT>()>::Zero();
  for (Isize i = 0; i < getElemAdjacencyNum<ParentElemT>(); i++) {
    for (Isize j = 0; j < T::kIntegralNum; j++) {
      parent_coords(Eigen::seqN(Eigen::fix<0>, Eigen::fix<getDim<ParentElemT>()>), i * T::kIntegralNum + j).noalias() =
          (ElemStandard<ParentElemT>::coord.row((i + 1) % getElemAdjacencyNum<ParentElemT>()) *
               coords_basis_functions[static_cast<Usize>(j * getElemAdjacencyNum<ElemT>() + 1)] +
           ElemStandard<ParentElemT>::coord.row(i) *
               coords_basis_functions[static_cast<Usize>(j * getElemAdjacencyNum<ElemT>())])
              .transpose();
    }
  }
  int num_components;
  int num_orientations;
  std::vector<double> basis_functions;
  gmsh::model::mesh::getBasisFunctions(
      getTopology<ParentElemT>(), {parent_coords.data(), parent_coords.data() + parent_coords.size()},
      fmt::format("Lagrange{}", static_cast<int>(P)), num_components, basis_functions, num_orientations);
  for (Isize i = 0; i < T::kIntegralNum * getElemAdjacencyNum<ParentElemT>(); i++) {
    for (Isize j = 0; j < ParentT::kBasisFunNum; j++) {
      elem_adjacency_integral.basis_fun_(i, j) =
          static_cast<Real>(basis_functions[static_cast<Usize>(i * ParentT::kBasisFunNum + j)]);
    }
  }
}

template <PolyOrder P, ElemType ElemT, MeshType MeshT>
inline void getAdjacencyElemIntegral(AdjacencyElemIntegral<P, ElemT, MeshT>& adjacency_elem_integral) {
  std::vector<double> local_coords = getElemGaussQuad(getElemAdjacencyIntegralOrder(P), adjacency_elem_integral);
  int num_components;
  int num_orientations;
  std::vector<double> cooords_basis_functions;
  gmsh::model::mesh::getBasisFunctions(getTopology<ElemT>(), local_coords, "Lagrange1", num_components,
                                       cooords_basis_functions, num_orientations);
  if constexpr (HasTri<MeshT>) {
    getAdjacencyElemIntegralFromParent<P, ElemT, ElemType::Tri, MeshT>(cooords_basis_functions,
                                                                       adjacency_elem_integral.tri_);
  }
  if constexpr (HasQuad<MeshT>) {
    getAdjacencyElemIntegralFromParent<P, ElemT, ElemType::Quad, MeshT>(cooords_basis_functions,
                                                                        adjacency_elem_integral.quad_);
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_ADJACENCY_INTEGRAL_HPP_
