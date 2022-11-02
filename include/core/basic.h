/**
 * @file basic.h
 * @brief The basic head file to define some aliases.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2022-11-02
 *
 * @version 0.0.1
 * @copyright Copyright (c) 2022 - 2022 by SubrosaDG developers
 */

#include <cstddef>
#include <numbers>

namespace SubrosaDG {

using index = std::size_t;

#ifdef SUBROSA_DG_SINGLE_PRECISION
using real = float;
#else
using real = double;
#endif

constexpr real Pi = std::numbers::pi_v<real>;
constexpr real E = std::numbers::e_v<real>;

}  // namespace SubrosaDG
