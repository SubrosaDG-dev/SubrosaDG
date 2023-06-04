/**
 * @file get_integral_num.hpp
 * @brief The get integral number header file.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-06-02
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_GET_INTEGRAL_NUM_HPP_
#define SUBROSA_DG_GET_INTEGRAL_NUM_HPP_

// clang-format off

#include <array>                // for array

#include "basic/data_type.hpp"  // for Usize
#include "mesh/elem_type.hpp"   // for ElemInfo, kQuad, kTri, kLine

// clang-format on

namespace SubrosaDG {

inline constexpr std::array<int, 12> kLineIntegralNum{1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6};
inline constexpr std::array<int, 12> kTriIntegralNum{1, 1, 3, 4, 6, 7, 12, 13, 16, 19, 25, 27};
inline constexpr std::array<int, 12> kQuadIntegralNum{1, 3, 7, 4, 9, 9, 16, 16, 25, 25, 36, 36};

template <ElemInfo ElemT>
inline consteval int getElemIntegralNum(int poly_order);

template <>
inline consteval int getElemIntegralNum<kLine>(int poly_order) {
  return kLineIntegralNum[static_cast<Usize>(2 * poly_order + 1)];
}

template <>
inline consteval int getElemIntegralNum<kTri>(int poly_order) {
  return kTriIntegralNum[static_cast<Usize>(2 * poly_order + 1)];
}

template <>
inline consteval int getElemIntegralNum<kQuad>(int poly_order) {
  return kQuadIntegralNum[static_cast<Usize>(2 * poly_order + 1)];
}

template <ElemInfo ElemT>
inline consteval int getElemAdjacencyIntegralNum(int poly_order);

template <>
inline consteval int getElemAdjacencyIntegralNum<kTri>(int poly_order) {
  return kLineIntegralNum[static_cast<Usize>(2 * poly_order + 1)] * kTri.kAdjacencyNum;
}

template <>
inline consteval int getElemAdjacencyIntegralNum<kQuad>(int poly_order) {
  return kLineIntegralNum[static_cast<Usize>(2 * poly_order + 1)] * kQuad.kAdjacencyNum;
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_INTEGRAL_NUM_HPP_
