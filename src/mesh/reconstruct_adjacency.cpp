/**
 * @file reconstruct_adjacency.cpp
 * @brief The source file to reconstrcuct adjacency from mesh.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-05
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

// clang-format off

#include "mesh/reconstruct_adjacency.h"

#include <gmsh.h>                 // for createEdges, getEdges, getElementEdgeNodes, getElementType, getElementsByType
#include <map>                    // for map, operator==, _Rb_tree_iterator
#include <Eigen/Core>             // for Matrix, DenseCoeffsBase, Dynamic, Vector
#include <cstddef>                // for size_t
#include <vector>                 // for vector, allocator
#include <algorithm>              // for minmax_element_result, __minmax_element_fn, minmax_element
#include <utility>                // for pair, make_pair
#include <string_view>            // for string_view, basic_string_view
#include <functional>             // for identity, less

#include "basic/data_types.h"     // for Usize, Isize, Real
#include "mesh/mesh_structure.h"  // for AdjanencyElement, MeshSupplemental

// clang-format on

namespace SubrosaDG::Internal {

void reconstructAdjacency(const Eigen::Matrix<Real, 3, Eigen::Dynamic>& nodes, AdjanencyElement& interior_line,
                          AdjanencyElement& boundary_line, const MeshSupplemental& boundary_supplemental) {
  gmsh::model::mesh::createEdges();
  std::map<Usize, std::pair<bool, std::vector<Isize>>> edge_element_map;
  getEdgeElementMap(edge_element_map, std::make_pair("Triangle", 3));
  getEdgeElementMap(edge_element_map, std::make_pair("Quadrangle", 4));
  std::vector<Usize> interior_edge_tag;
  std::vector<Usize> boundary_edge_tag;
  for (const auto& [edge_tag, element_pair] : edge_element_map) {
    const auto& [flag, elements_index] = element_pair;
    if (flag) [[likely]] {
      interior_edge_tag.push_back(edge_tag);
    } else [[unlikely]] {
      boundary_edge_tag.push_back(edge_tag);
    }
  }
  const auto& [interior_min, interior_max] = std::ranges::minmax_element(interior_edge_tag);
  interior_line.elements_range_ = std::make_pair(*interior_min, *interior_max);
  interior_line.elements_num_ = interior_max - interior_min + 1;
  const auto& [boundary_min, boundary_max] = std::ranges::minmax_element(boundary_edge_tag);
  boundary_line.elements_range_ = std::make_pair(*boundary_min, *boundary_max);
  boundary_line.elements_num_ = boundary_max - boundary_min + 1;
  interior_line.elements_nodes_.resize(6, interior_line.elements_num_);
  boundary_line.elements_nodes_.resize(6, boundary_line.elements_num_);
  interior_line.elements_index_.resize(4, interior_line.elements_num_);
  boundary_line.elements_index_.resize(4, boundary_line.elements_num_);
  for (const auto& edge_tag : interior_edge_tag) {
    const auto& [flag, elements_index] = edge_element_map[edge_tag];
    for (Isize i = 0; i < 6; i++) {
      interior_line.elements_nodes_(i, static_cast<Isize>(edge_tag) - interior_line.elements_range_.first) =
          nodes(i / 3, elements_index[static_cast<Usize>(i / 3)] - 1);
    }
    for (Isize i = 0; i < 4; i++) {
      interior_line.elements_index_(i, static_cast<Isize>(edge_tag) - interior_line.elements_range_.first) =
          elements_index[static_cast<Usize>(i)];
    }
  }
  for (const auto& edge_tag : boundary_edge_tag) {
    const auto& [flag, elements_index] = edge_element_map[edge_tag];
    for (Isize i = 0; i < 6; i++) {
      boundary_line.elements_nodes_(i, static_cast<Isize>(edge_tag) - boundary_line.elements_range_.first) =
          nodes(i / 3, elements_index[static_cast<Usize>(i / 3)] - 1);
    }
    for (Isize i = 0; i < 3; i++) {
      boundary_line.elements_index_(i, static_cast<Isize>(edge_tag) - boundary_line.elements_range_.first) =
          elements_index[static_cast<Usize>(i)];
    }
    boundary_line.elements_index_(3, static_cast<Isize>(edge_tag) - boundary_line.elements_range_.first) =
        boundary_supplemental.index_(static_cast<Isize>(edge_tag) - boundary_supplemental.range_.first);
  }
}

void getEdgeElementMap(std::map<Usize, std::pair<bool, std::vector<Isize>>>& edge_element_map,
                       const std::pair<std::string_view, Usize>& element_type_info) {
  std::vector<std::size_t> edge_nodes_tags;
  int element_type = gmsh::model::mesh::getElementType(element_type_info.first.data(), 1);
  gmsh::model::mesh::getElementEdgeNodes(element_type, edge_nodes_tags);
  std::vector<int> edge_orientations;
  std::vector<std::size_t> edge_tags;
  gmsh::model::mesh::getEdges(edge_nodes_tags, edge_tags, edge_orientations);
  std::vector<std::size_t> element_tags;
  std::vector<std::size_t> element_node_tags;
  gmsh::model::mesh::getElementsByType(element_type, element_tags, element_node_tags);
  for (std::size_t i = 0; i < edge_tags.size(); i++) {
    if (!edge_element_map.contains(edge_tags[i])) {
      edge_element_map[edge_tags[i]].first = false;
      edge_element_map[edge_tags[i]].second.emplace_back(static_cast<Isize>(edge_nodes_tags[2 * i]));
      edge_element_map[edge_tags[i]].second.emplace_back(static_cast<Isize>(edge_nodes_tags[2 * i + 1]));
      edge_element_map[edge_tags[i]].second.emplace_back(
          static_cast<Isize>(element_tags[i / element_type_info.second]));
    } else {
      edge_element_map[edge_tags[i]].first = true;
      edge_element_map[edge_tags[i]].second.emplace_back(
          static_cast<Isize>(element_tags[i / element_type_info.second]));
    }
  }
}

}  // namespace SubrosaDG::Internal
