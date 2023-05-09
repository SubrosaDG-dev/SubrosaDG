/**
 * @file reconstruct_edge.cpp
 * @brief The source file to reconstrcuct edge from mesh.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-05
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

// clang-format off

#include "mesh/reconstruct_edge.h"

#include <gmsh.h>                 // for createEdges, getEdges, getElementEdgeNodes, getElementType, getElementsByType
#include <map>                    // for map, operator==, _Rb_tree_iterator
#include <Eigen/Core>             // for Matrix, Dynamic
#include <cstddef>                // for size_t
#include <memory>                 // for make_unique, unique_ptr, allocator, shared_ptr, __shared_ptr_access
#include <vector>                 // for vector
#include <algorithm>              // for max_element, min_element, max
#include <utility>                // for pair, make_pair
#include <string_view>            // for string_view, basic_string_view

#include "basic/data_types.h"     // for Isize, Usize, Real
#include "mesh/mesh_structure.h"  // for Edge, MeshSupplementalInfo

// clang-format on

namespace SubrosaDG::Internal {

// NOTE: use std::make_pair(std::ref(a), std::ref(b)) to avoid copy
Isize reconstructEdge(const std::shared_ptr<Eigen::Matrix<Real, 3, Eigen::Dynamic>>& nodes,
                      std::pair<Edge&, Edge&> edge, const MeshSupplementalInfo& mesh_supplemental_info) {
  gmsh::model::mesh::createEdges();
  std::map<Usize, std::pair<bool, std::vector<Isize>>> edge_element_map;
  getEdgeElementMap(edge_element_map, std::make_pair("Triangle", 3));
  getEdgeElementMap(edge_element_map, std::make_pair("Quadrangle", 4));
  std::vector<Usize> interior_edge_tag;
  std::vector<Usize> boundary_edge_tag;
  for (const auto& [edge_tag, element_pair] : edge_element_map) {
    const auto& [flag, element_index] = element_pair;
    if (flag) {
      interior_edge_tag.push_back(edge_tag);
    } else {
      boundary_edge_tag.push_back(edge_tag);
    }
  }
  edge.first.edge_num_ = std::make_pair(*std::min_element(interior_edge_tag.begin(), interior_edge_tag.end()),
                                        *std::max_element(interior_edge_tag.begin(), interior_edge_tag.end()));
  edge.second.edge_num_ = std::make_pair(*std::min_element(boundary_edge_tag.begin(), boundary_edge_tag.end()),
                                         *std::max_element(boundary_edge_tag.begin(), boundary_edge_tag.end()));
  Isize interior_edge_num = edge.first.edge_num_.second - edge.first.edge_num_.first + 1;
  Isize boundary_edge_num = edge.second.edge_num_.second - edge.second.edge_num_.first + 1;
  edge.first.edge_nodes_ = std::make_unique<Eigen::Matrix<Real, 6, Eigen::Dynamic>>(6, interior_edge_num);
  edge.second.edge_nodes_ = std::make_unique<Eigen::Matrix<Real, 6, Eigen::Dynamic>>(6, boundary_edge_num);
  edge.first.edge_index_ = std::make_unique<Eigen::Matrix<Isize, 4, Eigen::Dynamic>>(4, interior_edge_num);
  edge.second.edge_index_ = std::make_unique<Eigen::Matrix<Isize, 4, Eigen::Dynamic>>(4, boundary_edge_num);
  for (const auto& edge_tag : interior_edge_tag) {
    const auto& [flag, element_index] = edge_element_map[edge_tag];
    for (Isize i = 0; i < 6; i++) {
      edge.first.edge_nodes_->operator()(i, static_cast<Isize>(edge_tag) - edge.first.edge_num_.first) =
          nodes->operator()(i / 3, element_index[static_cast<Usize>(i / 3)] - 1);
    }
    for (Isize i = 0; i < 4; i++) {
      edge.first.edge_index_->operator()(i, static_cast<Isize>(edge_tag) - edge.first.edge_num_.first) =
          element_index[static_cast<Usize>(i)];
    }
  }
  for (const auto& edge_tag : boundary_edge_tag) {
    const auto& [flag, element_index] = edge_element_map[edge_tag];
    for (Isize i = 0; i < 6; i++) {
      edge.second.edge_nodes_->operator()(i, static_cast<Isize>(edge_tag) - edge.second.edge_num_.first) =
          nodes->operator()(i / 3, element_index[static_cast<Usize>(i / 3)] - 1);
    }
    for (Isize i = 0; i < 3; i++) {
      edge.second.edge_index_->operator()(i, static_cast<Isize>(edge_tag) - edge.second.edge_num_.first) =
          element_index[static_cast<Usize>(i)];
    }
    edge.second.edge_index_->operator()(3, static_cast<Isize>(edge_tag) - edge.second.edge_num_.first) =
        mesh_supplemental_info.boundary_index_->operator()(static_cast<Isize>(edge_tag) -
                                                           mesh_supplemental_info.boundary_num_.first);
  }
  return std::max(edge.first.edge_num_.second, edge.second.edge_num_.second);
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
