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
#include "Utils/Constant.cpp"

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

template <typename AdjacencyElementTrait>
inline void AdjacencyElementMesh<AdjacencyElementTrait>::getAdjacencyElementQuality() {
#pragma omp parallel for default(none) schedule(nonmonotonic : auto) shared(Eigen::Dynamic)
  for (Isize i = 0; i < this->number_; i++) {
    std::vector<double> element_inner_radius;
    gmsh::model::mesh::getElementQualities({static_cast<std::size_t>(this->gmsh_tag_(i))}, element_inner_radius,
                                           "innerRadius");
    this->inner_radius_(i) = static_cast<Real>(element_inner_radius[0]);
  }
}

template <typename VolumeElementTrait>
inline void VolumeElementMesh<VolumeElementTrait>::computeVolumeElementOtherNodeCoordinate() {
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
inline void AdjacencyElementMesh<AdjacencyElementTrait>::computeAdjacencyElementOtherNodeCoordinate(
    const bool is_initialization) {
  const Isize end = is_initialization ? this->number_ : this->rotate_number_;
  tbb::parallel_for(tbb::blocked_range<Isize>(0, end), [&](const tbb::blocked_range<Isize>& range) -> void {
    for (Isize i = range.begin(); i != range.end(); i++) {
      Isize element_index;
      if (is_initialization) {
        element_index = i;
      } else if (i < this->interior_rotate_number_) {
        element_index = i + this->interior_static_number_;
      } else if (i < this->interior_rotate_number_ + this->boundary_rotate_number_) {
        element_index = i + this->interior_static_number_ + this->boundary_static_number_;
      } else {
        element_index = i + this->interior_static_number_ + this->boundary_static_number_ + this->interface_number_;
      }
      this->center_node_coordinate_(element_index).setZero();
      for (Isize j = 0; j < AdjacencyElementTrait::kAllNodeNumber; j++) {
        this->center_node_coordinate_(element_index) += this->node_coordinate_(element_index).col(j);
      }
      this->center_node_coordinate_(element_index) /= static_cast<Real>(AdjacencyElementTrait::kAllNodeNumber);
      for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
        this->quadrature_node_coordinate_(element_index).col(j) =
            this->node_coordinate_(element_index) * this->nodal_basis_function_.row(j).transpose();
      }
    }
  });
}

template <typename AdjacencyElementTrait>
inline void AdjacencyElementMeshDevice<AdjacencyElementTrait>::computeAdjacencyElementOtherNodeCoordinate(
    [[maybe_unused]] const bool is_initialization) {
  queue.submit([&](sycl::handler& cgh) -> void {
    cgh.parallel_for(getNdRange(this->rotate_number_), [=, this](sycl::nd_item<1> index) -> void {
      const auto i = static_cast<Isize>(index.get_global_id(0));
      if (i >= this->rotate_number_) {
        return;
      }
      Isize element_index;
      if (i < this->interior_rotate_number_) {
        element_index = i + this->interior_static_number_;
      } else if (i < this->interior_rotate_number_ + this->boundary_rotate_number_) {
        element_index = i + this->interior_static_number_ + this->boundary_static_number_;
      } else {
        element_index = i + this->interior_static_number_ + this->boundary_static_number_ + this->interface_number_;
      }
      const AdjacencyElementMeshDevice<AdjacencyElementTrait>* self = this;
      const Device::View<
          const Device::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kAllNodeNumber>>
          node_coordinate = self->node_coordinate_.view(element_index, this->number_);
      Device::View<Device::Vector<Real, AdjacencyElementTrait::kDimension + 1>> center_node_coordinate =
          this->center_node_coordinate_.view(element_index, this->number_);
      Device::View<
          Device::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kQuadratureNumber>>
          quadrature_node_coordinate = this->quadrature_node_coordinate_.view(element_index, this->number_);
      center_node_coordinate.setZero();
      for (Isize j = 0; j < AdjacencyElementTrait::kAllNodeNumber; j++) {
        for (Isize m = 0; m < AdjacencyElementTrait::kDimension + 1; m++) {
          center_node_coordinate(m) += node_coordinate(m, j);
        }
      }
      for (Isize m = 0; m < AdjacencyElementTrait::kDimension + 1; m++) {
        center_node_coordinate(m) /= static_cast<Real>(AdjacencyElementTrait::kAllNodeNumber);
      }
      for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
        for (Isize m = 0; m < AdjacencyElementTrait::kDimension + 1; m++) {
          Real sum = 0.0_r;
          for (Isize n = 0; n < AdjacencyElementTrait::kAllNodeNumber; n++) {
            sum += node_coordinate(m, n) * this->nodal_basis_function_(j, n);
          }
          quadrature_node_coordinate(m, j) = sum;
        }
      }
    });
  });
}

template <typename VolumeElementTrait>
inline void VolumeElementMesh<VolumeElementTrait>::computeVolumeElementJacobian(const bool is_initialization) {
  const Isize end = is_initialization ? this->number_ : this->rotate_number_;
  tbb::parallel_for(tbb::blocked_range<Isize>(0, end), [&](const tbb::blocked_range<Isize>& range) -> void {
    for (Isize i = range.begin(); i != range.end(); i++) {
      const Isize element_index = is_initialization ? i : i + this->static_number_;
      Eigen::Matrix<Real, VolumeElementTrait::kDimension, VolumeElementTrait::kDimension> jacobian;
      Real jacobian_determinant;
      Eigen::Matrix<Real, VolumeElementTrait::kDimension, VolumeElementTrait::kDimension>
          jacobian_inverse_multiply_determinate;
      for (Isize j = 0; j < VolumeElementTrait::kQuadratureNumber; j++) {
        jacobian.noalias() =
            this->node_coordinate_(element_index) *
            this->nodal_gradient_basis_function_(
                    Eigen::seqN(j * VolumeElementTrait::kDimension, Eigen::fix<VolumeElementTrait::kDimension>),
                    Eigen::placeholders::all)
                .transpose();
        if constexpr (Is1dElement<VolumeElementTrait>) {
          jacobian_determinant = jacobian(0, 0);
          jacobian_inverse_multiply_determinate(0, 0) = 1.0_r;
        } else if constexpr (Is2dElement<VolumeElementTrait>) {
          jacobian_determinant = jacobian(0, 0) * jacobian(1, 1) - jacobian(0, 1) * jacobian(1, 0);
          jacobian_inverse_multiply_determinate(0, 0) = jacobian(1, 1);
          jacobian_inverse_multiply_determinate(0, 1) = -jacobian(0, 1);
          jacobian_inverse_multiply_determinate(1, 0) = -jacobian(1, 0);
          jacobian_inverse_multiply_determinate(1, 1) = jacobian(0, 0);
        } else if constexpr (Is3dElement<VolumeElementTrait>) {
          // The inverse of the Jacobian matrix is computed by the cofactor method for better performance than the
          // LU decomposition.
          jacobian_inverse_multiply_determinate.row(0) = jacobian.col(1).cross(jacobian.col(2));
          jacobian_inverse_multiply_determinate.row(1) = jacobian.col(2).cross(jacobian.col(0));
          jacobian_inverse_multiply_determinate.row(2) = jacobian.col(0).cross(jacobian.col(1));
          jacobian_determinant = jacobian_inverse_multiply_determinate.row(0) * jacobian.col(0);
        }
        this->jacobian_determinant_multiply_weight_(element_index)(j) =
            jacobian_determinant * this->quadrature_weight_(j);
        this->jacobian_transpose_inverse_multiply_determinate_and_weight_(element_index)(
                Eigen::placeholders::all,
                Eigen::seqN(j * VolumeElementTrait::kDimension, Eigen::fix<VolumeElementTrait::kDimension>))
            .noalias() = jacobian_inverse_multiply_determinate.transpose() * this->quadrature_weight_(j);
      }
    }
  });
}

template <typename VolumeElementTrait>
inline void VolumeElementMeshDevice<VolumeElementTrait>::computeVolumeElementJacobian(
    [[maybe_unused]] const bool is_initialization) {
  queue.submit([&](sycl::handler& cgh) -> void {
    cgh.parallel_for(getNdRange(this->rotate_number_), [=, this](sycl::nd_item<1> index) -> void {
      const auto i = static_cast<Isize>(index.get_global_id(0));
      if (i >= this->rotate_number_) {
        return;
      }
      const Isize element_index = i + this->static_number_;
      const VolumeElementMeshDevice<VolumeElementTrait>* self = this;
      Device::StaticMatrix<Real, VolumeElementTrait::kDimension, VolumeElementTrait::kDimension> jacobian;
      Real jacobian_determinant;
      Device::StaticMatrix<Real, VolumeElementTrait::kDimension, VolumeElementTrait::kDimension>
          jacobian_inverse_multiply_determinate;
      const Device::View<const Device::Matrix<Real, VolumeElementTrait::kDimension, VolumeElementTrait::kAllNodeNumber>>
          node_coordinate = self->node_coordinate_.view(element_index, this->number_);
      Device::View<Device::Vector<Real, VolumeElementTrait::kQuadratureNumber>> jacobian_determinant_multiply_weight =
          this->jacobian_determinant_multiply_weight_.view(element_index, this->number_);
      Device::View<Device::Matrix<Real, VolumeElementTrait::kDimension,
                                  VolumeElementTrait::kDimension * VolumeElementTrait::kQuadratureNumber>>
          jacobian_transpose_inverse_multiply_determinate_and_weight =
              this->jacobian_transpose_inverse_multiply_determinate_and_weight_.view(element_index, this->number_);
      for (Isize j = 0; j < VolumeElementTrait::kQuadratureNumber; j++) {
        for (Isize m = 0; m < VolumeElementTrait::kDimension; m++) {
          for (Isize n = 0; n < VolumeElementTrait::kDimension; n++) {
            Real sum = 0.0_r;
            for (Isize k = 0; k < VolumeElementTrait::kAllNodeNumber; k++) {
              sum += node_coordinate(m, k) *
                     this->nodal_gradient_basis_function_(j * VolumeElementTrait::kDimension + n, k);
            }
            jacobian(m, n) = sum;
          }
        }
        if constexpr (Is1dElement<VolumeElementTrait>) {
          jacobian_determinant = jacobian(0, 0);
          jacobian_inverse_multiply_determinate(0, 0) = 1.0_r;
        } else if constexpr (Is2dElement<VolumeElementTrait>) {
          jacobian_determinant = jacobian(0, 0) * jacobian(1, 1) - jacobian(0, 1) * jacobian(1, 0);
          jacobian_inverse_multiply_determinate(0, 0) = jacobian(1, 1);
          jacobian_inverse_multiply_determinate(0, 1) = -jacobian(0, 1);
          jacobian_inverse_multiply_determinate(1, 0) = -jacobian(1, 0);
          jacobian_inverse_multiply_determinate(1, 1) = jacobian(0, 0);
        } else if constexpr (Is3dElement<VolumeElementTrait>) {
          jacobian_inverse_multiply_determinate(0, 0) =
              jacobian(1, 1) * jacobian(2, 2) - jacobian(2, 1) * jacobian(1, 2);
          jacobian_inverse_multiply_determinate(0, 1) =
              jacobian(2, 1) * jacobian(0, 2) - jacobian(0, 1) * jacobian(2, 2);
          jacobian_inverse_multiply_determinate(0, 2) =
              jacobian(0, 1) * jacobian(1, 2) - jacobian(1, 1) * jacobian(0, 2);
          jacobian_inverse_multiply_determinate(1, 0) =
              jacobian(1, 2) * jacobian(2, 0) - jacobian(2, 2) * jacobian(1, 0);
          jacobian_inverse_multiply_determinate(1, 1) =
              jacobian(2, 2) * jacobian(0, 0) - jacobian(0, 2) * jacobian(2, 0);
          jacobian_inverse_multiply_determinate(1, 2) =
              jacobian(0, 2) * jacobian(1, 0) - jacobian(1, 2) * jacobian(0, 0);
          jacobian_inverse_multiply_determinate(2, 0) =
              jacobian(1, 0) * jacobian(2, 1) - jacobian(2, 0) * jacobian(1, 1);
          jacobian_inverse_multiply_determinate(2, 1) =
              jacobian(2, 0) * jacobian(0, 1) - jacobian(0, 0) * jacobian(2, 1);
          jacobian_inverse_multiply_determinate(2, 2) =
              jacobian(0, 0) * jacobian(1, 1) - jacobian(1, 0) * jacobian(0, 1);
          jacobian_determinant = 0.0_r;
          for (Isize m = 0; m < VolumeElementTrait::kDimension; m++) {
            jacobian_determinant += jacobian_inverse_multiply_determinate(m, 0) * jacobian(0, m);
          }
        }
        jacobian_determinant_multiply_weight(j) = jacobian_determinant * this->quadrature_weight_(j);
        for (Isize m = 0; m < VolumeElementTrait::kDimension; m++) {
          for (Isize n = 0; n < VolumeElementTrait::kDimension; n++) {
            jacobian_transpose_inverse_multiply_determinate_and_weight(m, j * VolumeElementTrait::kDimension + n) =
                jacobian_inverse_multiply_determinate(n, m) * this->quadrature_weight_(j);
          }
        }
      }
    });
  });
}

template <typename AdjacencyElementTrait>
inline void AdjacencyElementMesh<AdjacencyElementTrait>::computeAdjacencyElementJacobianDeterminant() {
  tbb::parallel_for(tbb::blocked_range<Isize>(0, this->number_), [&](const tbb::blocked_range<Isize>& range) -> void {
    for (Isize i = range.begin(); i != range.end(); i++) {
      if constexpr (Is0dElement<AdjacencyElementTrait>) {
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
        if constexpr (Is1dElement<AdjacencyElementTrait>) {
          jacobian_determinant = jacobian.norm();
        } else if constexpr (Is2dElement<AdjacencyElementTrait>) {
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

template <typename AdjacencyElementTrait>
inline void AdjacencyElementMesh<AdjacencyElementTrait>::computeAdjacencyElementNormalVector(
    const bool is_initialization) {
  const Isize end = is_initialization ? this->number_ : this->rotate_number_;
  tbb::parallel_for(tbb::blocked_range<Isize>(0, end), [&](const tbb::blocked_range<Isize>& range) -> void {
    for (Isize i = range.begin(); i != range.end(); i++) {
      Isize element_index;
      if (is_initialization) {
        element_index = i;
      } else if (i < this->interior_rotate_number_) {
        element_index = i + this->interior_static_number_;
      } else if (i < this->interior_rotate_number_ + this->boundary_rotate_number_) {
        element_index = i + this->interior_static_number_ + this->boundary_static_number_;
      } else {
        element_index = i + this->interior_static_number_ + this->boundary_static_number_ + this->interface_number_;
      }
      if constexpr (Is0dElement<AdjacencyElementTrait>) {
        if (this->adjacency_sequence_in_left_parent_(element_index) == 0) {
          this->normal_vector_(element_index)(0, 0) = -1.0_r;
        } else if (this->adjacency_sequence_in_left_parent_(element_index) == 1) {
          this->normal_vector_(element_index)(0, 0) = 1.0_r;
        }
      } else if constexpr (Is1dElement<AdjacencyElementTrait>) {
        for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
          this->normal_vector_(element_index)(0, j) =
              this->nodal_gradient_basis_function_.row(j) * this->node_coordinate_(element_index).row(1).transpose();
          this->normal_vector_(element_index)(1, j) =
              -this->nodal_gradient_basis_function_.row(j) * this->node_coordinate_(element_index).row(0).transpose();
          this->normal_vector_(element_index).col(j).normalize();
        }
      } else if constexpr (Is2dElement<AdjacencyElementTrait>) {
        for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
          const Eigen::Vector<Real, AdjacencyElementTrait::kDimension + 1> partial_xi =
              this->nodal_gradient_basis_function_.row(j * AdjacencyElementTrait::kDimension) *
              this->node_coordinate_(element_index).transpose();
          const Eigen::Vector<Real, AdjacencyElementTrait::kDimension + 1> partial_eta =
              this->nodal_gradient_basis_function_.row(j * AdjacencyElementTrait::kDimension + 1) *
              this->node_coordinate_(element_index).transpose();
          this->normal_vector_(element_index).col(j) = partial_xi.cross(partial_eta).normalized();
        }
      }
    }
  });
}

template <typename AdjacencyElementTrait>
inline void AdjacencyElementMeshDevice<AdjacencyElementTrait>::computeAdjacencyElementNormalVector(
    [[maybe_unused]] const bool is_initialization) {
  queue.submit([&](sycl::handler& cgh) -> void {
    cgh.parallel_for(getNdRange(this->rotate_number_), [=, this](sycl::nd_item<1> index) -> void {
      const auto i = static_cast<Isize>(index.get_global_id(0));
      if (i >= this->rotate_number_) {
        return;
      }
      Isize element_index;
      if (i < this->interior_rotate_number_) {
        element_index = i + this->interior_static_number_;
      } else if (i < this->interior_rotate_number_ + this->boundary_rotate_number_) {
        element_index = i + this->interior_static_number_ + this->boundary_static_number_;
      } else {
        element_index = i + this->interior_static_number_ + this->boundary_static_number_ + this->interface_number_;
      }
      const AdjacencyElementMeshDevice<AdjacencyElementTrait>* self = this;
      const Device::View<
          const Device::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kAllNodeNumber>>
          node_coordinate = self->node_coordinate_.view(element_index, this->number_);
      Device::View<
          Device::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kQuadratureNumber>>
          normal_vector = this->normal_vector_.view(element_index, this->number_);
      if constexpr (Is0dElement<AdjacencyElementTrait>) {
        if (this->adjacency_sequence_in_left_parent_(element_index) == 0) {
          normal_vector(0, 0) = -1.0_r;
        } else if (this->adjacency_sequence_in_left_parent_(element_index) == 1) {
          normal_vector(0, 0) = 1.0_r;
        }
      } else if constexpr (Is1dElement<AdjacencyElementTrait>) {
        for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
          Real sum_x = 0.0_r;
          Real sum_y = 0.0_r;
          for (Isize m = 0; m < AdjacencyElementTrait::kAllNodeNumber; m++) {
            sum_x += this->nodal_gradient_basis_function_(j, m) * node_coordinate(1, m);
            sum_y -= this->nodal_gradient_basis_function_(j, m) * node_coordinate(0, m);
          }
          const Real norm = sycl::sqrt(sum_x * sum_x + sum_y * sum_y);
          normal_vector(0, j) = sum_x / norm;
          normal_vector(1, j) = sum_y / norm;
        }
      } else if constexpr (Is2dElement<AdjacencyElementTrait>) {
        for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
          Device::StaticVector<Real, AdjacencyElementTrait::kDimension + 1> partial_xi;
          Device::StaticVector<Real, AdjacencyElementTrait::kDimension + 1> partial_eta;
          for (Isize m = 0; m < AdjacencyElementTrait::kDimension + 1; m++) {
            Real sum_xi = 0.0_r;
            Real sum_eta = 0.0_r;
            for (Isize n = 0; n < AdjacencyElementTrait::kAllNodeNumber; n++) {
              sum_xi += this->nodal_gradient_basis_function_(j * AdjacencyElementTrait::kDimension, n) *
                        node_coordinate(m, n);
              sum_eta += this->nodal_gradient_basis_function_(j * AdjacencyElementTrait::kDimension + 1, n) *
                         node_coordinate(m, n);
            }
            partial_xi(m) = sum_xi;
            partial_eta(m) = sum_eta;
          }
          Device::StaticVector<Real, AdjacencyElementTrait::kDimension + 1> cross_product;
          cross_product(0) = partial_xi(1) * partial_eta(2) - partial_xi(2) * partial_eta(1);
          cross_product(1) = partial_xi(2) * partial_eta(0) - partial_xi(0) * partial_eta(2);
          cross_product(2) = partial_xi(0) * partial_eta(1) - partial_xi(1) * partial_eta(0);
          const Real norm = sycl::sqrt(cross_product(0) * cross_product(0) + cross_product(1) * cross_product(1) +
                                       cross_product(2) * cross_product(2));
          for (Isize m = 0; m < AdjacencyElementTrait::kDimension + 1; m++) {
            normal_vector(m, j) = cross_product(m) / norm;
          }
        }
      }
    });
  });
}

template <typename AdjacencyElementTrait>
[[nodiscard]] inline Isize AdjacencyElementMesh<AdjacencyElementTrait>::getAdjacencyElementIndexFromGlobalCoordinate(
    const Eigen::Vector<Real, AdjacencyElementTrait::kDimension + 1>& global_coordinate,
    const BoundaryConditionEnum boundary_condition) const {
  Isize result_element_index = -1;
  Real min_center_node_coordinate_normalization_difference = kRealMax;
  for (Isize i = 0; i < this->interface_number_; i++) {
    const Isize interafce_element_index =
        boundary_condition == BoundaryConditionEnum::InterfaceStatic
            ? i + this->interior_number_ + this->boundary_number_ + this->interface_number_
            : i + this->interior_number_ + this->boundary_number_;
    const Real center_node_coordinate_normalization_difference =
        ((this->center_node_coordinate_(interafce_element_index).array() - global_coordinate.array()) /
         this->inner_radius_(interafce_element_index))
            .matrix()
            .norm();
    if (center_node_coordinate_normalization_difference < min_center_node_coordinate_normalization_difference) {
      min_center_node_coordinate_normalization_difference = center_node_coordinate_normalization_difference;
      result_element_index = interafce_element_index;
    }
  }
  return result_element_index;
}

template <typename AdjacencyElementTrait>
[[nodiscard]] inline Isize
AdjacencyElementMeshDevice<AdjacencyElementTrait>::getAdjacencyElementIndexFromGlobalCoordinate(
    const Device::StaticVector<Real, AdjacencyElementTrait::kDimension + 1>& global_coordinate,
    const BoundaryConditionEnum boundary_condition) const {
  Isize result_element_index = -1;
  Real min_center_node_coordinate_normalization_difference = kRealMax;
  for (Isize i = 0; i < this->interface_number_; i++) {
    const Isize interafce_element_index =
        boundary_condition == BoundaryConditionEnum::InterfaceStatic
            ? i + this->interior_number_ + this->boundary_number_ + this->interface_number_
            : i + this->interior_number_ + this->boundary_number_;
    const Device::View<const Device::Vector<Real, AdjacencyElementTrait::kDimension + 1>> center_node_coordinate =
        this->center_node_coordinate_.view(interafce_element_index, this->number_);
    const Real inner_radius = this->inner_radius_(interafce_element_index);
    Real center_node_coordinate_normalization_difference = 0.0_r;
    for (Isize m = 0; m < AdjacencyElementTrait::kDimension + 1; m++) {
      const Real difference = (center_node_coordinate(m) - global_coordinate(m)) / inner_radius;
      center_node_coordinate_normalization_difference += difference * difference;
    }
    center_node_coordinate_normalization_difference = sycl::sqrt(center_node_coordinate_normalization_difference);
    if (center_node_coordinate_normalization_difference < min_center_node_coordinate_normalization_difference) {
      min_center_node_coordinate_normalization_difference = center_node_coordinate_normalization_difference;
      result_element_index = interafce_element_index;
    }
  }
  return result_element_index;
}

template <typename AdjacencyElementTrait>
inline void AdjacencyElementMesh<AdjacencyElementTrait>::computeAdjacencyElementLocalCoordinateFromGlobalCoordinate(
    const Eigen::Vector<Real, AdjacencyElementTrait::kDimension + 1>& global_coordinate,
    Eigen::Vector<Real, AdjacencyElementTrait::kDimension>& adjacency_local_coordinate, const Isize element_index,
    const Real tolerance) const {
  Eigen::Vector<Real, AdjacencyElementTrait::kAllBasisFunctionNumber> basis_function_value;
  Eigen::Matrix<Real, AdjacencyElementTrait::kDimension, AdjacencyElementTrait::kAllBasisFunctionNumber>
      basis_function_gradient_value;
  Eigen::Vector<Real, AdjacencyElementTrait::kDimension + 1> difference_vector;
  Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kDimension> jacobian;
  Eigen::Matrix<Real, AdjacencyElementTrait::kDimension, AdjacencyElementTrait::kDimension> hessian;
  Eigen::Matrix<Real, AdjacencyElementTrait::kDimension, AdjacencyElementTrait::kDimension> hessian_inverse;
  Eigen::Vector<Real, AdjacencyElementTrait::kDimension> b_vector;
  Eigen::Vector<Real, AdjacencyElementTrait::kDimension> residual;
  const Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kAllNodeNumber>&
      node_coordinate = this->node_coordinate_(element_index);
  adjacency_local_coordinate.setZero();
  for (Isize iteration = 0; iteration < 100; iteration++) {
    this->computeLagrangeValue(adjacency_local_coordinate, basis_function_value);
    this->computeLagrangeGradientValue(adjacency_local_coordinate, basis_function_gradient_value);
    difference_vector.noalias() = node_coordinate * basis_function_value - global_coordinate;
    jacobian.noalias() = node_coordinate * basis_function_gradient_value.transpose();
    b_vector.noalias() = jacobian.transpose() * difference_vector;
    hessian.noalias() = jacobian.transpose() * jacobian;
    if constexpr (Is1dElement<AdjacencyElementTrait>) {
      const Real hessian_determinant = hessian(0, 0);
      hessian_inverse(0, 0) = 1.0_r / hessian_determinant;
    } else if constexpr (Is2dElement<AdjacencyElementTrait>) {
      const Real hessian_determinant = hessian(0, 0) * hessian(1, 1) - hessian(0, 1) * hessian(1, 0);
      hessian_inverse(0, 0) = hessian(1, 1) / hessian_determinant;
      hessian_inverse(0, 1) = -hessian(0, 1) / hessian_determinant;
      hessian_inverse(1, 0) = -hessian(1, 0) / hessian_determinant;
      hessian_inverse(1, 1) = hessian(0, 0) / hessian_determinant;
    }
    residual.noalias() = hessian_inverse * b_vector;
    if (residual.norm() < tolerance) {
      return;
    }
    adjacency_local_coordinate -= residual;
  }
}

template <typename AdjacencyElementTrait>
inline void
AdjacencyElementMeshDevice<AdjacencyElementTrait>::computeAdjacencyElementLocalCoordinateFromGlobalCoordinate(
    const Device::StaticVector<Real, AdjacencyElementTrait::kDimension + 1>& global_coordinate,
    Device::StaticVector<Real, AdjacencyElementTrait::kDimension>& adjacency_local_coordinate,
    const Isize element_index, const Real tolerance) const {
  Device::StaticVector<Real, AdjacencyElementTrait::kAllBasisFunctionNumber> basis_function_value;
  Device::StaticMatrix<Real, AdjacencyElementTrait::kDimension, AdjacencyElementTrait::kAllBasisFunctionNumber>
      basis_function_gradient_value;
  Device::StaticVector<Real, AdjacencyElementTrait::kDimension + 1> difference_vector;
  Device::StaticMatrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kDimension> jacobian;
  Device::StaticMatrix<Real, AdjacencyElementTrait::kDimension, AdjacencyElementTrait::kDimension> hessian;
  Device::StaticMatrix<Real, AdjacencyElementTrait::kDimension, AdjacencyElementTrait::kDimension> hessian_inverse;
  Device::StaticVector<Real, AdjacencyElementTrait::kDimension> b_vector;
  Device::StaticVector<Real, AdjacencyElementTrait::kDimension> residual;
  const Device::View<
      const Device::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kAllNodeNumber>>
      node_coordinate = this->node_coordinate_.view(element_index, this->number_);
  adjacency_local_coordinate.setZero();
  for (Isize iteration = 0; iteration < 100; iteration++) {
    this->computeLagrangeValue(adjacency_local_coordinate, basis_function_value);
    this->computeLagrangeGradientValue(adjacency_local_coordinate, basis_function_gradient_value);
    for (Isize m = 0; m < AdjacencyElementTrait::kDimension + 1; m++) {
      Real sum = 0.0_r;
      for (Isize n = 0; n < AdjacencyElementTrait::kAllNodeNumber; n++) {
        sum += node_coordinate(m, n) * basis_function_value(n);
      }
      difference_vector(m) = sum - global_coordinate(m);
    }
    for (Isize m = 0; m < AdjacencyElementTrait::kDimension + 1; m++) {
      for (Isize n = 0; n < AdjacencyElementTrait::kDimension; n++) {
        Real sum = 0.0_r;
        for (Isize k = 0; k < AdjacencyElementTrait::kAllNodeNumber; k++) {
          sum += node_coordinate(m, k) * basis_function_gradient_value(n, k);
        }
        jacobian(m, n) = sum;
      }
    }
    for (Isize m = 0; m < AdjacencyElementTrait::kDimension; m++) {
      Real sum = 0.0_r;
      for (Isize n = 0; n < AdjacencyElementTrait::kDimension + 1; n++) {
        sum += jacobian(n, m) * difference_vector(n);
      }
      b_vector(m) = sum;
    }
    for (Isize m = 0; m < AdjacencyElementTrait::kDimension; m++) {
      for (Isize n = 0; n < AdjacencyElementTrait::kDimension; n++) {
        Real sum = 0.0_r;
        for (Isize k = 0; k < AdjacencyElementTrait::kDimension + 1; k++) {
          sum += jacobian(k, m) * jacobian(k, n);
        }
        hessian(m, n) = sum;
      }
    }
    if constexpr (Is1dElement<AdjacencyElementTrait>) {
      const Real hessian_determinant = hessian(0, 0);
      hessian_inverse(0, 0) = 1.0_r / hessian_determinant;
    } else if constexpr (Is2dElement<AdjacencyElementTrait>) {
      const Real hessian_determinant = hessian(0, 0) * hessian(1, 1) - hessian(0, 1) * hessian(1, 0);
      hessian_inverse(0, 0) = hessian(1, 1) / hessian_determinant;
      hessian_inverse(0, 1) = -hessian(0, 1) / hessian_determinant;
      hessian_inverse(1, 0) = -hessian(1, 0) / hessian_determinant;
      hessian_inverse(1, 1) = hessian(0, 0) / hessian_determinant;
    }
    for (Isize m = 0; m < AdjacencyElementTrait::kDimension; m++) {
      Real sum = 0.0_r;
      for (Isize n = 0; n < AdjacencyElementTrait::kDimension; n++) {
        sum += hessian_inverse(m, n) * b_vector(n);
      }
      residual(m) = sum;
    }
    Real residual_norm = 0.0_r;
    for (Isize m = 0; m < AdjacencyElementTrait::kDimension; m++) {
      residual_norm += residual(m) * residual(m);
    }
    residual_norm = sycl::sqrt(residual_norm);
    if (residual_norm < tolerance) {
      return;
    }
    for (Isize m = 0; m < AdjacencyElementTrait::kDimension; m++) {
      adjacency_local_coordinate(m) -= residual(m);
    }
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GEOMETRY_CPP_
