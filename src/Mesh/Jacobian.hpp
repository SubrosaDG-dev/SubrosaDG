/**
 * @file Jacobian.hpp
 * @brief The header file of SubrosaDG Jacobian.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-14
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_JACOBIAN_HPP_
#define SUBROSA_DG_JACOBIAN_HPP_

#include <gmsh.h>

#include <Eigen/Core>
#include <cstddef>
#include <vector>

#include "Mesh/ReadControl.hpp"
#include "Utils/BasicDataType.hpp"

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

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_JACOBIAN_HPP_
