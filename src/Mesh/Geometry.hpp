/**
 * @file Geometry.hpp
 * @brief The header file of SubrosaDG Geometry.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-12-17
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_GEOMETRY_HPP_
#define SUBROSA_DG_GEOMETRY_HPP_

#include <gmsh.h>

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <cmath>
#include <compare>
#include <cstddef>
#include <vector>

#include "Mesh/ReadControl.hpp"
#include "Solver/SimulationControl.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Constant.hpp"
#include "Utils/Enum.hpp"

namespace SubrosaDG {

template <typename ElementTrait>
inline void ElementMesh<ElementTrait>::getElementJacobian() {
  for (Isize i = 0; i < this->number_; i++) {
    std::vector<double> jacobians;
    std::vector<double> determinants;
    std::vector<double> coord;
    gmsh::model::mesh::getJacobian(static_cast<std::size_t>(this->element_(i).gmsh_tag_),
                                   this->gaussian_quadrature_.local_coord_, jacobians, determinants, coord);
    for (Isize j = 0; j < ElementTrait::kQuadratureNumber; j++) {
      Eigen::Matrix<Real, ElementTrait::kDimension, ElementTrait::kDimension> jacobian_transpose;
      for (Isize k = 0; k < ElementTrait::kDimension; k++) {
        this->element_(i).gaussian_quadrature_node_coordinate_(k, j) =
            static_cast<Real>(coord[static_cast<Usize>(j * 3 + k)]);
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
                                   this->gaussian_quadrature_.local_coord_, jacobians, determinants, coord);
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
          (this->gaussian_quadrature_.weight_.array() * this->element_(i).jacobian_determinant_.array()))
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
    const MeshInformation& information,
    const AdjacencyElementMesh<AdjacencyLineTrait<ElementTrait::kPolynomialOrder>>& line) {
  for (Isize i = 0; i < this->number_; i++) {
    const std::vector<PerAdjacencyElementInformation>& sub_index_and_type =
        information.gmsh_tag_to_sub_index_and_type_.at(this->element_(i).gmsh_tag_);
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
  requires(AdjacencyElementTrait::kElementType == ElementEnum::Point)
inline void calculateNormVec(const Isize adjacency_sequence_in_parent, Eigen::Matrix<Real, 1, 1>& transition_matrix) {
  if (adjacency_sequence_in_parent == 0) {
    transition_matrix(0) = -1.0;
  } else if (adjacency_sequence_in_parent == 1) {
    transition_matrix(0) = 1.0;
  }
}

template <typename AdjacencyElementTrait>
  requires(AdjacencyElementTrait::kElementType == ElementEnum::Line)
inline void calculateNormVec(const Eigen::Matrix<Real, 2, AdjacencyElementTrait::kAllNodeNumber>& node_coordinate,
                             Eigen::Matrix<Real, 2, 2>& transition_matrix) {
  const Eigen::Rotation2D<Real> rotation{-kPi / 2.0};
  const Eigen::Vector<Real, 2> normal_vector =
      rotation * (node_coordinate.col(1) - node_coordinate.col(0)).normalized();
  transition_matrix.col(0) = normal_vector;
  transition_matrix.col(1) << -normal_vector.y(), normal_vector.x();
}

template <typename AdjacencyElementTrait>
  requires(AdjacencyElementTrait::kElementType == ElementEnum::Triangle)
inline void calculateNormVec(const Eigen::Matrix<Real, 3, AdjacencyElementTrait::kAllNodeNumber>& node_coordinate,
                             Eigen::Matrix<Real, 3, 3>& transition_matrix) {
  // NOTE: this normal vector is just for show, it is not used in the calculation
  const Eigen::Vector<Real, 3> normal_vector = (node_coordinate.col(1) - node_coordinate.col(0)).normalized();
  // transition from (1, 0, 0) to normal_vector, calculated by Mathematica
  // assuming normal_vector = {nx, ny, nz} is normalized and ny^2 + nz^2 != 0
  // \begin{BNiceMatrix}
  //   nx&-ny                                    &-nz\\
  //   ny&\dfrac{nx ny^{2}+nz^{2}}{ny^{2}+nz^{2}}&\dfrac{(nx-1) ny nz}{ny^{2}+nz^{2}}\\
  //   nz&\dfrac{(nx-1) ny nz}{ny^{2}+nz^{2}}    &\dfrac{nx nz^{2}+ny^{2}}{ny^{2}+nz^{2}}\\
  // \end{BNiceMatrix}
  const Real ny2addnz2 = std::pow(normal_vector.y(), 2) + std::pow(normal_vector.z(), 2);
  if ((ny2addnz2 <=> kRealMin) == std::partial_ordering::less) [[unlikely]] {
    transition_matrix.setIdentity();
  } else {
    transition_matrix.col(0) = normal_vector;
    transition_matrix.col(1) << -normal_vector.y(),
        (normal_vector.x() * std::pow(normal_vector.y(), 2) + std::pow(normal_vector.z(), 2)) / ny2addnz2,
        ((normal_vector.x() - 1) * normal_vector.y() * normal_vector.z()) / ny2addnz2;
    transition_matrix.col(2) << -normal_vector.z(),
        ((normal_vector.x() - 1) * normal_vector.y() * normal_vector.z()) / ny2addnz2,
        (normal_vector.x() * std::pow(normal_vector.z(), 2) + std::pow(normal_vector.y(), 2)) / ny2addnz2;
  }
}

template <typename AdjacencyElementTrait>
inline void AdjacencyElementMesh<AdjacencyElementTrait>::calculateAdjacencyElementTransitionMatrix() {
  for (Isize i = 0; i < this->interior_number_ + this->boundary_number_; i++) {
    if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
      calculateNormVec<AdjacencyElementTrait>(this->element_(i).adjacency_sequence_in_parent_(0),
                                              this->element_(i).transition_matrix_);
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
      calculateNormVec<AdjacencyElementTrait>(this->element_(i).node_coordinate_, this->element_(i).transition_matrix_);
    }
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GEOMETRY_HPP_
