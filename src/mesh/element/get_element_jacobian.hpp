/**
 * @file get_element_jacobian.hpp
 * @brief The get element jacobian header file.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-22
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_GET_ELEMENT_JACOBIAN_HPP_
#define SUBROSA_DG_GET_ELEMENT_JACOBIAN_HPP_

// clang-format off

#include <gmsh.h>                  // for getJacobian
#include <algorithm>               // for copy
#include <cstddef>                 // for size_t
#include <vector>                  // for vector

#include "basic/data_types.hpp"    // for Isize, Real
#include "mesh/element_types.hpp"  // for ElementType

// clang-format on

namespace SubrosaDG {

template <SubrosaDG::Isize Dimension, ElementType Type>
struct AdjacencyElementMesh;
template <SubrosaDG::Isize Dimension, ElementType Type>
struct ElementMesh;

template <Isize Dimension, ElementType Type>
inline void getElementJacobian(ElementMesh<Dimension, Type>& element_mesh) {
  element_mesh.elements_jacobian_.resize(element_mesh.elements_num_);
  const std::vector<double> local_coord{0.0, 0.0, 0.0};
  std::vector<double> jacobians;
  std::vector<double> determinants;
  std::vector<double> coord;
  for (Isize i = 0; i < element_mesh.elements_num_; i++) {
    gmsh::model::mesh::getJacobian(static_cast<std::size_t>(i + element_mesh.elements_range_.first), local_coord,
                                   jacobians, determinants, coord);
    element_mesh.elements_jacobian_(i) = static_cast<Real>(determinants[0]);
  }
}

template <Isize Dimension, ElementType Type>
inline void getElementJacobian(AdjacencyElementMesh<Dimension, Type>& Adjacency_element_mesh) {
  Adjacency_element_mesh.elements_jacobian_.resize(Adjacency_element_mesh.elements_num_.second);
  const std::vector<double> local_coord{0.0, 0.0, 0.0};
  std::vector<double> jacobians;
  std::vector<double> determinants;
  std::vector<double> coord;
  for (Isize i = 0; i < Adjacency_element_mesh.elements_num_.first; i++) {
    gmsh::model::mesh::getJacobian(static_cast<std::size_t>(i + Adjacency_element_mesh.internal_elements_range_.first),
                                   local_coord, jacobians, determinants, coord);
    Adjacency_element_mesh.elements_jacobian_(i) = static_cast<Real>(determinants[0]);
  }
  for (Isize i = 0; i < Adjacency_element_mesh.elements_num_.second - Adjacency_element_mesh.elements_num_.first; i++) {
    gmsh::model::mesh::getJacobian(static_cast<std::size_t>(i + Adjacency_element_mesh.boundary_elements_range_.first),
                                   local_coord, jacobians, determinants, coord);
    Adjacency_element_mesh.elements_jacobian_(i + Adjacency_element_mesh.elements_num_.first) =
        static_cast<Real>(determinants[0]);
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_ELEMENT_JACOBIAN_HPP_
