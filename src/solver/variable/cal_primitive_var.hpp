/**
 * @file cal_primitive_var.hpp
 * @brief The head file to calculate the primitive variable.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-31
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CAL_PRIMITIVE_VAR_HPP_
#define SUBROSA_DG_CAL_PRIMITIVE_VAR_HPP_

#include <Eigen/Core>
#include <array>

#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "config/flow_var.hpp"
#include "config/thermo_model.hpp"

namespace SubrosaDG {

inline void calPrimitiveVar(const ThermoModel<EquModel::Euler>& thermo_model, const FarfieldVar<2>& farfield_var,
                            Eigen::Vector<Real, 5>& primitive_var) {
  const Real rho = farfield_var.rho_;
  const Real u = farfield_var.u_[0];
  const Real v = farfield_var.u_[1];
  const Real p = farfield_var.p_;
  const Real capital_e = (thermo_model.c_p_ - thermo_model.r_) * farfield_var.capital_t_ + 0.5 * (u * u + v * v);
  primitive_var << rho, u, v, p, capital_e;
}

inline void calPrimitiveVar(const ThermoModel<EquModel::Euler>& thermo_model, const FarfieldVar<3>& farfield_var,
                            Eigen::Vector<Real, 6>& primitive_var) {
  const Real rho = farfield_var.rho_;
  const Real u = farfield_var.u_[0];
  const Real v = farfield_var.u_[1];
  const Real w = farfield_var.u_[2];
  const Real p = farfield_var.p_;
  const Real capital_e =
      (thermo_model.c_p_ - thermo_model.r_) * farfield_var.capital_t_ + 0.5 * (u * u + v * v + w * w);
  primitive_var << rho, u, v, w, p, capital_e;
}

inline void calPrimitiveVar(const ThermoModel<EquModel::Euler>& thermo_model,
                            const Eigen::Vector<Real, 4>& conserved_var, Eigen::Vector<Real, 5>& primitive_var) {
  const Real rho = conserved_var(0);
  const Real u = conserved_var(1) / rho;
  const Real v = conserved_var(2) / rho;
  const Real capital_e = conserved_var(3) / rho;
  const Real p = (thermo_model.gamma_ - 1.0) * rho * (capital_e - 0.5 * (u * u + v * v));
  primitive_var << rho, u, v, p, capital_e;
}

inline void calPrimitiveVar(const ThermoModel<EquModel::Euler>& thermo_model,
                            const Eigen::Vector<Real, 5>& conserved_var, Eigen::Vector<Real, 6>& primitive_var) {
  const Real rho = conserved_var(0);
  const Real u = conserved_var(1) / rho;
  const Real v = conserved_var(2) / rho;
  const Real w = conserved_var(3) / rho;
  const Real capital_e = conserved_var(4) / rho;
  const Real p = (thermo_model.gamma_ - 1.0) * rho * (capital_e - 0.5 * (u * u + v * v + w * w));
  primitive_var << rho, u, v, w, p, capital_e;
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CAL_PRIMITIVE_VAR_HPP_
