/**
 * @file configs.hpp
 * @brief The configs head file to define some configs.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-16
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CONFIGS_HPP_
#define SUBROSA_DG_CONFIGS_HPP_

// clang-format off

#include <array>                 // for array

#include "basic/data_types.hpp"  // for Real, Isize, Usize

// clang-format on

namespace SubrosaDG {

enum class TimeIntegrationType : SubrosaDG::Usize;
enum class EquationOfState : SubrosaDG::Usize;

template <TimeIntegrationType Type>
struct TimeIntegration {
  const Isize iteration_;
  const Real cfl_;
  const Isize tolerance_;

  constexpr TimeIntegration(const Isize iteration, const Real cfl, const Isize tolerance)
      : iteration_(iteration), cfl_(cfl), tolerance_(tolerance) {}
};

template <EquationOfState Type>
struct ThermodynamicModel {
  const Real gamma_;
  const Real c_p_;
  const Real r_;
  const Real mu_;

  inline constexpr ThermodynamicModel(const Real gamma, const Real c_p, const Real r)
      : gamma_(gamma), c_p_(c_p), r_(r), mu_(0.0) {}

  inline constexpr ThermodynamicModel(const Real gamma, const Real c_p, const Real r, const Real mu)
      : gamma_(gamma), c_p_(c_p), r_(r), mu_(mu) {}
};

struct FlowParameter {
  const std::array<Real, 3> u_;
  const Real rho_;
  const Real p_;
  const Real t_;

  inline constexpr FlowParameter(const std::array<Real, 3> u, const Real rho, const Real p, const Real t)
      : u_(u), rho_(rho), p_(p), t_(t) {}
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CONFIGS_HPP_
