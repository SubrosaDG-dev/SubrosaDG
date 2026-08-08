/**
 * @file Polynomial.cpp
 * @brief The header file of SubrosaDG polynomial.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2026-04-17
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_POLYNOMIAL_CPP_
#define SUBROSA_DG_POLYNOMIAL_CPP_

#include <gmsh.h>

#include <cmath>
#include <string>
#include <vector>

#include "Solver/SimulationControl.cpp"
#include "Utils/BasicDataType.cpp"
#include "Utils/Constant.cpp"
#include "Utils/Enum.cpp"

namespace SubrosaDG {

Real gamma(const Isize n) {
  Real result = static_cast<Real>(n - 1);
  for (Isize i = n - 2; i > 0; i--) {
    result *= static_cast<Real>(i);
  }
  return result;
}

struct Jacobi {
  static Real value(const Isize degree, const int alpha, const int beta, const Real x) {
    Real p0 = 1.0_r;
    if (degree == 0) {
      return p0;
    }
    Real p1 = ((alpha + beta + 2) * x + (alpha - beta)) / 2.0_r;
    if (degree == 1) {
      return p1;
    }

    for (Isize i = 1; i < degree; i++) {
      const Real v = 2.0_r * i + (alpha + beta);
      const Real a1 = 2.0_r * (i + 1.0_r) * (i + (alpha + beta + 1.0_r)) * v;
      const Real a2 = (v + 1.0_r) * (alpha * alpha - beta * beta);
      const Real a3 = v * (v + 1.0_r) * (v + 2.0_r);
      const Real a4 = 2.0_r * (i + alpha) * (i + beta) * (v + 2.0_r);

      const Real pn = ((a2 + a3 * x) * p1 - a4 * p0) / a1;
      p0 = p1;
      p1 = pn;
    }
    return p1;
  }

  static std::vector<Real> roots(const Isize degree, const int alpha, const int beta) {
    const Real tolerance = 4.0_r * kRealEpsilon;
    const Usize n_points = (alpha == beta ? static_cast<Usize>(degree) / 2 : static_cast<Usize>(degree));
    std::vector<Real> x(static_cast<Usize>(degree), 0.0_r);
    for (Usize k = 0; k < n_points; k++) {
      Real r = -std::cos(static_cast<Real>(2 * k + 1) / (2 * degree) * kPi);
      if (k > 0) {
        r = (r + x[k - 1]) / 2.0_r;
      }
      for (Isize iter = 1; iter < 1000; iter++) {
        Real s = 0.0_r;
        for (Usize i = 0; i < k; i++) {
          s += 1.0_r / (r - x[i]);
        }
        const Real derivative = (alpha + beta + degree + 1.0_r) * value(degree - 1, alpha + 1, beta + 1, r) / 2.0_r;
        const Real f = value(degree, alpha, beta, r);
        const Real delta = f / (f * s - derivative);
        r += delta;
        if (std::abs(delta) < tolerance) {
          break;
        }
      }
      x[k] = r;
    }
    for (Usize k = n_points; k < static_cast<Usize>(degree); k++) {
      x[k] = -x[static_cast<Usize>(degree) - k - 1];
    }
    return x;
  }
};

template <ElementEnum ElementType, int PolynomialOrder>
struct Lagrange {
  static std::vector<Real> points() {
    constexpr int kElementGmshTypeNumber{getElementGmshTypeNumber<ElementType, PolynomialOrder>()};
    std::string element_name;
    int dim;
    int order;
    int num_nodes;
    std::vector<double> local_node_coord;
    int num_primary_nodes;
    gmsh::model::mesh::getElementProperties(kElementGmshTypeNumber, element_name, dim, order, num_nodes,
                                            local_node_coord, num_primary_nodes);
    return std::vector<Real>{local_node_coord.begin(), local_node_coord.end()};
  }

  static std::vector<Isize> monomials() {
    std::vector<Isize> monomials;
    if constexpr (ElementType == ElementEnum::Line) {
      for (Isize i = 0; i <= PolynomialOrder; i++) {
        monomials.emplace_back(i);
      }
    } else if constexpr (ElementType == ElementEnum::Triangle) {
      for (Isize j = 0; j <= PolynomialOrder; j++) {
        for (Isize i = 0; i <= PolynomialOrder - j; i++) {
          monomials.emplace_back(i);
          monomials.emplace_back(j);
        }
      }
    } else if constexpr (ElementType == ElementEnum::Quadrangle) {
      for (Isize j = 0; j <= PolynomialOrder; j++) {
        for (Isize i = 0; i <= PolynomialOrder; i++) {
          monomials.emplace_back(i);
          monomials.emplace_back(j);
        }
      }
    } else if constexpr (ElementType == ElementEnum::Tetrahedron) {
      for (Isize k = 0; k <= PolynomialOrder; k++) {
        for (Isize j = 0; j <= PolynomialOrder - k; j++) {
          for (Isize i = 0; i <= PolynomialOrder - j - k; i++) {
            monomials.emplace_back(i);
            monomials.emplace_back(j);
            monomials.emplace_back(k);
          }
        }
      }
    } else if constexpr (ElementType == ElementEnum::Pyramid) {
      for (Isize k = 0; k <= PolynomialOrder; k++) {
        for (Isize j = 0; j <= PolynomialOrder - k; j++) {
          for (Isize i = 0; i <= PolynomialOrder - k; i++) {
            monomials.emplace_back(i);
            monomials.emplace_back(j);
            monomials.emplace_back(k);
          }
        }
      }
    } else if constexpr (ElementType == ElementEnum::Hexahedron) {
      for (Isize k = 0; k <= PolynomialOrder; k++) {
        for (Isize j = 0; j <= PolynomialOrder; j++) {
          for (Isize i = 0; i <= PolynomialOrder; i++) {
            monomials.emplace_back(i);
            monomials.emplace_back(j);
            monomials.emplace_back(k);
          }
        }
      }
    }
    return monomials;
  }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_POLYNOMIAL_CPP_
