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

#include <Eigen/Core>

#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "config/flow_var.hpp"
#include "config/thermo_model.hpp"
#include "solver/variable/get_var_num.hpp"

namespace SubrosaDG {

template <EquModel EquModelT>
  requires(EquModelT == EquModel::Euler || EquModelT == EquModel::NS)
inline void calConservedVar(const ThermoModel<EquModelT>& thermo_model, const FlowVar<2, EquModelT>& flow_var,
                            Eigen::Vector<Real, getConservedVarNum<EquModelT>(2)>& conserved_var) {
  const Real rho = flow_var.rho_;
  const Real rho_u = rho * flow_var.u_[0];
  const Real rho_v = rho * flow_var.u_[1];
  const Real rho_capital_e = rho * (thermo_model.c_v_ * flow_var.capital_t_ +
                                    0.5 * (flow_var.u_[0] * flow_var.u_[0] + flow_var.u_[1] * flow_var.u_[1]));
  conserved_var << rho, rho_u, rho_v, rho_capital_e;
}

template <EquModel EquModelT>
  requires(EquModelT == EquModel::Euler || EquModelT == EquModel::NS)
inline void calConservedVar(const ThermoModel<EquModelT>& thermo_model, const FlowVar<3, EquModelT>& flow_var,
                            Eigen::Vector<Real, getConservedVarNum<EquModelT>(3)>& conserved_var) {
  const Real rho = flow_var.rho_;
  const Real rho_u = rho * flow_var.u_[0];
  const Real rho_v = rho * flow_var.u_[1];
  const Real rho_w = rho * flow_var.u_[2];
  const Real rho_capital_e =
      rho *
      (thermo_model.c_v_ * flow_var.capital_t_ +
       0.5 * (flow_var.u_[0] * flow_var.u_[0] + flow_var.u_[1] * flow_var.u_[1] + flow_var.u_[2] * flow_var.u_[2]));
  conserved_var << rho, rho_u, rho_v, rho_w, rho_capital_e;
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CAL_CONSERVED_VAR_HPP_
