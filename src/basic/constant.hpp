/**
 * @file constant.hpp
 * @brief The constants head file to define some constants.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-04-10
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CONSTANT_HPP_
#define SUBROSA_DG_CONSTANT_HPP_

#include <limits>
#include <numbers>

#include "basic/data_type.hpp"

namespace SubrosaDG {

inline constexpr Real kPi = std::numbers::pi_v<Real>;
inline constexpr Real kEuler = std::numbers::e_v<Real>;

inline constexpr Real kMax = std::numeric_limits<Real>::max();
inline constexpr Real kEpsilon = std::numeric_limits<Real>::epsilon();
inline constexpr int kSignificantDigits = std::numeric_limits<Real>::digits10;

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CONSTANT_HPP_
