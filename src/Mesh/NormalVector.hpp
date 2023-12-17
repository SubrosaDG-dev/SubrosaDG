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
#include <cmath>
#include <unordered_map>
#include <vector>

#include "Mesh/ReadControl.hpp"
#include "Solver/SimulationControl.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Constant.hpp"
#include "Utils/Enum.hpp"

namespace SubrosaDG {

template <typename ElementTrait>
inline void ElementMesh<ElementTrait>::calculateElementMeshSize(
    const std::unordered_map<Isize, std::vector<PerAdjacencyElementInformation>>& gmsh_tag_to_sub_index_and_type,
    const AdjacencyElementMesh<AdjacencyPointTrait<ElementTrait::kPolynomialOrder>>& point) {
  for (Isize i = 0; i < this->number_; i++) {
    const std::vector<PerAdjacencyElementInformation>& sub_index_and_type =
        gmsh_tag_to_sub_index_and_type.at(this->element_(i).gmsh_tag_);
    Real adjacency_size = 0;
    for (Usize j = 0; j < ElementTrait::kAdjacencyNumber; j++) {
      adjacency_size += point.element_(sub_index_and_type[j].element_index_).jacobian_determinant_.transpose() *
                        point.gaussian_quadrature_.weight_;
    }
    this->element_(i).size_ =
        (this->element_(i).jacobian_determinant_.transpose() * this->gaussian_quadrature_.weight_);
    this->element_(i).size_ /=
        (adjacency_size * std::pow((static_cast<Real>(ElementTrait::kPolynomialOrder) + 1.0), 2.0));
  }
}

template <typename ElementTrait>
inline void ElementMesh<ElementTrait>::calculateElementMeshSize(
    const std::unordered_map<Isize, std::vector<PerAdjacencyElementInformation>>& gmsh_tag_to_sub_index_and_type,
    const AdjacencyElementMesh<AdjacencyLineTrait<ElementTrait::kPolynomialOrder>>& line) {
  for (Isize i = 0; i < this->number_; i++) {
    const std::vector<PerAdjacencyElementInformation>& sub_index_and_type =
        gmsh_tag_to_sub_index_and_type.at(this->element_(i).gmsh_tag_);
    Real adjacency_size = 0;
    for (Usize j = 0; j < ElementTrait::kAdjacencyNumber; j++) {
      adjacency_size += line.element_(sub_index_and_type[j].element_index_).jacobian_determinant_.transpose() *
                        line.gaussian_quadrature_.weight_;
    }
    this->element_(i).size_ =
        (this->element_(i).jacobian_determinant_.transpose() * this->gaussian_quadrature_.weight_);
    this->element_(i).size_ /=
        (adjacency_size * std::pow((static_cast<Real>(ElementTrait::kPolynomialOrder) + 1.0), 2.0));
  }
}

template <typename AdjacencyElementTrait>
  requires(AdjacencyElementTrait::kElementType == Element::Point)
inline void calculateNormVec(const Isize adjacency_sequence_in_parent, Eigen::Vector<Real, 1>& normal_vector) {
  if (adjacency_sequence_in_parent == 0) {
    normal_vector(0) = -1.0;
  } else if (adjacency_sequence_in_parent == 1) {
    normal_vector(0) = 1.0;
  }
}

template <typename AdjacencyElementTrait>
  requires(AdjacencyElementTrait::kElementType == Element::Line)
inline void calculateNormVec(const Eigen::Matrix<Real, 2, AdjacencyElementTrait::kAllNodeNumber>& node_coordinate,
                             Eigen::Vector<Real, 2>& normal_vector) {
  Eigen::Rotation2D<Real> rotation{-kPi / 2.0};
  normal_vector = rotation * (node_coordinate.col(1) - node_coordinate.col(0)).normalized();
}

template <typename AdjacencyElementTrait>
inline void AdjacencyElementMesh<AdjacencyElementTrait>::calculateAdjacencyElementNormalVector() {
  for (Isize i = 0; i < this->interior_number_ + this->boundary_number_; i++) {
    if constexpr (AdjacencyElementTrait::kElementType == Element::Point) {
      calculateNormVec<AdjacencyElementTrait>(this->element_(i).adjacency_sequence_in_parent_(0),
                                              this->element_(i).normal_vector_);
    } else if constexpr (AdjacencyElementTrait::kElementType == Element::Line) {
      calculateNormVec<AdjacencyElementTrait>(this->element_(i).node_coordinate_, this->element_(i).normal_vector_);
    }
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_NORMAL_VECTOR_HPP_
