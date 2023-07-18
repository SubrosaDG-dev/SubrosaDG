/**
 * @file get_integral.hpp
 * @brief The header file to get element integral.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-12
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_GET_INTEGRAL_HPP_
#define SUBROSA_DG_GET_INTEGRAL_HPP_

#include "basic/concept.hpp"
#include "basic/enum.hpp"
#include "integral/get_adjacency_integral.hpp"
#include "integral/get_elem_integral.hpp"
#include "integral/integral_structure.hpp"

namespace SubrosaDG {

template <PolyOrder P, MeshType MeshT>
inline void getIntegral(Integral<2, P, MeshT>& integral) {
  if constexpr (HasTri<MeshT>) {
    getElemIntegral(integral.tri_);
  }
  if constexpr (HasQuad<MeshT>) {
    getElemIntegral(integral.quad_);
  }
  getAdjacencyElemIntegral(integral.line_);
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_INTEGRAL_HPP_
