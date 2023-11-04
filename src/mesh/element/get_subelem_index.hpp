/**
 * @file get_subelem_index.hpp
 * @brief The head file to get the sub-element index.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-08-12
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_GET_SUBELEM_MAT_HPP_
#define SUBROSA_DG_GET_SUBELEM_MAT_HPP_

#include "basic/enum.hpp"
#include "mesh/mesh_structure.hpp"

namespace SubrosaDG {

template <PolyOrder P, ElemType ElemT>
  requires(ElemT == ElemType::Line)
inline void getSubElemIndex(ElemMesh<2, P, ElemT>& elem_mesh) {
  switch (P) {
    // clang-format off
  case PolyOrder::P1:
    elem_mesh.subelem_index_ << 0,
                                1;
    break;
  case PolyOrder::P2:
    elem_mesh.subelem_index_ << 0, 2,
                                2, 1;
    break;
  case PolyOrder::P3:
    elem_mesh.subelem_index_ << 0, 2, 3,
                                2, 3, 1;
    // clang-format on
  }
}

template <PolyOrder P, ElemType ElemT>
  requires(ElemT == ElemType::Tri)
inline void getSubElemIndex(ElemMesh<2, P, ElemT>& elem_mesh) {
  switch (P) {
    // clang-format off
  case PolyOrder::P1:
    elem_mesh.subelem_index_ << 0,
                                1,
                                2;
    break;
  case PolyOrder::P2:
    elem_mesh.subelem_index_ << 0, 3, 3, 5,
                                3, 4, 1, 4,
                                5, 5, 4, 2;
    break;
  case PolyOrder::P3:
    elem_mesh.subelem_index_ << 0, 3, 3, 4, 4, 8, 9, 9, 7,
                                3, 9, 4, 5, 1, 9, 6, 5, 6,
                                8, 8, 9, 9, 5, 7, 7, 6, 2;
    // clang-format on
  }
}

template <PolyOrder P, ElemType ElemT>
  requires(ElemT == ElemType::Quad)
inline void getSubElemIndex(ElemMesh<2, P, ElemT>& elem_mesh) {
  switch (P) {
    // clang-format off
  case PolyOrder::P1:
    elem_mesh.subelem_index_ << 0,
                                1,
                                2,
                                3;
    break;
  case PolyOrder::P2:
    elem_mesh.subelem_index_ << 0, 4, 7, 8,
                                4, 1, 8, 5,
                                8, 5, 6, 2,
                                7, 8, 3, 6;
    break;
  case PolyOrder::P3:
    elem_mesh.subelem_index_ << 0,  4,  5,  11, 12, 13, 10, 15, 14,
                                4,  5,  1,  12, 13,  6, 15, 14,  7,
                                12, 13, 6,  15, 14,  7,  9,  8,  2,
                                11, 12, 13, 10, 15, 14,  3,  9,  8;
    // clang-format on
  }
}

}  // namespace SubrosaDG
#endif  // SUBROSA_DG_GET_SUBELEM_MAT_HPP_
