/**
 * @file cal_output_var.hpp
 * @brief The head file to calculate the output variable.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-08-11
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CAL_OUTPUT_VAR_HPP_
#define SUBROSA_DG_CAL_OUTPUT_VAR_HPP_

#include <Eigen/Core>

#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "config/thermo_model.hpp"
#include "solver/variable/get_var_num.hpp"
#include "view/variable/get_output_var_num.hpp"

namespace SubrosaDG {

template <EquModel EquModelT>
  requires(EquModelT == EquModel::Euler || EquModelT == EquModel::NS)
inline void calOutputVar(const ThermoModel<EquModel::Euler>& thermo_model,
                         const Eigen::Vector<Real, getConservedVarNum<EquModelT>(2)>& conserved_var,
                         Eigen::Vector<Real, getOutputVarNum<EquModelT>(2)>& output_var) {
  const Real rho = conserved_var(0);
  const Real u = conserved_var(1) / rho;
  const Real v = conserved_var(2) / rho;
  const Real capital_t = (conserved_var(3) / rho - 0.5 * (u * u + v * v)) / thermo_model.c_v_;
  const Real p = rho * capital_t / thermo_model.gamma_;
  output_var << rho, u, v, p, capital_t;
}

template <EquModel EquModelT>
  requires(EquModelT == EquModel::Euler || EquModelT == EquModel::NS)
inline void calOutputVar(const ThermoModel<EquModel::Euler>& thermo_model,
                         const Eigen::Vector<Real, getConservedVarNum<EquModelT>(3)>& conserved_var,
                         Eigen::Vector<Real, getOutputVarNum<EquModelT>(3)>& output_var) {
  const Real rho = conserved_var(0);
  const Real u = conserved_var(1) / rho;
  const Real v = conserved_var(2) / rho;
  const Real w = conserved_var(3) / rho;
  const Real capital_t = (conserved_var(3) / rho - 0.5 * (u * u + v * v + w * w)) / thermo_model.c_v_;
  const Real p = rho * capital_t / thermo_model.gamma_;
  output_var << rho, u, v, w, p, capital_t;
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CAL_OUTPUT_VAR_HPP_
