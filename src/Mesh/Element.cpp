/**
 * @file Element.cpp
 * @brief The header file of Element.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_ELEMENT_CPP_
#define SUBROSA_DG_ELEMENT_CPP_

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <gmsh.h>

#include <Eigen/Core>
#include <array>
#include <cstddef>
#include <format>
#include <iostream>
#include <magic_enum/magic_enum.hpp>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Mesh/ReadControl.cpp"
#include "Solver/SimulationControl.cpp"
#include "Utils/BasicDataType.cpp"
#include "Utils/Concept.cpp"
#include "Utils/Enum.cpp"

namespace SubrosaDG {

template <typename AdjacencyElementTrait, typename VolumeElementTrait>
inline std::pair<Isize, Isize> getAdjacencyElmentParentAndSelfSequence(const Isize adjacency_number) {
  if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point ||
                AdjacencyElementTrait::kElementType == ElementEnum::Line) {
    return std::make_pair(adjacency_number / VolumeElementTrait::kAdjacencyNumber,
                          adjacency_number % VolumeElementTrait::kAdjacencyNumber);
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
    if constexpr (VolumeElementTrait::kElementType == ElementEnum::Tetrahedron) {
      return std::make_pair(adjacency_number / VolumeElementTrait::kAdjacencyNumber,
                            adjacency_number % VolumeElementTrait::kAdjacencyNumber);
    } else if constexpr (VolumeElementTrait::kElementType == ElementEnum::Pyramid) {
      return std::make_pair(adjacency_number / (VolumeElementTrait::kAdjacencyNumber - 1),
                            adjacency_number % (VolumeElementTrait::kAdjacencyNumber - 1));
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
    if constexpr (VolumeElementTrait::kElementType == ElementEnum::Pyramid) {
      return std::make_pair(adjacency_number, 4);
    } else if constexpr (VolumeElementTrait::kElementType == ElementEnum::Hexahedron) {
      return std::make_pair(adjacency_number / VolumeElementTrait::kAdjacencyNumber,
                            adjacency_number % VolumeElementTrait::kAdjacencyNumber);
    }
  }
}

template <typename AdjacencyElementTrait>
template <Is1dElement VolumeElementTrait>
inline void AdjacencyElementMeshSupplemental<AdjacencyElementTrait>::getAdjacencyElementPerVolumeInformationMap(
    const MeshPhysical& physical)
  requires Is0dElement<AdjacencyElementTrait>
{
  std::vector<std::size_t> element_tags;
  std::vector<std::size_t> element_node_tags;
  gmsh::model::mesh::getElementsByType(VolumeElementTrait::kGmshTypeNumber, element_tags, element_node_tags);
  std::vector<std::size_t> adjacency_node_tags;
  const auto element_number = static_cast<Usize>(element_tags.size());
  const auto all_adjacency_number = static_cast<Usize>(element_tags.size() * 2);
  for (Usize i = 0; i < element_number; i++) {
    adjacency_node_tags.emplace_back(element_node_tags[i * VolumeElementTrait::kAllNodeNumber]);
    adjacency_node_tags.emplace_back(element_node_tags[i * VolumeElementTrait::kAllNodeNumber + 1]);
  }
  for (Usize i = 0; i < all_adjacency_number; i++) {
    const auto point_tag = static_cast<Isize>(adjacency_node_tags[i]);
    if (!this->information_map_.contains(point_tag)) {
      this->information_map_[point_tag].node_tag_[0] = point_tag;
    } else {
      this->information_map_[point_tag].is_recorded_ = true;
    }
    const std::pair<Isize, Isize> parent_and_self_sequence =
        getAdjacencyElmentParentAndSelfSequence<AdjacencyElementTrait, VolumeElementTrait>(static_cast<Isize>(i));
    const auto parent_gmsh_tag = static_cast<Isize>(element_tags[static_cast<Usize>(parent_and_self_sequence.first)]);
    const auto parent_gmsh_physical_index =
        static_cast<Usize>(physical.gmsh_tag_to_element_physical_information_.at(parent_gmsh_tag).gmsh_physical_index_);
    this->information_map_[point_tag].parent_gmsh_tag_.emplace_back(parent_gmsh_tag);
    this->information_map_[point_tag].adjacency_sequence_.emplace_back(parent_and_self_sequence.second);
    this->information_map_[point_tag].parent_gmsh_type_.emplace_back(VolumeElementTrait::kGmshTypeNumber);
    if (physical.information_[static_cast<Usize>(parent_gmsh_physical_index) - 1].interior_condition_type_ ==
        InteriorConditionEnum::Rotate) {
      this->information_map_[point_tag].parent_interior_condition_type_.emplace_back(InteriorConditionEnum::Rotate);
    } else {
      this->information_map_[point_tag].parent_interior_condition_type_.emplace_back(InteriorConditionEnum::Static);
    }
  }
}

template <typename AdjacencyElementTrait>
template <Is2dElement VolumeElementTrait>
inline void AdjacencyElementMeshSupplemental<AdjacencyElementTrait>::getAdjacencyElementPerVolumeInformationMap(
    const MeshPhysical& physical)
  requires Is1dElement<AdjacencyElementTrait>
{
  std::vector<std::size_t> adjacency_node_tags;
  gmsh::model::mesh::getElementEdgeNodes(VolumeElementTrait::kGmshTypeNumber, adjacency_node_tags);
  std::vector<std::size_t> adjacency_basic_node_tags;
  gmsh::model::mesh::getElementEdgeNodes(VolumeElementTrait::kGmshTypeNumber, adjacency_basic_node_tags, -1, true);
  const auto all_adjacency_number =
      static_cast<Usize>(adjacency_node_tags.size() / AdjacencyElementTrait::kAllNodeNumber);
  std::vector<std::size_t> edge_tags;
  std::vector<int> edge_orientations;
  gmsh::model::mesh::getEdges(adjacency_basic_node_tags, edge_tags, edge_orientations);
  std::vector<std::size_t> element_tags;
  std::vector<std::size_t> element_node_tags;
  gmsh::model::mesh::getElementsByType(VolumeElementTrait::kGmshTypeNumber, element_tags, element_node_tags);
  for (Usize i = 0; i < all_adjacency_number; i++) {
    const auto edge_tag = static_cast<Isize>(edge_tags[i]);
    if (!this->information_map_.contains(edge_tag)) {
      for (Usize j = 0; j < AdjacencyElementTrait::kAllNodeNumber; j++) {
        this->information_map_[edge_tag].node_tag_[j] =
            static_cast<Isize>(adjacency_node_tags[i * AdjacencyElementTrait::kAllNodeNumber + j]);
      }
    } else {
      this->information_map_[edge_tag].is_recorded_ = true;
    }
    const std::pair<Isize, Isize> parent_and_self_sequence =
        getAdjacencyElmentParentAndSelfSequence<AdjacencyElementTrait, VolumeElementTrait>(static_cast<Isize>(i));
    const auto parent_gmsh_tag = static_cast<Isize>(element_tags[static_cast<Usize>(parent_and_self_sequence.first)]);
    const auto parent_gmsh_physical_index =
        static_cast<Usize>(physical.gmsh_tag_to_element_physical_information_.at(parent_gmsh_tag).gmsh_physical_index_);
    this->information_map_[edge_tag].parent_gmsh_tag_.emplace_back(parent_gmsh_tag);
    this->information_map_[edge_tag].adjacency_sequence_.emplace_back(parent_and_self_sequence.second);
    this->information_map_[edge_tag].parent_gmsh_type_.emplace_back(VolumeElementTrait::kGmshTypeNumber);
    if (physical.information_[static_cast<Usize>(parent_gmsh_physical_index) - 1].interior_condition_type_ ==
        InteriorConditionEnum::Rotate) {
      this->information_map_[edge_tag].parent_interior_condition_type_.emplace_back(InteriorConditionEnum::Rotate);
    } else {
      this->information_map_[edge_tag].parent_interior_condition_type_.emplace_back(InteriorConditionEnum::Static);
    }
  }
}

template <typename AdjacencyElementTrait>
template <Is3dElement VolumeElementTrait>
inline void AdjacencyElementMeshSupplemental<AdjacencyElementTrait>::getAdjacencyElementPerVolumeInformationMap(
    const MeshPhysical& physical)
  requires Is2dElement<AdjacencyElementTrait>
{
  std::vector<std::size_t> adjacency_node_tags;
  gmsh::model::mesh::getElementFaceNodes(VolumeElementTrait::kGmshTypeNumber, AdjacencyElementTrait::kBasicNodeNumber,
                                         adjacency_node_tags);
  std::vector<std::size_t> adjacency_basic_node_tags;
  gmsh::model::mesh::getElementFaceNodes(VolumeElementTrait::kGmshTypeNumber, AdjacencyElementTrait::kBasicNodeNumber,
                                         adjacency_basic_node_tags, -1, true);
  const auto all_adjacency_number =
      static_cast<Usize>(adjacency_node_tags.size() / AdjacencyElementTrait::kAllNodeNumber);
  std::vector<std::size_t> face_tags;
  std::vector<int> face_orientations;
  gmsh::model::mesh::getFaces(AdjacencyElementTrait::kBasicNodeNumber, adjacency_basic_node_tags, face_tags,
                              face_orientations);
  std::vector<std::size_t> element_tags;
  std::vector<std::size_t> element_node_tags;
  gmsh::model::mesh::getElementsByType(VolumeElementTrait::kGmshTypeNumber, element_tags, element_node_tags);
  std::array<std::size_t, AdjacencyElementTrait::kBasicNodeNumber> right_basic_node_tag;
  for (Usize i = 0; i < all_adjacency_number; i++) {
    const auto face_tag = static_cast<Isize>(face_tags[i]);
    if (!this->information_map_.contains(face_tag)) {
      for (Usize j = 0; j < AdjacencyElementTrait::kAllNodeNumber; j++) {
        this->information_map_[face_tag].node_tag_[j] =
            static_cast<Isize>(adjacency_node_tags[i * AdjacencyElementTrait::kAllNodeNumber + j]);
      }
    } else {
      if (this->information_map_[face_tag].is_recorded_) [[unlikely]] {
        throw std::runtime_error(fmt::format("The adjacency element with node tag {} is recorded more than twice.",
                                             fmt::join(this->information_map_[face_tag].node_tag_, " ")));
      }
      for (Usize j = 0; j < AdjacencyElementTrait::kBasicNodeNumber; j++) {
        right_basic_node_tag[j] = adjacency_node_tags[i * AdjacencyElementTrait::kAllNodeNumber + j];
      }
      this->information_map_[face_tag].right_rotation_ = static_cast<Isize>(std::distance(
          right_basic_node_tag.begin(), std::find(right_basic_node_tag.begin(), right_basic_node_tag.end(),
                                                  this->information_map_[face_tag].node_tag_[0])));
      this->information_map_[face_tag].is_recorded_ = true;
    }
    const std::pair<Isize, Isize> parent_and_self_sequence =
        getAdjacencyElmentParentAndSelfSequence<AdjacencyElementTrait, VolumeElementTrait>(static_cast<Isize>(i));
    const auto parent_gmsh_tag = static_cast<Isize>(element_tags[static_cast<Usize>(parent_and_self_sequence.first)]);
    const auto parent_gmsh_physical_index =
        static_cast<Usize>(physical.gmsh_tag_to_element_physical_information_.at(parent_gmsh_tag).gmsh_physical_index_);
    this->information_map_[face_tag].parent_gmsh_tag_.emplace_back(parent_gmsh_tag);
    this->information_map_[face_tag].adjacency_sequence_.emplace_back(parent_and_self_sequence.second);
    this->information_map_[face_tag].parent_gmsh_type_.emplace_back(VolumeElementTrait::kGmshTypeNumber);
    if (physical.information_[static_cast<Usize>(parent_gmsh_physical_index) - 1].interior_condition_type_ ==
        InteriorConditionEnum::Rotate) {
      this->information_map_[face_tag].parent_interior_condition_type_.emplace_back(InteriorConditionEnum::Rotate);
    } else {
      this->information_map_[face_tag].parent_interior_condition_type_.emplace_back(InteriorConditionEnum::Static);
    }
  }
}

template <typename AdjacencyElementTrait>
inline void AdjacencyElementMeshSupplemental<AdjacencyElementTrait>::fixAdjacencyElementInformationMap(
    const MeshPhysical& physical)
  requires Is0dElement<AdjacencyElementTrait>
{
  for (Isize i = 0; i < physical.number_; i++) {
    if (physical.information_[static_cast<Usize>(i)].boundary_condition_type_ != BoundaryConditionEnum::Periodic) {
      continue;
    }
    std::vector<int> entity_tags;
    gmsh::model::getEntitiesForPhysicalGroup(0, i + 1, entity_tags);
    std::vector<std::size_t> element_tags;
    std::vector<std::size_t> element_node_tags;
    gmsh::model::mesh::getElementsByType(AdjacencyElementTrait::kGmshTypeNumber, element_tags, element_node_tags);
    const std::pair<Isize, Isize> node_tag =
        std::make_pair(static_cast<Isize>(element_node_tags[0]), static_cast<Isize>(element_node_tags[1]));
    this->information_map_[node_tag.first].is_recorded_ = true;
    this->information_map_[node_tag.first].parent_gmsh_tag_.emplace_back(
        this->information_map_[node_tag.second].parent_gmsh_tag_[0]);
    this->information_map_[node_tag.first].adjacency_sequence_.emplace_back(
        this->information_map_[node_tag.second].adjacency_sequence_[0]);
    this->information_map_[node_tag.first].parent_gmsh_type_.emplace_back(
        this->information_map_[node_tag.second].parent_gmsh_type_[0]);
    this->information_map_[node_tag.first].parent_interior_condition_type_.emplace_back(
        this->information_map_[node_tag.second].parent_interior_condition_type_[0]);
    this->information_map_.erase(node_tag.second);
  }
}

template <typename AdjacencyElementTrait>
inline void AdjacencyElementMeshSupplemental<AdjacencyElementTrait>::fixAdjacencyElementInformationMap(
    const MeshPhysical& physical)
  requires Is1dElement<AdjacencyElementTrait>
{
  for (Isize i = 0; i < physical.number_; i++) {
    if (physical.information_[static_cast<Usize>(i)].boundary_condition_type_ != BoundaryConditionEnum::Periodic) {
      continue;
    }
    std::vector<int> entity_tags;
    gmsh::model::getEntitiesForPhysicalGroup(AdjacencyElementTrait::kDimension, i + 1, entity_tags);
    std::vector<int> entity_tags_master;
    gmsh::model::mesh::getPeriodic(AdjacencyElementTrait::kDimension, entity_tags, entity_tags_master);
    std::unordered_map<int, int> periodic_entity_tag_map;
    for (Usize j = 0; j < entity_tags.size(); j++) {
      if (entity_tags[j] != entity_tags_master[j]) {
        periodic_entity_tag_map[entity_tags[j]] = entity_tags_master[j];
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
      for (Usize j = 0; j < element_tags_master.size(); j++) {
        const auto element_tag_master = static_cast<Isize>(element_tags_master[j]);
        const auto element_tag = static_cast<Isize>(element_tags[j]);
        this->information_map_[element_tag_master].is_recorded_ = true;
        this->information_map_[element_tag_master].parent_gmsh_tag_.emplace_back(
            this->information_map_[element_tag].parent_gmsh_tag_[0]);
        this->information_map_[element_tag_master].adjacency_sequence_.emplace_back(
            this->information_map_[element_tag].adjacency_sequence_[0]);
        this->information_map_[element_tag_master].parent_gmsh_type_.emplace_back(
            this->information_map_[element_tag].parent_gmsh_type_[0]);
        this->information_map_[element_tag_master].parent_interior_condition_type_.emplace_back(
            this->information_map_[element_tag].parent_interior_condition_type_[0]);
        this->information_map_.erase(element_tag);
      }
    }
  }
}

template <typename AdjacencyElementTrait>
inline void AdjacencyElementMeshSupplemental<AdjacencyElementTrait>::fixAdjacencyElementInformationMap(
    const MeshPhysical& physical)
  requires Is2dElement<AdjacencyElementTrait>
{
  for (Isize i = 0; i < physical.number_; i++) {
    if (physical.information_[static_cast<Usize>(i)].boundary_condition_type_ != BoundaryConditionEnum::Periodic) {
      continue;
    }
    std::vector<int> entity_tags;
    gmsh::model::getEntitiesForPhysicalGroup(AdjacencyElementTrait::kDimension, i + 1, entity_tags);
    std::vector<int> entity_tags_master;
    gmsh::model::mesh::getPeriodic(AdjacencyElementTrait::kDimension, entity_tags, entity_tags_master);
    // NOTE: Here the entity_tags contains both master and slave entity tags, and the order is not guaranteed.
    // So we have to find the master entity (entity_tags_master) which contains only master entity tags.
    std::unordered_map<int, int> periodic_entity_tag_map;
    for (Usize j = 0; j < entity_tags.size(); j++) {
      if (entity_tags[j] != entity_tags_master[j]) {
        periodic_entity_tag_map[entity_tags[j]] = entity_tags_master[j];
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
      for (Usize j = 0; j < node_tags.size(); j++) {
        node_tags_map[node_tags_master[j]] = node_tags[j];
      }
      std::vector<std::array<Isize, AdjacencyElementTrait::kAllNodeNumber>> element_node_tags_array(
          element_tags_master.size());
      for (Usize j = 0; j < element_tags_master.size(); j++) {
        for (Usize k = 0; k < AdjacencyElementTrait::kAllNodeNumber; k++) {
          element_node_tags_array[j][k] =
              static_cast<Isize>(element_node_tags[j * AdjacencyElementTrait::kAllNodeNumber + k]);
        }
      }
      for (Usize j = 0; j < element_tags_master.size(); j++) {
        const auto element_tag_master = static_cast<Isize>(element_tags_master[j]);
        const auto element_tag = static_cast<Isize>(element_tags[j]);
        this->information_map_[element_tag_master].is_recorded_ = true;
        // NOTE: The right rotation is calculated by the order of the node tags in the master entity. The node_tag_map
        // is used to find the corresponding node tag in the slave entity. Then the right rotation is calculated by the
        // distance between the first node tag in the master entity and the corresponding node tag in the slave entity.
        this->information_map_[element_tag_master].right_rotation_ = static_cast<Isize>(std::distance(
            element_node_tags_array[j].begin(),
            std::find(element_node_tags_array[j].begin(), element_node_tags_array[j].end(),
                      node_tags_map[element_node_tags_master[j * AdjacencyElementTrait::kAllNodeNumber]])));
        this->information_map_[element_tag_master].parent_gmsh_tag_.emplace_back(
            this->information_map_[element_tag].parent_gmsh_tag_[0]);
        this->information_map_[element_tag_master].adjacency_sequence_.emplace_back(
            this->information_map_[element_tag].adjacency_sequence_[0]);
        this->information_map_[element_tag_master].parent_gmsh_type_.emplace_back(
            this->information_map_[element_tag].parent_gmsh_type_[0]);
        this->information_map_[element_tag_master].parent_interior_condition_type_.emplace_back(
            this->information_map_[element_tag].parent_interior_condition_type_[0]);
        this->information_map_.erase(element_tag);
      }
    }
  }
}

template <typename AdjacencyElementTrait>
template <MeshModelEnum MeshModelType>
inline void AdjacencyElementMeshSupplemental<AdjacencyElementTrait>::getAdjacencyElementMeshSupplemental(
    const MeshPhysical& physical) {
  if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
    this->template getAdjacencyElementPerVolumeInformationMap<VolumeLineTrait<AdjacencyElementTrait::kPolynomialOrder>>(
        physical);
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
    if constexpr (HasTriangle<MeshModelType>) {
      this->template getAdjacencyElementPerVolumeInformationMap<
          VolumeTriangleTrait<AdjacencyElementTrait::kPolynomialOrder>>(physical);
    }
    if constexpr (HasQuadrangle<MeshModelType>) {
      this->template getAdjacencyElementPerVolumeInformationMap<
          VolumeQuadrangleTrait<AdjacencyElementTrait::kPolynomialOrder>>(physical);
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
    if constexpr (HasTetrahedron<MeshModelType>) {
      this->template getAdjacencyElementPerVolumeInformationMap<
          VolumeTetrahedronTrait<AdjacencyElementTrait::kPolynomialOrder>>(physical);
    }
    if constexpr (HasPyramid<MeshModelType>) {
      this->template getAdjacencyElementPerVolumeInformationMap<
          VolumePyramidTrait<AdjacencyElementTrait::kPolynomialOrder>>(physical);
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
    if constexpr (HasPyramid<MeshModelType>) {
      this->template getAdjacencyElementPerVolumeInformationMap<
          VolumePyramidTrait<AdjacencyElementTrait::kPolynomialOrder>>(physical);
    }
    if constexpr (HasHexahedron<MeshModelType>) {
      this->template getAdjacencyElementPerVolumeInformationMap<
          VolumeHexahedronTrait<AdjacencyElementTrait::kPolynomialOrder>>(physical);
    }
  }
  this->fixAdjacencyElementInformationMap(physical);
  for (const auto& [adjacency_tag, per_adjacency_element_information] : this->information_map_) {
    if (per_adjacency_element_information.is_recorded_) {
      if (per_adjacency_element_information.parent_interior_condition_type_[0] == InteriorConditionEnum::Static &&
          per_adjacency_element_information.parent_interior_condition_type_[1] == InteriorConditionEnum::Static) {
        this->interior_static_tag_.emplace_back(adjacency_tag);
      } else if (per_adjacency_element_information.parent_interior_condition_type_[0] ==
                     InteriorConditionEnum::Rotate &&
                 per_adjacency_element_information.parent_interior_condition_type_[1] ==
                     InteriorConditionEnum::Rotate) {
        this->interior_rotate_tag_.emplace_back(adjacency_tag);
      } else {
        this->interface_tag_.emplace_back(adjacency_tag);
      }
    } else {
      if (per_adjacency_element_information.parent_interior_condition_type_[0] == InteriorConditionEnum::Static) {
        this->boundary_static_tag_.emplace_back(adjacency_tag);
      } else {
        this->boundary_rotate_tag_.emplace_back(adjacency_tag);
      }
    }
  }
}

template <typename VolumeElementTrait>
inline void VolumeElementMesh<VolumeElementTrait>::getVolumeElementMesh(
    const Eigen::Matrix<Real, VolumeElementTrait::kDimension, Eigen::Dynamic>& node_coordinate,
    MeshPhysical& physical) {
  std::vector<std::size_t> element_tags;
  std::vector<std::size_t> node_tags;
  gmsh::model::mesh::getElementsByType(VolumeElementTrait::kGmshTypeNumber, element_tags, node_tags);
  this->number_ = static_cast<Isize>(element_tags.size());
  if (this->number_ == 0) [[unlikely]] {
    throw std::runtime_error(
        std::format("{} element number is zero.", magic_enum::enum_name(VolumeElementTrait::kElementType)));
  }
  this->gmsh_tag_.resize(this->number_);
  this->gmsh_physical_index_.resize(this->number_);
  this->interior_condition_type_.resize(this->number_);
  this->node_tag_.resize(this->number_);
  this->jacobian_determinant_multiply_weight_.resize(this->number_);
  this->node_coordinate_.resize(this->number_);
  this->quadrature_node_coordinate_.resize(this->number_);
  this->local_mass_matrix_inverse_.resize(this->number_);
  this->jacobian_transpose_inverse_multiply_determinate_and_weight_.resize(this->number_);
  this->minimum_edge_.resize(this->number_);
  this->node_coordinate_initial_.resize(this->number_);
  std::vector<std::size_t> static_element_index;
  std::vector<std::size_t> rotate_element_index;
  for (Isize i = 0; i < this->number_; i++) {
    const auto gmsh_tag = static_cast<Isize>(element_tags[static_cast<Usize>(i)]);
    const auto gmsh_physical_index =
        static_cast<Usize>(physical.gmsh_tag_to_element_physical_information_.at(gmsh_tag).gmsh_physical_index_);
    if (physical.information_[static_cast<Usize>(gmsh_physical_index) - 1].interior_condition_type_ ==
        InteriorConditionEnum::Rotate) {
      rotate_element_index.emplace_back(i);
    } else {
      static_element_index.emplace_back(i);
    }
  }
  this->static_number_ = static_cast<Isize>(static_element_index.size());
  this->rotate_number_ = static_cast<Isize>(rotate_element_index.size());
  for (Isize i = 0; i < this->number_; i++) {
    if (i < this->static_number_) {
      this->gmsh_tag_(i) =
          static_cast<Isize>(element_tags[static_cast<Usize>(static_element_index[static_cast<Usize>(i)])]);
      this->interior_condition_type_(i) = InteriorConditionEnum::Static;
    } else {
      this->gmsh_tag_(i) = static_cast<Isize>(
          element_tags[static_cast<Usize>(rotate_element_index[static_cast<Usize>(i - this->static_number_)])]);
      this->interior_condition_type_(i) = InteriorConditionEnum::Rotate;
    }
    const auto gmsh_physical_index = static_cast<Usize>(
        physical.gmsh_tag_to_element_physical_information_.at(this->gmsh_tag_(i)).gmsh_physical_index_);
    this->gmsh_physical_index_(i) = static_cast<Isize>(gmsh_physical_index);
    physical.information_[gmsh_physical_index - 1].element_number_++;
    physical.information_[gmsh_physical_index - 1].vtk_element_number_ += VolumeElementTrait::kVtkElementNumber;
    physical.information_[gmsh_physical_index - 1].element_gmsh_type_.emplace_back(VolumeElementTrait::kGmshTypeNumber);
    physical.information_[gmsh_physical_index - 1].element_gmsh_tag_.emplace_back(this->gmsh_tag_(i));
    physical.information_[gmsh_physical_index - 1].node_number_ += VolumeElementTrait::kAllNodeNumber;
    physical.information_[gmsh_physical_index - 1].vtk_node_number_ += VolumeElementTrait::kVtkAllNodeNumber;
    physical.gmsh_tag_to_element_physical_information_[this->gmsh_tag_(i)].element_index_ = i;
    for (Isize j = 0; j < VolumeElementTrait::kAllNodeNumber; j++) {
      const auto node_tag =
          static_cast<Isize>(node_tags[static_cast<Usize>(i * VolumeElementTrait::kAllNodeNumber + j)]);
      this->node_coordinate_(i).col(j) = node_coordinate.col(node_tag - 1);
      this->node_tag_(i)(j) = node_tag;
    }
    this->node_coordinate_initial_(i) = this->node_coordinate_(i);
  }
  this->getVolumeElementQuality();
  this->computeVolumeElementOtherNodeCoordinate();
  this->computeVolumeElementJacobian();
  this->computeVolumeElementLocalMassMatrixInverse();
}

template <typename AdjacencyElementTrait>
inline void AdjacencyElementMesh<AdjacencyElementTrait>::getAdjacencyElementInteriorMesh(
    const Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, Eigen::Dynamic>& node_coordinate,
    const AdjacencyElementMeshSupplemental<AdjacencyElementTrait>& adjacency_element_mesh_supplemental,
    MeshPhysical& physical) {
  std::size_t max_tag;
  gmsh::model::mesh::getMaxElementTag(max_tag);
  const int entity_tag = gmsh::model::addDiscreteEntity(AdjacencyElementTrait::kDimension);
  std::vector<std::size_t> interior_gmsh_tag(static_cast<Usize>(this->interior_number_));
  std::vector<std::size_t> interior_node_tag(
      static_cast<Usize>(this->interior_number_ * AdjacencyElementTrait::kAllNodeNumber));
  Isize interior_tag;
  for (Isize i = 0; i < this->interior_number_; i++) {
    const Isize element_index = i;
    if (i < this->interior_static_number_) {
      interior_tag = adjacency_element_mesh_supplemental.interior_static_tag_[static_cast<Usize>(i)];
      this->interior_condition_type_(element_index) = InteriorConditionEnum::Static;
    } else {
      interior_tag = adjacency_element_mesh_supplemental
                         .interior_rotate_tag_[static_cast<Usize>(i - this->interior_static_number_)];
      this->interior_condition_type_(element_index) = InteriorConditionEnum::Rotate;
    }
    this->gmsh_tag_(element_index) = static_cast<Isize>(max_tag) + i + 1;
    interior_gmsh_tag[static_cast<Usize>(i)] = static_cast<std::size_t>(this->gmsh_tag_(element_index));
    const PerAdjacencyElementInformation<AdjacencyElementTrait>& adjacency_element_information =
        adjacency_element_mesh_supplemental.information_map_.at(interior_tag);
    for (Isize j = 0; j < AdjacencyElementTrait::kAllNodeNumber; j++) {
      this->node_coordinate_(element_index).col(j) =
          node_coordinate.col(adjacency_element_information.node_tag_[static_cast<Usize>(j)] - 1);
      this->node_tag_(element_index)(j) = adjacency_element_information.node_tag_[static_cast<Usize>(j)];
      interior_node_tag[static_cast<Usize>(i * AdjacencyElementTrait::kAllNodeNumber + j)] =
          static_cast<std::size_t>(adjacency_element_information.node_tag_[static_cast<Usize>(j)]);
    }
    physical.gmsh_tag_to_element_physical_information_[this->gmsh_tag_(element_index)].element_index_ = element_index;
    this->adjacency_right_rotation_(element_index) = adjacency_element_information.right_rotation_;
    this->left_parent_index_each_type_(element_index) =
        physical.gmsh_tag_to_element_physical_information_.at(adjacency_element_information.parent_gmsh_tag_[0])
            .element_index_;
    this->right_parent_index_each_type_(element_index) =
        physical.gmsh_tag_to_element_physical_information_.at(adjacency_element_information.parent_gmsh_tag_[1])
            .element_index_;
    this->adjacency_sequence_in_left_parent_(element_index) = adjacency_element_information.adjacency_sequence_[0];
    this->adjacency_sequence_in_right_parent_(element_index) = adjacency_element_information.adjacency_sequence_[1];
    this->left_parent_gmsh_type_number_(element_index) = adjacency_element_information.parent_gmsh_type_[0];
    this->right_parent_gmsh_type_number_(element_index) = adjacency_element_information.parent_gmsh_type_[1];
    this->node_coordinate_initial_(element_index) = this->node_coordinate_(element_index);
  }
  gmsh::model::mesh::addElementsByType(entity_tag, AdjacencyElementTrait::kGmshTypeNumber, interior_gmsh_tag,
                                       interior_node_tag);
}

template <typename AdjacencyElementTrait>
inline void AdjacencyElementMesh<AdjacencyElementTrait>::getAdjacencyElementBoundaryMesh(
    const Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, Eigen::Dynamic>& node_coordinate,
    const AdjacencyElementMeshSupplemental<AdjacencyElementTrait>& adjacency_element_mesh_supplemental,
    MeshPhysical& physical) {
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
  Isize boundary_tag;
  for (Isize i = 0; i < this->boundary_number_; i++) {
    const Isize element_index = i + this->interior_number_;
    if (i < this->boundary_static_number_) {
      boundary_tag = adjacency_element_mesh_supplemental.boundary_static_tag_[static_cast<Usize>(i)];
      this->interior_condition_type_(element_index) = InteriorConditionEnum::Static;
    } else {
      boundary_tag = adjacency_element_mesh_supplemental
                         .boundary_rotate_tag_[static_cast<Usize>(i - this->boundary_static_number_)];
      this->interior_condition_type_(element_index) = InteriorConditionEnum::Rotate;
    }
    const PerAdjacencyElementInformation<AdjacencyElementTrait>& adjacency_element_information =
        adjacency_element_mesh_supplemental.information_map_.at(boundary_tag);
    unordered_array<Isize, AdjacencyElementTrait::kBasicNodeNumber> node_tag;
    for (Isize j = 0; j < AdjacencyElementTrait::kBasicNodeNumber; j++) {
      node_tag[static_cast<Usize>(j)] = adjacency_element_information.node_tag_[static_cast<Usize>(j)];
    }
    for (Isize j = 0; j < AdjacencyElementTrait::kAllNodeNumber; j++) {
      this->node_coordinate_(element_index).col(j) =
          node_coordinate.col(adjacency_element_information.node_tag_[static_cast<Usize>(j)] - 1);
      this->node_tag_(element_index)(j) = adjacency_element_information.node_tag_[static_cast<Usize>(j)];
    }
    try {
      this->gmsh_tag_(element_index) = node_tag_element_map.at(node_tag);
    } catch (const std::out_of_range& error) {
      std::cout << fmt::format("Cannot find adjacency element with node tag: {}", fmt::join(node_tag, " ")) << '\n';
      std::cout << "Check your physical group definition or computational mesh type." << '\n';
    }
    const auto gmsh_physical_index = static_cast<Usize>(
        physical.gmsh_tag_to_element_physical_information_.at(this->gmsh_tag_(element_index)).gmsh_physical_index_);
    this->gmsh_physical_index_(element_index) = static_cast<Isize>(gmsh_physical_index);
    this->boundary_condition_type_(element_index) =
        physical.information_[gmsh_physical_index - 1].boundary_condition_type_;
    physical.information_[gmsh_physical_index - 1].element_number_++;
    physical.information_[gmsh_physical_index - 1].vtk_element_number_ += AdjacencyElementTrait::kVtkElementNumber;
    physical.information_[gmsh_physical_index - 1].element_gmsh_type_.emplace_back(
        AdjacencyElementTrait::kGmshTypeNumber);
    physical.information_[gmsh_physical_index - 1].element_gmsh_tag_.emplace_back(this->gmsh_tag_(element_index));
    physical.information_[gmsh_physical_index - 1].node_number_ += AdjacencyElementTrait::kAllNodeNumber;
    physical.information_[gmsh_physical_index - 1].vtk_node_number_ += AdjacencyElementTrait::kVtkAllNodeNumber;
    physical.gmsh_tag_to_element_physical_information_[this->gmsh_tag_(element_index)].element_index_ = element_index;
    this->left_parent_index_each_type_(element_index) =
        physical.gmsh_tag_to_element_physical_information_.at(adjacency_element_information.parent_gmsh_tag_[0])
            .element_index_;
    this->adjacency_sequence_in_left_parent_(element_index) = adjacency_element_information.adjacency_sequence_[0];
    this->left_parent_gmsh_type_number_(element_index) = adjacency_element_information.parent_gmsh_type_[0];
    this->node_coordinate_initial_(element_index) = this->node_coordinate_(element_index);
  }
}

template <typename AdjacencyElementTrait>
inline void AdjacencyElementMesh<AdjacencyElementTrait>::getAdjacencyElementInterfaceMesh(
    const Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, Eigen::Dynamic>& node_coordinate,
    const AdjacencyElementMeshSupplemental<AdjacencyElementTrait>& adjacency_element_mesh_supplemental,
    MeshPhysical& physical) {
  std::size_t max_tag;
  gmsh::model::mesh::getMaxElementTag(max_tag);
  const int entity_tag = gmsh::model::addDiscreteEntity(AdjacencyElementTrait::kDimension);
  std::vector<std::size_t> interface_gmsh_tag(2 * static_cast<Usize>(this->interface_number_));
  std::vector<std::size_t> interface_node_tag(
      2 * static_cast<Usize>(this->interface_number_ * AdjacencyElementTrait::kAllNodeNumber));
  std::vector<std::size_t> reverse_interface_gmsh_tag(static_cast<Usize>(this->interface_number_));
  for (Isize i = 0; i < this->interface_number_; i++) {
    const PerAdjacencyElementInformation<AdjacencyElementTrait>& adjacency_element_information =
        adjacency_element_mesh_supplemental.information_map_.at(
            adjacency_element_mesh_supplemental.interface_tag_[static_cast<Usize>(i)]);
    const Isize interface_static_index = i + this->interior_number_ + this->boundary_number_;
    const Isize interface_rotate_index = i + this->interior_number_ + this->boundary_number_ + this->interface_number_;
    this->gmsh_tag_(interface_static_index) = static_cast<Isize>(max_tag) + i + 1;
    this->gmsh_tag_(interface_rotate_index) = static_cast<Isize>(max_tag) + i + this->interface_number_ + 1;
    interface_gmsh_tag[static_cast<Usize>(i)] = static_cast<std::size_t>(this->gmsh_tag_(interface_static_index));
    interface_gmsh_tag[static_cast<Usize>(i + this->interface_number_)] =
        static_cast<std::size_t>(this->gmsh_tag_(interface_rotate_index));
    this->interior_condition_type_(interface_static_index) = InteriorConditionEnum::Interface;
    this->interior_condition_type_(interface_rotate_index) = InteriorConditionEnum::Interface;
    this->boundary_condition_type_(interface_static_index) = BoundaryConditionEnum::InterfaceStatic;
    this->boundary_condition_type_(interface_rotate_index) = BoundaryConditionEnum::InterfaceRotate;
    physical.gmsh_tag_to_element_physical_information_[this->gmsh_tag_(interface_static_index)].element_index_ =
        interface_static_index;
    physical.gmsh_tag_to_element_physical_information_[this->gmsh_tag_(interface_rotate_index)].element_index_ =
        interface_rotate_index;
    if (adjacency_element_information.parent_interior_condition_type_[0] == InteriorConditionEnum::Static) {
      for (Isize j = 0; j < AdjacencyElementTrait::kAllNodeNumber; j++) {
        this->node_coordinate_(interface_static_index).col(j) =
            node_coordinate.col(adjacency_element_information.node_tag_[static_cast<Usize>(j)] - 1);
        this->node_tag_(interface_static_index)(j) = adjacency_element_information.node_tag_[static_cast<Usize>(j)];
        interface_node_tag[static_cast<Usize>(i * AdjacencyElementTrait::kAllNodeNumber + j)] =
            static_cast<std::size_t>(adjacency_element_information.node_tag_[static_cast<Usize>(j)]);
        interface_node_tag[static_cast<Usize>((i + this->interface_number_) * AdjacencyElementTrait::kAllNodeNumber +
                                              j)] =
            static_cast<std::size_t>(adjacency_element_information.node_tag_[static_cast<Usize>(j)]);
      }
      this->left_parent_index_each_type_(interface_static_index) =
          physical.gmsh_tag_to_element_physical_information_.at(adjacency_element_information.parent_gmsh_tag_[0])
              .element_index_;
      this->adjacency_sequence_in_left_parent_(interface_static_index) =
          adjacency_element_information.adjacency_sequence_[0];
      this->left_parent_gmsh_type_number_(interface_static_index) = adjacency_element_information.parent_gmsh_type_[0];
      this->left_parent_index_each_type_(interface_rotate_index) =
          physical.gmsh_tag_to_element_physical_information_.at(adjacency_element_information.parent_gmsh_tag_[1])
              .element_index_;
      this->adjacency_sequence_in_left_parent_(interface_rotate_index) =
          adjacency_element_information.adjacency_sequence_[1];
      this->left_parent_gmsh_type_number_(interface_rotate_index) = adjacency_element_information.parent_gmsh_type_[1];
      reverse_interface_gmsh_tag[static_cast<Usize>(i)] =
          static_cast<std::size_t>(this->gmsh_tag_(interface_rotate_index));
    } else {
      for (Isize j = 0; j < AdjacencyElementTrait::kAllNodeNumber; j++) {
        this->node_coordinate_(interface_rotate_index).col(j) =
            node_coordinate.col(adjacency_element_information.node_tag_[static_cast<Usize>(j)] - 1);
        this->node_tag_(interface_rotate_index)(j) = adjacency_element_information.node_tag_[static_cast<Usize>(j)];
        interface_node_tag[static_cast<Usize>((i + this->interface_number_) * AdjacencyElementTrait::kAllNodeNumber +
                                              j)] =
            static_cast<std::size_t>(adjacency_element_information.node_tag_[static_cast<Usize>(j)]);
        interface_node_tag[static_cast<Usize>(i * AdjacencyElementTrait::kAllNodeNumber + j)] =
            static_cast<std::size_t>(adjacency_element_information.node_tag_[static_cast<Usize>(j)]);
      }
      this->left_parent_index_each_type_(interface_rotate_index) =
          physical.gmsh_tag_to_element_physical_information_.at(adjacency_element_information.parent_gmsh_tag_[0])
              .element_index_;
      this->adjacency_sequence_in_left_parent_(interface_rotate_index) =
          adjacency_element_information.adjacency_sequence_[0];
      this->left_parent_gmsh_type_number_(interface_rotate_index) = adjacency_element_information.parent_gmsh_type_[0];
      this->left_parent_index_each_type_(interface_static_index) =
          physical.gmsh_tag_to_element_physical_information_.at(adjacency_element_information.parent_gmsh_tag_[1])
              .element_index_;
      this->adjacency_sequence_in_left_parent_(interface_static_index) =
          adjacency_element_information.adjacency_sequence_[1];
      this->left_parent_gmsh_type_number_(interface_static_index) = adjacency_element_information.parent_gmsh_type_[1];
      reverse_interface_gmsh_tag[static_cast<Usize>(i)] =
          static_cast<std::size_t>(this->gmsh_tag_(interface_static_index));
    }
  }
  gmsh::model::mesh::addElementsByType(entity_tag, AdjacencyElementTrait::kGmshTypeNumber, interface_gmsh_tag,
                                       interface_node_tag);
  gmsh::model::mesh::reverseElements(reverse_interface_gmsh_tag);
  std::vector<std::size_t> element_tags;
  std::vector<std::size_t> node_tags;
  gmsh::model::mesh::getElementsByType(AdjacencyElementTrait::kGmshTypeNumber, element_tags, node_tags, entity_tag);
  for (Isize i = 0; i < this->interface_number_; i++) {
    const PerAdjacencyElementInformation<AdjacencyElementTrait>& adjacency_element_information =
        adjacency_element_mesh_supplemental.information_map_.at(
            adjacency_element_mesh_supplemental.interface_tag_[static_cast<Usize>(i)]);
    const Isize interface_static_index = i + this->interior_number_ + this->boundary_number_;
    const Isize interface_rotate_index = i + this->interior_number_ + this->boundary_number_ + this->interface_number_;
    if (adjacency_element_information.parent_interior_condition_type_[0] == InteriorConditionEnum::Static) {
      for (Isize j = 0; j < AdjacencyElementTrait::kAllNodeNumber; j++) {
        const auto node_tag = static_cast<Isize>(
            node_tags[static_cast<Usize>((i + this->interface_number_) * AdjacencyElementTrait::kAllNodeNumber + j)]);
        this->node_coordinate_(interface_rotate_index).col(j) = node_coordinate.col(node_tag - 1);
        this->node_tag_(interface_rotate_index)(j) = node_tag;
      }
    } else {
      for (Isize j = 0; j < AdjacencyElementTrait::kAllNodeNumber; j++) {
        const auto node_tag =
            static_cast<Isize>(node_tags[static_cast<Usize>(i * AdjacencyElementTrait::kAllNodeNumber + j)]);
        this->node_coordinate_(interface_static_index).col(j) = node_coordinate.col(node_tag - 1);
        this->node_tag_(interface_static_index)(j) = node_tag;
      }
    }
    this->node_coordinate_initial_(interface_static_index) = this->node_coordinate_(interface_static_index);
    this->node_coordinate_initial_(interface_rotate_index) = this->node_coordinate_(interface_rotate_index);
  }
}

template <typename AdjacencyElementTrait>
template <MeshModelEnum MeshModelType>
inline void AdjacencyElementMesh<AdjacencyElementTrait>::getAdjacencyElementMesh(
    const Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, Eigen::Dynamic>& node_coordinate,
    MeshPhysical& physical) {
  AdjacencyElementMeshSupplemental<AdjacencyElementTrait> adjacency_element_mesh_supplemental;
  adjacency_element_mesh_supplemental.template getAdjacencyElementMeshSupplemental<MeshModelType>(physical);
  this->interface_number_ = static_cast<Isize>(adjacency_element_mesh_supplemental.interface_tag_.size());
  this->interior_static_number_ = static_cast<Isize>(adjacency_element_mesh_supplemental.interior_static_tag_.size());
  this->boundary_static_number_ = static_cast<Isize>(adjacency_element_mesh_supplemental.boundary_static_tag_.size());
  this->static_number_ = this->interior_static_number_ + this->boundary_static_number_ + this->interface_number_;
  this->interior_rotate_number_ = static_cast<Isize>(adjacency_element_mesh_supplemental.interior_rotate_tag_.size());
  this->boundary_rotate_number_ = static_cast<Isize>(adjacency_element_mesh_supplemental.boundary_rotate_tag_.size());
  this->rotate_number_ = this->interior_rotate_number_ + this->boundary_rotate_number_ + this->interface_number_;
  this->interior_number_ = this->interior_static_number_ + this->interior_rotate_number_;
  this->boundary_number_ = this->boundary_static_number_ + this->boundary_rotate_number_;
  this->number_ = this->interior_number_ + this->boundary_number_ + 2 * this->interface_number_;
  this->gmsh_tag_.resize(this->number_);
  this->gmsh_physical_index_.resize(this->number_);
  this->interior_condition_type_.resize(this->number_);
  this->node_tag_.resize(this->number_);
  this->jacobian_determinant_multiply_weight_.resize(this->number_);
  this->node_coordinate_.resize(this->number_);
  this->center_node_coordinate_.resize(this->number_);
  this->quadrature_node_coordinate_.resize(this->number_);
  this->adjacency_right_rotation_.resize(this->number_);
  this->boundary_condition_type_.resize(this->number_);
  this->left_parent_index_each_type_.resize(this->number_);
  this->right_parent_index_each_type_.resize(this->number_);
  this->adjacency_sequence_in_left_parent_.resize(this->number_);
  this->adjacency_sequence_in_right_parent_.resize(this->number_);
  this->left_parent_gmsh_type_number_.resize(this->number_);
  this->right_parent_gmsh_type_number_.resize(this->number_);
  this->inner_radius_.resize(this->number_);
  this->normal_vector_.resize(this->number_);
  this->node_coordinate_initial_.resize(this->number_);
  this->getAdjacencyElementInteriorMesh(node_coordinate, adjacency_element_mesh_supplemental, physical);
  this->getAdjacencyElementBoundaryMesh(node_coordinate, adjacency_element_mesh_supplemental, physical);
  this->getAdjacencyElementInterfaceMesh(node_coordinate, adjacency_element_mesh_supplemental, physical);
  this->getAdjacencyElementQuality();
  this->computeAdjacencyElementOtherNodeCoordinate();
  this->computeAdjacencyElementJacobianDeterminant();
  this->computeAdjacencyElementNormalVector();
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_ELEMENT_CPP_
