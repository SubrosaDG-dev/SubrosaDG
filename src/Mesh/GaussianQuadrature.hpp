/**
 * @file GaussianQuadrature.hpp
 * @brief The header file of GaussianQuadrature.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-06
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_GAUSSIAN_QUADRATURE_HPP_
#define SUBROSA_DG_GAUSSIAN_QUADRATURE_HPP_

#include <gmsh.h>

#include <Eigen/Core>
#include <format>
#include <utility>
#include <vector>

#include "Utils/BasicDataType.hpp"

namespace SubrosaDG {

template <typename ElementTrait>
struct ElementGaussianQuadrature {
  std::vector<double> local_coord_;
  Eigen::Matrix<Real, ElementTrait::kDimension, ElementTrait::kQuadratureNumber> node_coordinate_;
  Eigen::Vector<Real, ElementTrait::kQuadratureNumber> weight_;

  inline ElementGaussianQuadrature();
};

template <typename ElementTrait>
inline std::pair<std::vector<double>, std::vector<double>> getElementGaussianQuadrature() {
  std::vector<double> local_coord;
  std::vector<double> weights;
  gmsh::model::mesh::getIntegrationPoints(ElementTrait::kGmshTypeNumber,
                                          std::format("Gauss{}", ElementTrait::kQuadratureOrder), local_coord, weights);
  return std::make_pair(local_coord, weights);
}

template <typename ElementTrait>
inline ElementGaussianQuadrature<ElementTrait>::ElementGaussianQuadrature() {
  const auto [local_coord, weights] = getElementGaussianQuadrature<ElementTrait>();
  this->local_coord_ = local_coord;
  for (Isize i = 0; i < ElementTrait::kQuadratureNumber; i++) {
    for (Isize j = 0; j < ElementTrait::kDimension; j++) {
      this->node_coordinate_(j, i) = static_cast<Real>(local_coord[static_cast<Usize>(i * 3 + j)]);
    }
    this->weight_(i) = static_cast<Real>(weights[static_cast<Usize>(i)]);
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GAUSSIAN_QUADRATURE_HPP_
