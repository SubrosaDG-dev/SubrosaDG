/**
 * @file get_adjacency_mesh.hpp
 * @brief The get adjacency element mesh head file.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-06-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_GET_ADJACENCY_MESH_HPP_
#define SUBROSA_DG_GET_ADJACENCY_MESH_HPP_

#include <gmsh.h>

#include <Eigen/Core>
#include <algorithm>
#include <array>
#include <functional>
#include <map>
#include <ranges>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "basic/concept.hpp"
#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "mesh/get_elem_info.hpp"
#include "mesh/get_mesh_supplemental.hpp"
#include "mesh/mesh_structure.hpp"

namespace SubrosaDG {

template <PolyOrder P, ElemType ElemT>
struct AdjacencyElemMeshSupplemental {
  bool is_recorded_;
  std::array<Isize, getNodeNum<ElemT>(P)> node_tags_;
  std::vector<Isize> parent_index_;
  std::vector<Isize> adjacency_index_;
  std::vector<Isize> typology_index_;
};

template <PolyOrder P, ElemType ElemT, ElemType ParentElemT, MeshType MeshT>
  requires Is2dElem<ParentElemT>
inline void getAdjacencyParentElem(std::map<Isize, AdjacencyElemMeshSupplemental<P, ElemT>>& adjacency_elem_map) {
  std::vector<Usize> edge_nodes_tags;
  std::vector<Usize> edge_2_nodes_tags;
  gmsh::model::mesh::getElementEdgeNodes(getTopology<ParentElemT>(P), edge_nodes_tags);
  Usize edge_num = edge_nodes_tags.size() / getNodeNum<ElemT>(P);
  for (Usize i = 0; i < edge_num; i++) {
    edge_2_nodes_tags.emplace_back(edge_nodes_tags[getNodeNum<ElemT>(P) * i]);
    edge_2_nodes_tags.emplace_back(edge_nodes_tags[getNodeNum<ElemT>(P) * i + 1]);
  }
  std::vector<int> edge_orientations;
  std::vector<Usize> edge_tags;
  gmsh::model::mesh::getEdges(edge_2_nodes_tags, edge_tags, edge_orientations);
  std::vector<Usize> elem_tags;
  std::vector<Usize> elem_node_tags;
  gmsh::model::mesh::getElementsByType(getTopology<ParentElemT>(P), elem_tags, elem_node_tags);
  for (Usize i = 0; i < edge_tags.size(); i++) {
    if (!adjacency_elem_map.contains(static_cast<Isize>(edge_tags[i]))) {  // NOTE: resize
      adjacency_elem_map[static_cast<Isize>(edge_tags[i])].is_recorded_ = false;
      for (Usize j = 0; j < getNodeNum<ElemT>(P); j++) {
        adjacency_elem_map[static_cast<Isize>(edge_tags[i])].node_tags_[j] =
            static_cast<Isize>(edge_nodes_tags[getNodeNum<ElemT>(P) * i + j]);
      }
    } else {
      adjacency_elem_map[static_cast<Isize>(edge_tags[i])].is_recorded_ = true;
    }
    adjacency_elem_map[static_cast<Isize>(edge_tags[i])].parent_index_.emplace_back(
        elem_tags[i / getElemAdjacencyNum<ParentElemT>()] - elem_tags.front());
    adjacency_elem_map[static_cast<Isize>(edge_tags[i])].adjacency_index_.emplace_back(
        i % getElemAdjacencyNum<ParentElemT>());
    if constexpr (IsMixed<MeshT>) {
      adjacency_elem_map[static_cast<Isize>(edge_tags[i])].typology_index_.emplace_back(getTopology<ParentElemT>(P));
    }
  }
}

template <PolyOrder P, ElemType ElemT, ElemType ParentElemT, MeshType MeshT>
  requires Is3dElem<ElemT>
inline void getAdjacencyParentElem(std::map<Isize, AdjacencyElemMeshSupplemental<P, ElemT>>& adjacency_elem_map);

template <PolyOrder P, ElemType ElemT, MeshType MeshT>
  requires Is1dElem<ElemT>
inline std::map<Isize, AdjacencyElemMeshSupplemental<P, ElemT>> getAdjacencyElemMap() {
  gmsh::model::mesh::createEdges();
  std::map<Isize, AdjacencyElemMeshSupplemental<P, ElemT>> adjacency_elem_map;
  if constexpr (HasTri<MeshT>) {
    getAdjacencyParentElem<P, ElemT, ElemType::Tri, MeshT>(adjacency_elem_map);
  }
  if constexpr (HasQuad<MeshT>) {
    getAdjacencyParentElem<P, ElemT, ElemType::Quad, MeshT>(adjacency_elem_map);
  }
  return adjacency_elem_map;
}

template <int Dim, PolyOrder P, ElemType ElemT, MeshType MeshT>
inline void getAdjacencyInternalElemMesh(
    const Eigen::Matrix<Real, Dim, Eigen::Dynamic>& node,
    const std::map<Isize, AdjacencyElemMeshSupplemental<P, ElemT>>& adjacency_elem_map,
    const std::vector<Isize>& internal_tag, AdjacencyElemMesh<Dim, P, ElemT, MeshT>& adjacency_elem_mesh) {
  Usize max_elem_tag;
  gmsh::model::mesh::getMaxElementTag(max_elem_tag);
  const auto [internal_min_tag, internal_max_tag] = std::ranges::minmax_element(internal_tag);
  adjacency_elem_mesh.internal_.range_ =
      std::make_pair(max_elem_tag + 1, *internal_max_tag - *internal_min_tag + static_cast<Isize>(max_elem_tag) + 1);
  adjacency_elem_mesh.internal_.num_ = *internal_max_tag - *internal_min_tag + 1;
  adjacency_elem_mesh.internal_.elem_.resize(adjacency_elem_mesh.internal_.num_);
  int entity_tag = gmsh::model::addDiscreteEntity(getDim<ElemT>());
  std::vector<Usize> elem_tags;
  std::vector<Usize> node_tags;
  for (Isize i = 0; i < adjacency_elem_mesh.internal_.num_; i++) {
    const AdjacencyElemMeshSupplemental<P, ElemT>& adjacency_elem_supplemental =
        adjacency_elem_map.at(i + *internal_min_tag);
    for (Usize j = 0; j < getNodeNum<ElemT>(P); j++) {
      adjacency_elem_mesh.internal_.elem_(i).node_.col(static_cast<Isize>(j)) =
          node.col(static_cast<Isize>(adjacency_elem_supplemental.node_tags_[j] - 1));
      adjacency_elem_mesh.internal_.elem_(i).index_(static_cast<Isize>(j)) = adjacency_elem_supplemental.node_tags_[j];
      node_tags.emplace_back(adjacency_elem_supplemental.node_tags_[j]);
    }
    for (Usize j = 0; j < 2; j++) {
      adjacency_elem_mesh.internal_.elem_(i).parent_index_(static_cast<Isize>(j)) =
          adjacency_elem_supplemental.parent_index_[j];
      adjacency_elem_mesh.internal_.elem_(i).adjacency_index_(static_cast<Isize>(j)) =
          adjacency_elem_supplemental.adjacency_index_[j];
      if constexpr (IsMixed<MeshT>) {
        adjacency_elem_mesh.internal_.elem_(i).typology_index_(static_cast<Isize>(j)) =
            static_cast<int>(adjacency_elem_supplemental.typology_index_[j]);
      }
    }
    elem_tags.emplace_back(static_cast<Usize>(i) + max_elem_tag + 1);
  }
  gmsh::model::mesh::addElementsByType(entity_tag, getTopology<ElemT>(P), elem_tags, node_tags);
}

template <int Dim, PolyOrder P, ElemType ElemT, MeshType MeshT>
inline void getAdjacencyBoundaryElemMesh(
    const Eigen::Matrix<Real, Dim, Eigen::Dynamic>& node, const MeshSupplemental<ElemT>& boundary_supplemental,
    const std::map<Isize, AdjacencyElemMeshSupplemental<P, ElemT>>& adjacency_elem_map,
    const std::vector<Isize>& boundary_tag, AdjacencyElemMesh<Dim, P, ElemT, MeshT>& adjacency_elem_mesh) {
  const auto [boundary_min_tag, boundary_max_tag] = std::ranges::minmax_element(boundary_tag);
  adjacency_elem_mesh.boundary_.range_ = std::make_pair(*boundary_min_tag, *boundary_max_tag);
  adjacency_elem_mesh.boundary_.num_ = *boundary_max_tag - *boundary_min_tag + 1;
  adjacency_elem_mesh.boundary_.elem_.resize(adjacency_elem_mesh.boundary_.num_);
  for (Isize i = 0; i < adjacency_elem_mesh.boundary_.num_; i++) {
    const AdjacencyElemMeshSupplemental<P, ElemT>& adjacency_elem_supplemental =
        adjacency_elem_map.at(i + *boundary_min_tag);
    for (Usize j = 0; j < getNodeNum<ElemT>(P); j++) {
      adjacency_elem_mesh.boundary_.elem_(i).node_.col(static_cast<Isize>(j)) =
          node.col(static_cast<Isize>(adjacency_elem_supplemental.node_tags_[j] - 1));
      adjacency_elem_mesh.boundary_.elem_(i).index_(static_cast<Isize>(j)) = adjacency_elem_supplemental.node_tags_[j];
    }
    adjacency_elem_mesh.boundary_.elem_(i).parent_index_(0) = adjacency_elem_supplemental.parent_index_[0];
    adjacency_elem_mesh.boundary_.elem_(i).adjacency_index_(0) = adjacency_elem_supplemental.adjacency_index_[0];
    if constexpr (IsMixed<MeshT>) {
      adjacency_elem_mesh.boundary_.elem_(i).typology_index_(0) =
          static_cast<int>(adjacency_elem_supplemental.typology_index_[0]);
    }
    adjacency_elem_mesh.boundary_.elem_(i).parent_index_(1) = boundary_supplemental.index_(i);
  }
}

template <int Dim, PolyOrder P, ElemType ElemT, MeshType MeshT>
inline void getAdjacencyElemMesh(const Eigen::Matrix<Real, Dim, Eigen::Dynamic>& node,
                                 const std::unordered_map<std::string_view, Boundary>& boundary_type_map,
                                 AdjacencyElemMesh<Dim, P, ElemT, MeshT>& adjacency_elem_mesh) {
  std::map<Isize, AdjacencyElemMeshSupplemental<P, ElemT>> adjacency_elem_map = getAdjacencyElemMap<P, ElemT, MeshT>();
  std::vector<Isize> internal_tag;
  std::vector<Isize> boundary_tag;
  for (const auto& [edge_tag, adjacency_elem_supplemental] : adjacency_elem_map) {
    if (adjacency_elem_supplemental.is_recorded_) [[likely]] {
      internal_tag.push_back(edge_tag);
    } else [[unlikely]] {
      boundary_tag.push_back(edge_tag);
    }
  }
  getAdjacencyInternalElemMesh(node, adjacency_elem_map, internal_tag, adjacency_elem_mesh);
  MeshSupplemental<ElemT> boundary_supplemental;
  getMeshSupplemental<Boundary, P, ElemT>(boundary_type_map, boundary_supplemental);
  getAdjacencyBoundaryElemMesh(node, boundary_supplemental, adjacency_elem_map, boundary_tag, adjacency_elem_mesh);
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_ADJACENCY_MESH_HPP_
