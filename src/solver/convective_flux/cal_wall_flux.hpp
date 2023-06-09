/**
 * @file cal_wall_flux.hpp
 * @brief The head file to calculate the wall flux.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-30
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CAL_WALL_FLUX_HPP_
#define SUBROSA_DG_CAL_WALL_FLUX_HPP_

#include <Eigen/Core>

#include "basic/data_type.hpp"

namespace SubrosaDG {

inline void calWallFlux(const Eigen::Vector<Real, 2>& norm_vec, const Eigen::Vector<Real, 5>& l_primitive_var,
                        Eigen::Vector<Real, 4>& wall_flux) {
  wall_flux << 0, l_primitive_var(3) * norm_vec.x(), l_primitive_var(3) * norm_vec.y(), 0;
}

inline void calWallFlux(const Eigen::Vector<Real, 3>& norm_vec, const Eigen::Vector<Real, 6>& l_primitive_var,
                        Eigen::Vector<Real, 5>& wall_flux) {
  wall_flux << 0, l_primitive_var(4) * norm_vec.x(), l_primitive_var(4) * norm_vec.y(),
      l_primitive_var(4) * norm_vec.z(), 0;
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CAL_WALL_FLUX_HPP_
