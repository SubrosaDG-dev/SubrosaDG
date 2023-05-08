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

#include <gmsh.h>                 // for getElementType, createEdges, getEdges, getElementEdgeNodes, getElementsByType
#include <map>                    // for map, operator==, _Rb_tree_iterator
#include <Eigen/Core>             // for Matrix, Dynamic
#include <cstddef>                // for size_t
#include <memory>                 // for unique_ptr, make_unique
#include <string>                 // for allocator, string
#include <unordered_map>          // for unordered_map
#include <vector>                 // for vector
#include <algorithm>              // for max_element
#include <utility>                // for move, pair
#include <string_view>            // for string_view, hash, operator==

#include "mesh/mesh_map.h"        // for kElementPointNumMap
#include "basic/data_types.h"     // for Usize, Isize
#include "mesh/mesh_structure.h"  // for Mesh2d, MeshSupplementalInfo

// clang-format on

namespace SubrosaDG::Internal {

void reconstructEdge(Mesh2d& mesh, const MeshSupplementalInfo& mesh_supplemental_info) {
  gmsh::model::mesh::createEdges();
  EdgeInfo triangle_edge_info{"Triangle"};
  EdgeInfo quadrangle_edge_info{"Quadrangle"};
  std::vector<Usize> num_edges{getMaxEdgeTag(triangle_edge_info), getMaxEdgeTag(quadrangle_edge_info)};
  mesh.num_edges_ = *std::max_element(num_edges.begin(), num_edges.end());
  mesh.iedges_ = std::make_unique<Eigen::Matrix<Isize, 4, Eigen::Dynamic>>(4, mesh.num_edges_);
  std::map<Usize, bool> edges2_elements;
  reconstructElementEdge(edges2_elements, triangle_edge_info, mesh);
  reconstructElementEdge(edges2_elements, quadrangle_edge_info, mesh);
  for (const auto& [tag, flag] : edges2_elements) {
    if (!flag) {
      mesh.iedges_->operator()(3, static_cast<Isize>(tag - 1)) = mesh_supplemental_info.iboundary_->operator()(
          static_cast<Isize>(tag - mesh_supplemental_info.num_boundary_.first));
    }
  }
}

EdgeInfo::EdgeInfo(std::string element_name) : element_name_(std::move(element_name)) {}

Usize getMaxEdgeTag(EdgeInfo& edge_info) {
  Usize max_edge_tag = 0;
  std::vector<int> edge_orientations;
  int element_type = gmsh::model::mesh::getElementType(edge_info.element_name_, 1);
  gmsh::model::mesh::getElementEdgeNodes(element_type, edge_info.edge_nodes_);
  if (!edge_info.edge_nodes_.empty()) {
    gmsh::model::mesh::getEdges(edge_info.edge_nodes_, edge_info.edge_tags_, edge_orientations);
    max_edge_tag = *std::max_element(edge_info.edge_tags_.begin(), edge_info.edge_tags_.end());
  }
  return max_edge_tag;
}

void reconstructElementEdge(std::map<Usize, bool>& edges2_elements, EdgeInfo& edge_info, Mesh2d& mesh) {
  int element_type = gmsh::model::mesh::getElementType(edge_info.element_name_, 1);
  Usize element_point_num = kElementPointNumMap.at(edge_info.element_name_);
  std::vector<std::size_t> element_node_tags;
  gmsh::model::mesh::getElementsByType(element_type, edge_info.element_tags_, element_node_tags);
  for (std::size_t i = 0; i < edge_info.edge_tags_.size(); i++) {
    if (!edges2_elements.contains(edge_info.edge_tags_[i])) {
      edges2_elements[edge_info.edge_tags_[i]] = false;
      mesh.iedges_->operator()(0, static_cast<Isize>(edge_info.edge_tags_[i] - 1)) =
          static_cast<Isize>(edge_info.edge_nodes_[2 * i]);
      mesh.iedges_->operator()(1, static_cast<Isize>(edge_info.edge_tags_[i] - 1)) =
          static_cast<Isize>(edge_info.edge_nodes_[2 * i + 1]);
      mesh.iedges_->operator()(2, static_cast<Isize>(edge_info.edge_tags_[i] - 1)) =
          static_cast<Isize>(edge_info.element_tags_[i / element_point_num]);
    } else {
      edges2_elements[edge_info.edge_tags_[i]] = true;
      mesh.iedges_->operator()(3, static_cast<Isize>(edge_info.edge_tags_[i] - 1)) =
          static_cast<Isize>(edge_info.element_tags_[i / element_point_num]);
    }
  }
}

}  // namespace SubrosaDG::Internal
