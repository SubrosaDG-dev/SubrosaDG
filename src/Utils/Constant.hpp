/**
 * @file Constant.hpp
 * @brief The constants head file to define some constants.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-04-10
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CONSTANT_HPP_
#define SUBROSA_DG_CONSTANT_HPP_

#include <iosfwd>
#include <limits>
#include <numbers>

#include "Utils/BasicDataType.hpp"

namespace SubrosaDG {

inline constexpr Real kPi = std::numbers::pi_v<Real>;
inline consteval Real toRadian(Real degree) { return degree * kPi / 180.0; }
inline constexpr Real kEuler = std::numbers::e_v<Real>;

inline constexpr std::streamsize kRealSize = sizeof(Real);

inline constexpr Real kRealMin = std::numeric_limits<Real>::min();
inline constexpr Real kRealMax = std::numeric_limits<Real>::max();
inline constexpr Real kRealEpsilon = std::numeric_limits<Real>::epsilon();
inline constexpr int kRealSignificantDigits = std::numeric_limits<Real>::digits10;

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CONSTANT_HPP_
