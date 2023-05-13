/**
 * @file calculate_measure.cpp
 * @brief The source file to calculate mesh measure.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-06
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

// clang-format off

#include "mesh/calculate_measure.h"

#include <Eigen/Geometry>         // for MatrixBase::cross
#include <memory>                 // for make_unique, unique_ptr

#include "mesh/mesh_structure.h"  // for Element

// clang-format on

namespace SubrosaDG::Internal {

std::unique_ptr<Eigen::Vector<Real, Eigen::Dynamic>> calculateElementMeasure(const Element& element) {
  auto area = std::make_unique<Eigen::Vector<Real, Eigen::Dynamic>>(element.elements_num_);
  Eigen::Matrix<Real, 3, Eigen::Dynamic> nodes;
  nodes.resize(3, element.nodes_num_per_element_);
  for (Isize i = 0; i < element.elements_num_; i++) {
    nodes = element.elements_nodes_.col(i).reshaped(3, element.nodes_num_per_element_);
    area->operator()(i) = calculatePolygonArea(nodes);
  }
  return area;
}

Real calculatePolygonArea(const Eigen::Matrix<Real, 3, Eigen::Dynamic>& nodes) {
  Eigen::Vector<Real, 3> cross_product = nodes.col(nodes.cols() - 1).cross(nodes.col(0));
  for (Isize i = 0; i < nodes.cols() - 1; i++) {
    cross_product += nodes.col(i).cross(nodes.col(i + 1));
  }
  return 0.5 * cross_product.norm();
}

}  // namespace SubrosaDG::Internal
