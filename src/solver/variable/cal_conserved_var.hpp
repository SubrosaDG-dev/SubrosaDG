/**
 * @file cal_conserved_var.hpp
 * @brief The head file to calculate the conserved variable.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-31
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CAL_CONSERVED_VAR_HPP_
#define SUBROSA_DG_CAL_CONSERVED_VAR_HPP_

// clang-format off

#include <Eigen/Core>           // for Matrix, CommaInitializer, Vector, DenseBase
#include <array>                // for array

#include "basic/data_type.hpp"  // for Real
#include "basic/config.hpp"     // for InitVar, ThermoModel
#include "basic/enum.hpp"       // for EquModel

// clang-format on

namespace SubrosaDG {

inline void calConservedVar(const ThermoModel<EquModel::Euler>& thermo_model, const InitVar<2>& init_var,
                            Eigen::Vector<Real, 4>& conserved_var) {
  const Real rho = init_var.rho_;
  const Real rho_u = rho * init_var.u_[0];
  const Real rho_v = rho * init_var.u_[1];
  const Real rho_capital_e = rho * ((thermo_model.c_p_ - thermo_model.r_) * init_var.capital_t_ +
                                    0.5 * (init_var.u_[0] * init_var.u_[0] + init_var.u_[1] * init_var.u_[1]));
  conserved_var << rho, rho_u, rho_v, rho_capital_e;
}

inline void calConservedVar(const ThermoModel<EquModel::Euler>& thermo_model, const InitVar<3>& init_var,
                            Eigen::Vector<Real, 5>& conserved_var) {
  const Real rho = init_var.rho_;
  const Real rho_u = rho * init_var.u_[0];
  const Real rho_v = rho * init_var.u_[1];
  const Real rho_w = rho * init_var.u_[2];
  const Real rho_capital_e =
      rho *
      ((thermo_model.c_p_ - thermo_model.r_) * init_var.capital_t_ +
       0.5 * (init_var.u_[0] * init_var.u_[0] + init_var.u_[1] * init_var.u_[1] + init_var.u_[2] * init_var.u_[2]));
  conserved_var << rho, rho_u, rho_v, rho_w, rho_capital_e;
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CAL_CONSERVED_VAR_HPP_
