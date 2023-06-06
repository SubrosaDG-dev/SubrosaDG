/**
 * @file cal_convective_var.hpp
 * @brief The head file to calculate the convective variables.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-17
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CAL_CONVECTIVE_VAR_HPP_
#define SUBROSA_DG_CAL_CONVECTIVE_VAR_HPP_

// clang-format off

#include <Eigen/Core>           // for Block, CommaInitializer, Vector, Matrix, DenseCoeffsBase, DenseBase, DenseBas...

#include "basic/data_type.hpp"  // for Real

// clang-format on

namespace SubrosaDG {

inline void calConvectiveVar(const Eigen::Vector<Real, 5>& primitive_var, Eigen::Matrix<Real, 4, 2>& convective_var) {
  const Real rho = primitive_var(0);
  const Real u = primitive_var(1);
  const Real v = primitive_var(2);
  const Real p = primitive_var(3);
  const Real capital_e = primitive_var(4);
  convective_var.col(0) << rho * u, rho * u * u + p, rho * u * v, rho * u * capital_e + p * u;
  convective_var.col(1) << rho * v, rho * u * v, rho * v * v + p, rho * v * capital_e + p * v;
}

inline void calConvectiveVar(const Eigen::Vector<Real, 6>& primitive_var, Eigen::Matrix<Real, 5, 3>& convective_var) {
  const Real rho = primitive_var(0);
  const Real u = primitive_var(1);
  const Real v = primitive_var(2);
  const Real w = primitive_var(3);
  const Real p = primitive_var(4);
  const Real capital_e = primitive_var(5);
  convective_var.col(0) << rho * u, rho * u * u + p, rho * u * v, rho * u * w, rho * u * capital_e + p * u;
  convective_var.col(1) << rho * v, rho * u * v, rho * v * v + p, rho * v * w, rho * v * capital_e + p * v;
  convective_var.col(2) << rho * w, rho * u * w, rho * v * w, rho * w * w + p, rho * w * capital_e + p * w;
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CAL_CONVECTIVE_VAR_HPP_
