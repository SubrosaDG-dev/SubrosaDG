/**
 * @file Quadrature.cpp
 * @brief The header file of Quadrature.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-06
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_QUADRATURE_CPP_
#define SUBROSA_DG_QUADRATURE_CPP_

#include <gmsh.h>

#include <Eigen/Core>
#include <format>
#include <utility>
#include <vector>

#include "Mesh/Polynomial.cpp"
#include "Utils/BasicDataType.cpp"
#include "Utils/Transformation.cpp"

namespace SubrosaDG {

struct GaussLegendre {
  static std::pair<std::vector<Real>, std::vector<Real>> get(const Isize degree) {
    const Isize alpha = 0;
    const Isize beta = 0;
    std::vector<Real> points = Jacobi::roots(degree, alpha, beta);
    std::vector<Real> weights(static_cast<Usize>(degree), 0.0_r);
    const Real factor = std::pow(2, alpha + beta + 1) * gamma(alpha + degree + 1) * gamma(beta + degree + 1) /
                        (gamma(degree + 1) * gamma(alpha + beta + degree + 1));
    for (Isize i = 0; i < degree; i++) {
      const Real pp = (degree + alpha + beta + 1) *
                      Jacobi::value(degree - 1, alpha + 1, beta + 1, points[static_cast<Usize>(i)]) / 2.0_r;
      weights[static_cast<Usize>(i)] =
          factor / (pp * pp * (1 - points[static_cast<Usize>(i)] * points[static_cast<Usize>(i)]));
    }
    return std::make_pair(points, weights);
  }
};

struct GaussLobattoLegendre {
  static std::pair<std::vector<Real>, std::vector<Real>> get(const Isize degree) {
    const Isize alpha = 0;
    const Isize beta = 0;
    std::vector<Real> points = Jacobi::roots(degree - 2, alpha + 1, beta + 1);
    points.insert(points.begin(), -1.0_r);
    points.push_back(1.0_r);
    std::vector<Real> weights(static_cast<Usize>(degree), 0.0_r);
    const Real factor = std::pow(2, alpha + beta + 1) * gamma(alpha + degree) * gamma(beta + degree) /
                        ((degree - 1) * gamma(degree) * gamma(alpha + beta + degree + 1));
    for (Isize i = 0; i < degree; i++) {
      const Real s = Jacobi::value(degree - 1, alpha, beta, points[static_cast<Usize>(i)]);
      weights[static_cast<Usize>(i)] = factor / (s * s);
    }
    weights[0] *= (beta + 1);
    weights[static_cast<Usize>(degree - 1)] *= (alpha + 1);
    return std::make_pair(points, weights);
  }
};

struct GaussRadauLegendre {
  static std::pair<std::vector<Real>, std::vector<Real>> get(const Isize degree) {
    const Isize alpha = 0;
    const Isize beta = 0;
    std::vector<Real> points = Jacobi::roots(degree - 1, alpha, beta + 1);
    points.insert(points.begin(), -1.0_r);
    std::vector<Real> weights(static_cast<Usize>(degree), 0.0_r);
    const Real factor = std::pow(2, alpha + beta) * gamma(alpha + degree) * gamma(beta + degree) /
                        ((beta + degree) * gamma(degree) * gamma(alpha + beta + degree + 1));
    for (Isize i = 0; i < degree; i++) {
      const Real s = Jacobi::value(degree - 1, alpha, beta, points[static_cast<Usize>(i)]);
      weights[static_cast<Usize>(i)] = factor * (1 - points[static_cast<Usize>(i)]) / (s * s);
    }
    weights[0] *= (beta + 1);
    return std::make_pair(points, weights);
  }
};

template <ElementEnum ElementType, int QuadratureOrder>
static std::pair<std::vector<double>, std::vector<double>> getGaussQuadrature() {
  constexpr int kElementGmshTypeNumber{getElementGmshTypeNumber<ElementType, 1>()};
  std::vector<double> local_coord;
  std::vector<double> weights;
  gmsh::model::mesh::getIntegrationPoints(kElementGmshTypeNumber, std::format("Gauss{}", QuadratureOrder), local_coord,
                                          weights);
  return std::make_pair(local_coord, weights);
}

template <typename ElementTrait>
struct ElementQuadrature {
  Eigen::Matrix<Real, ElementTrait::kDimension, ElementTrait::kQuadratureNumber> quadrature_local_coordinate_;
  Eigen::Vector<Real, ElementTrait::kQuadratureNumber> quadrature_weight_;

  ElementQuadrature() {
    const auto& [local_coordinate, weights] =
        getGaussQuadrature<ElementTrait::kElementType, ElementTrait::kQuadratureOrder>();
    for (Isize i = 0; i < ElementTrait::kQuadratureNumber; i++) {
      for (Isize j = 0; j < ElementTrait::kDimension; j++) {
        this->quadrature_local_coordinate_(j, i) = static_cast<Real>(local_coordinate[static_cast<Usize>(i * 3 + j)]);
      }
      this->quadrature_weight_(i) = static_cast<Real>(weights[static_cast<Usize>(i)]);
    }
  }
};

template <typename ElementTrait>
struct ElementQuadratureDevice {
  Device::Matrix<Real, ElementTrait::kDimension, ElementTrait::kQuadratureNumber> quadrature_local_coordinate_;
  Device::Vector<Real, ElementTrait::kQuadratureNumber> quadrature_weight_;

  void transferElementQuadratureToDevice(const ElementQuadrature<ElementTrait>& element_quadrature) {
    Utils::transferToDevice<Real, ElementTrait::kDimension, ElementTrait::kQuadratureNumber>(
        element_quadrature.quadrature_local_coordinate_, this->quadrature_local_coordinate_);
    Utils::transferToDevice<Real, ElementTrait::kQuadratureNumber, 1>(element_quadrature.quadrature_weight_,
                                                                      this->quadrature_weight_);
  }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_QUADRATURE_CPP_
