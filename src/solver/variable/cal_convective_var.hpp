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

#include <Eigen/Core>

#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "solver/variable/get_var_num.hpp"

namespace SubrosaDG {

template <EquModel EquModelT>
  requires(EquModelT == EquModel::Euler || EquModelT == EquModel::NS)
inline void calConvectiveVar(const Eigen::Vector<Real, getPrimitiveVarNum<EquModelT>(2)>& primitive_var,
                             Eigen::Matrix<Real, getConservedVarNum<EquModelT>(2), 2>& convective_var) {
  const Real rho = primitive_var(0);
  const Real u = primitive_var(1);
  const Real v = primitive_var(2);
  const Real p = primitive_var(3);
  const Real capital_e = primitive_var(4);
  convective_var.col(0) << rho * u, rho * u * u + p, rho * u * v, rho * u * capital_e + p * u;
  convective_var.col(1) << rho * v, rho * u * v, rho * v * v + p, rho * v * capital_e + p * v;
}

template <EquModel EquModelT>
  requires(EquModelT == EquModel::Euler || EquModelT == EquModel::NS)
inline void calConvectiveVar(const Eigen::Vector<Real, getPrimitiveVarNum<EquModelT>(3)>& primitive_var,
                             Eigen::Matrix<Real, getConservedVarNum<EquModelT>(3), 3>& convective_var) {
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
