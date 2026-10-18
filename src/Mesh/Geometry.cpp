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
#include <Eigen/Geometry>
#include <Eigen/LU>
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <map>
#include <numeric>
#include <utility>
#include <vector>

#include "Mesh/Polynomial.cpp"
#include "Mesh/ReadControl.cpp"
#include "Utils/BasicDataType.cpp"
#include "Utils/Concept.cpp"
#include "Utils/Constant.cpp"

namespace SubrosaDG {

template <typename VolumeElementTrait>
inline void VolumeElementMesh<VolumeElementTrait>::getVolumeElementQuality() {
  std::vector<std::size_t> element_tags(static_cast<std::size_t>(this->number_));
  for (Usize i = 0; i < static_cast<Usize>(this->number_); i++) {
    element_tags[i] = static_cast<std::size_t>(this->gmsh_tag_(i));
  }
  std::vector<double> element_min_edge;
  gmsh::model::mesh::getElementQualities(element_tags, element_min_edge, "minEdge");
  for (Usize i = 0; i < static_cast<Usize>(this->number_); i++) {
    this->minimum_edge_(i) = static_cast<Real>(element_min_edge[i]);
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

struct InterfaceRange {
  bool is_endcap_;
  Real surface_coordinate_;
  Real lower_;
  Real upper_;
  Real query_lower_;
  Real query_upper_;
};

template <typename AdjacencyElementTrait>
[[nodiscard]] inline Real getInterfaceMinimumRadius(
    std::vector<std::array<Real, AdjacencyElementTrait::kDimension>> points) {
  std::ranges::sort(points);
  points.erase(std::ranges::unique(points).begin(), points.end());
  std::vector<Eigen::Vector<Real, AdjacencyElementTrait::kDimension>> hull;
  hull.reserve(2 * points.size());
  // Build the lower and upper chains of the convex hull in opposite directions.
  for (const auto& coordinates : points) {
    const Eigen::Vector<Real, AdjacencyElementTrait::kDimension> point{coordinates[0], coordinates[1]};
    while (hull.size() >= 2) {
      const Eigen::Vector<Real, AdjacencyElementTrait::kDimension> a = hull.back() - hull[hull.size() - 2];
      const Eigen::Vector<Real, AdjacencyElementTrait::kDimension> b = point - hull.back();
      if (a.x() * b.y() - a.y() * b.x() > 0.0_r) {
        break;
      }
      hull.pop_back();
    }
    hull.push_back(point);
  }
  const std::size_t lower_size = hull.size();
  for (Isize i = static_cast<Isize>(points.size()) - 2; i >= 0; i--) {
    const auto& coordinates = points[static_cast<Usize>(i)];
    const Eigen::Vector<Real, AdjacencyElementTrait::kDimension> point{coordinates[0], coordinates[1]};
    while (hull.size() > lower_size) {
      const Eigen::Vector<Real, AdjacencyElementTrait::kDimension> a = hull.back() - hull[hull.size() - 2];
      const Eigen::Vector<Real, AdjacencyElementTrait::kDimension> b = point - hull.back();
      if (a.x() * b.y() - a.y() * b.x() > 0.0_r) {
        break;
      }
      hull.pop_back();
    }
    hull.push_back(point);
  }
  hull.pop_back();
  bool contains_origin = true;
  Real distance_squared = kRealMax;
  for (Usize i = 0; i < hull.size(); i++) {
    const auto& a = hull[i];
    const Eigen::Vector<Real, AdjacencyElementTrait::kDimension> edge = hull[(i + 1) % hull.size()] - a;
    contains_origin = contains_origin && edge.x() * (-a.y()) - edge.y() * (-a.x()) >= 0.0_r;
    const Real parameter = std::clamp(-a.dot(edge) / edge.squaredNorm(), 0.0_r, 1.0_r);
    distance_squared = std::min(distance_squared, (a + parameter * edge).squaredNorm());
  }
  return contains_origin ? 0.0_r : std::sqrt(distance_squared);
}

template <typename AdjacencyElementTrait>
inline void AdjacencyElementMesh<AdjacencyElementTrait>::initializeInterfaceCandidateCache(
    [[maybe_unused]] const Eigen::Vector<Real, AdjacencyElementTrait::kDimension + 1>& rotation_center,
    [[maybe_unused]] const Eigen::Vector<Real, 3>& rotation_axis) {
  if (this->interface_number_ == 0) {
    return;
  }
  this->interface_candidate_group_.resize(2 * this->interface_number_);
  if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
    this->interface_candidate_offset_.resize(3);
    this->interface_candidate_offset_ << 0, this->interface_number_, 2 * this->interface_number_;
    this->interface_candidate_index_.resize(2 * this->interface_number_);
    for (Isize i = 0; i < this->interface_number_; i++) {
      this->interface_candidate_group_(i) = 0;
      this->interface_candidate_group_(i + this->interface_number_) = 1;
      this->interface_candidate_index_(i) =
          this->interior_number_ + this->boundary_number_ + this->interface_number_ + i;
      this->interface_candidate_index_(i + this->interface_number_) =
          this->interior_number_ + this->boundary_number_ + i;
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
    const Eigen::Vector<Real, 3> transverse_x = rotation_axis.unitOrthogonal();
    const Eigen::Vector<Real, 3> transverse_y = rotation_axis.cross(transverse_x);
    // A tensor Bernstein control net bounds the entire high-order face, not just its nodes.
    Eigen::Matrix<Real, AdjacencyElementTrait::kAllNodeNumber, AdjacencyElementTrait::kAllNodeNumber> bernstein_matrix;
    for (Isize i = 0; i < AdjacencyElementTrait::kAllNodeNumber; i++) {
      const Real u = (this->lagrange_node_coordinate_(0, i) + 1.0_r) / 2.0_r;
      const Real v = (this->lagrange_node_coordinate_(1, i) + 1.0_r) / 2.0_r;
      for (Isize j = 0; j <= AdjacencyElementTrait::kPolynomialOrder; j++) {
        for (Isize k = 0; k <= AdjacencyElementTrait::kPolynomialOrder; k++) {
          bernstein_matrix(i, j * (AdjacencyElementTrait::kPolynomialOrder + 1) + k) =
              Bernstein<AdjacencyElementTrait::kPolynomialOrder>::value(j, u) *
              Bernstein<AdjacencyElementTrait::kPolynomialOrder>::value(k, v);
        }
      }
    }
    const Eigen::Matrix<Real, AdjacencyElementTrait::kAllNodeNumber, AdjacencyElementTrait::kAllNodeNumber>
        bernstein_inverse = bernstein_matrix.inverse();
    std::vector<InterfaceRange> ranges(static_cast<Usize>(2 * this->interface_number_));
    for (Isize i = 0; i < 2 * this->interface_number_; i++) {
      const Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kAllNodeNumber>
          coordinates = this->node_coordinate_initial_(this->interior_number_ + this->boundary_number_ + i).colwise() -
                        rotation_center;
      const Eigen::Matrix<Real, 1, AdjacencyElementTrait::kAllNodeNumber> heights =
          rotation_axis.transpose() * coordinates;
      const Eigen::Matrix<Real, 1, AdjacencyElementTrait::kAllNodeNumber> radii =
          (coordinates - rotation_axis * heights).colwise().norm();
      const bool is_endcap = heights.maxCoeff() - heights.minCoeff() <= kRealTolerance;
      const Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kAllNodeNumber> control =
          coordinates * bernstein_inverse.transpose();
      // Only quadrature points query the opposite side. Their axial coordinate and radius are rotation invariant.
      const Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kQuadratureNumber>
          query_coordinates = coordinates * this->nodal_basis_function_.transpose();
      const Eigen::Matrix<Real, 1, AdjacencyElementTrait::kQuadratureNumber> query_heights =
          rotation_axis.transpose() * query_coordinates;
      InterfaceRange& range = ranges[static_cast<Usize>(i)];
      range.is_endcap_ = is_endcap;
      range.surface_coordinate_ = is_endcap ? heights.mean() : radii.mean();
      if (is_endcap) {
        // The control-net convex hull bounds the whole face, including its interior radial minimum.
        const Eigen::Matrix<Real, 1, AdjacencyElementTrait::kAllNodeNumber> x = transverse_x.transpose() * control;
        const Eigen::Matrix<Real, 1, AdjacencyElementTrait::kAllNodeNumber> y = transverse_y.transpose() * control;
        std::vector<std::array<Real, AdjacencyElementTrait::kDimension>> points;
        points.reserve(AdjacencyElementTrait::kAllNodeNumber);
        for (Isize j = 0; j < AdjacencyElementTrait::kAllNodeNumber; j++) {
          points.push_back({x(j), y(j)});
        }
        range.lower_ = getInterfaceMinimumRadius<AdjacencyElementTrait>(std::move(points));
        range.upper_ = (x.array().square() + y.array().square()).sqrt().maxCoeff();
        const Eigen::Matrix<Real, 1, AdjacencyElementTrait::kQuadratureNumber> query_radii =
            (query_coordinates - rotation_axis * query_heights).colwise().norm();
        range.query_lower_ = query_radii.minCoeff();
        range.query_upper_ = query_radii.maxCoeff();
      } else {
        const Eigen::Matrix<Real, 1, AdjacencyElementTrait::kAllNodeNumber> z = rotation_axis.transpose() * control;
        range.lower_ = z.minCoeff();
        range.upper_ = z.maxCoeff();
        range.query_lower_ = query_heights.minCoeff();
        range.query_upper_ = query_heights.maxCoeff();
      }
    }
    // The mesh reader creates static i and rotate i from the same original interface face.
    std::vector<Isize> parents(static_cast<Usize>(this->interface_number_));
    std::ranges::iota(parents, 0);
    const auto root = [&](Isize i) -> Isize {
      while (parents[static_cast<Usize>(i)] != i) {
        parents[static_cast<Usize>(i)] = parents[static_cast<Usize>(parents[static_cast<Usize>(i)])];
        i = parents[static_cast<Usize>(i)];
      }
      return i;
    };
    std::map<std::pair<Isize, Isize>, std::vector<Isize>> edge_faces;
    for (Isize i = 0; i < this->interface_number_; i++) {
      for (Isize edge = 0; edge < AdjacencyElementTrait::kAdjacencyNumber; edge++) {
        const Isize a = this->node_tag_(this->interior_number_ + this->boundary_number_ + i)(edge);
        const Isize b = this->node_tag_(this->interior_number_ + this->boundary_number_ +
                                        i)((edge + 1) % AdjacencyElementTrait::kAdjacencyNumber);
        auto& neighbors = edge_faces[{std::min(a, b), std::max(a, b)}];
        for (const Isize neighbor : neighbors) {
          const InterfaceRange& face_range = ranges[static_cast<Usize>(i)];
          const InterfaceRange& neighbor_range = ranges[static_cast<Usize>(neighbor)];
          if (face_range.is_endcap_ == neighbor_range.is_endcap_ &&
              std::abs(face_range.surface_coordinate_ - neighbor_range.surface_coordinate_) <= kRealTolerance) {
            parents[static_cast<Usize>(root(i))] = root(neighbor);
          }
        }
        neighbors.push_back(i);
      }
    }
    std::map<Isize, std::vector<Isize>> surface_faces;
    for (Isize i = 0; i < this->interface_number_; i++) {
      surface_faces[root(i)].push_back(i);
      surface_faces[root(i)].push_back(i + this->interface_number_);
    }
    const auto lower_coordinate = [&](const Isize index) -> Real { return ranges[static_cast<Usize>(index)].lower_; };
    std::vector<Isize> offsets{0};
    std::vector<Isize> candidates;
    for (const auto& [surface, faces] : surface_faces) {
      std::array<std::vector<Isize>, 2> side_faces;
      std::array<std::vector<Real>, 2> upper_prefix;
      for (const Isize i : faces) {
        side_faces[i < this->interface_number_ ? 0 : 1].push_back(i);
      }
      for (Usize side = 0; side < side_faces.size(); side++) {
        auto& entries = side_faces[side];
        std::ranges::sort(entries, {}, lower_coordinate);
        Real upper = -kRealMax;
        for (const Isize i : entries) {
          upper = std::max(upper, ranges[static_cast<Usize>(i)].upper_);
          upper_prefix[side].push_back(upper);
        }
      }
      std::map<std::vector<Isize>, Isize> cache_groups;
      for (const Isize i : faces) {
        const InterfaceRange& query = ranges[static_cast<Usize>(i)];
        const Usize target_side = i < this->interface_number_ ? 1 : 0;
        const auto& entries = side_faces[target_side];
        const auto& prefix = upper_prefix[target_side];
        const Real lower = query.query_lower_ - kRealTolerance;
        const Real upper = query.query_upper_ + kRealTolerance;
        // Search actual interval overlaps; sharing a coarse bin does not imply geometric overlap.
        const auto begin = std::ranges::lower_bound(prefix, lower) - prefix.begin();
        const auto end = std::ranges::upper_bound(entries, upper, {}, lower_coordinate) - entries.begin();
        std::vector<Isize> matched;
        for (auto j = begin; j < end; j++) {
          const Isize index = entries[static_cast<Usize>(j)];
          if (ranges[static_cast<Usize>(index)].upper_ >= lower) {
            matched.push_back(this->interior_number_ + this->boundary_number_ + index);
          }
        }
        // The paired original face always covers this face's quadrature points at initialization.
        assert(!matched.empty());
        std::ranges::sort(matched);
        const auto cache_id = static_cast<Isize>(offsets.size() - 1);
        const auto [group, inserted] = cache_groups.try_emplace(matched, cache_id);
        this->interface_candidate_group_(i) = group->second;
        if (inserted) {
          candidates.insert(candidates.end(), matched.begin(), matched.end());
          offsets.push_back(static_cast<Isize>(candidates.size()));
        }
      }
    }
    this->interface_candidate_offset_.resize(static_cast<Isize>(offsets.size()));
    this->interface_candidate_index_.resize(static_cast<Isize>(candidates.size()));
    std::ranges::copy(offsets, this->interface_candidate_offset_.data());
    std::ranges::copy(candidates, this->interface_candidate_index_.data());
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
[[nodiscard]] inline Isize AdjacencyElementMesh<AdjacencyElementTrait>::getAdjacencyOppositeElementIndexFromGlobalCoordinate(
    const Eigen::Vector<Real, AdjacencyElementTrait::kDimension + 1>& rotation_center,
    const Eigen::Matrix<Real, AdjacencyElementTrait::kDimension + 1, AdjacencyElementTrait::kDimension + 1>&
        rotation_matrix,
    const Eigen::Vector<Real, AdjacencyElementTrait::kDimension + 1>& global_coordinate,
    const Isize interface_local_index) const {
  Isize result_element_index = -1;
  Real minimum_distance = kRealMax;
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
    Real distance = 0.0_r;
    const Isize interface_element_index = this->interface_candidate_index_(i);
    if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
      distance =
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
      distance = height * height;
      for (Isize j = 0; j < AdjacencyElementTrait::kAdjacencyNumber; j++) {
        const Real signed_distance =
            edge_normal_vector.col(j).transpose() * (projected_coordinate - basic_node_coordinate.col(j));
        if (signed_distance < -kRealTolerance) {
          distance = kRealMax;
          break;
        }
      }
    }
    if (distance < minimum_distance) {
      minimum_distance = distance;
      result_element_index = interface_element_index;
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
  Real minimum_distance = kRealMax;
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
    Real distance = 0.0_r;
    const Isize interface_element_index = this->interface_candidate_index_(i);
    if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
      const Device::View<const Device::Vector<Real, AdjacencyElementTrait::kDimension + 1>> center_node_coordinate =
          self->center_node_coordinate_.view(interface_element_index - this->interior_number_ - this->boundary_number_,
                                             2 * this->interface_number_);
      for (Isize m = 0; m < AdjacencyElementTrait::kDimension + 1; m++) {
        distance += (center_node_coordinate(m) - selection_coordinate(m)) *
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
      distance = height * height;
      for (Isize j = 0; j < AdjacencyElementTrait::kAdjacencyNumber; j++) {
        Real signed_distance = 0.0_r;
        for (Isize m = 0; m < AdjacencyElementTrait::kDimension + 1; m++) {
          signed_distance += edge_normal_vector(m, j) * (projected_coordinate(m) - basic_node_coordinate(m, j));
        }
        if (signed_distance < -kRealTolerance) {
          distance = kRealMax;
          break;
        }
      }
    }
    if (distance < minimum_distance) {
      minimum_distance = distance;
      result_element_index = interface_element_index;
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
