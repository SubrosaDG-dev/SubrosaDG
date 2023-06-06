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

// clang-format off

#include <Eigen/Core>           // for Vector, DenseCoeffsBase, CommaInitializer, DenseBase, Matrix

#include "basic/data_type.hpp"  // for Real

// clang-format on

namespace SubrosaDG {

template <int Dim>
inline void calWallFlux(const Eigen::Vector<Real, Dim>& norm_vec, const Eigen::Vector<Real, Dim + 3>& l_primitive_var,
                        Eigen::Vector<Real, Dim + 2>& wall_flux);

template <>
inline void calWallFlux<2>(const Eigen::Vector<Real, 2>& norm_vec, const Eigen::Vector<Real, 5>& l_primitive_var,
                           Eigen::Vector<Real, 4>& wall_flux) {
  wall_flux << 0, l_primitive_var(3) * norm_vec(0), l_primitive_var(3) * norm_vec(1), 0;
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CAL_WALL_FLUX_HPP_
