/**
 * @file config_defines.h
 * @brief The header file for SubrosaDG configurations.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-04-24
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CONFIG_DEFINES_H_
#define SUBROSA_DG_CONFIG_DEFINES_H_

// clang-format off

#include "basic/data_types.h"  // for Usize

// clang-format on

namespace SubrosaDG::Internal {

enum class SimulationType : Usize {
  Euler = 0,
  NavierStokes,
};

enum class TimeIntegrationType : Usize {
  ExplicitEuler = 0,
  ImplicitEuler,
  RungeKutta3,
};

enum class EquationOfState : Usize {
  IdealGas = 0,
};

enum class BoundaryType : Usize {
  Farfield = 0,
  Wall,
};

enum class NoVisFluxType : Usize {
  Central = 0,
  Roe,
  HLLC,
};

}  // namespace SubrosaDG::Internal

#endif  // SUBROSA_DG_CONFIG_DEFINES_H_
