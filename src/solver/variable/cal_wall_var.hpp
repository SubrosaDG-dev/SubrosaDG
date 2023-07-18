/**
 * @file cal_wall_var.hpp
 * @brief The head file to calculate the wall variables.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-07-16
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CAL_WALL_VAR_HPP_
#define SUBROSA_DG_CAL_WALL_VAR_HPP_

#include <Eigen/Core>

#include "basic/data_type.hpp"

namespace SubrosaDG {

inline void calWallPrimitiveVar(const Eigen::Vector<Real, 5>& primitive_var,
                                Eigen::Vector<Real, 5>& wall_primitive_var) {
  wall_primitive_var(0) = primitive_var(0);
  wall_primitive_var(1) = 0;
  wall_primitive_var(2) = 0;
  wall_primitive_var(3) = primitive_var(3);
  wall_primitive_var(3) =
      primitive_var(4) - 0.5 * (primitive_var(1) * primitive_var(1) + primitive_var(2) * primitive_var(2));
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CAL_WALL_VAR_HPP_
