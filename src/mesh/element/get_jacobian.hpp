/**
 * @file get_jacobian.hpp
 * @brief The get element jacobian header file.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-22
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_GET_JACOBIAN_HPP_
#define SUBROSA_DG_GET_JACOBIAN_HPP_

// clang-format off

#include <gmsh.h>               // for getJacobian
#include <algorithm>            // for copy
#include <vector>               // for vector

#include "basic/data_type.hpp"  // for Isize, Real, Usize
#include "mesh/elem_type.hpp"   // for ElemInfo

// clang-format on

namespace SubrosaDG {

template <int Dim, ElemInfo ElemT>
struct ElemMesh;
template <int Dim, ElemInfo ElemT>
struct AdjacencyElemMesh;

template <int Dim, ElemInfo ElemT>
inline void getElemJacobian(ElemMesh<Dim, ElemT>& elem_mesh) {
  const std::vector<double> local_coord{0.0, 0.0, 0.0};
  std::vector<double> jacobians;
  std::vector<double> determinants;
  std::vector<double> coord;
  for (Isize i = 0; i < elem_mesh.num_; i++) {
    gmsh::model::mesh::getJacobian(static_cast<Usize>(i + elem_mesh.range_.first), local_coord, jacobians, determinants,
                                   coord);
    elem_mesh.elem_(i).jacobian_ = static_cast<Real>(determinants[0]);
  }
}

template <Isize Dim, ElemInfo ElemT>
inline void getElemJacobian(AdjacencyElemMesh<Dim, ElemT>& adjacency_elem_mesh) {
  const std::vector<double> local_coord{0.0, 0.0, 0.0};
  std::vector<double> jacobians;
  std::vector<double> determinants;
  std::vector<double> coord;
  for (Isize i = 0; i < adjacency_elem_mesh.internal_.num_; i++) {
    gmsh::model::mesh::getJacobian(static_cast<Usize>(i + adjacency_elem_mesh.internal_.range_.first), local_coord,
                                   jacobians, determinants, coord);
    adjacency_elem_mesh.internal_.elem_(i).jacobian_ = static_cast<Real>(determinants[0]);
  }
  for (Isize i = 0; i < adjacency_elem_mesh.boundary_.num_; i++) {
    gmsh::model::mesh::getJacobian(static_cast<Usize>(i + adjacency_elem_mesh.boundary_.range_.first), local_coord,
                                   jacobians, determinants, coord);
    adjacency_elem_mesh.boundary_.elem_(i).jacobian_ = static_cast<Real>(determinants[0]);
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_JACOBIAN_HPP_