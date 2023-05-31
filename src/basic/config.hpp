/**
 * @file config.hpp
 * @brief The configs head file to define some configs.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-16
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CONFIG_HPP_
#define SUBROSA_DG_CONFIG_HPP_

// clang-format off

#include <array>                // for array

#include "basic/data_type.hpp"  // for Real, Isize, Usize
#include "basic/enum.hpp"       // for SimulationEquation, ConvectiveFlux (ptr only)

// clang-format on

namespace SubrosaDG {

template <Usize Step, bool IsImplicit>
struct TimeDiscrete {
  inline static constexpr Usize kStep{Step};
  inline static constexpr bool kIsImplicit{IsImplicit};
};

inline constexpr TimeDiscrete<1, false> kExplicitEuler;

inline constexpr TimeDiscrete<3, false> kRungeKutta3;

inline constexpr TimeDiscrete<1, true> kImplicitEuler;

template <TimeDiscrete TimeDiscreteT>
struct TimeVar {
  const Isize iter_;
  const Real cfl_;
  const Isize tole_;

  inline constexpr TimeVar(const Isize iter, const Real cfl, const Isize tole) : iter_(iter), cfl_(cfl), tole_(tole) {}
};

template <ConvectiveFlux ConvectiveFluxT>
struct SpaceDiscrete {};

template <SimulationEquation SimulationEquationT>
struct ThermodynamicModel {};

template <>
struct ThermodynamicModel<SimulationEquation::Euler> {
  const Real gamma_;
  const Real c_p_;
  const Real r_;

  inline constexpr ThermodynamicModel(const Real gamma, const Real c_p, const Real r)
      : gamma_(gamma), c_p_(c_p), r_(r) {}
};

template <>
struct ThermodynamicModel<SimulationEquation::NavierStokes> : ThermodynamicModel<SimulationEquation::Euler> {
  const Real mu_;
  inline constexpr ThermodynamicModel(const Real gamma, const Real c_p, const Real r, const Real mu)
      : ThermodynamicModel<SimulationEquation::Euler>(gamma, c_p, r), mu_(mu) {}
};

template <Isize Dim>
struct FlowVar {
  const std::array<Real, Dim> u_;
  const Real rho_;
  const Real p_;
  const Real capital_t_;

  inline constexpr FlowVar(const std::array<Real, Dim> u, const Real rho, const Real p, const Real capital_t)
      : u_(u), rho_(rho), p_(p), capital_t_(capital_t) {}
};

template <Isize Dim>
struct InitVar : FlowVar<Dim> {
  using FlowVar<Dim>::FlowVar;
};

template <Isize Dim>
struct FarfieldVar : FlowVar<Dim> {
  using FlowVar<Dim>::FlowVar;
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CONFIG_HPP_
