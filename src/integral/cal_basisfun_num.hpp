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

#include "mesh/elem_type.hpp"  // for ElemInfo, kLine, kQuad, kTri

// clang-format on

namespace SubrosaDG {

template <ElemInfo ElemT>
inline consteval int calBasisFunNum(int poly_order);

template <>
inline consteval int calBasisFunNum<kLine>(int poly_order) {
  return poly_order + 1;
}

template <>
inline consteval int calBasisFunNum<kTri>(int poly_order) {
  return (poly_order + 1) * (poly_order + 2) / 2;
}

template <>
inline consteval int calBasisFunNum<kQuad>(int poly_order) {
  return (poly_order + 1) * (poly_order + 1);
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CAL_BASISFUN_NUM_HPP_