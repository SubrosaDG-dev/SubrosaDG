/**
 * @file get_elem_info.hpp
 * @brief The get element info header file.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-16
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_GET_ELEM_INFO_HPP_
#define SUBROSA_DG_GET_ELEM_INFO_HPP_

#include <Eigen/Core>
#include <array>

#include "basic/concept.hpp"
#include "basic/data_type.hpp"
#include "basic/enum.hpp"

namespace SubrosaDG {

inline constexpr std::array<int, 6> kLineTopology{1, 1, 8, 26, 27, 28};
inline constexpr std::array<int, 6> kTriTopology{2, 2, 9, 21, 23, 25};
inline constexpr std::array<int, 6> kQuadTopology{3, 3, 10, 36, 37, 38};

template <ElemType ElemT>
inline consteval int getDim() {
  if constexpr (Is1dElem<ElemT>) {
    return 1;
  } else if constexpr (Is2dElem<ElemT>) {
    return 2;
  } else if constexpr (Is3dElem<ElemT>) {
    return 3;
  }
}

template <ElemType ElemT>
inline consteval int getTopology(const PolyOrder poly_order) {
  if constexpr (ElemT == ElemType::Line) {
    return kLineTopology[static_cast<Usize>(poly_order)];
  } else if constexpr (ElemT == ElemType::Tri) {
    return kTriTopology[static_cast<Usize>(poly_order)];
  } else if constexpr (ElemT == ElemType::Quad) {
    return kQuadTopology[static_cast<Usize>(poly_order)];
  }
}

template <ElemType ElemT>
inline consteval int getNodeNum(const PolyOrder poly_order) {
  if constexpr (ElemT == ElemType::Line) {
    return static_cast<int>(poly_order) + 1;
  } else if constexpr (ElemT == ElemType::Tri) {
    return (static_cast<int>(poly_order) + 1) * (static_cast<int>(poly_order) + 2) / 2;
  } else if constexpr (ElemT == ElemType::Quad) {
    return (static_cast<int>(poly_order) + 1) * (static_cast<int>(poly_order) + 1);
  }
}

template <ElemType ElemT>
inline consteval int getElemAdjacencyNum() {
  if constexpr (ElemT == ElemType::Line) {
    return 2;
  } else if constexpr (ElemT == ElemType::Tri) {
    return 3;
  } else if constexpr (ElemT == ElemType::Quad) {
    return 4;
  }
}

template <ElemType ElemT>
inline consteval int getSubElemNum(PolyOrder poly_order) {
  if constexpr (Is1dElem<ElemT>) {
    return (static_cast<int>(poly_order));
  } else if constexpr (Is2dElem<ElemT>) {
    return (static_cast<int>(poly_order) * static_cast<int>(poly_order));
  } else if constexpr (Is3dElem<ElemT>) {
    return (static_cast<int>(poly_order) * static_cast<int>(poly_order) * static_cast<int>(poly_order));
  }
}

template <ElemType ElemT, PolyOrder P>
  requires(ElemT == ElemType::Line)
inline void getNodeOrder(Eigen::Vector<int, getNodeNum<ElemT>(P)>& node_order) {
  switch (P) {
  case PolyOrder::P1:
    node_order << 0, 1;
    break;
  case PolyOrder::P2:
    node_order << 0, 2, 1;
    break;
  case PolyOrder::P3:
    node_order << 0, 2, 3, 1;
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_ELEM_INFO_HPP_
