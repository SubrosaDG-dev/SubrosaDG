/**
 * @file reconstruct_adjacency.hpp
 * @brief The head file to reconstrcuct adjacency from mesh.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-04-27
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_RECONSTRUCT_ADJACENCY_HPP_
#define SUBROSA_DG_RECONSTRUCT_ADJACENCY_HPP_

// clang-format off

#include <gmsh.h>                  // for addDiscreteEntity, addElementsByType, createEdges, getEdges, getElementEdg...
#include <map>                     // for map, operator==, _Rb_tree_iterator
#include <Eigen/Core>              // for Matrix, DenseCoeffsBase, Dynamic
#include <vector>                  // for vector
#include <utility>                 // for pair, make_pair
#include <algorithm>               // for max, minmax_element_result, __minmax_element_fn, minmax_element
#include <cstddef>                 // for size_t
#include <functional>              // for identity, less

#include "basic/data_types.hpp"    // for Usize, Isize, Real
#include "mesh/element_types.hpp"  // for ElementType, kQuadrangle, kTriangle

// clang-format on

namespace SubrosaDG {

template <ElementType Type>
struct AdjanentElementMesh;

template <ElementType Type>
struct MeshSupplemental;

template <ElementType Type>
inline void getEdgeElementMap(std::map<Usize, std::pair<bool, std::vector<Isize>>>& edge_element_map) {
  std::vector<std::size_t> edge_nodes_tags;
  gmsh::model::mesh::getElementEdgeNodes(Type.kElementTag, edge_nodes_tags);
  std::vector<int> edge_orientations;
  std::vector<std::size_t> edge_tags;
  gmsh::model::mesh::getEdges(edge_nodes_tags, edge_tags, edge_orientations);
  std::vector<std::size_t> element_tags;
  std::vector<std::size_t> element_node_tags;
  gmsh::model::mesh::getElementsByType(Type.kElementTag, element_tags, element_node_tags);
  for (std::size_t i = 0; i < edge_tags.size(); i++) {
    if (!edge_element_map.contains(edge_tags[i])) {
      edge_element_map[edge_tags[i]].first = false;
      edge_element_map[edge_tags[i]].second.emplace_back(static_cast<Isize>(edge_nodes_tags[2 * i]));
      edge_element_map[edge_tags[i]].second.emplace_back(static_cast<Isize>(edge_nodes_tags[2 * i + 1]));
      edge_element_map[edge_tags[i]].second.emplace_back(
          static_cast<Isize>(element_tags[i / Type.kNodesNumPerElement]));
    } else {
      edge_element_map[edge_tags[i]].first = true;
      edge_element_map[edge_tags[i]].second.emplace_back(
          static_cast<Isize>(element_tags[i / Type.kNodesNumPerElement]));
    }
  }
}

template <ElementType Type>
inline void reconstructAdjacency(const Eigen::Matrix<Real, 3, Eigen::Dynamic>& nodes,
                                 AdjanentElementMesh<Type>& internal, AdjanentElementMesh<Type>& boundary,
                                 const MeshSupplemental<Type>& boundary_supplemental) {
  gmsh::model::mesh::createEdges();
  std::map<Usize, std::pair<bool, std::vector<Isize>>> edge_element_map;
  if constexpr (Type.kDimension == 1) {
    getEdgeElementMap<kTriangle>(edge_element_map);
    getEdgeElementMap<kQuadrangle>(edge_element_map);
  }
  std::vector<Usize> internal_tag;
  std::vector<Usize> boundary_tag;
  std::size_t max_element_tag;
  gmsh::model::mesh::getMaxElementTag(max_element_tag);
  for (const auto& [edge_tag, element_pair] : edge_element_map) {
    const auto& [flag, elements_index] = element_pair;
    if (flag) [[likely]] {
      internal_tag.push_back(edge_tag);
    } else [[unlikely]] {
      boundary_tag.push_back(edge_tag);
    }
  }
  const auto& [internal_min, internal_max] = std::ranges::minmax_element(internal_tag);
  internal.elements_range_ = std::make_pair(max_element_tag + 1, *internal_max - *internal_min + max_element_tag + 1);
  internal.elements_num_ = internal_max - internal_min + 1;
  const auto& [boundary_min, boundary_max] = std::ranges::minmax_element(boundary_tag);
  boundary.elements_range_ = std::make_pair(*boundary_min, *boundary_max);
  boundary.elements_num_ = boundary_max - boundary_min + 1;
  internal.elements_nodes_.resize(Type.kNodesNumPerElement * 3, internal.elements_num_);
  boundary.elements_nodes_.resize(Type.kNodesNumPerElement * 3, boundary.elements_num_);
  internal.elements_index_.resize(Type.kNodesNumPerElement + 2, internal.elements_num_);
  boundary.elements_index_.resize(Type.kNodesNumPerElement + 2, boundary.elements_num_);
  int entity_tag = gmsh::model::addDiscreteEntity(Type.kDimension);
  std::vector<std::size_t> element_tags;
  std::vector<std::size_t> node_tags;
  for (const auto& edge_tag : internal_tag) {
    const auto& [flag, elements_index] = edge_element_map[edge_tag];
    for (Isize i = 0; i < Type.kNodesNumPerElement * 3; i++) {
      internal.elements_nodes_(i, static_cast<Isize>(edge_tag - *internal_min)) =
          nodes(i / 3, elements_index[static_cast<Usize>(i / 3)] - 1);
    }
    for (Isize i = 0; i < Type.kNodesNumPerElement + 2; i++) {
      internal.elements_index_(i, static_cast<Isize>(edge_tag - *internal_min)) = elements_index[static_cast<Usize>(i)];
    }
    element_tags.emplace_back(static_cast<std::size_t>(edge_tag - *internal_min + max_element_tag + 1));
    for (Isize i = 0; i < Type.kNodesNumPerElement; i++) {
      node_tags.emplace_back(elements_index[static_cast<Usize>(i)]);
    }
  }
  gmsh::model::mesh::addElementsByType(entity_tag, Type.kElementTag, element_tags, node_tags);
  for (const auto& edge_tag : boundary_tag) {
    const auto& [flag, elements_index] = edge_element_map[edge_tag];
    for (Isize i = 0; i < Type.kNodesNumPerElement * 3; i++) {
      boundary.elements_nodes_(i, static_cast<Isize>(edge_tag) - boundary.elements_range_.first) =
          nodes(i / 3, elements_index[static_cast<Usize>(i / 3)] - 1);
    }
    for (Isize i = 0; i < Type.kNodesNumPerElement + 1; i++) {
      boundary.elements_index_(i, static_cast<Isize>(edge_tag) - boundary.elements_range_.first) =
          elements_index[static_cast<Usize>(i)];
    }
    boundary.elements_index_(Type.kNodesNumPerElement + 1,
                             static_cast<Isize>(edge_tag) - boundary.elements_range_.first) =
        boundary_supplemental.index_(static_cast<Isize>(edge_tag) - boundary_supplemental.range_.first);
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_RECONSTRUCT_ADJACENCY_HPP_
