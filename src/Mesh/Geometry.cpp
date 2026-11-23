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

#include <oneapi/tbb.h>

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <iterator>
#include <map>
#include <stdexcept>
#include <sycl/sycl.hpp>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Mesh/Polynomial.cpp"
#include "Mesh/ReadControl.cpp"
#include "Utils/BasicDataType.cpp"
#include "Utils/Concept.cpp"
#include "Utils/Constant.cpp"

namespace SubrosaDG {

template <typename VolumeElementTrait>
template <typename... AdjacencyElementMesh>
inline void VolumeElementMesh<VolumeElementTrait>::computeVolumeElementCharacteristicLength(
    const AdjacencyElementMesh&... adjacency_element_mesh) {
  this->characteristic_length_.setZero();
  const auto add_boundary_measure = [&](const auto& adjacency_mesh) {
    for (Isize i = 0; i < adjacency_mesh.number_; i++) {
      const Real measure = adjacency_mesh.jacobian_determinant_multiply_weight_(i).sum();
      if (adjacency_mesh.left_parent_gmsh_type_number_(i) == VolumeElementTrait::kGmshTypeNumber) {
        this->characteristic_length_(adjacency_mesh.left_parent_index_each_type_(i)) += measure;
      }
      if (i < adjacency_mesh.interior_number_ &&
          adjacency_mesh.right_parent_gmsh_type_number_(i) == VolumeElementTrait::kGmshTypeNumber) {
        this->characteristic_length_(adjacency_mesh.right_parent_index_each_type_(i)) += measure;
      }
    }
  };
  (add_boundary_measure(adjacency_element_mesh), ...);
  tbb::parallel_for(tbb::blocked_range<Isize>(0, this->number_), [&](const tbb::blocked_range<Isize>& range) -> void {
    for (Isize i = range.begin(); i != range.end(); i++) {
      const Real volume = this->jacobian_determinant_multiply_weight_(i).sum();
      const Real boundary_measure = this->characteristic_length_(i);
      // h = 2*d*V/S is an equivalent diameter, not a general polyhedron's exact insphere diameter.
      // In 1D the two endpoint measures sum to 2. Rigid rotation preserves V, S, and h.
      this->characteristic_length_(i) = 2.0_r * VolumeElementTrait::kDimension * volume / boundary_measure;
    }
  });
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
inline void AdjacencyElementMesh<AdjacencyElementTrait>::computeAdjacencyElementQuadratureNodeCoordinate(
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
      for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
        this->quadrature_node_coordinate_(element_index).col(j) =
            this->node_coordinate_(element_index) * this->nodal_basis_function_.row(j).transpose();
      }
    }
  });
}

template <typename AdjacencyElementTrait>
inline void AdjacencyElementMeshDevice<AdjacencyElementTrait>::computeAdjacencyElementQuadratureNodeCoordinate(
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
          quadrature_node_coordinate = this->quadrature_node_coordinate_.view(element_index, this->number_);
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
template <int PointNumber>
[[nodiscard]] inline Real AdjacencyElementInterfaceCandidateCache<AdjacencyElementTrait>::computeMinimumRadius(
    const Eigen::Matrix<Real, 2, PointNumber>& projected_control_coordinate) {
  std::array<Eigen::Vector<Real, 2>, PointNumber> sorted_point;
  for (Isize i = 0; i < PointNumber; i++) {
    sorted_point[static_cast<Usize>(i)] = projected_control_coordinate.col(i);
  }
  const auto lexicographical_less = [](const Eigen::Vector<Real, 2>& lhs, const Eigen::Vector<Real, 2>& rhs) -> bool {
    return lhs(0) < rhs(0) || (lhs(0) == rhs(0) && lhs(1) < rhs(1));
  };
  const auto cross_2d = [](const Eigen::Vector<Real, 2>& lhs, const Eigen::Vector<Real, 2>& rhs) -> Real {
    return lhs(0) * rhs(1) - lhs(1) * rhs(0);
  };
  std::ranges::sort(sorted_point, lexicographical_less);
  const Isize point_number =
      static_cast<Isize>(std::distance(sorted_point.begin(), std::ranges::unique(sorted_point).begin()));
  if (point_number == 1) {
    return sorted_point[0].norm();
  }

  // Build the lower and upper chains of the 2D convex hull in counter-clockwise order.
  std::array<Eigen::Vector<Real, 2>, 2 * PointNumber> hull;
  Isize hull_size = 0;
  const auto append_hull_point = [&](const Eigen::Vector<Real, 2>& point, const Isize minimum_size) -> void {
    while (hull_size > minimum_size) {
      const Eigen::Vector<Real, 2> previous_edge =
          hull[static_cast<Usize>(hull_size - 1)] - hull[static_cast<Usize>(hull_size - 2)];
      const Eigen::Vector<Real, 2> current_edge = point - hull[static_cast<Usize>(hull_size - 1)];
      if (cross_2d(previous_edge, current_edge) > 0.0_r) {
        break;
      }
      hull_size--;
    }
    hull[static_cast<Usize>(hull_size)] = point;
    hull_size++;
  };
  for (Isize i = 0; i < point_number; i++) {
    append_hull_point(sorted_point[static_cast<Usize>(i)], 1);
  }
  const Isize lower_size = hull_size;
  for (Isize i = point_number - 2; i >= 0; i--) {
    append_hull_point(sorted_point[static_cast<Usize>(i)], lower_size);
  }
  hull_size--;

  bool contains_origin = hull_size >= 3;
  Real minimum_distance_squared = kRealMax;
  for (Isize i = 0; i < hull_size; i++) {
    const Eigen::Vector<Real, 2>& vertex = hull[static_cast<Usize>(i)];
    const Eigen::Vector<Real, 2> edge = hull[static_cast<Usize>((i + 1) % hull_size)] - vertex;
    contains_origin = contains_origin && cross_2d(edge, -vertex) >= 0.0_r;
    const Real projection_parameter = std::clamp(-vertex.dot(edge) / edge.squaredNorm(), 0.0_r, 1.0_r);
    minimum_distance_squared = std::min(minimum_distance_squared, (vertex + projection_parameter * edge).squaredNorm());
  }
  return contains_origin ? 0.0_r : std::sqrt(minimum_distance_squared);
}

template <typename AdjacencyElementTrait>
[[nodiscard]] inline Eigen::Matrix<Real, AdjacencyElementTrait::kAllNodeNumber, AdjacencyElementTrait::kAllNodeNumber>
AdjacencyElementInterfaceCandidateCache<AdjacencyElementTrait>::computeBernsteinInverse(
    const Eigen::Matrix<Real, AdjacencyElementTrait::kDimension, AdjacencyElementTrait::kAllNodeNumber>&
        lagrange_node_coordinate) {
  // A tensor Bernstein control net bounds the entire high-order face, not just its nodes.
  Eigen::Matrix<Real, AdjacencyElementTrait::kAllNodeNumber, AdjacencyElementTrait::kAllNodeNumber> bernstein_matrix;
  for (Isize i = 0; i < AdjacencyElementTrait::kAllNodeNumber; i++) {
    const Real local_u = (lagrange_node_coordinate(0, i) + 1.0_r) / 2.0_r;
    const Real local_v = (lagrange_node_coordinate(1, i) + 1.0_r) / 2.0_r;
    for (Isize j = 0; j <= AdjacencyElementTrait::kPolynomialOrder; j++) {
      for (Isize k = 0; k <= AdjacencyElementTrait::kPolynomialOrder; k++) {
        bernstein_matrix(i, j * (AdjacencyElementTrait::kPolynomialOrder + 1) + k) =
            Bernstein<AdjacencyElementTrait::kPolynomialOrder>::value(j, local_u) *
            Bernstein<AdjacencyElementTrait::kPolynomialOrder>::value(k, local_v);
      }
    }
  }
  return bernstein_matrix.inverse();
}

template <typename AdjacencyElementTrait>
inline void AdjacencyElementInterfaceCandidateCache<AdjacencyElementTrait>::computeElementRange(
    const AdjacencyElementMesh<AdjacencyElementTrait>& mesh,
    const Eigen::Vector<Real, AdjacencyElementTrait::kDimension + 1>& rotation_center,
    const Eigen::Vector<Real, 3>& rotation_axis) {
  const Eigen::Vector<Real, 3> transverse_x = rotation_axis.unitOrthogonal();
  const Eigen::Vector<Real, 3> transverse_y = rotation_axis.cross(transverse_x);
  Eigen::Matrix<Real, 2, 3> transverse_basis;
  transverse_basis.row(0) = transverse_x.transpose();
  transverse_basis.row(1) = transverse_y.transpose();
  const Eigen::Matrix<Real, AdjacencyElementTrait::kAllNodeNumber, AdjacencyElementTrait::kAllNodeNumber>
      bernstein_inverse = computeBernsteinInverse(mesh.lagrange_node_coordinate_);
  const Isize interface_begin_index = mesh.interior_number_ + mesh.boundary_number_;
  this->element_range_.resize(static_cast<Usize>(2 * mesh.interface_number_));
  tbb::parallel_for(
      tbb::blocked_range<Isize>(0, 2 * mesh.interface_number_), [&](const tbb::blocked_range<Isize>& range) -> void {
        for (Isize i = range.begin(); i != range.end(); i++) {
          const Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kAllNodeNumber>
              relative_node_coordinate =
                  mesh.node_coordinate_initial_(interface_begin_index + i).colwise() - rotation_center;
          const Eigen::Matrix<Real, 1, AdjacencyElementTrait::kAllNodeNumber> node_height =
              rotation_axis.transpose() * relative_node_coordinate;
          const bool is_endcap = node_height.maxCoeff() - node_height.minCoeff() <= kRealTolerance;
          const Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kAllNodeNumber>
              control_node_coordinate = relative_node_coordinate * bernstein_inverse.transpose();
          // Only quadrature points query the opposite side. Their axial coordinate and radius are rotation invariant.
          const Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kQuadratureNumber>
              relative_quadrature_coordinate = relative_node_coordinate * mesh.nodal_basis_function_.transpose();
          const Eigen::Matrix<Real, 1, AdjacencyElementTrait::kQuadratureNumber> quadrature_height =
              rotation_axis.transpose() * relative_quadrature_coordinate;
          InterfaceElementRange& current_range = this->element_range_[static_cast<Usize>(i)];
          current_range.is_endcap_ = is_endcap;
          if (is_endcap) {
            current_range.surface_coordinate_ = node_height.mean();
            // The control-net convex hull bounds the whole face, including its interior radial minimum.
            const Eigen::Matrix<Real, 2, AdjacencyElementTrait::kAllNodeNumber> transverse_control_coordinate =
                transverse_basis * control_node_coordinate;
            current_range.element_bound_lower_ =
                computeMinimumRadius<AdjacencyElementTrait::kAllNodeNumber>(transverse_control_coordinate);
            current_range.element_bound_upper_ = transverse_control_coordinate.colwise().norm().maxCoeff();
            const Eigen::Matrix<Real, 1, AdjacencyElementTrait::kQuadratureNumber> quadrature_radius =
                (relative_quadrature_coordinate - rotation_axis * quadrature_height).colwise().norm();
            current_range.quadrature_bound_lower_ = quadrature_radius.minCoeff();
            current_range.quadrature_bound_upper_ = quadrature_radius.maxCoeff();
          } else {
            const Eigen::Matrix<Real, 1, AdjacencyElementTrait::kAllNodeNumber> node_radius =
                (relative_node_coordinate - rotation_axis * node_height).colwise().norm();
            current_range.surface_coordinate_ = node_radius.mean();
            const Eigen::Matrix<Real, 1, AdjacencyElementTrait::kAllNodeNumber> control_height =
                rotation_axis.transpose() * control_node_coordinate;
            current_range.element_bound_lower_ = control_height.minCoeff();
            current_range.element_bound_upper_ = control_height.maxCoeff();
            current_range.quadrature_bound_lower_ = quadrature_height.minCoeff();
            current_range.quadrature_bound_upper_ = quadrature_height.maxCoeff();
          }
        }
      });
}

template <typename AdjacencyElementTrait>
inline void AdjacencyElementInterfaceCandidateCache<AdjacencyElementTrait>::groupInterfaceSurface(
    const AdjacencyElementMesh<AdjacencyElementTrait>& mesh) {
  // The mesh reader creates static i and rotate i from the same original interface face.
  Eigen::Array<Isize, Eigen::Dynamic, 1> surface_parent(mesh.interface_number_);
  for (Isize i = 0; i < mesh.interface_number_; i++) {
    surface_parent(i) = i;
  }
  const auto find_surface_root = [&](Isize interface_index) -> Isize {
    while (surface_parent(interface_index) != interface_index) {
      surface_parent(interface_index) = surface_parent(surface_parent(interface_index));
      interface_index = surface_parent(interface_index);
    }
    return interface_index;
  };
  const Isize interface_begin_index = mesh.interior_number_ + mesh.boundary_number_;
  std::unordered_map<unordered_array<Isize, 2>, std::vector<Isize>> edge_interface_map;
  for (Isize i = 0; i < mesh.interface_number_; i++) {
    const Eigen::Vector<Isize, AdjacencyElementTrait::kAllNodeNumber>& node_tag =
        mesh.node_tag_(interface_begin_index + i);
    const InterfaceElementRange& current_range = this->element_range_[static_cast<Usize>(i)];
    for (Isize edge = 0; edge < AdjacencyElementTrait::kAdjacencyNumber; edge++) {
      unordered_array<Isize, 2> edge_node_tag;
      edge_node_tag[0] = node_tag(edge);
      edge_node_tag[1] = node_tag((edge + 1) % AdjacencyElementTrait::kAdjacencyNumber);
      std::vector<Isize>& neighbor_interface_index = edge_interface_map[edge_node_tag];
      for (const Isize neighbor_index : neighbor_interface_index) {
        const InterfaceElementRange& neighbor_range = this->element_range_[static_cast<Usize>(neighbor_index)];
        if (current_range.is_endcap_ == neighbor_range.is_endcap_ &&
            std::abs(current_range.surface_coordinate_ - neighbor_range.surface_coordinate_) <= kRealTolerance) {
          surface_parent(find_surface_root(i)) = find_surface_root(neighbor_index);
        }
      }
      neighbor_interface_index.emplace_back(i);
    }
  }
  Eigen::Array<Isize, Eigen::Dynamic, 1> surface_index =
      Eigen::Array<Isize, Eigen::Dynamic, 1>::Constant(mesh.interface_number_, -1);
  this->surface_static_interface_index_.clear();
  for (Isize i = 0; i < mesh.interface_number_; i++) {
    const Isize root_index = find_surface_root(i);
    if (surface_index(root_index) < 0) {
      surface_index(root_index) = static_cast<Isize>(this->surface_static_interface_index_.size());
      this->surface_static_interface_index_.emplace_back();
    }
    this->surface_static_interface_index_[static_cast<Usize>(surface_index(root_index))].emplace_back(i);
  }
}

template <typename AdjacencyElementTrait>
inline void AdjacencyElementInterfaceCandidateCache<AdjacencyElementTrait>::buildCandidateCache(
    AdjacencyElementMesh<AdjacencyElementTrait>& mesh) {
  const Isize interface_begin_index = mesh.interior_number_ + mesh.boundary_number_;
  const auto element_bound_lower = [&](const Isize interface_local_index) -> Real {
    return this->element_range_[static_cast<Usize>(interface_local_index)].element_bound_lower_;
  };
  std::vector<Isize> candidate_offset{0};
  std::vector<Isize> candidate_index;
  for (const std::vector<Isize>& static_interface_index : this->surface_static_interface_index_) {
    std::array<std::vector<Isize>, 2> side_interface_index;
    side_interface_index[0] = static_interface_index;
    side_interface_index[1].reserve(static_interface_index.size());
    for (const Isize static_index : static_interface_index) {
      side_interface_index[1].emplace_back(static_index + mesh.interface_number_);
    }
    std::array<std::vector<Real>, 2> side_upper_prefix;
    for (Usize side = 0; side < side_interface_index.size(); side++) {
      std::vector<Isize>& current_side_index = side_interface_index[side];
      std::ranges::sort(current_side_index, {}, element_bound_lower);
      side_upper_prefix[side].reserve(current_side_index.size());
      Real running_upper = -kRealMax;
      for (const Isize interface_local_index : current_side_index) {
        running_upper = std::max(running_upper,
                                 this->element_range_[static_cast<Usize>(interface_local_index)].element_bound_upper_);
        side_upper_prefix[side].emplace_back(running_upper);
      }
    }
    std::map<std::vector<Isize>, Isize> cache_group_map;
    for (Usize side = 0; side < 2; side++) {
      const Usize target_side = 1 - side;
      const std::vector<Isize>& target_side_index = side_interface_index[target_side];
      const std::vector<Real>& target_side_upper_prefix = side_upper_prefix[target_side];
      for (const Isize interface_local_index : side_interface_index[side]) {
        const InterfaceElementRange& query_range = this->element_range_[static_cast<Usize>(interface_local_index)];
        const Real query_lower = query_range.quadrature_bound_lower_ - kRealTolerance;
        const Real query_upper = query_range.quadrature_bound_upper_ + kRealTolerance;
        // Search actual interval overlaps; sharing a coarse bin does not imply geometric overlap.
        const Isize begin = static_cast<Isize>(std::distance(
            target_side_upper_prefix.begin(), std::ranges::lower_bound(target_side_upper_prefix, query_lower)));
        const Isize end = static_cast<Isize>(
            std::distance(target_side_index.begin(),
                          std::ranges::upper_bound(target_side_index, query_upper, {}, element_bound_lower)));
        std::vector<Isize> matched_element_index;
        for (Isize j = begin; j < end; j++) {
          const Isize candidate_local_index = target_side_index[static_cast<Usize>(j)];
          if (this->element_range_[static_cast<Usize>(candidate_local_index)].element_bound_upper_ >= query_lower) {
            matched_element_index.emplace_back(interface_begin_index + candidate_local_index);
          }
        }
        // The paired original face always covers this face's quadrature points at initialization.
        if (matched_element_index.empty()) [[unlikely]] {
          throw std::runtime_error(std::format("Cannot find interface candidate element for interface local index {}.",
                                               interface_local_index));
        }
        std::ranges::sort(matched_element_index);
        const Isize cache_id = static_cast<Isize>(candidate_offset.size()) - 1;
        const auto [group_iterator, is_inserted] =
            cache_group_map.try_emplace(std::move(matched_element_index), cache_id);
        mesh.interface_candidate_group_(interface_local_index) = group_iterator->second;
        if (is_inserted) {
          candidate_index.insert(candidate_index.end(), group_iterator->first.begin(), group_iterator->first.end());
          candidate_offset.emplace_back(static_cast<Isize>(candidate_index.size()));
        }
      }
    }
  }
  mesh.interface_candidate_offset_.resize(static_cast<Isize>(candidate_offset.size()));
  mesh.interface_candidate_index_.resize(static_cast<Isize>(candidate_index.size()));
  std::ranges::copy(candidate_offset, mesh.interface_candidate_offset_.data());
  std::ranges::copy(candidate_index, mesh.interface_candidate_index_.data());
}

template <typename AdjacencyElementTrait>
inline void AdjacencyElementInterfaceCandidateCache<AdjacencyElementTrait>::compute(
    AdjacencyElementMesh<AdjacencyElementTrait>& mesh,
    const Eigen::Vector<Real, AdjacencyElementTrait::kDimension + 1>& rotation_center,
    const Eigen::Vector<Real, 3>& rotation_axis) {
  this->computeElementRange(mesh, rotation_center, rotation_axis);
  this->groupInterfaceSurface(mesh);
  this->buildCandidateCache(mesh);
}

template <typename AdjacencyElementTrait>
inline void AdjacencyElementMesh<AdjacencyElementTrait>::computeAdjacencyElementInterfaceCandidateCache(
    [[maybe_unused]] const Eigen::Vector<Real, AdjacencyElementTrait::kDimension + 1>& rotation_center,
    [[maybe_unused]] const Eigen::Vector<Real, 3>& rotation_axis) {
  if (this->interface_number_ == 0) {
    return;
  }
  this->interface_candidate_group_.resize(2 * this->interface_number_);
  if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
    const Isize interface_begin_index = this->interior_number_ + this->boundary_number_;
    this->interface_candidate_offset_.resize(3);
    this->interface_candidate_offset_ << 0, this->interface_number_, 2 * this->interface_number_;
    this->interface_candidate_index_.resize(2 * this->interface_number_);
    for (Isize i = 0; i < this->interface_number_; i++) {
      this->interface_candidate_group_(i) = 0;
      this->interface_candidate_group_(i + this->interface_number_) = 1;
      this->interface_candidate_index_(i) = interface_begin_index + this->interface_number_ + i;
      this->interface_candidate_index_(i + this->interface_number_) = interface_begin_index + i;
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
    AdjacencyElementInterfaceCandidateCache<AdjacencyElementTrait> interface_candidate_cache;
    interface_candidate_cache.compute(*this, rotation_center, rotation_axis);
  }
}

template <typename AdjacencyElementTrait>
inline void AdjacencyElementMesh<AdjacencyElementTrait>::computeAdjacencyElementInterfaceSelectionGeometry() {
  tbb::parallel_for(
      tbb::blocked_range<Isize>(0, 2 * this->interface_number_), [&](const tbb::blocked_range<Isize>& range) -> void {
        for (Isize i = range.begin(); i != range.end(); i++) {
          const Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kBasicNodeNumber>&
              basic_node_coordinate = this->node_coordinate_(this->interior_number_ + this->boundary_number_ + i)(
                  Eigen::placeholders::all, Eigen::seqN(0, AdjacencyElementTrait::kBasicNodeNumber));
          if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
            this->center_node_coordinate_(i) = basic_node_coordinate.rowwise().mean();
          } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
            Eigen::Vector<Real, AdjacencyElementTrait::kDimension + 1> vertical_vector =
                (basic_node_coordinate.col(1) - basic_node_coordinate.col(0))
                    .cross(basic_node_coordinate.col(3) - basic_node_coordinate.col(0));
            vertical_vector.normalize();
            this->vertical_vector_(i) = vertical_vector;
            for (Isize j = 0; j < AdjacencyElementTrait::kAdjacencyNumber; j++) {
              Eigen::Vector<Real, AdjacencyElementTrait::kDimension + 1> edge_direction =
                  basic_node_coordinate.col((j + 1) % AdjacencyElementTrait::kAdjacencyNumber) -
                  basic_node_coordinate.col(j);
              edge_direction.normalize();
              this->edge_normal_vector_(i).col(j) = vertical_vector.cross(edge_direction);
            }
          }
        }
      });
}

template <typename AdjacencyElementTrait>
[[nodiscard]] inline Isize
AdjacencyElementMesh<AdjacencyElementTrait>::getAdjacencyOppositeElementIndexFromGlobalCoordinate(
    const Eigen::Vector<Real, AdjacencyElementTrait::kDimension + 1>& rotation_center,
    const Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kDimension + 1>&
        rotation_matrix,
    const Eigen::Vector<Real, AdjacencyElementTrait::kDimension + 1>& global_coordinate,
    const Isize interface_local_index) const {
  Isize result_element_index = -1;
  Real minimum_distance_squared = kRealMax;
  const Isize cache_id = this->interface_candidate_group_(interface_local_index);
  const Isize begin = this->interface_candidate_offset_(cache_id);
  const Isize end = this->interface_candidate_offset_(cache_id + 1);
  // Here, for searches from the static side to the rotate side, we perform the calculations using inversely rotated
  // coordinates, avoiding the need to update the physical quantities used for lookups on the rotate side; for searches
  // from the rotate side to the static side, no changes are required.
  Eigen::Vector<Real, AdjacencyElementTrait::kDimension + 1> selection_coordinate;
  if (interface_local_index < this->interface_number_) {
    selection_coordinate = rotation_center + rotation_matrix.transpose() * (global_coordinate - rotation_center);
  } else {
    selection_coordinate = global_coordinate;
  }
  for (Isize i = begin; i < end; i++) {
    Real distance_squared = 0.0_r;
    const Isize interface_element_index = this->interface_candidate_index_(i);
    if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
      distance_squared =
          (this->center_node_coordinate_(interface_element_index - this->interior_number_ - this->boundary_number_)
               .array() -
           selection_coordinate.array())
              .matrix()
              .squaredNorm();
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
      const Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kBasicNodeNumber>&
          basic_node_coordinate = this->node_coordinate_initial_(interface_element_index)(
              Eigen::placeholders::all, Eigen::seqN(0, AdjacencyElementTrait::kBasicNodeNumber));
      const Eigen::Vector<Real, AdjacencyElementTrait::kDimension + 1>& vertical_vector =
          this->vertical_vector_(interface_element_index - this->interior_number_ - this->boundary_number_);
      const Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kAdjacencyNumber>&
          edge_normal_vector =
              this->edge_normal_vector_(interface_element_index - this->interior_number_ - this->boundary_number_);
      const Real height = vertical_vector.transpose() * (selection_coordinate - basic_node_coordinate.col(0));
      const Eigen::Vector<Real, AdjacencyElementTrait::kDimension + 1> projected_coordinate =
          selection_coordinate - height * vertical_vector;
      distance_squared = height * height;
      for (Isize j = 0; j < AdjacencyElementTrait::kAdjacencyNumber; j++) {
        const Real signed_distance_squared =
            edge_normal_vector.col(j).transpose() * (projected_coordinate - basic_node_coordinate.col(j));
        if (signed_distance_squared < -kRealTolerance) {
          distance_squared = kRealMax;
          break;
        }
      }
    }
    if (distance_squared < minimum_distance_squared) {
      minimum_distance_squared = distance_squared;
      result_element_index = interface_element_index;
    }
  }
  if (result_element_index < 0) {
    Real minimum_projection_distance_squared = kRealMax;
    const Real reference_tolerance = kRealTolerance;  // Dimensionless reference-domain tolerance.
    for (Isize k = begin; k < end; k++) {
      const Isize candidate = this->interface_candidate_index_(k);
      Eigen::Vector<Real, AdjacencyElementTrait::kDimension> local_coordinate;
      // The inverse map reads current nodes, so use the original current query, not selection_coordinate.
      this->computeAdjacencyElementLocalCoordinateFromGlobalCoordinate(global_coordinate, local_coordinate, candidate);
      if ((local_coordinate.array().abs() > 1.0_r + reference_tolerance).any()) {
        continue;
      }
      Eigen::Vector<Real, AdjacencyElementTrait::kAllBasisFunctionNumber> basis_function_value;
      this->computeLagrangeValue(local_coordinate, basis_function_value);
      const Eigen::Vector<Real, AdjacencyElementTrait::kDimension + 1> projected_coordinate =
          this->node_coordinate_(candidate) * basis_function_value;
      const Real distance_squared = (projected_coordinate - global_coordinate).squaredNorm();
      if (distance_squared < minimum_projection_distance_squared) {
        minimum_projection_distance_squared = distance_squared;
        result_element_index = candidate;
      }
    }
  }
  return result_element_index;
}

template <typename AdjacencyElementTrait>
[[nodiscard]] inline Isize
AdjacencyElementMeshDevice<AdjacencyElementTrait>::getAdjacencyOppositeElementIndexFromGlobalCoordinate(
    const Device::Vector<Real, AdjacencyElementTrait::kDimension + 1>& rotation_center,
    const Device::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kDimension + 1>&
        rotation_matrix,
    const Device::StaticVector<Real, AdjacencyElementTrait::kDimension + 1>& global_coordinate,
    const Isize interface_local_index) const {
  Isize result_element_index = -1;
  Real minimum_distance_squared = kRealMax;
  const Isize cache_id = this->interface_candidate_group_(interface_local_index);
  const Isize begin = this->interface_candidate_offset_(cache_id);
  const Isize end = this->interface_candidate_offset_(cache_id + 1);
  // Here, for searches from the static side to the rotate side, we perform the calculations using inversely rotated
  // coordinates, avoiding the need to update the physical quantities used for lookups on the rotate side; for searches
  // from the rotate side to the static side, no changes are required.
  Device::StaticVector<Real, AdjacencyElementTrait::kDimension + 1> selection_coordinate;
  const AdjacencyElementMeshDevice<AdjacencyElementTrait>* self = this;
  if (interface_local_index < this->interface_number_) {
    for (Isize m = 0; m < AdjacencyElementTrait::kDimension + 1; m++) {
      selection_coordinate(m) = rotation_center(m);
      for (Isize n = 0; n < AdjacencyElementTrait::kDimension + 1; n++) {
        selection_coordinate(m) += rotation_matrix(n, m) * (global_coordinate(n) - rotation_center(n));
      }
    }
  } else {
    for (Isize m = 0; m < AdjacencyElementTrait::kDimension + 1; m++) {
      selection_coordinate(m) = global_coordinate(m);
    }
  }
  for (Isize i = begin; i < end; i++) {
    Real distance_squared = 0.0_r;
    const Isize interface_element_index = this->interface_candidate_index_(i);
    if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
      const Device::View<const Device::Vector<Real, AdjacencyElementTrait::kDimension + 1>> center_node_coordinate =
          self->center_node_coordinate_.view(interface_element_index - this->interior_number_ - this->boundary_number_,
                                             2 * this->interface_number_);
      for (Isize m = 0; m < AdjacencyElementTrait::kDimension + 1; m++) {
        distance_squared += (center_node_coordinate(m) - selection_coordinate(m)) *
                            (center_node_coordinate(m) - selection_coordinate(m));
      }
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
      Device::StaticVector<Real, AdjacencyElementTrait::kDimension + 1> projected_coordinate;
      const Device::View<
          const Device::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kBasicNodeNumber>>
          basic_node_coordinate = self->node_coordinate_initial_.slice(
              interface_element_index, this->number_, Device::Slice<AdjacencyElementTrait::kDimension + 1>::all(),
              Device::Slice<AdjacencyElementTrait::kBasicNodeNumber>::seqN(0));
      const Device::View<const Device::Vector<Real, AdjacencyElementTrait::kDimension + 1>> vertical_vector =
          self->vertical_vector_.view(interface_element_index - this->interior_number_ - this->boundary_number_,
                                      2 * this->interface_number_);
      const Device::View<
          const Device::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kAdjacencyNumber>>
          edge_normal_vector = self->edge_normal_vector_.view(
              interface_element_index - this->interior_number_ - this->boundary_number_, 2 * this->interface_number_);
      Real height = 0.0_r;
      for (Isize m = 0; m < AdjacencyElementTrait::kDimension + 1; m++) {
        height += vertical_vector(m) * (selection_coordinate(m) - basic_node_coordinate(m, 0));
      }
      for (Isize m = 0; m < AdjacencyElementTrait::kDimension + 1; m++) {
        projected_coordinate(m) = selection_coordinate(m) - height * vertical_vector(m);
      }
      distance_squared = height * height;
      for (Isize j = 0; j < AdjacencyElementTrait::kAdjacencyNumber; j++) {
        Real signed_distance_squared = 0.0_r;
        for (Isize m = 0; m < AdjacencyElementTrait::kDimension + 1; m++) {
          signed_distance_squared += edge_normal_vector(m, j) * (projected_coordinate(m) - basic_node_coordinate(m, j));
        }
        if (signed_distance_squared < -kRealTolerance) {
          distance_squared = kRealMax;
          break;
        }
      }
    }
    if (distance_squared < minimum_distance_squared) {
      minimum_distance_squared = distance_squared;
      result_element_index = interface_element_index;
    }
  }
  if (result_element_index < 0) {
    Real minimum_projection_distance_squared = kRealMax;
    const Real reference_tolerance = kRealTolerance;  // Dimensionless reference-domain tolerance.
    for (Isize k = begin; k < end; k++) {
      const Isize candidate = this->interface_candidate_index_(k);
      Device::StaticVector<Real, AdjacencyElementTrait::kDimension> local_coordinate;
      // Match the current query to current nodes, as in the host fallback.
      this->computeAdjacencyElementLocalCoordinateFromGlobalCoordinate(global_coordinate, local_coordinate, candidate);
      bool valid_local_coordinate = true;
      for (Isize m = 0; m < AdjacencyElementTrait::kDimension; m++) {
        if (sycl::fabs(local_coordinate(m)) > 1.0_r + reference_tolerance) {
          valid_local_coordinate = false;
          break;
        }
      }
      if (!valid_local_coordinate) {
        continue;
      }
      Device::StaticVector<Real, AdjacencyElementTrait::kAllBasisFunctionNumber> basis_function_value;
      this->computeLagrangeValue(local_coordinate, basis_function_value);
      const Device::View<
          const Device::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kAllNodeNumber>>
          node_coordinate = this->node_coordinate_.view(candidate, this->number_);
      Real distance_squared = 0.0_r;
      for (Isize m = 0; m < AdjacencyElementTrait::kDimension + 1; m++) {
        Real projected_coordinate = 0.0_r;
        for (Isize n = 0; n < AdjacencyElementTrait::kAllNodeNumber; n++) {
          projected_coordinate += node_coordinate(m, n) * basis_function_value(n);
        }
        const Real difference = projected_coordinate - global_coordinate(m);
        distance_squared += difference * difference;
      }
      if (distance_squared < minimum_projection_distance_squared) {
        minimum_projection_distance_squared = distance_squared;
        result_element_index = candidate;
      }
    }
  }
  return result_element_index;
}

template <typename AdjacencyElementTrait>
inline void AdjacencyElementMesh<AdjacencyElementTrait>::computeAdjacencyElementLocalCoordinateFromGlobalCoordinate(
    const Eigen::Vector<Real, AdjacencyElementTrait::kDimension + 1>& global_coordinate,
    Eigen::Vector<Real, AdjacencyElementTrait::kDimension>& adjacency_local_coordinate,
    const Isize element_index) const {
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
    if (residual.norm() < kRealTolerance) {
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
    const Isize element_index) const {
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
    if (residual_norm < kRealTolerance) {
      return;
    }
    for (Isize m = 0; m < AdjacencyElementTrait::kDimension; m++) {
      adjacency_local_coordinate(m) -= residual(m);
    }
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GEOMETRY_CPP_
