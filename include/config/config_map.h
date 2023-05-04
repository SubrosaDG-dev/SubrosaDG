/**
 * @file config_map.h
 * @brief The config map head file.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-04
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CONFIG_MAP_H_
#define SUBROSA_DG_CONFIG_MAP_H_

// clang-format off

#include <string_view>              // for string_view, hash, operator==
#include <unordered_map>            // for unordered_map

#include "config/config_defines.h"  // for NoVisFluxType, TimeIntegrationType, BoundaryType, SimulationType, Equatio...

// clang-format on

namespace SubrosaDG::Internal {

template <typename T>
inline const std::unordered_map<std::string_view, T> kConfigMap{};

template <>
inline const std::unordered_map<std::string_view, SimulationType> kConfigMap<SimulationType>{
    {"Euler", SimulationType::Euler},
    {"NavierStokes", SimulationType::NavierStokes},
};

template <>
inline const std::unordered_map<std::string_view, NoVisFluxType> kConfigMap<NoVisFluxType>{
    {"Central", NoVisFluxType::Central},
    {"Roe", NoVisFluxType::Roe},
    {"HLLC", NoVisFluxType::HLLC},
};

template <>
inline const std::unordered_map<std::string_view, TimeIntegrationType> kConfigMap<TimeIntegrationType>{
    {"ExplicitEuler", TimeIntegrationType::ExplicitEuler},
    {"ImplicitEuler", TimeIntegrationType::ImplicitEuler},
    {"RungeKutta3", TimeIntegrationType::RungeKutta3},
};

template <>
inline const std::unordered_map<std::string_view, BoundaryType> kConfigMap<BoundaryType>{
    {"Farfield", BoundaryType::Farfield},
    {"Wall", BoundaryType::Wall},
};

template <>
inline const std::unordered_map<std::string_view, EquationOfState> kConfigMap<EquationOfState>{
    {"IdealGas", EquationOfState::IdealGas},
};

}  // namespace SubrosaDG::Internal

#endif  // SUBROSA_DG_CONFIG_MAP_H_
