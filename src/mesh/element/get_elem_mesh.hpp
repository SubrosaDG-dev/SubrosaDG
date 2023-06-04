/**
 * @file get_elem_mesh.hpp
 * @brief The get element mesh header file.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-22
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_GET_ELEM_MESH_HPP_
#define SUBROSA_DG_GET_ELEM_MESH_HPP_

// clang-format off

#include <gmsh.h>               // for getElementsByType, addDiscreteEntity, addElementsByType, createEdges, getEdges
#include <Eigen/Core>           // for DenseBase::col, Dynamic, Matrix
#include <utility>              // for make_pair
#include <vector>               // for vector
#include <map>                  // for map, operator==, _Rb_tree_iterator
#include <algorithm>            // for max, minmax_element_result, __minmax_element_fn, minmax_element
#include <functional>           // for identity, less

#include "basic/data_type.hpp"  // for Isize, Usize, Real
#include "basic/enum.hpp"       // for MeshType
#include "mesh/elem_type.hpp"   // for ElemInfo, kQuad, kTri

// clang-format on

namespace SubrosaDG {

template <int Dim, ElemInfo ElemT>
struct ElemMesh;
template <int Dim, ElemInfo ElemT>
struct AdjacencyElemMesh;
template <int ElemDim>
struct MeshSupplemental;

struct AdjacencyElemMeshSupplemental {
  bool is_recorded_;
  std::vector<Isize> index_;
};

template <int Dim, ElemInfo ElemT>
inline void getElemMesh(const Eigen::Matrix<Real, Dim, Eigen::Dynamic>& nodes, ElemMesh<Dim, ElemT>& elem_mesh) {
  std::vector<Usize> elem_tags;
  std::vector<Usize> elem_node_tag;
  gmsh::model::mesh::getElementsByType(ElemT.kTopology, elem_tags, elem_node_tag);
  if (elem_tags.empty()) {
    elem_mesh.range_ = std::make_pair(0, 0);
    elem_mesh.num_ = 0;
  } else {
    elem_mesh.range_ = std::make_pair(elem_tags.front(), elem_tags.back());
    elem_mesh.num_ = static_cast<Isize>(elem_tags.size());
    elem_mesh.elem_.resize(elem_mesh.num_);
    for (Isize i = 0; i < elem_mesh.num_; i++) {
      for (Isize j = 0; j < ElemT.kNodeNum; j++) {
        auto node_tag = static_cast<Isize>(elem_node_tag[static_cast<Usize>(i * ElemT.kNodeNum + j)]);
        elem_mesh.elem_(i).node_.col(j) = nodes.col(node_tag - 1);
        elem_mesh.elem_(i).index_(j) = node_tag;
      }
    }
  }
}

template <ElemInfo ElemT>
inline void getAdjacencyElemTypeMap2d(std::map<Isize, AdjacencyElemMeshSupplemental>& adjacency_elem_map) {
  std::vector<Usize> edge_nodes_tags;
  gmsh::model::mesh::getElementEdgeNodes(ElemT.kTopology, edge_nodes_tags);
  std::vector<int> edge_orientations;
  std::vector<Usize> edge_tags;
  gmsh::model::mesh::getEdges(edge_nodes_tags, edge_tags, edge_orientations);
  std::vector<Usize> elem_tags;
  std::vector<Usize> elem_node_tags;
  gmsh::model::mesh::getElementsByType(ElemT.kTopology, elem_tags, elem_node_tags);
  for (Usize i = 0; i < edge_tags.size(); i++) {
    if (!adjacency_elem_map.contains(static_cast<Isize>(edge_tags[i]))) {
      adjacency_elem_map[static_cast<Isize>(edge_tags[i])].is_recorded_ = false;
      adjacency_elem_map[static_cast<Isize>(edge_tags[i])].index_.emplace_back(edge_nodes_tags[2 * i]);
      adjacency_elem_map[static_cast<Isize>(edge_tags[i])].index_.emplace_back(edge_nodes_tags[2 * i + 1]);
    } else {
      adjacency_elem_map[static_cast<Isize>(edge_tags[i])].is_recorded_ = true;
    }
    adjacency_elem_map[static_cast<Isize>(edge_tags[i])].index_.emplace_back(elem_tags[i / ElemT.kNodeNum] -
                                                                             elem_tags.front());
    adjacency_elem_map[static_cast<Isize>(edge_tags[i])].index_.emplace_back(ElemT.kTopology);
    adjacency_elem_map[static_cast<Isize>(edge_tags[i])].index_.emplace_back(i % ElemT.kNodeNum);
  }
}

template <MeshType MeshT>
inline std::map<Isize, AdjacencyElemMeshSupplemental> getAdjacencyElemMap2d() {
  gmsh::model::mesh::createEdges();
  std::map<Isize, AdjacencyElemMeshSupplemental> adjacency_elem_map;
  if constexpr (MeshT == MeshType::Tri) {
    getAdjacencyElemTypeMap2d<kTri>(adjacency_elem_map);
  } else if constexpr (MeshT == MeshType::Quad) {
    getAdjacencyElemTypeMap2d<kQuad>(adjacency_elem_map);
  } else if constexpr (MeshT == MeshType::TriQuad) {
    getAdjacencyElemTypeMap2d<kTri>(adjacency_elem_map);
    getAdjacencyElemTypeMap2d<kQuad>(adjacency_elem_map);
  }
  return adjacency_elem_map;
}

template <MeshType MeshT, ElemInfo ElemT>
inline std::map<Isize, AdjacencyElemMeshSupplemental> getAdjacencyElemMap() {
  if constexpr (ElemT.kDim == 1) {
    return getAdjacencyElemMap2d<MeshT>();
  }
}

template <int Dim, ElemInfo ElemT>
inline void getAdjacencyInternalElemMesh(const Eigen::Matrix<Real, Dim, Eigen::Dynamic>& node,
                                         const std::map<Isize, AdjacencyElemMeshSupplemental>& adjacency_elem_map,
                                         const std::vector<Isize>& internal_tag,
                                         AdjacencyElemMesh<Dim, ElemT>& adjacency_elem_mesh) {
  Usize max_elem_tag;
  gmsh::model::mesh::getMaxElementTag(max_elem_tag);
  const auto [internal_min_tag, internal_max_tag] = std::ranges::minmax_element(internal_tag);
  adjacency_elem_mesh.internal_.range_ =
      std::make_pair(max_elem_tag + 1, *internal_max_tag - *internal_min_tag + static_cast<Isize>(max_elem_tag) + 1);
  adjacency_elem_mesh.internal_.num_ = *internal_max_tag - *internal_min_tag + 1;
  adjacency_elem_mesh.internal_.elem_.resize(adjacency_elem_mesh.internal_.num_);
  int entity_tag = gmsh::model::addDiscreteEntity(ElemT.kDim);
  std::vector<Usize> elem_tags;
  std::vector<Usize> node_tags;
  for (Isize i = 0; i < adjacency_elem_mesh.internal_.num_; i++) {
    const AdjacencyElemMeshSupplemental& adjacency_elem_supplemental = adjacency_elem_map.at(i + *internal_min_tag);
    for (Usize j = 0; j < ElemT.kNodeNum; j++) {
      adjacency_elem_mesh.internal_.elem_(i).node_.col(static_cast<Isize>(j)) =
          node.col(static_cast<Isize>(adjacency_elem_supplemental.index_[j] - 1));
      adjacency_elem_mesh.internal_.elem_(i).index_(static_cast<Isize>(j)) = adjacency_elem_supplemental.index_[j];
      node_tags.emplace_back(adjacency_elem_supplemental.index_[j]);
    }
    for (Usize j = 0; j < 6; j++) {
      adjacency_elem_mesh.internal_.elem_(i).index_(ElemT.kNodeNum + static_cast<Isize>(j)) =
          adjacency_elem_supplemental.index_[ElemT.kNodeNum + j];
    }
    elem_tags.emplace_back(static_cast<Usize>(i) + max_elem_tag + 1);
  }
  gmsh::model::mesh::addElementsByType(entity_tag, ElemT.kTopology, elem_tags, node_tags);
}

template <int Dim, ElemInfo ElemT>
inline void getAdjacencyBoundaryElemMesh(const Eigen::Matrix<Real, Dim, Eigen::Dynamic>& node,
                                         const MeshSupplemental<ElemT.kDim>& boundary_supplemental,
                                         const std::map<Isize, AdjacencyElemMeshSupplemental>& adjacency_elem_map,
                                         const std::vector<Isize>& boundary_tag,
                                         AdjacencyElemMesh<Dim, ElemT>& adjacency_elem_mesh) {
  const auto [boundary_min_tag, boundary_max_tag] = std::ranges::minmax_element(boundary_tag);
  adjacency_elem_mesh.boundary_.range_ = std::make_pair(*boundary_min_tag, *boundary_max_tag);
  adjacency_elem_mesh.boundary_.num_ = *boundary_max_tag - *boundary_min_tag + 1;
  adjacency_elem_mesh.boundary_.elem_.resize(adjacency_elem_mesh.boundary_.num_);
  for (Isize i = 0; i < adjacency_elem_mesh.boundary_.num_; i++) {
    const AdjacencyElemMeshSupplemental& adjacency_elem_supplemental = adjacency_elem_map.at(i + *boundary_min_tag);
    for (Usize j = 0; j < ElemT.kNodeNum; j++) {
      adjacency_elem_mesh.boundary_.elem_(i).node_.col(static_cast<Isize>(j)) =
          node.col(static_cast<Isize>(adjacency_elem_supplemental.index_[j] - 1));
      adjacency_elem_mesh.boundary_.elem_(i).index_(static_cast<Isize>(j)) = adjacency_elem_supplemental.index_[j];
    }
    for (Usize j = 0; j < 3; j++) {
      adjacency_elem_mesh.boundary_.elem_(i).index_(ElemT.kNodeNum + static_cast<Isize>(j)) =
          adjacency_elem_supplemental.index_[ElemT.kNodeNum + j];
    }
    adjacency_elem_mesh.boundary_.elem_(i).index_(ElemT.kNodeNum + 3) = boundary_supplemental.index_(i);
  }
}

template <int Dim, MeshType MeshT, ElemInfo ElemT>
inline void getAdjacencyElemMesh(const Eigen::Matrix<Real, Dim, Eigen::Dynamic>& node,
                                 const MeshSupplemental<ElemT.kDim>& boundary_supplemental,
                                 AdjacencyElemMesh<Dim, ElemT>& adjacency_elem_mesh) {
  std::map<Isize, AdjacencyElemMeshSupplemental> adjacency_elem_map = getAdjacencyElemMap<MeshT, ElemT>();
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
  getAdjacencyBoundaryElemMesh(node, boundary_supplemental, adjacency_elem_map, boundary_tag, adjacency_elem_mesh);
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_ELEM_MESH_HPP_
