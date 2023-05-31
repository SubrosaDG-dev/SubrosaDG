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
#include <Eigen/Core>           // for Dynamic, Matrix
#include <cstddef>              // for size_t
#include <utility>              // for pair, make_pair
#include <vector>               // for vector
#include <map>                  // for map, operator==, _Rb_tree_iterator
#include <algorithm>            // for max, minmax_element_result, __minmax_element_fn, minmax_element
#include <functional>           // for identity, less

#include "basic/data_type.hpp"  // for Usize, Isize, Real
#include "mesh/elem_type.hpp"   // for ElemInfo, kQuad, kTri

// clang-format on

namespace SubrosaDG {

template <SubrosaDG::Isize Dim, ElemInfo ElemT>
struct ElemMesh;
template <Isize Dim, ElemInfo ElemT>
struct AdjacencyElemMesh;
template <ElemInfo ElemT>
struct MeshSupplemental;

template <Isize Dim, ElemInfo ElemT>
inline void getElemMesh(const Eigen::Matrix<Real, Dim, Eigen::Dynamic>& nodes, ElemMesh<Dim, ElemT>& elem_mesh) {
  std::vector<std::size_t> elem_tags;
  std::vector<std::size_t> elem_node_tag;
  gmsh::model::mesh::getElementsByType(ElemT.kTag, elem_tags, elem_node_tag);
  if (elem_tags.empty()) {
    elem_mesh.range_ = std::make_pair(0, 0);
    elem_mesh.num_ = 0;
  } else {
    elem_mesh.range_ = std::make_pair(elem_tags.front(), elem_tags.back());
    elem_mesh.num_ = static_cast<Isize>(elem_tags.size());
    elem_mesh.node_.resize(ElemT.kDim * ElemT.kNodeNum, elem_mesh.num_);
    elem_mesh.index_.resize(ElemT.kNodeNum, elem_mesh.num_);
    for (const auto elem_tag : elem_tags) {
      for (Isize i = 0; i < ElemT.kNodeNum; i++) {
        auto node_tag = static_cast<Isize>(elem_node_tag[static_cast<Usize>(
            (static_cast<Isize>(elem_tag) - elem_mesh.range_.first) * ElemT.kNodeNum + i)]);
        for (Isize j = 0; j < ElemT.kDim; j++) {
          elem_mesh.node_(i * Dim + j, static_cast<Isize>(elem_tag) - elem_mesh.range_.first) = nodes(j, node_tag - 1);
        }
        elem_mesh.index_(i, static_cast<Isize>(elem_tag) - elem_mesh.range_.first) = node_tag;
      }
    }
  }
}

template <ElemInfo ElemT>
inline void getEdgeElemMap(std::map<Usize, std::pair<bool, std::vector<Isize>>>& edge_elem_map) {
  std::vector<std::size_t> edge_nodes_tags;
  gmsh::model::mesh::getElementEdgeNodes(ElemT.kTag, edge_nodes_tags);
  std::vector<int> edge_orientations;
  std::vector<std::size_t> edge_tags;
  gmsh::model::mesh::getEdges(edge_nodes_tags, edge_tags, edge_orientations);
  std::vector<std::size_t> element_tags;
  std::vector<std::size_t> element_node_tags;
  gmsh::model::mesh::getElementsByType(ElemT.kTag, element_tags, element_node_tags);
  for (std::size_t i = 0; i < edge_tags.size(); i++) {
    if (!edge_elem_map.contains(edge_tags[i])) {
      edge_elem_map[edge_tags[i]].first = false;
      edge_elem_map[edge_tags[i]].second.emplace_back(static_cast<Isize>(edge_nodes_tags[2 * i]));
      edge_elem_map[edge_tags[i]].second.emplace_back(static_cast<Isize>(edge_nodes_tags[2 * i + 1]));
    } else {
      edge_elem_map[edge_tags[i]].first = true;
    }
    edge_elem_map[edge_tags[i]].second.emplace_back(static_cast<Isize>(element_tags[i / ElemT.kNodeNum]));
    edge_elem_map[edge_tags[i]].second.emplace_back(static_cast<Isize>(i % ElemT.kNodeNum));
  }
}

template <Isize Dim, ElemInfo ElemT>
inline std::map<Usize, std::pair<bool, std::vector<Isize>>> getAdjacencyElemMap() {
  if constexpr (ElemT.kDim == 1) {
    gmsh::model::mesh::createEdges();
    std::map<Usize, std::pair<bool, std::vector<Isize>>> edge_elem_map;
    getEdgeElemMap<kTri>(edge_elem_map);
    getEdgeElemMap<kQuad>(edge_elem_map);
    return edge_elem_map;
  }
}

template <Isize Dim, ElemInfo ElemT>
inline void getAdjacencyElemMesh(const Eigen::Matrix<Real, Dim, Eigen::Dynamic>& node,
                                 AdjacencyElemMesh<Dim, ElemT>& adjacency_elem_mesh,
                                 const MeshSupplemental<ElemT>& boundary_supplemental) {
  std::map<Usize, std::pair<bool, std::vector<Isize>>> adjacency_element_map = getAdjacencyElemMap<Dim, ElemT>();
  std::vector<Usize> internal_tag;
  std::vector<Usize> boundary_tag;
  std::size_t max_element_tag;
  gmsh::model::mesh::getMaxElementTag(max_element_tag);
  for (const auto& [edge_tag, element_pair] : adjacency_element_map) {
    const auto& [flag, elements_index] = element_pair;
    if (flag) [[likely]] {
      internal_tag.push_back(edge_tag);
    } else [[unlikely]] {
      boundary_tag.push_back(edge_tag);
    }
  }
  const auto [internal_min, internal_max] = std::ranges::minmax_element(internal_tag);
  const auto [boundary_min, boundary_max] = std::ranges::minmax_element(boundary_tag);
  adjacency_elem_mesh.internal_range_ =
      std::make_pair(max_element_tag + 1, *internal_max - *internal_min + max_element_tag + 1);
  auto internal_elements_num = static_cast<Isize>(*internal_max - *internal_min + 1);
  adjacency_elem_mesh.boundary_range_ = std::make_pair(*boundary_min, *boundary_max);
  auto boundary_elements_num = static_cast<Isize>(*boundary_max - *boundary_min + 1);
  adjacency_elem_mesh.num_tag_ = std::make_pair(internal_elements_num, internal_elements_num + boundary_elements_num);
  adjacency_elem_mesh.node_.resize(ElemT.kNodeNum * Dim, internal_elements_num + boundary_elements_num);
  adjacency_elem_mesh.index_.resize(ElemT.kNodeNum + 4, internal_elements_num + boundary_elements_num);
  int entity_tag = gmsh::model::addDiscreteEntity(ElemT.kDim);
  std::vector<std::size_t> element_tags;
  std::vector<std::size_t> node_tags;
  for (const auto edge_tag : internal_tag) {
    const auto& [flag, elements_index] = adjacency_element_map[edge_tag];
    for (Isize i = 0; i < ElemT.kNodeNum * Dim; i++) {
      adjacency_elem_mesh.node_(i, static_cast<Isize>(edge_tag - *internal_min)) =
          node(i % Dim, elements_index[static_cast<Usize>(i / Dim)] - 1);
    }
    for (Isize i = 0; i < ElemT.kNodeNum + 4; i++) {
      adjacency_elem_mesh.index_(i, static_cast<Isize>(edge_tag - *internal_min)) =
          elements_index[static_cast<Usize>(i)];
    }
    element_tags.emplace_back(static_cast<std::size_t>(edge_tag - *internal_min + max_element_tag + 1));
    for (Isize i = 0; i < ElemT.kNodeNum; i++) {
      node_tags.emplace_back(elements_index[static_cast<Usize>(i)]);
    }
  }
  gmsh::model::mesh::addElementsByType(entity_tag, ElemT.kTag, element_tags, node_tags);
  for (const auto& edge_tag : boundary_tag) {
    const auto& [flag, elements_index] = adjacency_element_map[edge_tag];
    for (Isize i = 0; i < ElemT.kNodeNum * Dim; i++) {
      adjacency_elem_mesh.node_(i, static_cast<Isize>(edge_tag - *boundary_min) + internal_elements_num) =
          node(i % Dim, elements_index[static_cast<Usize>(i / Dim)] - 1);
    }
    for (Isize i = 0; i < ElemT.kNodeNum + 2; i++) {
      adjacency_elem_mesh.index_(i, static_cast<Isize>(edge_tag - *boundary_min) + internal_elements_num) =
          elements_index[static_cast<Usize>(i)];
    }
    adjacency_elem_mesh.index_(ElemT.kNodeNum + 2,
                               static_cast<Isize>(edge_tag - *boundary_min) + internal_elements_num) =
        boundary_supplemental.index_(static_cast<Isize>(edge_tag) - boundary_supplemental.range_.first);
    adjacency_elem_mesh.index_(ElemT.kNodeNum + 3,
                               static_cast<Isize>(edge_tag - *boundary_min) + internal_elements_num) = 0;
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_ELEM_MESH_HPP_
