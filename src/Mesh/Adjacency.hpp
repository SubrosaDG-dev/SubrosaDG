/**
 * @file Adjacency.hpp
 * @brief The header file of SubrosaDG Adjacency.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-14
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_ADJACENCY_HPP_
#define SUBROSA_DG_ADJACENCY_HPP_

#include <fmt/format.h>
#include <gmsh.h>

#include <Eigen/Core>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "Mesh/ReadControl.hpp"
#include "Solver/SimulationControl.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Concept.hpp"
#include "Utils/Enum.hpp"

namespace SubrosaDG {

template <typename AdjacencyElementTrait, typename ElementTrait>
  requires Is1dElement<ElementTrait::kElementType>
inline void getAdjacencyElementMeshSupplementalMap(
    std::unordered_map<Isize, AdjacencyElementMeshSupplemental<AdjacencyElementTrait>>&
        adjacency_element_mesh_supplemental_map) {
  std::vector<std::size_t> element_tags;
  std::vector<std::size_t> element_node_tags;
  gmsh::model::mesh::getElementsByType(ElementTrait::kGmshTypeNumber, element_tags, element_node_tags);
  std::vector<std::size_t> adjacency_node_tags;
  Usize element_number = element_tags.size();
  Usize all_adjacency_number = element_tags.size() * 2;
  adjacency_element_mesh_supplemental_map.reserve(all_adjacency_number);
  for (Usize i = 0; i < element_number; i++) {
    adjacency_node_tags.emplace_back(element_node_tags[i * ElementTrait::kAllNodeNumber]);
    adjacency_node_tags.emplace_back(element_node_tags[i * ElementTrait::kAllNodeNumber + 1]);
  }
  for (Usize i = 0; i < all_adjacency_number; i++) {
    const auto point_tag = static_cast<Isize>(adjacency_node_tags[i]);
    if (!adjacency_element_mesh_supplemental_map.contains(point_tag)) {
      adjacency_element_mesh_supplemental_map[point_tag].node_tag_[0] = point_tag;
    } else {
      adjacency_element_mesh_supplemental_map[point_tag].is_recorded_ = true;
    }
    adjacency_element_mesh_supplemental_map[point_tag].parent_gmsh_tag_.emplace_back(
        element_tags[i / ElementTrait::kAdjacencyNumber]);
    adjacency_element_mesh_supplemental_map[point_tag].adjacency_sequence_in_parent_.emplace_back(
        i % ElementTrait::kAdjacencyNumber);
    adjacency_element_mesh_supplemental_map[point_tag].parent_gmsh_type_number_.emplace_back(
        ElementTrait::kGmshTypeNumber);
  }
}

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
    MeshInformation& information, const std::vector<Isize>& boundary_tag,
    const std::unordered_map<Isize, AdjacencyElementMeshSupplemental<AdjacencyElementTrait>>&
        adjacency_element_mesh_supplemental_map) {
  std::vector<std::size_t> element_tags;
  std::vector<std::size_t> node_tags;
  gmsh::model::mesh::getElementsByType(AdjacencyElementTrait::kGmshTypeNumber, element_tags, node_tags);
  std::unordered_map<unordered_array<Isize, AdjacencyElementTrait::kBasicNodeNumber>, Isize> node_tag_element_map;
  for (Isize i = 0; i < static_cast<Isize>(element_tags.size()); i++) {
    unordered_array<Isize, AdjacencyElementTrait::kBasicNodeNumber> node_tag;
    for (Isize j = 0; j < AdjacencyElementTrait::kBasicNodeNumber; j++) {
      node_tag[static_cast<Usize>(j)] =
          static_cast<Isize>(node_tags[static_cast<Usize>(i * AdjacencyElementTrait::kAllNodeNumber + j)]);
    }
    node_tag_element_map[node_tag] = static_cast<Isize>(element_tags[static_cast<Usize>(i)]);
  }
  for (Isize i = this->interior_number_; i < this->interior_number_ + this->boundary_number_; i++) {
    const AdjacencyElementMeshSupplemental<AdjacencyElementTrait>& adjacency_element_mesh_supplemental =
        adjacency_element_mesh_supplemental_map.at(boundary_tag[static_cast<Usize>(i - this->interior_number_)]);
    unordered_array<Isize, AdjacencyElementTrait::kBasicNodeNumber> node_tag;
    for (Isize j = 0; j < AdjacencyElementTrait::kBasicNodeNumber; j++) {
      node_tag[static_cast<Usize>(j)] = adjacency_element_mesh_supplemental.node_tag_[static_cast<Usize>(j)];
    }
    for (Isize j = 0; j < AdjacencyElementTrait::kAllNodeNumber; j++) {
      this->element_(i).node_coordinate_.col(j) =
          node_coordinate.col(adjacency_element_mesh_supplemental.node_tag_[static_cast<Usize>(j)] - 1);
      this->element_(i).node_tag_(j) = adjacency_element_mesh_supplemental.node_tag_[static_cast<Usize>(j)];
    }
    try {
      this->element_(i).gmsh_tag_ = node_tag_element_map.at(node_tag);
    } catch (const std::out_of_range& error) {
      std::cout << fmt::format("Cannot find adjacency element with node tag: {}", fmt::join(node_tag, ", ")) << '\n';
      std::cout << "Check your physical group definition." << '\n';
    }
    this->element_(i).gmsh_physical_name_ =
        information.gmsh_tag_to_element_information_.at(this->element_(i).gmsh_tag_).gmsh_physical_name_;
    this->element_(i).element_index_ = i;
    information.physical_group_information_[this->element_(i).gmsh_physical_name_].element_number_++;
    information.physical_group_information_[this->element_(i).gmsh_physical_name_].element_gmsh_type_.emplace_back(
        AdjacencyElementTrait::kGmshTypeNumber);
    information.physical_group_information_[this->element_(i).gmsh_physical_name_].element_gmsh_tag_.emplace_back(
        this->element_(i).gmsh_tag_);
    information.physical_group_information_[this->element_(i).gmsh_physical_name_].node_number_ +=
        AdjacencyElementTrait::kAllNodeNumber;
    information.gmsh_tag_to_element_information_[this->element_(i).gmsh_tag_].element_index_ = i;
    this->element_(i).parent_index_each_type_(0) =
        information.gmsh_tag_to_element_information_.at(adjacency_element_mesh_supplemental.parent_gmsh_tag_[0])
            .element_index_;
    this->element_(i).adjacency_sequence_in_parent_(0) =
        adjacency_element_mesh_supplemental.adjacency_sequence_in_parent_[0];
    this->element_(i).parent_gmsh_type_number_(0) = adjacency_element_mesh_supplemental.parent_gmsh_type_number_[0];
    information.gmsh_tag_to_sub_index_and_type_[adjacency_element_mesh_supplemental.parent_gmsh_tag_[0]].emplace_back(
        i, AdjacencyElementTrait::kGmshTypeNumber);
    for (Isize j = 0; j < AdjacencyElementTrait::kAllNodeNumber; j++) {
      information.physical_group_information_[this->element_(i).gmsh_physical_name_].node_gmsh_tag_.emplace_back(
          adjacency_element_mesh_supplemental.node_tag_[static_cast<Usize>(j)]);
    }
  }
}

template <typename AdjacencyElementTrait>
inline void AdjacencyElementMesh<AdjacencyElementTrait>::getAdjacencyElementInteriorMesh(
    const Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, Eigen::Dynamic>& node_coordinate,
    MeshInformation& information, const std::vector<Isize>& interior_tag,
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
    this->element_(i).element_index_ = i;
    interior_gmsh_tag.emplace_back(max_tag + static_cast<Usize>(i) + 1);
    for (Isize j = 0; j < AdjacencyElementTrait::kAllNodeNumber; j++) {
      this->element_(i).node_coordinate_.col(j) =
          node_coordinate.col(adjacency_element_mesh_supplemental.node_tag_[static_cast<Usize>(j)] - 1);
      this->element_(i).node_tag_(j) = adjacency_element_mesh_supplemental.node_tag_[static_cast<Usize>(j)];
      interior_node_tag.emplace_back(adjacency_element_mesh_supplemental.node_tag_[static_cast<Usize>(j)]);
    }
    information.gmsh_tag_to_element_information_[this->element_(i).gmsh_tag_].element_index_ = i;
    for (Isize j = 0; j < 2; j++) {
      this->element_(i).parent_index_each_type_(j) =
          information.gmsh_tag_to_element_information_
              .at(adjacency_element_mesh_supplemental.parent_gmsh_tag_[static_cast<Usize>(j)])
              .element_index_;
      this->element_(i).adjacency_sequence_in_parent_(j) =
          adjacency_element_mesh_supplemental.adjacency_sequence_in_parent_[static_cast<Usize>(j)];
      this->element_(i).parent_gmsh_type_number_(j) =
          adjacency_element_mesh_supplemental.parent_gmsh_type_number_[static_cast<Usize>(j)];
      information
          .gmsh_tag_to_sub_index_and_type_[adjacency_element_mesh_supplemental.parent_gmsh_tag_[static_cast<Usize>(j)]]
          .emplace_back(i, AdjacencyElementTrait::kGmshTypeNumber);
    }
  }
  gmsh::model::mesh::addElementsByType(entity_tag, AdjacencyElementTrait::kGmshTypeNumber, interior_gmsh_tag,
                                       interior_node_tag);
}

template <typename AdjacencyElementTrait>
template <MeshModelEnum MeshModelType>
inline void AdjacencyElementMesh<AdjacencyElementTrait>::getAdjacencyElementMesh(
    const Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, Eigen::Dynamic>& node_coordinate,
    MeshInformation& information) {
  std::unordered_map<Isize, AdjacencyElementMeshSupplemental<AdjacencyElementTrait>>
      adjacency_element_mesh_supplemental_map;
  if constexpr (AdjacencyElementTrait::kDimension == 0) {
    getAdjacencyElementMeshSupplementalMap<AdjacencyElementTrait, LineTrait<AdjacencyElementTrait::kPolynomialOrder>>(
        adjacency_element_mesh_supplemental_map);
  }
  if constexpr (AdjacencyElementTrait::kDimension == 1) {
    gmsh::model::mesh::createEdges();
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
  this->getAdjacencyElementBoundaryMesh(node_coordinate, information, boundary_tag,
                                        adjacency_element_mesh_supplemental_map);
  this->getAdjacencyElementInteriorMesh(node_coordinate, information, interior_tag,
                                        adjacency_element_mesh_supplemental_map);
  this->getAdjacencyElementJacobian();
  this->calculateAdjacencyElementNormalVector();
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_ADJACENCY_HPP_
