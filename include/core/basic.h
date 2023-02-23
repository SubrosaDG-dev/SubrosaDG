/**
 * @file basic.h
 * @brief The basic head file to define some aliases.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2022-11-02
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2022 by SubrosaDG developers
 */

#ifndef SUBROSA_DG_BASIC_H_
#define SUBROSA_DG_BASIC_H_

#include <cstddef>
#include <numbers>

namespace SubrosaDG {

#ifndef SUBROSA_DG_DEVELOP
#define DBG_MACRO_DISABLE
#endif

using index = std::size_t;

#ifdef SUBROSA_DG_SINGLE_PRECISION
using real = float;
#else
using real = double;
#endif

inline constexpr real kPi = std::numbers::pi_v<real>;
inline constexpr real kE = std::numbers::e_v<real>;

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_BASIC_H_
