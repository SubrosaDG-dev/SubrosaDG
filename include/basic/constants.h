/**
 * @file constants.h
 * @brief The constants head file to define some constants.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-04-10
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers
 */

#ifndef SUBROSA_DG_CONSTANTS_H_
#define SUBROSA_DG_CONSTANTS_H_

#include <numbers>

#include "basic/data_type.h"

namespace SubrosaDG {

inline constexpr Real kPi = std::numbers::pi_v<Real>;
inline constexpr Real kEuler = std::numbers::e_v<Real>;

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CONSTANTS_H_
