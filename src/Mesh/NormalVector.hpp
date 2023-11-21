/**
 * @file NormalVector.hpp
 * @brief The header file of SubrosaDG NormalVector.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-14
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_NORMAL_VECTOR_HPP_
#define SUBROSA_DG_NORMAL_VECTOR_HPP_

#include <Eigen/Core>
#include <Eigen/Geometry>

#include "Mesh/ReadControl.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Constant.hpp"

namespace SubrosaDG {

template <typename ElementTrait>
inline void ElementMesh<ElementTrait>::calculateElementProjectionMeasure() {
  for (Isize i = 0; i < this->number_; i++) {
    this->element_(i).projection_measure_ = this->element_(i).node_coordinate_.rowwise().maxCoeff() -
                                            this->element_(i).node_coordinate_.rowwise().minCoeff();
  }
}

template <typename AdjacencyElementTrait>
inline void calculateNormVec(const Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1,
                                                 AdjacencyElementTrait::kAllNodeNumber>& node_coordinate,
                             Eigen::Vector<Real, AdjacencyElementTrait::kDimension + 1>& normal_vector) {
  Eigen::Rotation2D<Real> rotation{-kPi / 2.0};
  normal_vector = rotation * (node_coordinate.col(1) - node_coordinate.col(0)).normalized();
}

template <typename AdjacencyElementTrait>
inline void AdjacencyElementMesh<AdjacencyElementTrait>::calculateAdjacencyElementNormalVector() {
  for (Isize i = 0; i < this->interior_number_ + this->boundary_number_; i++) {
    calculateNormVec<AdjacencyElementTrait>(this->element_(i).node_coordinate_, this->element_(i).normal_vector_);
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_NORMAL_VECTOR_HPP_
