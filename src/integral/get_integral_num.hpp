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

#include <array>

#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "mesh/get_elem_info.hpp"

namespace SubrosaDG {

inline consteval int getElemIntegralOrder(PolyOrder poly_order) { return 2 * static_cast<int>(poly_order); }

inline consteval int getElemAdjacencyIntegralOrder(PolyOrder poly_order) {
  return 2 * static_cast<int>(poly_order) + 1;
}

inline constexpr std::array<int, 12> kLineIntegralNum{1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6};
inline constexpr std::array<int, 12> kTriIntegralNum{1, 1, 3, 4, 6, 7, 12, 13, 16, 19, 25, 27};
inline constexpr std::array<int, 12> kQuadIntegralNum{1, 3, 7, 4, 9, 9, 16, 16, 25, 25, 36, 36};

template <ElemType ElemT>
inline consteval int getElemIntegralNum(PolyOrder poly_order);

template <>
inline consteval int getElemIntegralNum<ElemType::Line>(PolyOrder poly_order) {
  return kLineIntegralNum[static_cast<Usize>(getElemIntegralOrder(poly_order))];
}

template <>
inline consteval int getElemIntegralNum<ElemType::Tri>(PolyOrder poly_order) {
  return kTriIntegralNum[static_cast<Usize>(getElemIntegralOrder(poly_order))];
}

template <>
inline consteval int getElemIntegralNum<ElemType::Quad>(PolyOrder poly_order) {
  return kQuadIntegralNum[static_cast<Usize>(getElemIntegralOrder(poly_order))];
}

template <ElemType ElemT>
inline consteval int getAdjacencyElemIntegralNum(PolyOrder poly_order);

template <>
inline consteval int getAdjacencyElemIntegralNum<ElemType::Line>(PolyOrder poly_order) {
  return kLineIntegralNum[static_cast<Usize>(getElemAdjacencyIntegralOrder(poly_order))];
}

template <ElemType ElemT>
inline consteval int getElemAdjacencyIntegralNum(PolyOrder poly_order);

template <>
inline consteval int getElemAdjacencyIntegralNum<ElemType::Tri>(PolyOrder poly_order) {
  return kLineIntegralNum[static_cast<Usize>(getElemAdjacencyIntegralOrder(poly_order))] *
         getElemAdjacencyNum<ElemType::Tri>();
}

template <>
inline consteval int getElemAdjacencyIntegralNum<ElemType::Quad>(PolyOrder poly_order) {
  return kLineIntegralNum[static_cast<Usize>(getElemAdjacencyIntegralOrder(poly_order))] *
         getElemAdjacencyNum<ElemType::Quad>();
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_INTEGRAL_NUM_HPP_
