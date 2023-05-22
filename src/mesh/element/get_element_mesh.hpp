/**
 * @file get_element_mesh.hpp
 * @brief The get element mesh header file.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-22
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_GET_ELEMENT_MESH_HPP_
#define SUBROSA_DG_GET_ELEMENT_MESH_HPP_

// clang-format off

#include <gmsh.h>                  // for getElementsByType
#include <Eigen/Core>              // for Dynamic, Matrix
#include <cstddef>                 // for size_t
#include <utility>                 // for make_pair
#include <vector>                  // for vector

#include "basic/data_types.hpp"    // for Isize, Real, Usize
#include "mesh/element_types.hpp"  // for ElementType

// clang-format on

namespace SubrosaDG {

template <SubrosaDG::Isize Dimension, ElementType Type>
struct ElementMesh;

template <Isize Dimension, ElementType Type>
inline void getElementMesh(const Eigen::Matrix<Real, Dimension, Eigen::Dynamic>& nodes,
                           ElementMesh<Dimension, Type>& element_mesh) {
  std::vector<std::size_t> elem_tags;
  std::vector<std::size_t> elem_node_tag;
  gmsh::model::mesh::getElementsByType(Type.kElementTag, elem_tags, elem_node_tag);
  if (elem_tags.empty()) {
    element_mesh.elements_range_ = std::make_pair(0, 0);
    element_mesh.elements_num_ = 0;
  } else {
    element_mesh.elements_range_ = std::make_pair(elem_tags.front(), elem_tags.back());
    element_mesh.elements_num_ = static_cast<Isize>(elem_tags.size());
    element_mesh.elements_nodes_.resize(Type.kDimension * Type.kNodesNumPerElement, element_mesh.elements_num_);
    element_mesh.elements_index_.resize(Type.kNodesNumPerElement, element_mesh.elements_num_);
    for (const auto elem_tag : elem_tags) {
      for (Isize i = 0; i < Type.kNodesNumPerElement; i++) {
        auto node_tag = static_cast<Isize>(elem_node_tag[static_cast<Usize>(
            (static_cast<Isize>(elem_tag) - element_mesh.elements_range_.first) * Type.kNodesNumPerElement + i)]);
        for (Isize j = 0; j < Type.kDimension; j++) {
          element_mesh.elements_nodes_(i * Dimension + j,
                                       static_cast<Isize>(elem_tag) - element_mesh.elements_range_.first) =
              nodes(j, node_tag - 1);
        }
        element_mesh.elements_index_(i, static_cast<Isize>(elem_tag) - element_mesh.elements_range_.first) = node_tag;
      }
    }
  }
}

} // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_ELEMENT_MESH_HPP_
