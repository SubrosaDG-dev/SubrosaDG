/**
 * @file Adjacency.hpp
 * @brief The header file of SubrosaDG Adjacency.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-14
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_ADJACENCY_HPP_
#define SUBROSA_DG_ADJACENCY_HPP_

#include <gmsh.h>

#include <Eigen/Core>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include "Mesh/ReadControl.hpp"
#include "Solver/SimulationControl.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Concept.hpp"
#include "Utils/Enum.hpp"

namespace SubrosaDG {

template <typename AdjacencyElementTrait, typename ElementTrait>
  requires Is2dElement<ElementTrait::kElementType>
inline void getAdjacencyElementMeshSupplementalMap(
    std::unordered_map<Isize, AdjacencyElementMeshSupplemental<AdjacencyElementTrait>>&
        adjacency_element_mesh_supplemental_map) {
  std::vector<std::size_t> adjacency_node_tags;
  gmsh::model::mesh::getElementEdgeNodes(ElementTrait::kGmshTypeNumber, adjacency_node_tags);
  std::vector<std::size_t> basic_node_tags;
  Usize all_adjacency_number = adjacency_node_tags.size() / AdjacencyElementTrait::kAllNodeNumber;
  adjacency_element_mesh_supplemental_map.reserve(all_adjacency_number);
  for (Usize i = 0; i < all_adjacency_number; i++) {
    basic_node_tags.emplace_back(adjacency_node_tags[i * AdjacencyElementTrait::kAllNodeNumber]);
    basic_node_tags.emplace_back(adjacency_node_tags[i * AdjacencyElementTrait::kAllNodeNumber + 1]);
  }
  std::vector<std::size_t> edge_tags;
  std::vector<int> edge_orientations;
  gmsh::model::mesh::getEdges(basic_node_tags, edge_tags, edge_orientations);
  std::vector<std::size_t> element_tags;
  std::vector<std::size_t> element_node_tags;
  gmsh::model::mesh::getElementsByType(ElementTrait::kGmshTypeNumber, element_tags, element_node_tags);
  for (Usize i = 0; i < all_adjacency_number; i++) {
    const auto edge_tag = static_cast<Isize>(edge_tags[i]);
    if (!adjacency_element_mesh_supplemental_map.contains(edge_tag)) {
      for (Usize j = 0; j < AdjacencyElementTrait::kAllNodeNumber; j++) {
        adjacency_element_mesh_supplemental_map[edge_tag].node_tag_[j] =
            static_cast<Isize>(adjacency_node_tags[i * AdjacencyElementTrait::kAllNodeNumber + j]);
      }
    } else {
      adjacency_element_mesh_supplemental_map[edge_tag].is_recorded_ = true;
    }
    adjacency_element_mesh_supplemental_map[edge_tag].parent_gmsh_tag_.emplace_back(
        element_tags[i / ElementTrait::kAdjacencyNumber]);
    adjacency_element_mesh_supplemental_map[edge_tag].adjacency_sequence_in_parent_.emplace_back(
        i % ElementTrait::kAdjacencyNumber);
    adjacency_element_mesh_supplemental_map[edge_tag].parent_gmsh_type_number_.emplace_back(
        ElementTrait::kGmshTypeNumber);
  }
}

template <typename AdjacencyElementTrait>
inline void AdjacencyElementMesh<AdjacencyElementTrait>::getAdjacencyElementBoundaryMesh(
    const Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, Eigen::Dynamic>& node_coordinate,
    const std::unordered_map<Isize, std::string>& gmsh_tag_to_physical_name,
    std::unordered_map<Isize, Isize>& gmsh_tag_to_index, const std::vector<Isize>& boundary_tag,
    const std::unordered_map<Isize, AdjacencyElementMeshSupplemental<AdjacencyElementTrait>>&
        adjacency_element_mesh_supplemental_map) {
  std::vector<std::size_t> element_tags;
  std::vector<std::size_t> node_tags;
  gmsh::model::mesh::getElementsByType(AdjacencyElementTrait::kGmshTypeNumber, element_tags, node_tags);
  std::unordered_map<UnorderedArray<Isize, AdjacencyElementTrait::kBasicNodeNumber>, Isize> node_tag_element_map;
  for (Isize i = 0; i < static_cast<Isize>(element_tags.size()); i++) {
    UnorderedArray<Isize, AdjacencyElementTrait::kBasicNodeNumber> node_tag;
    for (Isize j = 0; j < AdjacencyElementTrait::kBasicNodeNumber; j++) {
      node_tag[static_cast<Usize>(j)] =
          static_cast<Isize>(node_tags[static_cast<Usize>(i * AdjacencyElementTrait::kAllNodeNumber + j)]);
    }
    node_tag_element_map[node_tag] = static_cast<Isize>(element_tags[static_cast<Usize>(i)]);
  }
  for (Isize i = this->interior_number_; i < this->interior_number_ + this->boundary_number_; i++) {
    const AdjacencyElementMeshSupplemental<AdjacencyElementTrait>& adjacency_element_mesh_supplemental =
        adjacency_element_mesh_supplemental_map.at(boundary_tag[static_cast<Usize>(i - this->interior_number_)]);
    UnorderedArray<Isize, AdjacencyElementTrait::kBasicNodeNumber> node_tag;
    for (Isize j = 0; j < AdjacencyElementTrait::kAllNodeNumber; j++) {
      this->element_(i).node_coordinate_.col(j) =
          node_coordinate.col(adjacency_element_mesh_supplemental.node_tag_[static_cast<Usize>(j)] - 1);
      this->element_(i).node_tag_(j) = adjacency_element_mesh_supplemental.node_tag_[static_cast<Usize>(j)];
    }
    for (Isize j = 0; j < AdjacencyElementTrait::kBasicNodeNumber; j++) {
      node_tag[static_cast<Usize>(j)] = adjacency_element_mesh_supplemental.node_tag_[static_cast<Usize>(j)];
    }
    this->element_(i).gmsh_tag_ = node_tag_element_map.at(node_tag);
    gmsh_tag_to_index[this->element_(i).gmsh_tag_] = i;
    this->element_(i).gmsh_physical_name_ = gmsh_tag_to_physical_name.at(this->element_(i).gmsh_tag_);
    this->element_(i).parent_index_each_type_[0] =
        gmsh_tag_to_index.at(adjacency_element_mesh_supplemental.parent_gmsh_tag_[0]);
    this->element_(i).adjacency_sequence_in_parent_[0] =
        adjacency_element_mesh_supplemental.adjacency_sequence_in_parent_[0];
    this->element_(i).parent_gmsh_type_number_[0] = adjacency_element_mesh_supplemental.parent_gmsh_type_number_[0];
  }
}

template <typename AdjacencyElementTrait>
inline void AdjacencyElementMesh<AdjacencyElementTrait>::getAdjacencyElementInteriorMesh(
    const Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, Eigen::Dynamic>& node_coordinate,
    const std::unordered_map<Isize, Isize>& gmsh_tag_to_index, const std::vector<Isize>& interior_tag,
    const std::unordered_map<Isize, AdjacencyElementMeshSupplemental<AdjacencyElementTrait>>&
        adjacency_element_mesh_supplemental_map) {
  std::size_t max_tag;
  gmsh::model::mesh::getMaxElementTag(max_tag);
  const int entity_tag = gmsh::model::addDiscreteEntity(AdjacencyElementTrait::kDimension);
  std::vector<std::size_t> interior_gmsh_tag;
  std::vector<std::size_t> interior_node_tag;
  for (Isize i = 0; i < this->interior_number_; i++) {
    const AdjacencyElementMeshSupplemental<AdjacencyElementTrait>& adjacency_element_mesh_supplemental =
        adjacency_element_mesh_supplemental_map.at(interior_tag[static_cast<Usize>(i)]);
    this->element_(i).gmsh_tag_ = static_cast<Isize>(max_tag) + i + 1;
    interior_gmsh_tag.emplace_back(max_tag + static_cast<Usize>(i) + 1);
    for (Isize j = 0; j < AdjacencyElementTrait::kAllNodeNumber; j++) {
      this->element_(i).node_coordinate_.col(j) =
          node_coordinate.col(adjacency_element_mesh_supplemental.node_tag_[static_cast<Usize>(j)] - 1);
      this->element_(i).node_tag_(j) = adjacency_element_mesh_supplemental.node_tag_[static_cast<Usize>(j)];
      interior_node_tag.emplace_back(adjacency_element_mesh_supplemental.node_tag_[static_cast<Usize>(j)]);
    }
    for (Isize j = 0; j < 2; j++) {
      this->element_(i).parent_index_each_type_(j) =
          gmsh_tag_to_index.at(adjacency_element_mesh_supplemental.parent_gmsh_tag_[static_cast<Usize>(j)]);
      this->element_(i).adjacency_sequence_in_parent_(j) =
          adjacency_element_mesh_supplemental.adjacency_sequence_in_parent_[static_cast<Usize>(j)];
      this->element_(i).parent_gmsh_type_number_(j) =
          adjacency_element_mesh_supplemental.parent_gmsh_type_number_[static_cast<Usize>(j)];
    }
  }
  gmsh::model::mesh::addElementsByType(entity_tag, AdjacencyElementTrait::kGmshTypeNumber, interior_gmsh_tag,
                                       interior_node_tag);
}

template <typename AdjacencyElementTrait>
template <MeshModel MeshModelType>
inline void AdjacencyElementMesh<AdjacencyElementTrait>::getAdjacencyElementMesh(
    const Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, Eigen::Dynamic>& node_coordinate,
    const std::unordered_map<Isize, std::string>& gmsh_tag_to_physical_name,
    std::unordered_map<Isize, Isize>& gmsh_tag_to_index) {
  if constexpr (AdjacencyElementTrait::kDimension == 1) {
    gmsh::model::mesh::createEdges();
  }
  std::unordered_map<Isize, AdjacencyElementMeshSupplemental<AdjacencyElementTrait>>
      adjacency_element_mesh_supplemental_map;
  if constexpr (HasTriangle<MeshModelType>) {
    getAdjacencyElementMeshSupplementalMap<AdjacencyElementTrait,
                                           TriangleTrait<AdjacencyElementTrait::kPolynomialOrder>>(
        adjacency_element_mesh_supplemental_map);
  }
  if constexpr (HasQuadrangle<MeshModelType>) {
    getAdjacencyElementMeshSupplementalMap<AdjacencyElementTrait,
                                           QuadrangleTrait<AdjacencyElementTrait::kPolynomialOrder>>(
        adjacency_element_mesh_supplemental_map);
  }
  std::vector<Isize> interior_tag;
  std::vector<Isize> boundary_tag;
  for (const auto& [edge_tag, adjacency_element_mesh_supplemental] : adjacency_element_mesh_supplemental_map) {
    if (adjacency_element_mesh_supplemental.is_recorded_) [[likely]] {
      interior_tag.emplace_back(edge_tag);
    } else {
      boundary_tag.emplace_back(edge_tag);
    }
  }
  this->interior_number_ = static_cast<Isize>(interior_tag.size());
  this->boundary_number_ = static_cast<Isize>(boundary_tag.size());
  this->element_.resize(this->interior_number_ + this->boundary_number_);
  this->getAdjacencyElementBoundaryMesh(node_coordinate, gmsh_tag_to_physical_name, gmsh_tag_to_index, boundary_tag,
                                        adjacency_element_mesh_supplemental_map);
  this->getAdjacencyElementInteriorMesh(node_coordinate, gmsh_tag_to_index, interior_tag,
                                        adjacency_element_mesh_supplemental_map);
  this->getAdjacencyElementJacobian();
  this->calculateAdjacencyElementNormalVector();
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_ADJACENCY_HPP_