/**
 * @file Geometry.cpp
 * @brief The header file of SubrosaDG Geometry.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-12-17
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2025 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_GEOMETRY_CPP_
#define SUBROSA_DG_GEOMETRY_CPP_

#include <gmsh.h>

#include <Eigen/Core>
#include <Eigen/LU>
#include <cstddef>
#include <vector>

#include "Mesh/ReadControl.cpp"
#include "Utils/BasicDataType.cpp"
#include "Utils/Concept.cpp"

namespace SubrosaDG {

template <typename ElementTrait>
inline void ElementMesh<ElementTrait>::getElementQuality() {
#pragma omp parallel for default(none) schedule(nonmonotonic : auto) shared(Eigen::Dynamic)
  for (Isize i = 0; i < this->number_; i++) {
    std::vector<double> element_min_edge;
    std::vector<double> element_inner_radius;
    gmsh::model::mesh::getElementQualities({static_cast<std::size_t>(this->element_(i).gmsh_tag_)}, element_min_edge,
                                           "minEdge");
    gmsh::model::mesh::getElementQualities({static_cast<std::size_t>(this->element_(i).gmsh_tag_)},
                                           element_inner_radius, "innerRadius");
    this->element_(i).minimum_edge_ = static_cast<Real>(element_min_edge[0]);
    this->element_(i).inner_radius_ = static_cast<Real>(element_inner_radius[0]);
  }
}

template <typename ElementTrait>
inline void ElementMesh<ElementTrait>::getElementJacobian() {
#pragma omp parallel for default(none) schedule(nonmonotonic : auto) shared(Eigen::Dynamic)
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
      this->element_(i).jacobian_determinant_mutiply_weight_(j) =
          static_cast<Real>(determinants[static_cast<Usize>(j)]) * this->quadrature_.weight_(j);
      this->element_(i).jacobian_transpose_inverse_mutiply_deteminate_and_weight_.col(j) =
          jacobian_transpose.inverse().reshaped() * this->element_(i).jacobian_determinant_mutiply_weight_(j);
    }
  }
}

template <typename AdjacencyElementTrait>
inline void AdjacencyElementMesh<AdjacencyElementTrait>::getAdjacencyElementJacobian() {
#pragma omp parallel for default(none) schedule(nonmonotonic : auto) shared(Eigen::Dynamic)
  for (Isize i = 0; i < this->interior_number_ + this->boundary_number_; i++) {
    std::vector<double> jacobians;
    std::vector<double> determinants;
    std::vector<double> coord;
    gmsh::model::mesh::getJacobian(static_cast<std::size_t>(this->element_(i).gmsh_tag_),
                                   this->quadrature_.local_coord_, jacobians, determinants, coord);
    for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
      for (Isize k = 0; k < AdjacencyElementTrait::kDimension + 1; k++) {
        this->element_(i).quadrature_node_coordinate_(k, j) = static_cast<Real>(coord[static_cast<Usize>(j * 3 + k)]);
      }
      this->element_(i).jacobian_determinant_mutiply_weight_(j) =
          static_cast<Real>(determinants[static_cast<Usize>(j)]) * this->quadrature_.weight_(j);
    }
  }
}

template <typename ElementTrait>
inline void ElementMesh<ElementTrait>::calculateElementLocalMassMatrixInverse() {
  tbb::parallel_for(tbb::blocked_range<Isize>(0, this->number_), [&](const tbb::blocked_range<Isize>& range) {
    for (Isize i = range.begin(); i != range.end(); i++) {
      this->element_(i).local_mass_matrix_inverse_.noalias() =
          (this->basis_function_.modal_value_.transpose() *
           (this->basis_function_.modal_value_.array().colwise() *
            this->element_(i).jacobian_determinant_mutiply_weight_.array())
               .matrix())
              .inverse();
    }
  });
}

template <typename AdjacencyElementTrait>
  requires Is0dElement<AdjacencyElementTrait::kElementType>
inline void calculateNormalVector(const Isize adjacency_sequence_in_parent,
                                  Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1,
                                                AdjacencyElementTrait::kQuadratureNumber>& normal_vector) {
  if (adjacency_sequence_in_parent == 0) {
    normal_vector(0, 0) = -1.0_r;
  } else if (adjacency_sequence_in_parent == 1) {
    normal_vector(0, 0) = 1.0_r;
  }
}

template <typename AdjacencyElementTrait>
  requires Is1dElement<AdjacencyElementTrait::kElementType>
inline void calculateNormalVector(
    const Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kAllNodeNumber>&
        node_coordinate,
    const Eigen::Array<
        Eigen::Matrix<Real, AdjacencyElementTrait::kQuadratureNumber, AdjacencyElementTrait::kBasisFunctionNumber>,
        AdjacencyElementTrait::kDimension, 1>& nodal_gradient_value,
    Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kQuadratureNumber>&
        normal_vector) {
  for (Isize i = 0; i < AdjacencyElementTrait::kQuadratureNumber; i++) {
    normal_vector(0, i) = nodal_gradient_value(0).row(i) * node_coordinate.row(1).transpose();
    normal_vector(1, i) = -nodal_gradient_value(0).row(i) * node_coordinate.row(0).transpose();
    normal_vector.col(i).normalize();
  }
}

template <typename AdjacencyElementTrait>
  requires Is2dElement<AdjacencyElementTrait::kElementType>
inline void calculateNormalVector(
    const Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kAllNodeNumber>&
        node_coordinate,
    const Eigen::Array<
        Eigen::Matrix<Real, AdjacencyElementTrait::kQuadratureNumber, AdjacencyElementTrait::kBasisFunctionNumber>,
        AdjacencyElementTrait::kDimension, 1>& nodal_gradient_value,
    Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kQuadratureNumber>&
        normal_vector) {
  for (Isize i = 0; i < AdjacencyElementTrait::kQuadratureNumber; i++) {
    const Eigen::Vector<Real, AdjacencyElementTrait::kDimension + 1> partial_xi_vector =
        nodal_gradient_value(0).row(i) * node_coordinate.transpose();
    const Eigen::Vector<Real, AdjacencyElementTrait::kDimension + 1> partial_eta_vector =
        nodal_gradient_value(1).row(i) * node_coordinate.transpose();
    normal_vector.col(i) = partial_xi_vector.cross(partial_eta_vector).normalized();
  }
}

template <typename AdjacencyElementTrait>
inline void AdjacencyElementMesh<AdjacencyElementTrait>::calculateAdjacencyElementNormalVector() {
  tbb::parallel_for(tbb::blocked_range<Isize>(0, this->interior_number_ + this->boundary_number_),
                    [&](const tbb::blocked_range<Isize>& range) {
                      for (Isize i = range.begin(); i != range.end(); i++) {
                        if constexpr (Is0dElement<AdjacencyElementTrait::kElementType>) {
                          calculateNormalVector<AdjacencyElementTrait>(
                              this->element_(i).adjacency_sequence_in_parent_(0), this->element_(i).normal_vector_);
                        } else if constexpr (Is1dElement<AdjacencyElementTrait::kElementType>) {
                          calculateNormalVector<AdjacencyElementTrait>(this->element_(i).node_coordinate_,
                                                                       this->basis_function_.nodal_gradient_value_,
                                                                       this->element_(i).normal_vector_);
                        } else if constexpr (Is2dElement<AdjacencyElementTrait::kElementType>) {
                          calculateNormalVector<AdjacencyElementTrait>(this->element_(i).node_coordinate_,
                                                                       this->basis_function_.nodal_gradient_value_,
                                                                       this->element_(i).normal_vector_);
                        }
                      }
                    });
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GEOMETRY_CPP_
