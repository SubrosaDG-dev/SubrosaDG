/**
 * @file cal_basisfun_num.hpp
 * @brief The head file to calculate the basis function number.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-31
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CAL_BASISFUN_NUM_HPP_
#define SUBROSA_DG_CAL_BASISFUN_NUM_HPP_

// clang-format off

#include "basic/data_type.hpp"  // for Isize
#include "mesh/elem_type.hpp"   // for ElemInfo, kLine, kQuad, kTri

// clang-format on

namespace SubrosaDG {

template <ElemInfo ElemT>
inline constexpr Isize getBasisFunNum(Isize poly_order);

template <>
inline constexpr Isize getBasisFunNum<kLine>(Isize poly_order) {
  return poly_order + 1;
}

template <>
inline constexpr Isize getBasisFunNum<kTri>(Isize poly_order) {
  return (poly_order + 1) * (poly_order + 2) / 2;
}

template <>
inline constexpr Isize getBasisFunNum<kQuad>(Isize poly_order) {
  return (poly_order + 1) * (poly_order + 1);
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CAL_BASISFUN_NUM_HPP_
