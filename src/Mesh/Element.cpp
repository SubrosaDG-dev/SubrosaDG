/**
 * @file Element.cpp
 * @brief The header file of Element.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2025 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_ELEMENT_CPP_
#define SUBROSA_DG_ELEMENT_CPP_

#include <gmsh.h>

#include <Eigen/Core>
#include <cstddef>
#include <format>
#include <magic_enum/magic_enum.hpp>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "Mesh/ReadControl.cpp"
#include "Utils/BasicDataType.cpp"

namespace SubrosaDG {

template <typename ElementTrait>
inline void ElementMesh<ElementTrait>::getElementMesh(
    const Eigen::Matrix<Real, ElementTrait::kDimension, Eigen::Dynamic>& node_coordinate,
    MeshInformation& information) {
  std::vector<std::size_t> element_tags;
  std::vector<std::size_t> node_tags;
  gmsh::model::mesh::getElementsByType(ElementTrait::kGmshTypeNumber, element_tags, node_tags);
  this->number_ = static_cast<Isize>(element_tags.size());
  if (this->number_ == 0) [[unlikely]] {
    throw std::runtime_error(
        std::format("{} element number is zero.", magic_enum::enum_name(ElementTrait::kElementType)));
  }
  this->element_.resize(this->number_);
  for (Isize i = 0; i < this->number_; i++) {
    this->element_(i).gmsh_tag_ = static_cast<Isize>(element_tags[static_cast<Usize>(i)]);
    this->element_(i).gmsh_physical_index_ =
        information.gmsh_tag_to_element_physical_information_.at(this->element_(i).gmsh_tag_).gmsh_physical_index_;
    this->element_(i).element_index_ = i;
    information.physical_[static_cast<Usize>(this->element_(i).gmsh_physical_index_) - 1].element_number_++;
    information.physical_[static_cast<Usize>(this->element_(i).gmsh_physical_index_) - 1].vtk_element_number_ +=
        ElementTrait::kVtkElementNumber;
    information.physical_[static_cast<Usize>(this->element_(i).gmsh_physical_index_) - 1]
        .element_gmsh_type_.emplace_back(ElementTrait::kGmshTypeNumber);
    information.physical_[static_cast<Usize>(this->element_(i).gmsh_physical_index_) - 1]
        .element_gmsh_tag_.emplace_back(this->element_(i).gmsh_tag_);
    information.physical_[static_cast<Usize>(this->element_(i).gmsh_physical_index_) - 1].node_number_ +=
        ElementTrait::kAllNodeNumber;
    information.physical_[static_cast<Usize>(this->element_(i).gmsh_physical_index_) - 1].vtk_node_number_ +=
        ElementTrait::kVtkAllNodeNumber;
    information.gmsh_tag_to_element_physical_information_[this->element_(i).gmsh_tag_].element_index_ = i;
    for (Isize j = 0; j < ElementTrait::kAllNodeNumber; j++) {
      const auto node_tag = static_cast<Isize>(node_tags[static_cast<Usize>(i * ElementTrait::kAllNodeNumber + j)]);
      this->element_(i).node_coordinate_.col(j) = node_coordinate.col(node_tag - 1);
      this->element_(i).node_tag_(j) = node_tag;
    }
  }
  this->getElementQuality();
  this->getElementJacobian();
  this->calculateElementLocalMassMatrixInverse();
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_ELEMENT_CPP_
