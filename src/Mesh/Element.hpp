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
    const Eigen::Matrix<Real, ElementTrait::kDimension, Eigen::Dynamic>& global_node_coordinate,
    const std::unordered_map<Isize, std::string>& global_gmsh_physical_name,
    std::unordered_map<Isize, Isize>& global_gmsh_tag) {
  std::vector<std::size_t> element_tags;
  std::vector<std::size_t> node_tags;
  gmsh::model::mesh::getElementsByType(ElementTrait::kGmshTypeNumber, element_tags, node_tags);
  this->number_ = static_cast<Isize>(element_tags.size());
  this->element_.resize(this->number_);
  for (Isize i = 0; i < this->number_; i++) {
    this->element_(i).gmsh_tag_ = static_cast<Isize>(element_tags[static_cast<Usize>(i)]);
    global_gmsh_tag[static_cast<Isize>(element_tags[static_cast<Usize>(i)])] = i;
    this->element_(i).gmsh_physical_name_ = global_gmsh_physical_name.at(this->element_(i).gmsh_tag_);
    for (Isize j = 0; j < ElementTrait::kAllNodeNumber; j++) {
      const auto node_tag = static_cast<Isize>(node_tags[static_cast<Usize>(i * ElementTrait::kAllNodeNumber + j)]);
      this->element_(i).node_coordinate_.col(j) = global_node_coordinate.col(node_tag - 1);
      this->element_(i).node_tag_(j) = node_tag;
    }
  }
  this->getElementJacobian();
  this->calculateElementProjectionMeasure();
  this->calculateElementLocalMassMatrixInverse();
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_ELEMENT_HPP_
