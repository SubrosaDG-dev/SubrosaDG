/**
 * @file enums.hpp
 * @brief The enum definitions head file to define some enums.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-16
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_ENUMS_HPP_
#define SUBROSA_DG_ENUMS_HPP_

// clang-format off

#include "basic/data_types.hpp"  // for Usize, Isize

// clang-format on

namespace SubrosaDG {

enum class SimulationType : Usize {
  Euler = 1,
  NavierStokes,
};

enum class NoVisFluxType : Usize {
  Central = 1,
  Roe,
  HLLC,
};

enum class TimeIntegrationType : Usize {
  ExplicitEuler = 1,
  ImplicitEuler,
  RungeKutta3,
};

enum class BoundaryType : Isize {
  Farfield = -1,
  Wall = -2,
};

enum class EquationOfState : Usize {
  IdealGas = 1,
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_ENUMS_HPP_
