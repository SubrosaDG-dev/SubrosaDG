/**
 * @file time_solver.hpp
 * @brief The time solver header file.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-06-13
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_TIME_SOLVER_HPP_
#define SUBROSA_DG_TIME_SOLVER_HPP_

#include <array>

#include "basic/config.hpp"
#include "basic/data_type.hpp"
#include "basic/enum.hpp"

namespace SubrosaDG {

template <TimeDiscrete TimeDiscreteT>
struct TimeSolver;

template <>
struct TimeSolver<TimeDiscrete::ExplicitEuler> : TimeVar {
  inline static constexpr std::array<Real, 1> kStepCoeffs{0.0};

  inline constexpr TimeSolver(const TimeVar& time_var) : TimeVar(time_var) {}
};

template <>
struct TimeSolver<TimeDiscrete::RungeKutta3> : TimeVar {
  inline static constexpr std::array<Real, 3> kStepCoeffs{0.0, 3.0 / 4.0, 1.0 / 3.0};

  inline constexpr TimeSolver(const TimeVar& time_var) : TimeVar(time_var) {}
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_TIME_SOLVER_HPP_
