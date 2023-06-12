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
inline consteval int getDim();

template <ElemType ElemT>
  requires Is1dElem<ElemT>
inline consteval int getDim() {
  return 1;
}

template <ElemType ElemT>
  requires Is2dElem<ElemT>
inline consteval int getDim() {
  return 2;
}

template <ElemType ElemT>
  requires Is3dElem<ElemT>
inline consteval int getDim() {
  return 3;
}

template <ElemType ElemT>
inline consteval int getTopology();

template <>
inline consteval int getTopology<ElemType::Line>() {
  return 1;
}

template <>
inline consteval int getTopology<ElemType::Tri>() {
  return 2;
}

template <>
inline consteval int getTopology<ElemType::Quad>() {
  return 3;
}

template <ElemType ElemT>
inline consteval int getNodeNum();

template <>
inline consteval int getNodeNum<ElemType::Line>() {
  return 2;
}

template <>
inline consteval int getNodeNum<ElemType::Tri>() {
  return 3;
}

template <>
inline consteval int getNodeNum<ElemType::Quad>() {
  return 4;
}

template <ElemType ElemT>
inline consteval int getAdjacencyNum() {
  return getNodeNum<ElemT>();
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_ELEM_INFO_HPP_
