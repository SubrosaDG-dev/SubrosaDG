/**
 * @file elem_type.hpp
 * @brief The element type head file to define some aliases.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-16
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_ELEM_TYPE_HPP_
#define SUBROSA_DG_ELEM_TYPE_HPP_

namespace SubrosaDG {

template <int Dim, int ElemTopology, int NodeNumPerElem, int AdjacencyElemNum>
struct ElemInfo {
  inline static constexpr int kDim{Dim};
  inline static constexpr int kTopology{ElemTopology};
  inline static constexpr int kNodeNum{NodeNumPerElem};
  inline static constexpr int kAdjacencyNum{AdjacencyElemNum};
};

inline constexpr ElemInfo<1, 1, 2, 2> kLine;
inline constexpr ElemInfo<2, 2, 3, 3> kTri;
inline constexpr ElemInfo<2, 3, 4, 4> kQuad;

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_ELEM_TYPE_HPP_
