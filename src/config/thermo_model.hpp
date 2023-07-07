/**
 * @file thermo_model.hpp
 * @brief The head file to define some thermo models.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-07-01
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_THERMO_MODEL_HPP_
#define SUBROSA_DG_THERMO_MODEL_HPP_

#include "basic/data_type.hpp"
#include "basic/enum.hpp"

namespace SubrosaDG {

template <EquModel EquModelT>
struct ThermoModel;

template <>
struct ThermoModel<EquModel::Euler> {
  const Real gamma_;
  const Real c_p_;
  const Real r_;

  inline consteval ThermoModel(const Real gamma, const Real c_p, const Real r) : gamma_(gamma), c_p_(c_p), r_(r) {}
};

template <>
struct ThermoModel<EquModel::NS> : ThermoModel<EquModel::Euler> {
  const Real mu_;
  const Real k_;

  inline consteval ThermoModel(const Real gamma, const Real c_p, const Real r, const Real mu, const Real k)
      : ThermoModel<EquModel::Euler>(gamma, c_p, r), mu_(mu), k_(k) {}
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_THERMO_MODEL_HPP_
