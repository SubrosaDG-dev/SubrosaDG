/**
 * @file Element.hpp
 * @brief The header file of Element.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_ELEMENT_HPP_
#define SUBROSA_DG_ELEMENT_HPP_

#include <gmsh.h>

#include <Eigen/Core>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include "Mesh/ReadControl.hpp"
#include "Utils/BasicDataType.hpp"

namespace SubrosaDG {

template <typename ElementTrait>
inline void ElementMesh<ElementTrait>::getElementMesh(
    const Eigen::Matrix<Real, ElementTrait::kDimension, Eigen::Dynamic>& node_coordinate,
    std::unordered_map<std::string, PhysicalGroupInformation>& physical_group_information,
    std::unordered_map<Isize, PerElementPhysicalGroupInformation>& gmsh_tag_to_element_information,
    Eigen::Vector<Isize, Eigen::Dynamic>& node_element_number) {
  std::vector<std::size_t> element_tags;
  std::vector<std::size_t> node_tags;
  gmsh::model::mesh::getElementsByType(ElementTrait::kGmshTypeNumber, element_tags, node_tags);
  this->number_ = static_cast<Isize>(element_tags.size());
  this->element_.resize(this->number_);
  for (Isize i = 0; i < this->number_; i++) {
    this->element_(i).gmsh_tag_ = static_cast<Isize>(element_tags[static_cast<Usize>(i)]);
    this->element_(i).gmsh_physical_name_ =
        gmsh_tag_to_element_information.at(this->element_(i).gmsh_tag_).gmsh_physical_name_;
    this->element_(i).element_index_ = i;
    physical_group_information[this->element_(i).gmsh_physical_name_].element_number_++;
    physical_group_information[this->element_(i).gmsh_physical_name_].element_gmsh_type_.emplace_back(
        ElementTrait::kGmshTypeNumber);
    physical_group_information[this->element_(i).gmsh_physical_name_].element_gmsh_tag_.emplace_back(
        this->element_(i).gmsh_tag_);
    physical_group_information[this->element_(i).gmsh_physical_name_].basic_node_number_ +=
        ElementTrait::kBasicNodeNumber;
    physical_group_information[this->element_(i).gmsh_physical_name_].all_node_number_ += ElementTrait::kAllNodeNumber;
    gmsh_tag_to_element_information[this->element_(i).gmsh_tag_].element_index_ = i;
    for (Isize j = 0; j < ElementTrait::kBasicNodeNumber; j++) {
      const auto node_tag = static_cast<Isize>(node_tags[static_cast<Usize>(i * ElementTrait::kAllNodeNumber + j)]);
      physical_group_information[this->element_(i).gmsh_physical_name_].basic_node_gmsh_tag_.emplace_back(node_tag);
    }
    for (Isize j = 0; j < ElementTrait::kAllNodeNumber; j++) {
      const auto node_tag = static_cast<Isize>(node_tags[static_cast<Usize>(i * ElementTrait::kAllNodeNumber + j)]);
      this->element_(i).node_coordinate_.col(j) = node_coordinate.col(node_tag - 1);
      this->element_(i).node_tag_(j) = node_tag;
      physical_group_information[this->element_(i).gmsh_physical_name_].all_node_gmsh_tag_.emplace_back(node_tag);
      node_element_number(node_tag - 1)++;
    }
  }
  this->getElementJacobian();
  this->calculateElementLocalMassMatrixInverse();
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_ELEMENT_HPP_
