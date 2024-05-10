/**
 * @file Geometry.hpp
 * @brief The header file of SubrosaDG Geometry.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-12-17
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_GEOMETRY_HPP_
#define SUBROSA_DG_GEOMETRY_HPP_

#include <gmsh.h>

#include <Eigen/Core>
#include <Eigen/LU>  // IWYU pragma: keep
#include <cstddef>
#include <vector>

#include "Mesh/ReadControl.hpp"
#include "Solver/SimulationControl.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Enum.hpp"

namespace SubrosaDG {

template <typename ElementTrait>
inline void ElementMesh<ElementTrait>::getElementJacobian() {
  for (Isize i = 0; i < this->number_; i++) {
    std::vector<double> jacobians;
    std::vector<double> determinants;
    std::vector<double> coord;
    gmsh::model::mesh::getJacobian(static_cast<std::size_t>(this->element_(i).gmsh_tag_),
                                   this->quadrature_.local_coord_, jacobians, determinants, coord);
    for (Isize j = 0; j < ElementTrait::kQuadratureNumber; j++) {
      Eigen::Matrix<Real, ElementTrait::kDimension, ElementTrait::kDimension> jacobian_transpose;
      for (Isize k = 0; k < ElementTrait::kDimension; k++) {
        this->element_(i).quadrature_node_coordinate_(k, j) = static_cast<Real>(coord[static_cast<Usize>(j * 3 + k)]);
        for (Isize l = 0; l < ElementTrait::kDimension; l++) {
          jacobian_transpose(k, l) = static_cast<Real>(jacobians[static_cast<Usize>(j * 9 + k * 3 + l)]);
        }
      }
      this->element_(i).jacobian_determinant_(j) = static_cast<Real>(determinants[static_cast<Usize>(j)]);
      this->element_(i).jacobian_transpose_inverse_.col(j) = jacobian_transpose.inverse().reshaped();
    }
  }
}

template <typename AdjacencyElementTrait>
inline void AdjacencyElementMesh<AdjacencyElementTrait>::getAdjacencyElementJacobian() {
  for (Isize i = 0; i < this->interior_number_ + this->boundary_number_; i++) {
    std::vector<double> jacobians;
    std::vector<double> determinants;
    std::vector<double> coord;
    gmsh::model::mesh::getJacobian(static_cast<std::size_t>(this->element_(i).gmsh_tag_),
                                   this->quadrature_.local_coord_, jacobians, determinants, coord);
    for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
      this->element_(i).jacobian_determinant_(j) = static_cast<Real>(determinants[static_cast<Usize>(j)]);
    }
  }
}

template <typename ElementTrait>
inline void ElementMesh<ElementTrait>::calculateElementLocalMassMatrixInverse() {
  for (Isize i = 0; i < this->number_; i++) {
    this->element_(i).local_mass_matrix_inverse_.noalias() =
        (this->basis_function_.value_.transpose() *
         (this->basis_function_.value_.array().colwise() *
          (this->quadrature_.weight_.array() * this->element_(i).jacobian_determinant_.array()))
             .matrix())
            .inverse();
  }
}

template <typename ElementTrait>
inline void ElementMesh<ElementTrait>::calculateElementMeshSize(
    const MeshInformation& information,
    const AdjacencyElementMesh<AdjacencyPointTrait<ElementTrait::kPolynomialOrder>>& point) {
  for (Isize i = 0; i < this->number_; i++) {
    const std::vector<PerAdjacencyElementInformation>& sub_index_and_type =
        information.gmsh_tag_to_sub_index_and_type_.at(this->element_(i).gmsh_tag_);
    Real adjacency_size = 0.0;
    for (Usize j = 0; j < ElementTrait::kAdjacencyNumber; j++) {
      adjacency_size += point.element_(sub_index_and_type[j].element_index_).jacobian_determinant_.transpose() *
                        point.quadrature_.weight_;
    }
    this->element_(i).size_ = (this->element_(i).jacobian_determinant_.transpose() * this->quadrature_.weight_);
    this->element_(i).size_ /= (adjacency_size * (2.0 * static_cast<Real>(ElementTrait::kPolynomialOrder) + 1.0));
  }
}

template <typename ElementTrait>
inline void ElementMesh<ElementTrait>::calculateElementMeshSize(
    const MeshInformation& information,
    const AdjacencyElementMesh<AdjacencyLineTrait<ElementTrait::kPolynomialOrder>>& line) {
  for (Isize i = 0; i < this->number_; i++) {
    const std::vector<PerAdjacencyElementInformation>& sub_index_and_type =
        information.gmsh_tag_to_sub_index_and_type_.at(this->element_(i).gmsh_tag_);
    Real adjacency_size = 0.0;
    for (Usize j = 0; j < ElementTrait::kAdjacencyNumber; j++) {
      adjacency_size += line.element_(sub_index_and_type[j].element_index_).jacobian_determinant_.transpose() *
                        line.quadrature_.weight_;
    }
    this->element_(i).size_ = (this->element_(i).jacobian_determinant_.transpose() * this->quadrature_.weight_);
    this->element_(i).size_ /= (adjacency_size * (2.0 * static_cast<Real>(ElementTrait::kPolynomialOrder) + 1.0));
  }
}

template <typename AdjacencyElementTrait>
  requires(AdjacencyElementTrait::kElementType == ElementEnum::Point)
inline void calculateNormalVector(const Isize adjacency_sequence_in_parent,
                                  Eigen::Matrix<Real, 1, AdjacencyElementTrait::kQuadratureNumber>& normal_vector) {
  if (adjacency_sequence_in_parent == 0) {
    normal_vector(0, 0) = -1.0;
  } else if (adjacency_sequence_in_parent == 1) {
    normal_vector(0, 0) = 1.0;
  }
}

template <typename AdjacencyElementTrait>
  requires(AdjacencyElementTrait::kElementType == ElementEnum::Line)
inline void calculateNormalVector(
    const Eigen::Matrix<Real, 2, AdjacencyElementTrait::kAllNodeNumber>& node_coordinate,
    const Eigen::Matrix<Real, AdjacencyElementTrait::kQuadratureNumber * AdjacencyElementTrait::kDimension,
                        AdjacencyElementTrait::kBasisFunctionNumber>& gradient_value,
    Eigen::Matrix<Real, 2, AdjacencyElementTrait::kQuadratureNumber>& normal_vector) {
  for (Isize i = 0; i < AdjacencyElementTrait::kQuadratureNumber; i++) {
    normal_vector(0, i) = gradient_value.row(i) * node_coordinate.row(1).transpose();
    normal_vector(1, i) = -gradient_value.row(i) * node_coordinate.row(0).transpose();
    normal_vector.col(i).normalize();
  }
}

template <typename AdjacencyElementTrait>
inline void AdjacencyElementMesh<AdjacencyElementTrait>::calculateAdjacencyElementNormalVector() {
  for (Isize i = 0; i < this->interior_number_ + this->boundary_number_; i++) {
    if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
      calculateNormalVector<AdjacencyElementTrait>(this->element_(i).adjacency_sequence_in_parent_(0),
                                                   this->element_(i).normal_vector_);
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
      calculateNormalVector<AdjacencyElementTrait>(
          this->element_(i).node_coordinate_, this->basis_function_.gradient_value_, this->element_(i).normal_vector_);
    }
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GEOMETRY_HPP_
