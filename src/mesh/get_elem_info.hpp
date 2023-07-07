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

#include "basic/concept.hpp"
#include "basic/enum.hpp"

namespace SubrosaDG {

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
inline consteval int getTopology() {
  if constexpr (ElemT == ElemType::Line) {
    return 1;
  } else if constexpr (ElemT == ElemType::Tri) {
    return 2;
  } else if constexpr (ElemT == ElemType::Quad) {
    return 3;
  }
}

template <ElemType ElemT>
inline consteval int getNodeNum() {
  if constexpr (ElemT == ElemType::Line) {
    return 2;
  } else if constexpr (ElemT == ElemType::Tri) {
    return 3;
  } else if constexpr (ElemT == ElemType::Quad) {
    return 4;
  }
}

template <ElemType ElemT>
inline consteval int getAdjacencyNum() {
  if constexpr (ElemT == ElemType::Line) {
    return 2;
  } else if constexpr (ElemT == ElemType::Tri) {
    return 3;
  } else if constexpr (ElemT == ElemType::Quad) {
    return 4;
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_ELEM_INFO_HPP_
