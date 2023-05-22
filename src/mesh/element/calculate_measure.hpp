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

#include <Eigen/Core>              // for Vector, DenseBase::col, Dynamic, Matrix, DenseBase::block, DenseBase::resh...
#include <memory>                  // for unique_ptr, make_unique
#include <Eigen/Geometry>          // for MatrixBase::cross

#include "basic/concepts.hpp"      // for Is1dElement, Is2dElement  // IWYU pragma: keep
#include "basic/data_types.hpp"    // for Real, Isize
#include "mesh/element_types.hpp"  // for ElementType

// clang-format on

namespace SubrosaDG {

template <Isize Dimension, ElementType Type>
struct ElementMesh;
template <Isize Dimension, ElementType Type>
struct AdjacencyElementMesh;

template <Isize Dimension, ElementType Type>
struct FElementMeasure {};

template <Isize Dimension, ElementType Type>
  requires Is1dElement<Type>
struct FElementMeasure<Dimension, Type> {
  inline static std::unique_ptr<Eigen::Vector<Real, Eigen::Dynamic>> calculateBase(
      Isize elements_num,
      const Eigen::Matrix<Real, Dimension * Type.kNodesNumPerElement, Eigen::Dynamic>& elements_nodes) {
    auto measure = std::make_unique<Eigen::Vector<Real, Eigen::Dynamic>>(elements_num);
    Eigen::Matrix<Real, 3, Type.kNodesNumPerElement> nodes = Eigen::Matrix<Real, 3, Type.kNodesNumPerElement>::Zero();
    for (Isize i = 0; i < elements_num; i++) {
      nodes.template block<Dimension, Type.kNodesNumPerElement>(0, 0) =
          elements_nodes.col(i).reshaped(Dimension, Type.kNodesNumPerElement);
      measure->operator()(i) = (nodes.col(1) - nodes.col(0)).norm();
    }
    return measure;
  };

  inline static std::unique_ptr<Eigen::Vector<Real, Eigen::Dynamic>> calculate(
      const ElementMesh<Dimension, Type>& element_mesh) {
    return calculateBase(element_mesh.elements_num_, element_mesh.elements_nodes_);
  };

  inline static std::unique_ptr<Eigen::Vector<Real, Eigen::Dynamic>> calculate(
      const AdjacencyElementMesh<Dimension, Type>& Adjacency_element_mesh) {
    return calculateBase(Adjacency_element_mesh.elements_num_.second, Adjacency_element_mesh.elements_nodes_);
  };
};

template <Isize Dimension, ElementType Type>
  requires Is2dElement<Type>
struct FElementMeasure<Dimension, Type> {
  inline static std::unique_ptr<Eigen::Vector<Real, Eigen::Dynamic>> calculateBase(
      Isize elements_num,
      const Eigen::Matrix<Real, Dimension * Type.kNodesNumPerElement, Eigen::Dynamic>& elements_nodes) {
    auto measure = std::make_unique<Eigen::Vector<Real, Eigen::Dynamic>>(elements_num);
    Eigen::Matrix<Real, 3, Type.kNodesNumPerElement> nodes = Eigen::Matrix<Real, 3, Type.kNodesNumPerElement>::Zero();
    for (Isize i = 0; i < elements_num; i++) {
      nodes.template block<Dimension, Type.kNodesNumPerElement>(0, 0) =
          elements_nodes.col(i).reshaped(Dimension, Type.kNodesNumPerElement);
      Eigen::Vector<Real, 3> cross_product = nodes.col(Type.kNodesNumPerElement - 1).cross(nodes.col(0));
      for (Isize j = 0; j < Type.kNodesNumPerElement - 1; j++) {
        cross_product += nodes.col(j).cross(nodes.col(j + 1));
      }
      measure->operator()(i) = 0.5 * cross_product.norm();
    }
    return measure;
  };

  inline static std::unique_ptr<Eigen::Vector<Real, Eigen::Dynamic>> calculate(
      const ElementMesh<Dimension, Type>& element_mesh) {
    return calculateBase(element_mesh.elements_num_, element_mesh.elements_nodes_);
  };

  inline static std::unique_ptr<Eigen::Vector<Real, Eigen::Dynamic>> calculate(
      const AdjacencyElementMesh<Dimension, Type>& Adjacency_element_mesh) {
    return calculateBase(Adjacency_element_mesh.elements_num_.second, Adjacency_element_mesh.elements_nodes_);
  };
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CALCULATE_MEASURE_HPP_
