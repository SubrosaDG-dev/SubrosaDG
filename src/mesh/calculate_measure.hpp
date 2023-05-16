/**
 * @file calculate_measure.hpp
 * @brief The header file to calculate element measure.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-06
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CALCULATE_MEASURE_HPP_
#define SUBROSA_DG_CALCULATE_MEASURE_HPP_

// clang-format off

#include <Eigen/Core>              // for DenseBase::col, Vector, DenseBase::reshaped, Dynamic, Matrix
#include <Eigen/Geometry>          // for MatrixBase::cross
#include <memory>                  // for make_unique, unique_ptr

#include "basic/data_types.hpp"    // for Real, Isize
#include "basic/concepts.hpp"      // for Is2dElement
#include "mesh/element_types.hpp"  // for ElementType

// clang-format on

namespace SubrosaDG {

template <ElementType Type>
struct ElementMeshNoIndex;

template <ElementType Type>
  requires Is2dElement<Type>
inline Real calculatePolygonArea(const Eigen::Matrix<Real, 3, Type.kNodesNumPerElement>& nodes) {
  Eigen::Vector<Real, 3> cross_product = nodes.col(Type.kNodesNumPerElement - 1).cross(nodes.col(0));
  for (Isize i = 0; i < Type.kNodesNumPerElement - 1; i++) {
    cross_product += nodes.col(i).cross(nodes.col(i + 1));
  }
  return 0.5 * cross_product.norm();
}

template <ElementType Type>
  requires Is2dElement<Type>
inline std::unique_ptr<Eigen::Vector<Real, Eigen::Dynamic>> calculateElementMeasure(
    const ElementMeshNoIndex<Type>& element_mesh_no_index) {
  auto area = std::make_unique<Eigen::Vector<Real, Eigen::Dynamic>>(element_mesh_no_index.elements_num_);
  Eigen::Matrix<Real, 3, Type.kNodesNumPerElement> nodes;
  for (Isize i = 0; i < element_mesh_no_index.elements_num_; i++) {
    nodes = element_mesh_no_index.elements_nodes_.col(i).reshaped(3, Type.kNodesNumPerElement);
    area->operator()(i) = calculatePolygonArea<Type>(nodes);
  }
  return area;
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CALCULATE_MEASURE_HPP_
