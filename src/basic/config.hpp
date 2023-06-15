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

#include <array>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "basic/data_type.hpp"
#include "basic/enum.hpp"

namespace SubrosaDG {

struct TimeVar {
  const int iter_;
  const Real cfl_;
  const Real tole_;

  inline consteval TimeVar(const int iter, const Real cfl, const Real tole) : iter_(iter), cfl_(cfl), tole_(tole) {}
};

template <EquModel EquModelT>
struct SpatialDiscrete {};

template <ConvectiveFlux ConvectiveFluxT>
struct SpatialDiscreteEuler : SpatialDiscrete<EquModel::Euler> {
  inline static constexpr ConvectiveFlux kConvectiveFlux{ConvectiveFluxT};
};

template <ConvectiveFlux ConvectiveFluxT, ViscousFlux ViscousFluxT>
struct SpatialDiscreteNS : SpatialDiscrete<EquModel::NS> {
  inline static constexpr ConvectiveFlux kConvectiveFlux{ConvectiveFluxT};
  inline static constexpr ViscousFlux kViscousFlux{ViscousFluxT};
};

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

template <int Dim>
struct FlowVar {
  const std::array<Real, Dim> u_;
  const Real rho_;
  const Real p_;
  const Real capital_t_;

  inline consteval FlowVar(const std::array<Real, Dim> u, const Real rho, const Real p, const Real capital_t)
      : u_(u), rho_(rho), p_(p), capital_t_(capital_t) {}
};

template <int Dim>
struct InitVar {
  const std::unordered_map<std::string_view, int> region_map_;
  const std::vector<FlowVar<Dim>> flow_var_;

  inline InitVar(std::unordered_map<std::string_view, int> region_map, const std::vector<FlowVar<Dim>>& flow_var)
      : region_map_(std::move(region_map)), flow_var_(flow_var) {}
};

template <int Dim>
struct FarfieldVar : FlowVar<Dim> {
  using FlowVar<Dim>::FlowVar;
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CONFIG_HPP_
