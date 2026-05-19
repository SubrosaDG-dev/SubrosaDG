/**
 * @file Geometry.cpp
 * @brief The header file of SubrosaDG Geometry.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-12-17
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
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

template <typename VolumeElementTrait>
inline void VolumeElementMesh<VolumeElementTrait>::getVolumeElementQuality() {
#pragma omp parallel for default(none) schedule(nonmonotonic : auto) shared(Eigen::Dynamic)
  for (Isize i = 0; i < this->number_; i++) {
    std::vector<double> element_min_edge;
    gmsh::model::mesh::getElementQualities({static_cast<std::size_t>(this->gmsh_tag_(i))}, element_min_edge, "minEdge");
    this->minimum_edge_(i) = static_cast<Real>(element_min_edge[0]);
  }
}

template <typename VolumeElementTrait>
inline void VolumeElementMesh<VolumeElementTrait>::computeVolumeElementQuadratureNodeCoordinate() {
  tbb::parallel_for(tbb::blocked_range<Isize>(0, this->number_), [&](const tbb::blocked_range<Isize>& range) -> void {
    for (Isize i = range.begin(); i != range.end(); i++) {
      for (Isize j = 0; j < VolumeElementTrait::kQuadratureNumber; j++) {
        this->quadrature_node_coordinate_(i).col(j) =
            this->node_coordinate_(i) * this->nodal_basis_function_.row(j).transpose();
      }
    }
  });
}

template <typename AdjacencyElementTrait>
inline void AdjacencyElementMesh<AdjacencyElementTrait>::computeAdjacencyElementQuadratureNodeCoordinate() {
  tbb::parallel_for(tbb::blocked_range<Isize>(0, this->number_), [&](const tbb::blocked_range<Isize>& range) -> void {
    for (Isize i = range.begin(); i != range.end(); i++) {
      for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
        this->quadrature_node_coordinate_(i).col(j) =
            this->node_coordinate_(i) * this->nodal_basis_function_.row(j).transpose();
      }
    }
  });
}

template <typename VolumeElementTrait>
inline void VolumeElementMesh<VolumeElementTrait>::computeVolumeElementJacobian() {
  tbb::parallel_for(tbb::blocked_range<Isize>(0, this->number_), [&](const tbb::blocked_range<Isize>& range) -> void {
    for (Isize i = range.begin(); i != range.end(); i++) {
      Eigen::Matrix<Real, VolumeElementTrait::kDimension, VolumeElementTrait::kDimension> jacobian;
      Real jacobian_determinant;
      Eigen::Matrix<Real, VolumeElementTrait::kDimension, VolumeElementTrait::kDimension>
          jacobian_inverse_multiply_determinate;
      for (Isize j = 0; j < VolumeElementTrait::kQuadratureNumber; j++) {
        jacobian.noalias() =
            this->node_coordinate_(i) *
            this->nodal_gradient_basis_function_(
                    Eigen::seqN(j * VolumeElementTrait::kDimension, Eigen::fix<VolumeElementTrait::kDimension>),
                    Eigen::placeholders::all)
                .transpose();
        if constexpr (VolumeElementTrait::kDimension == 1) {
          jacobian_determinant = jacobian(0, 0);
          jacobian_inverse_multiply_determinate(0, 0) = 1.0_r;
        } else if constexpr (VolumeElementTrait::kDimension == 2) {
          jacobian_determinant = jacobian(0, 0) * jacobian(1, 1) - jacobian(0, 1) * jacobian(1, 0);
          jacobian_inverse_multiply_determinate(0, 0) = jacobian(1, 1);
          jacobian_inverse_multiply_determinate(0, 1) = -jacobian(0, 1);
          jacobian_inverse_multiply_determinate(1, 0) = -jacobian(1, 0);
          jacobian_inverse_multiply_determinate(1, 1) = jacobian(0, 0);
        } else if constexpr (VolumeElementTrait::kDimension == 3) {
          // The inverse of the Jacobian matrix is computed by the cofactor method for better performance than the LU
          // decomposition.
          jacobian_inverse_multiply_determinate.row(0) = jacobian.col(1).cross(jacobian.col(2));
          jacobian_inverse_multiply_determinate.row(1) = jacobian.col(2).cross(jacobian.col(0));
          jacobian_inverse_multiply_determinate.row(2) = jacobian.col(0).cross(jacobian.col(1));
          jacobian_determinant = jacobian.col(0).dot(jacobian_inverse_multiply_determinate.row(0));
        }
        this->jacobian_determinant_multiply_weight_(i)(j) = jacobian_determinant * this->quadrature_weight_(j);
        this->jacobian_transpose_inverse_multiply_determinate_and_weight_(i)(
                Eigen::placeholders::all,
                Eigen::seqN(j * VolumeElementTrait::kDimension, Eigen::fix<VolumeElementTrait::kDimension>))
            .noalias() = jacobian_inverse_multiply_determinate.transpose() * this->quadrature_weight_(j);
      }
    }
  });
}

template <typename AdjacencyElementTrait>
inline void AdjacencyElementMesh<AdjacencyElementTrait>::computeAdjacencyElementJacobian() {
  tbb::parallel_for(tbb::blocked_range<Isize>(0, this->number_), [&](const tbb::blocked_range<Isize>& range) -> void {
    for (Isize i = range.begin(); i != range.end(); i++) {
      if constexpr (AdjacencyElementTrait::kDimension == 0) {
        this->jacobian_determinant_multiply_weight_(i)(0) = 1.0_r * this->quadrature_weight_(0);
        return;
      }
      Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kDimension> jacobian;
      Real jacobian_determinant;
      for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
        jacobian.noalias() =
            this->node_coordinate_(i) *
            this->nodal_gradient_basis_function_(
                    Eigen::seqN(j * AdjacencyElementTrait::kDimension, Eigen::fix<AdjacencyElementTrait::kDimension>),
                    Eigen::placeholders::all)
                .transpose();
        if constexpr (AdjacencyElementTrait::kDimension == 1) {
          jacobian_determinant = jacobian.norm();
        } else if constexpr (AdjacencyElementTrait::kDimension == 2) {
          jacobian_determinant = jacobian.col(0).cross(jacobian.col(1)).norm();
        }
        this->jacobian_determinant_multiply_weight_(i)(j) = jacobian_determinant * this->quadrature_weight_(j);
      }
    }
  });
}

template <typename VolumeElementTrait>
inline void VolumeElementMesh<VolumeElementTrait>::computeVolumeElementLocalMassMatrixInverse() {
  tbb::parallel_for(tbb::blocked_range<Isize>(0, this->number_), [&](const tbb::blocked_range<Isize>& range) -> void {
    for (Isize i = range.begin(); i != range.end(); i++) {
      this->local_mass_matrix_inverse_(i).noalias() =
          (this->nodal_basis_function_.transpose() *
           (this->nodal_basis_function_.array().colwise() * this->jacobian_determinant_multiply_weight_(i).array())
               .matrix())
              .inverse();
    }
  });
}

template <Is0dElement AdjacencyElementTrait>
inline void computeNormalVector(const Isize adjacency_sequence_in_parent,
                                Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1,
                                              AdjacencyElementTrait::kQuadratureNumber>& normal_vector) {
  if (adjacency_sequence_in_parent == 0) {
    normal_vector(0, 0) = -1.0_r;
  } else if (adjacency_sequence_in_parent == 1) {
    normal_vector(0, 0) = 1.0_r;
  }
}

template <Is1dElement AdjacencyElementTrait>
inline void computeNormalVector(
    const Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kAllNodeNumber>&
        node_coordinate,
    const Eigen::Matrix<Real, AdjacencyElementTrait::kDimension * AdjacencyElementTrait::kQuadratureNumber,
                        AdjacencyElementTrait::kBasisFunctionNumber>& nodal_gradient_basis_function,
    Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kQuadratureNumber>&
        normal_vector) {
  for (Isize i = 0; i < AdjacencyElementTrait::kQuadratureNumber; i++) {
    normal_vector(0, i) = nodal_gradient_basis_function.row(i) * node_coordinate.row(1).transpose();
    normal_vector(1, i) = -nodal_gradient_basis_function.row(i) * node_coordinate.row(0).transpose();
    normal_vector.col(i).normalize();
  }
}

template <Is2dElement AdjacencyElementTrait>
inline void computeNormalVector(
    const Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kAllNodeNumber>&
        node_coordinate,
    const Eigen::Matrix<Real, AdjacencyElementTrait::kDimension * AdjacencyElementTrait::kQuadratureNumber,
                        AdjacencyElementTrait::kBasisFunctionNumber>& nodal_gradient_basis_function,
    Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kQuadratureNumber>&
        normal_vector) {
  Eigen::Vector<Real, AdjacencyElementTrait::kDimension + 1> partial_xi;
  Eigen::Vector<Real, AdjacencyElementTrait::kDimension + 1> partial_eta;
  for (Isize i = 0; i < AdjacencyElementTrait::kQuadratureNumber; i++) {
    partial_xi.noalias() =
        nodal_gradient_basis_function.row(i * AdjacencyElementTrait::kDimension) * node_coordinate.transpose();
    partial_eta.noalias() =
        nodal_gradient_basis_function.row(i * AdjacencyElementTrait::kDimension + 1) * node_coordinate.transpose();
    normal_vector.col(i) = partial_xi.cross(partial_eta).normalized();
  }
}

template <typename AdjacencyElementTrait>
inline void AdjacencyElementMesh<AdjacencyElementTrait>::computeAdjacencyElementNormalVector() {
  tbb::parallel_for(tbb::blocked_range<Isize>(0, this->number_), [&](const tbb::blocked_range<Isize>& range) -> void {
    for (Isize i = range.begin(); i != range.end(); i++) {
      if constexpr (Is0dElement<AdjacencyElementTrait>) {
        computeNormalVector<AdjacencyElementTrait>(this->adjacency_sequence_in_left_parent_(i),
                                                   this->normal_vector_(i));
      } else if constexpr (Is1dElement<AdjacencyElementTrait>) {
        computeNormalVector<AdjacencyElementTrait>(this->node_coordinate_(i), this->nodal_gradient_basis_function_,
                                                   this->normal_vector_(i));
      } else if constexpr (Is2dElement<AdjacencyElementTrait>) {
        computeNormalVector<AdjacencyElementTrait>(this->node_coordinate_(i), this->nodal_gradient_basis_function_,
                                                   this->normal_vector_(i));
      }
    }
  });
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GEOMETRY_CPP_
