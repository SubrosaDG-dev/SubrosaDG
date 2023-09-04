/**
 * @file flow_var.hpp
 * @brief The head file to define some flow variables.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-07-01
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_FLOW_VAR_HPP_
#define SUBROSA_DG_FLOW_VAR_HPP_

#include <array>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "basic/data_type.hpp"
#include "basic/enum.hpp"

namespace SubrosaDG {

template <int Dim, EquModel EquModelT>
  requires(EquModelT == EquModel::Euler || EquModelT == EquModel::NS)
struct FlowVar {
  const std::array<Real, Dim> u_;
  const Real rho_;
  const Real p_;
  const Real capital_t_;

  inline consteval FlowVar(const std::array<Real, Dim> u, const Real rho, const Real p, const Real capital_t)
      : u_(u), rho_(rho), p_(p), capital_t_(capital_t) {}
};

template <int Dim, EquModel EquModelT>
struct InitVar {
  const std::unordered_map<std::string_view, int> region_map_;
  const std::vector<FlowVar<Dim, EquModelT>> flow_var_;

  inline InitVar(std::unordered_map<std::string_view, int> region_map,
                 const std::vector<FlowVar<Dim, EquModelT>>& flow_var)
      : region_map_(std::move(region_map)), flow_var_(flow_var) {}
};

template <int Dim, EquModel EquModelT>
struct FarfieldVar : FlowVar<Dim, EquModelT> {
  using FlowVar<Dim, EquModelT>::FlowVar;
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_FLOW_VAR_HPP_
