/**
 * @file cal_mesh_measure.cpp
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

#include "mesh/cal_mesh_measure.h"

#include <Eigen/Geometry>         // for MatrixBase::cross
#include <memory>                 // for make_unique, unique_ptr
#include <utility>                // for pair

#include "mesh/mesh_structure.h"  // for Element

// clang-format on

namespace SubrosaDG::Internal {

void calculateMeshMeasure(Element& element) {
  element.element_area_ = std::make_unique<Eigen::Vector<Real, Eigen::Dynamic>>(element.element_num_.second -
                                                                                element.element_num_.first + 1);
  Eigen::Matrix<Real, 3, Eigen::Dynamic> nodes;
  nodes.resize(3, element.element_type_info_.second);
  for (Isize i = element.element_num_.first; i < element.element_num_.second; i++) {
    nodes = element.element_nodes_->col(i - element.element_num_.first).reshaped(3, element.element_type_info_.second);
    element.element_area_->operator()(i - element.element_num_.first) = calculatePolygonArea(nodes);
  }
}

Real calculatePolygonArea(Eigen::Matrix<Real, 3, Eigen::Dynamic>& nodes) {
  Eigen::Vector<Real, 3> cross_product = nodes.col(0).cross(nodes.col(nodes.cols() - 1));
  for (Isize i = 0; i < nodes.cols() - 1; i++) {
    cross_product += nodes.col(i).cross(nodes.col(i + 1));
  }
  return 0.5 * cross_product.norm();
}

}  // namespace SubrosaDG::Internal
