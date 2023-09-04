/**
 * @file get_subelem_num.hpp
 * @brief The head file to get the number of subelements.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-08-12
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_GET_SUBELEM_NUM_HPP_
#define SUBROSA_DG_GET_SUBELEM_NUM_HPP_

#include "basic/concept.hpp"
#include "basic/enum.hpp"

namespace SubrosaDG {

template <ElemType ElemT>
inline consteval int getSubElemNum(PolyOrder poly_order);

template <ElemType ElemT>
  requires Is1dElem<ElemT>
inline consteval int getSubElemNum(PolyOrder poly_order) {
  return (static_cast<int>(poly_order));
}

template <ElemType ElemT>
  requires Is2dElem<ElemT>
inline consteval int getSubElemNum(PolyOrder poly_order) {
  return (static_cast<int>(poly_order) * static_cast<int>(poly_order));
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_SUBELEM_NUM_HPP_
