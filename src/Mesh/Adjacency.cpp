/**
 * @file Adjacency.cpp
 * @brief The header file of SubrosaDG Adjacency.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-14
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_ADJACENCY_CPP_
#define SUBROSA_DG_ADJACENCY_CPP_

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <gmsh.h>

#include <Eigen/Core>
#include <array>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Mesh/ReadControl.cpp"
#include "Solver/SimulationControl.cpp"
#include "Utils/BasicDataType.cpp"
#include "Utils/Concept.cpp"
#include "Utils/Enum.cpp"

namespace SubrosaDG {

template <typename AdjacencyElementTrait, typename ElementTrait>
inline std::pair<Isize, Isize> getAdjacencyElmentParentAndSelfSequence(const Isize adjacency_number) {
  if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point ||
                AdjacencyElementTrait::kElementType == ElementEnum::Line) {
    return std::make_pair(adjacency_number / ElementTrait::kAdjacencyNumber,
                          adjacency_number % ElementTrait::kAdjacencyNumber);
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
    if constexpr (ElementTrait::kElementType == ElementEnum::Tetrahedron) {
      return std::make_pair(adjacency_number / ElementTrait::kAdjacencyNumber,
                            adjacency_number % ElementTrait::kAdjacencyNumber);
    } else if constexpr (ElementTrait::kElementType == ElementEnum::Pyramid) {
      return std::make_pair(adjacency_number / (ElementTrait::kAdjacencyNumber - 1),
                            adjacency_number % (ElementTrait::kAdjacencyNumber - 1));
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
    if constexpr (ElementTrait::kElementType == ElementEnum::Pyramid) {
      return std::make_pair(adjacency_number, 4);
    } else if constexpr (ElementTrait::kElementType == ElementEnum::Hexahedron) {
      return std::make_pair(adjacency_number / ElementTrait::kAdjacencyNumber,
                            adjacency_number % ElementTrait::kAdjacencyNumber);
    }
  }
}

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
    const std::pair<Isize, Isize> parent_and_self_sequence =
        getAdjacencyElmentParentAndSelfSequence<AdjacencyElementTrait, ElementTrait>(static_cast<Isize>(i));
    adjacency_element_mesh_supplemental_map[point_tag].parent_gmsh_tag_.emplace_back(
        element_tags[static_cast<Usize>(parent_and_self_sequence.first)]);
    adjacency_element_mesh_supplemental_map[point_tag].adjacency_sequence_in_parent_.emplace_back(
        parent_and_self_sequence.second);
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
  std::vector<std::size_t> adjacency_basic_node_tags;
  gmsh::model::mesh::getElementEdgeNodes(ElementTrait::kGmshTypeNumber, adjacency_basic_node_tags, -1, true);
  Usize all_adjacency_number = adjacency_node_tags.size() / AdjacencyElementTrait::kAllNodeNumber;
  std::vector<std::size_t> edge_tags;
  std::vector<int> edge_orientations;
  gmsh::model::mesh::getEdges(adjacency_basic_node_tags, edge_tags, edge_orientations);
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
    const std::pair<Isize, Isize> parent_and_self_sequence =
        getAdjacencyElmentParentAndSelfSequence<AdjacencyElementTrait, ElementTrait>(static_cast<Isize>(i));
    adjacency_element_mesh_supplemental_map[edge_tag].parent_gmsh_tag_.emplace_back(
        element_tags[static_cast<Usize>(parent_and_self_sequence.first)]);
    adjacency_element_mesh_supplemental_map[edge_tag].adjacency_sequence_in_parent_.emplace_back(
        parent_and_self_sequence.second);
    adjacency_element_mesh_supplemental_map[edge_tag].parent_gmsh_type_number_.emplace_back(
        ElementTrait::kGmshTypeNumber);
  }
}

template <typename AdjacencyElementTrait, typename ElementTrait>
  requires Is3dElement<ElementTrait::kElementType>
inline void getAdjacencyElementMeshSupplementalMap(
    std::unordered_map<Isize, AdjacencyElementMeshSupplemental<AdjacencyElementTrait>>&
        adjacency_element_mesh_supplemental_map) {
  std::vector<std::size_t> adjacency_node_tags;
  gmsh::model::mesh::getElementFaceNodes(ElementTrait::kGmshTypeNumber, AdjacencyElementTrait::kBasicNodeNumber,
                                         adjacency_node_tags);
  std::vector<std::size_t> adjacency_basic_node_tags;
  gmsh::model::mesh::getElementFaceNodes(ElementTrait::kGmshTypeNumber, AdjacencyElementTrait::kBasicNodeNumber,
                                         adjacency_basic_node_tags, -1, true);
  Usize all_adjacency_number = adjacency_node_tags.size() / AdjacencyElementTrait::kAllNodeNumber;
  std::vector<std::size_t> face_tags;
  std::vector<int> face_orientations;
  gmsh::model::mesh::getFaces(AdjacencyElementTrait::kBasicNodeNumber, adjacency_basic_node_tags, face_tags,
                              face_orientations);
  std::vector<std::size_t> element_tags;
  std::vector<std::size_t> element_node_tags;
  gmsh::model::mesh::getElementsByType(ElementTrait::kGmshTypeNumber, element_tags, element_node_tags);
  std::array<std::size_t, AdjacencyElementTrait::kBasicNodeNumber> right_basic_node_tag;
  for (Usize i = 0; i < all_adjacency_number; i++) {
    const auto face_tag = static_cast<Isize>(face_tags[i]);
    if (!adjacency_element_mesh_supplemental_map.contains(face_tag)) {
      for (Usize j = 0; j < AdjacencyElementTrait::kAllNodeNumber; j++) {
        adjacency_element_mesh_supplemental_map[face_tag].node_tag_[j] =
            static_cast<Isize>(adjacency_node_tags[i * AdjacencyElementTrait::kAllNodeNumber + j]);
      }
    } else {
      if (adjacency_element_mesh_supplemental_map[face_tag].is_recorded_) [[unlikely]] {
        throw std::runtime_error(
            fmt::format("The adjacency element with node tag {} is recorded more than twice.",
                        fmt::join(adjacency_element_mesh_supplemental_map[face_tag].node_tag_, " ")));
      }
      for (Usize j = 0; j < AdjacencyElementTrait::kBasicNodeNumber; j++) {
        right_basic_node_tag[j] = adjacency_node_tags[i * AdjacencyElementTrait::kAllNodeNumber + j];
      }
      adjacency_element_mesh_supplemental_map[face_tag].right_rotation_ = std::distance(
          right_basic_node_tag.begin(), std::find(right_basic_node_tag.begin(), right_basic_node_tag.end(),
                                                  adjacency_element_mesh_supplemental_map[face_tag].node_tag_[0]));
      adjacency_element_mesh_supplemental_map[face_tag].is_recorded_ = true;
    }
    const std::pair<Isize, Isize> parent_and_self_sequence =
        getAdjacencyElmentParentAndSelfSequence<AdjacencyElementTrait, ElementTrait>(static_cast<Isize>(i));
    adjacency_element_mesh_supplemental_map[face_tag].parent_gmsh_tag_.emplace_back(
        element_tags[static_cast<Usize>(parent_and_self_sequence.first)]);
    adjacency_element_mesh_supplemental_map[face_tag].adjacency_sequence_in_parent_.emplace_back(
        parent_and_self_sequence.second);
    adjacency_element_mesh_supplemental_map[face_tag].parent_gmsh_type_number_.emplace_back(
        ElementTrait::kGmshTypeNumber);
  }
}

template <typename AdjacencyElementTrait>
  requires Is0dElement<AdjacencyElementTrait::kElementType>
inline void fixAdjacencyElementMeshSupplementalMap(
    const MeshInformation& information,
    std::unordered_map<Isize, AdjacencyElementMeshSupplemental<AdjacencyElementTrait>>&
        adjacency_element_mesh_supplemental_map) {
  for (const auto [boundary_physical_index, boundary_physical_type] : information.boundary_condition_type_) {
    if (boundary_physical_type != BoundaryConditionEnum::Periodic) {
      continue;
    }
    std::vector<int> entity_tags;
    gmsh::model::getEntitiesForPhysicalGroup(0, information.physical_information_.at(boundary_physical_index).gmsh_tag_,
                                             entity_tags);
    std::vector<std::size_t> element_tags;
    std::vector<std::size_t> element_node_tags;
    gmsh::model::mesh::getElementsByType(AdjacencyElementTrait::kGmshTypeNumber, element_tags, element_node_tags);
    const std::pair<Isize, Isize> node_tag =
        std::make_pair(static_cast<Isize>(element_node_tags[0]), static_cast<Isize>(element_node_tags[1]));
    adjacency_element_mesh_supplemental_map[node_tag.first].is_recorded_ = true;
    adjacency_element_mesh_supplemental_map[node_tag.first].parent_gmsh_tag_.emplace_back(
        adjacency_element_mesh_supplemental_map[node_tag.second].parent_gmsh_tag_[0]);
    adjacency_element_mesh_supplemental_map[node_tag.first].adjacency_sequence_in_parent_.emplace_back(
        adjacency_element_mesh_supplemental_map[node_tag.second].adjacency_sequence_in_parent_[0]);
    adjacency_element_mesh_supplemental_map[node_tag.first].parent_gmsh_type_number_.emplace_back(
        adjacency_element_mesh_supplemental_map[node_tag.second].parent_gmsh_type_number_[0]);
    adjacency_element_mesh_supplemental_map.erase(node_tag.second);
  }
}

template <typename AdjacencyElementTrait>
  requires Is1dElement<AdjacencyElementTrait::kElementType>
inline void fixAdjacencyElementMeshSupplementalMap(
    const MeshInformation& information,
    std::unordered_map<Isize, AdjacencyElementMeshSupplemental<AdjacencyElementTrait>>&
        adjacency_element_mesh_supplemental_map) {
  for (const auto [boundary_physical_index, boundary_physical_type] : information.boundary_condition_type_) {
    if (boundary_physical_type != BoundaryConditionEnum::Periodic) {
      continue;
    }
    std::vector<int> entity_tags;
    gmsh::model::getEntitiesForPhysicalGroup(AdjacencyElementTrait::kDimension,
                                             information.physical_information_.at(boundary_physical_index).gmsh_tag_,
                                             entity_tags);
    std::vector<int> entity_tags_master;
    gmsh::model::mesh::getPeriodic(AdjacencyElementTrait::kDimension, entity_tags, entity_tags_master);
    std::unordered_map<int, int> periodic_entity_tag_map;
    for (Usize i = 0; i < entity_tags.size(); i++) {
      if (entity_tags[i] != entity_tags_master[i]) {
        periodic_entity_tag_map[entity_tags[i]] = entity_tags_master[i];
      }
    }
    for (const auto [entity_tag, entity_tag_master] : periodic_entity_tag_map) {
      std::vector<std::size_t> element_tags;
      std::vector<std::size_t> element_node_tags;
      gmsh::model::mesh::getElementsByType(AdjacencyElementTrait::kGmshTypeNumber, element_tags, element_node_tags,
                                           entity_tag);
      std::vector<std::size_t> element_tags_master;
      std::vector<std::size_t> element_node_tags_master;
      gmsh::model::mesh::getElementsByType(AdjacencyElementTrait::kGmshTypeNumber, element_tags_master,
                                           element_node_tags_master, entity_tag_master);
      for (Usize i = 0; i < element_tags_master.size(); i++) {
        const auto element_tag_master = static_cast<Isize>(element_tags_master[i]);
        const auto element_tag = static_cast<Isize>(element_tags[i]);
        adjacency_element_mesh_supplemental_map[element_tag_master].is_recorded_ = true;
        adjacency_element_mesh_supplemental_map[element_tag_master].parent_gmsh_tag_.emplace_back(
            adjacency_element_mesh_supplemental_map[element_tag].parent_gmsh_tag_[0]);
        adjacency_element_mesh_supplemental_map[element_tag_master].adjacency_sequence_in_parent_.emplace_back(
            adjacency_element_mesh_supplemental_map[element_tag].adjacency_sequence_in_parent_[0]);
        adjacency_element_mesh_supplemental_map[element_tag_master].parent_gmsh_type_number_.emplace_back(
            adjacency_element_mesh_supplemental_map[element_tag].parent_gmsh_type_number_[0]);
        adjacency_element_mesh_supplemental_map.erase(element_tag);
      }
    }
  }
}

template <typename AdjacencyElementTrait>
  requires Is2dElement<AdjacencyElementTrait::kElementType>
inline void fixAdjacencyElementMeshSupplementalMap(
    const MeshInformation& information,
    std::unordered_map<Isize, AdjacencyElementMeshSupplemental<AdjacencyElementTrait>>&
        adjacency_element_mesh_supplemental_map) {
  for (const auto [boundary_physical_index, boundary_physical_type] : information.boundary_condition_type_) {
    if (boundary_physical_type != BoundaryConditionEnum::Periodic) {
      continue;
    }
    std::vector<int> entity_tags;
    gmsh::model::getEntitiesForPhysicalGroup(AdjacencyElementTrait::kDimension,
                                             information.physical_information_.at(boundary_physical_index).gmsh_tag_,
                                             entity_tags);
    std::vector<int> entity_tags_master;
    gmsh::model::mesh::getPeriodic(AdjacencyElementTrait::kDimension, entity_tags, entity_tags_master);
    // NOTE: Here the entity_tags contains both master and slave entity tags, and the order is not guaranteed.
    // So we have to find the master entity (entity_tags_master) which contains only master entity tags.
    std::unordered_map<int, int> periodic_entity_tag_map;
    for (Usize i = 0; i < entity_tags.size(); i++) {
      if (entity_tags[i] != entity_tags_master[i]) {
        periodic_entity_tag_map[entity_tags[i]] = entity_tags_master[i];
      }
    }
    for (const auto [entity_tag, entity_tag_master] : periodic_entity_tag_map) {
      std::vector<std::size_t> element_tags;
      std::vector<std::size_t> element_node_tags;
      gmsh::model::mesh::getElementsByType(AdjacencyElementTrait::kGmshTypeNumber, element_tags, element_node_tags,
                                           entity_tag);
      std::vector<std::size_t> element_tags_master;
      std::vector<std::size_t> element_node_tags_master;
      gmsh::model::mesh::getElementsByType(AdjacencyElementTrait::kGmshTypeNumber, element_tags_master,
                                           element_node_tags_master, entity_tag_master);
      int tag_master;
      std::vector<std::size_t> node_tags;
      std::vector<std::size_t> node_tags_master;
      std::vector<double> affine_transform;
      gmsh::model::mesh::getPeriodicNodes(AdjacencyElementTrait::kDimension, entity_tag, tag_master, node_tags,
                                          node_tags_master, affine_transform);
      std::unordered_map<std::size_t, std::size_t> node_tags_map;
      for (Usize i = 0; i < node_tags.size(); i++) {
        node_tags_map[node_tags_master[i]] = node_tags[i];
      }
      std::vector<std::array<Isize, AdjacencyElementTrait::kAllNodeNumber>> element_node_tags_array(
          element_tags_master.size());
      for (Usize i = 0; i < element_tags_master.size(); i++) {
        for (Usize j = 0; j < AdjacencyElementTrait::kAllNodeNumber; j++) {
          element_node_tags_array[i][j] =
              static_cast<Isize>(element_node_tags[i * AdjacencyElementTrait::kAllNodeNumber + j]);
        }
      }
      for (Usize i = 0; i < element_tags_master.size(); i++) {
        const auto element_tag_master = static_cast<Isize>(element_tags_master[i]);
        const auto element_tag = static_cast<Isize>(element_tags[i]);
        adjacency_element_mesh_supplemental_map[element_tag_master].is_recorded_ = true;
        // NOTE: The right rotation is calculated by the order of the node tags in the master entity. The node_tag_map
        // is used to find the corresponding node tag in the slave entity. Then the right rotation is calculated by the
        // distance between the first node tag in the master entity and the corresponding node tag in the slave entity.
        adjacency_element_mesh_supplemental_map[element_tag_master].right_rotation_ = std::distance(
            element_node_tags_array[i].begin(),
            std::find(element_node_tags_array[i].begin(), element_node_tags_array[i].end(),
                      node_tags_map[element_node_tags_master[i * AdjacencyElementTrait::kAllNodeNumber]]));
        adjacency_element_mesh_supplemental_map[element_tag_master].parent_gmsh_tag_.emplace_back(
            adjacency_element_mesh_supplemental_map[element_tag].parent_gmsh_tag_[0]);
        adjacency_element_mesh_supplemental_map[element_tag_master].adjacency_sequence_in_parent_.emplace_back(
            adjacency_element_mesh_supplemental_map[element_tag].adjacency_sequence_in_parent_[0]);
        adjacency_element_mesh_supplemental_map[element_tag_master].parent_gmsh_type_number_.emplace_back(
            adjacency_element_mesh_supplemental_map[element_tag].parent_gmsh_type_number_[0]);
        adjacency_element_mesh_supplemental_map.erase(element_tag);
      }
    }
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
  std::size_t max_tag;
  gmsh::model::mesh::getMaxElementTag(max_tag);
  const int entity_tag = gmsh::model::addDiscreteEntity(AdjacencyElementTrait::kDimension);
  std::vector<std::size_t> boundary_gmsh_tag;
  std::vector<std::size_t> boundary_node_tag;
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
      boundary_node_tag.emplace_back(adjacency_element_mesh_supplemental.node_tag_[static_cast<Usize>(j)]);
    }
    try {
      this->element_(i).gmsh_tag_ = node_tag_element_map.at(node_tag);
    } catch (const std::out_of_range& error) {
      std::cout << fmt::format("Cannot find adjacency element with node tag: {}", fmt::join(node_tag, " ")) << '\n';
      std::cout << "Check your physical group definition or computational mesh type." << '\n';
    }
    this->element_(i).gmsh_physical_index_ =
        information.gmsh_tag_to_element_physical_information_.at(this->element_(i).gmsh_tag_).gmsh_physical_index_;
    this->element_(i).element_index_ = i;
    boundary_gmsh_tag.emplace_back(max_tag + static_cast<Usize>(i) + 1);
    this->element_(i).gmsh_jacobian_tag_ = static_cast<Isize>(max_tag) + i + 1;
    const std::string gmsh_physical_name =
        information.physical_[static_cast<Usize>(this->element_(i).gmsh_physical_index_)];
    information.physical_information_[this->element_(i).gmsh_physical_index_].element_number_++;
    information.physical_information_[this->element_(i).gmsh_physical_index_].vtk_element_number_ +=
        AdjacencyElementTrait::kVtkElementNumber;
    information.physical_information_[this->element_(i).gmsh_physical_index_].element_gmsh_type_.emplace_back(
        AdjacencyElementTrait::kGmshTypeNumber);
    information.physical_information_[this->element_(i).gmsh_physical_index_].element_gmsh_tag_.emplace_back(
        this->element_(i).gmsh_tag_);
    information.physical_information_[this->element_(i).gmsh_physical_index_].node_number_ +=
        AdjacencyElementTrait::kAllNodeNumber;
    information.physical_information_[this->element_(i).gmsh_physical_index_].vtk_node_number_ +=
        AdjacencyElementTrait::kVtkAllNodeNumber;
    information.gmsh_tag_to_element_physical_information_[this->element_(i).gmsh_tag_].element_index_ = i;
    this->element_(i).parent_index_each_type_(0) = information.gmsh_tag_to_element_physical_information_
                                                       .at(adjacency_element_mesh_supplemental.parent_gmsh_tag_[0])
                                                       .element_index_;
    this->element_(i).adjacency_sequence_in_parent_(0) =
        adjacency_element_mesh_supplemental.adjacency_sequence_in_parent_[0];
    this->element_(i).parent_gmsh_type_number_(0) = adjacency_element_mesh_supplemental.parent_gmsh_type_number_[0];
  }
  gmsh::model::mesh::addElementsByType(entity_tag, AdjacencyElementTrait::kGmshTypeNumber, boundary_gmsh_tag,
                                       boundary_node_tag);
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
    this->element_(i).gmsh_jacobian_tag_ = static_cast<Isize>(max_tag) + i + 1;
    for (Isize j = 0; j < AdjacencyElementTrait::kAllNodeNumber; j++) {
      this->element_(i).node_coordinate_.col(j) =
          node_coordinate.col(adjacency_element_mesh_supplemental.node_tag_[static_cast<Usize>(j)] - 1);
      this->element_(i).node_tag_(j) = adjacency_element_mesh_supplemental.node_tag_[static_cast<Usize>(j)];
      interior_node_tag.emplace_back(adjacency_element_mesh_supplemental.node_tag_[static_cast<Usize>(j)]);
    }
    information.gmsh_tag_to_element_physical_information_[this->element_(i).gmsh_tag_].element_index_ = i;
    this->element_(i).adjacency_right_rotation_ = adjacency_element_mesh_supplemental.right_rotation_;
    for (Isize j = 0; j < 2; j++) {
      this->element_(i).parent_index_each_type_(j) =
          information.gmsh_tag_to_element_physical_information_
              .at(adjacency_element_mesh_supplemental.parent_gmsh_tag_[static_cast<Usize>(j)])
              .element_index_;
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
template <MeshModelEnum MeshModelType>
inline void AdjacencyElementMesh<AdjacencyElementTrait>::getAdjacencyElementMesh(
    const Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, Eigen::Dynamic>& node_coordinate,
    MeshInformation& information) {
  std::unordered_map<Isize, AdjacencyElementMeshSupplemental<AdjacencyElementTrait>>
      adjacency_element_mesh_supplemental_map;
  if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
    getAdjacencyElementMeshSupplementalMap<AdjacencyElementTrait, LineTrait<AdjacencyElementTrait::kPolynomialOrder>>(
        adjacency_element_mesh_supplemental_map);
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
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
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
    if constexpr (HasTetrahedron<MeshModelType>) {
      getAdjacencyElementMeshSupplementalMap<AdjacencyElementTrait,
                                             TetrahedronTrait<AdjacencyElementTrait::kPolynomialOrder>>(
          adjacency_element_mesh_supplemental_map);
    }
    if constexpr (HasPyramid<MeshModelType>) {
      getAdjacencyElementMeshSupplementalMap<AdjacencyElementTrait,
                                             PyramidTrait<AdjacencyElementTrait::kPolynomialOrder>>(
          adjacency_element_mesh_supplemental_map);
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
    if constexpr (HasPyramid<MeshModelType>) {
      getAdjacencyElementMeshSupplementalMap<AdjacencyElementTrait,
                                             PyramidTrait<AdjacencyElementTrait::kPolynomialOrder>>(
          adjacency_element_mesh_supplemental_map);
    }
    if constexpr (HasHexahedron<MeshModelType>) {
      getAdjacencyElementMeshSupplementalMap<AdjacencyElementTrait,
                                             HexahedronTrait<AdjacencyElementTrait::kPolynomialOrder>>(
          adjacency_element_mesh_supplemental_map);
    }
  }
  fixAdjacencyElementMeshSupplementalMap<AdjacencyElementTrait>(information, adjacency_element_mesh_supplemental_map);
  std::vector<Isize> interior_tag;
  std::vector<Isize> boundary_tag;
  for (const auto& [adjacency_tag, adjacency_element_mesh_supplemental] : adjacency_element_mesh_supplemental_map) {
    if (adjacency_element_mesh_supplemental.is_recorded_) {
      interior_tag.emplace_back(adjacency_tag);
    } else {
      boundary_tag.emplace_back(adjacency_tag);
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

#endif  // SUBROSA_DG_ADJACENCY_CPP_
