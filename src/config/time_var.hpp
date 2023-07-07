/**
 * @file time_var.hpp
 * @brief The head file to define some time variables.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-07-01
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_TIME_VAR_HPP_
#define SUBROSA_DG_TIME_VAR_HPP_

#include "basic/data_type.hpp"
#include "basic/enum.hpp"

namespace SubrosaDG {

template <TimeDiscrete TimeDiscreteT>
struct TimeVar {
  const int iter_;
  const Real cfl_;
  const Real tole_;

  inline consteval TimeVar(const int iter, const Real cfl, const Real tole) : iter_(iter), cfl_(cfl), tole_(tole) {}
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_TIME_VAR_HPP_
