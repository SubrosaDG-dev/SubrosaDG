/**
 * @file concepts.hpp
 * @brief The concepts head file to define some concepts.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-16
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CONCEPTS_HPP_
#define SUBROSA_DG_CONCEPTS_HPP_

// clang-format off

#include "mesh/element_types.hpp"  // for ElementType
#include "basic/data_types.hpp"    // for Isize

namespace SubrosaDG {

template <ElementType Type>
concept Is1dElement = Type.kDimension == 1;

template <ElementType Type>
concept Is2dElement = Type.kDimension == 2;

template <ElementType Type>
concept Is3dElement = Type.kDimension == 3;

}  // namespace SubrosaDG

// clang-format on

#endif  // SUBROSA_DG_CONCEPTS_HPP_
