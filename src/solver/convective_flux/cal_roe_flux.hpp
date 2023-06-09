/**
 * @file cal_roe_flux.hpp
 * @brief The head file to calculate the Roe flux.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-30
 *
 * @version 0.1.0
 * @copyr Copyr (c) 2022 - 2023 by SubrosaDG developers. All rs reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CAL_ROE_FLUX_HPP_
#define SUBROSA_DG_CAL_ROE_FLUX_HPP_

#include <Eigen/Core>
#include <cmath>

#include "basic/data_type.hpp"
#include "solver/variable/cal_convective_var.hpp"

namespace SubrosaDG {

enum class EquModel;
template <EquModel EquModelT>
struct ThermoModel;

template <EquModel EquModelT>
inline void calRoeFlux(const ThermoModel<EquModelT>& thermo_model, const Eigen::Vector<Real, 2>& norm_vec,
                       const Eigen::Vector<Real, 5>& l_primitive_var, const Eigen::Vector<Real, 5>& r_primitive_var,
                       Eigen::Vector<Real, 4>& roe_flux) {
  const Real l_sqrt_rho = std::sqrt(l_primitive_var(0));
  const Real r_sqrt_rho = std::sqrt(r_primitive_var(0));
  const Real rho_average = l_sqrt_rho + r_sqrt_rho;
  const Real roe_rho = std::sqrt(l_primitive_var(0) * r_primitive_var(0));
  const Real roe_u = (l_primitive_var(1) * l_sqrt_rho + r_primitive_var(1) * r_sqrt_rho) / rho_average;
  const Real roe_v = (l_primitive_var(2) * l_sqrt_rho + r_primitive_var(2) * r_sqrt_rho) / rho_average;
  const Real l_capital_h = l_primitive_var(4) + l_primitive_var(3) / l_primitive_var(0);
  const Real r_capital_h = r_primitive_var(4) + r_primitive_var(3) / r_primitive_var(0);
  const Real roe_capital_h = (l_capital_h * l_sqrt_rho + r_capital_h * r_sqrt_rho) / rho_average;
  const Real roe_q2 = roe_u * roe_u + roe_v * roe_v;
  const Real roe_norm_q = roe_u * norm_vec.x() + roe_v * norm_vec.y();
  const Real roe_a = std::sqrt((thermo_model.gamma_ - 1.0) * (roe_capital_h - 0.5 * roe_q2));
  const Real delta_rho = r_primitive_var(0) - l_primitive_var(0);
  const Real delta_u = r_primitive_var(1) - l_primitive_var(1);
  const Real delta_v = r_primitive_var(2) - l_primitive_var(2);
  const Real delta_p = r_primitive_var(3) - l_primitive_var(3);
  const Real delta_norm_q = delta_u * norm_vec.x() + delta_v * norm_vec.y();
  Eigen::Vector<Real, 4> roe_var_f1;
  roe_var_f1 << 1.0, roe_u - roe_a * norm_vec.x(), roe_v - roe_a * norm_vec.y(), roe_capital_h - roe_a * roe_norm_q;
  roe_var_f1 *= std::fabs(roe_norm_q - roe_a) * (delta_p - roe_rho * roe_a * delta_norm_q) / (2.0 * roe_a * roe_a);
  Eigen::Vector<Real, 4> roe_var_f2;
  roe_var_f2 << 1, roe_u, roe_v, 0.5 * roe_q2;
  roe_var_f2 *= delta_rho - delta_p / (roe_a * roe_a);
  Eigen::Vector<Real, 4> roe_var_f34;
  roe_var_f34 << 0, delta_u - delta_norm_q * norm_vec.x(), delta_v - delta_norm_q * norm_vec.y(),
      roe_u * delta_u + roe_v * delta_v - roe_norm_q * delta_norm_q;
  roe_var_f34 *= roe_rho;
  Eigen::Vector<Real, 4> roe_var_f5;
  roe_var_f5 << 1, roe_u + roe_a * norm_vec.x(), roe_v + roe_a * norm_vec.y(), roe_capital_h + roe_a * roe_norm_q;
  roe_var_f5 *= std::fabs(roe_norm_q + roe_a) * (delta_p + roe_rho * roe_a * delta_norm_q) / (2.0 * roe_a * roe_a);
  Eigen::Matrix<Real, 4, 2> l_convective_var;
  Eigen::Matrix<Real, 4, 2> r_convective_var;
  calConvectiveVar(l_primitive_var, l_convective_var);
  calConvectiveVar(r_primitive_var, r_convective_var);
  roe_flux.noalias() = ((l_convective_var + r_convective_var) * norm_vec -
                        (roe_var_f1 + std::fabs(roe_norm_q) * (roe_var_f2 + roe_var_f34) + roe_var_f5)) /
                       2.0;
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CAL_ROE_FLUX_HPP_
