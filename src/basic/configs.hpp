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

#include "basic/data_types.hpp"  // for Real, Usize, Isize

// clang-format on

namespace SubrosaDG {

enum class NoVisFlux : SubrosaDG::Usize;
enum class EquationOfState : SubrosaDG::Usize;

template <Usize Step, bool IsImplicit>
struct TimeDiscrete {
  inline static constexpr Usize kStep{Step};
  inline static constexpr bool kIsImplicit{IsImplicit};
};

inline constexpr TimeDiscrete<1, false> kExplicitEuler;

inline constexpr TimeDiscrete<1, true> kImplicitEuler;

inline constexpr TimeDiscrete<3, false> kRungeKutta3;

// template <typename TimeDiscreteType, TimeDiscreteType TimeDiscrete>

template <TimeDiscrete TimeDiscreteType>
struct TimeParameter {
  const Isize iteration_;
  const Real cfl_;
  const Isize tolerance_;

  inline constexpr TimeParameter(const Isize iteration, const Real cfl, const Isize tolerance)
      : iteration_(iteration), cfl_(cfl), tolerance_(tolerance) {}
};

template <NoVisFlux NoVisFluxType>
struct SpaceDiscrete {};

template <EquationOfState EquationOfStateType>
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

template <Isize Dimension>
struct FlowParameter {
  const std::array<Real, Dimension> u_;
  const Real rho_;
  const Real p_;
  const Real t_;

  inline constexpr FlowParameter(const std::array<Real, Dimension> u, const Real rho, const Real p, const Real t)
      : u_(u), rho_(rho), p_(p), t_(t) {}
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CONFIGS_HPP_
