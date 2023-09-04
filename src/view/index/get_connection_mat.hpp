/**
 * @file get_connection_mat.hpp
 * @brief The head file to get the connection matrix.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-08-12
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_GET_CONNECTION_MAT_HPP_
#define SUBROSA_DG_GET_CONNECTION_MAT_HPP_

#include <Eigen/Core>

#include "basic/concept.hpp"
#include "mesh/get_elem_info.hpp"
#include "view/index/get_subelem_num.hpp"
#include "view/view_structure.hpp"

namespace SubrosaDG {

template <PolyOrder P, ElemType ElemT>
  requires(ElemT == ElemType::Line)
inline void getElemSubElemConnectionMat(Eigen::Matrix<int, getNodeNum<ElemType::Line>(PolyOrder::P1),
                                                      getSubElemNum<ElemType::Line>(P)>& subelem_connection_mat) {
  switch (P) {
    // clang-format off
  case PolyOrder::P1:
    subelem_connection_mat << 0,
                              1;
    break;
  case PolyOrder::P2:
    subelem_connection_mat << 0, 2,
                              2, 1;
    break;
  case PolyOrder::P3:
    subelem_connection_mat << 0, 2, 3,
                              2, 3, 1;
    // clang-format on
  }
}

template <PolyOrder P, ElemType ElemT>
  requires(ElemT == ElemType::Tri)
inline void getElemSubElemConnectionMat(Eigen::Matrix<int, getNodeNum<ElemType::Tri>(PolyOrder::P1),
                                                      getSubElemNum<ElemType::Tri>(P)>& subelem_connection_mat) {
  switch (P) {
    // clang-format off
  case PolyOrder::P1:
    subelem_connection_mat << 0,
                              1,
                              2;
    break;
  case PolyOrder::P2:
    subelem_connection_mat << 0, 3, 3, 5,
                              3, 4, 1, 4,
                              5, 5, 4, 2;
    break;
  case PolyOrder::P3:
    subelem_connection_mat << 0, 3, 3, 4, 4, 8, 9, 9, 7,
                              3, 9, 4, 5, 1, 9, 6, 5, 6,
                              8, 8, 9, 9, 5, 7, 7, 6, 2;
    // clang-format on
  }
}

template <PolyOrder P, ElemType ElemT>
  requires(ElemT == ElemType::Quad)
inline void getElemSubElemConnectionMat(Eigen::Matrix<int, getNodeNum<ElemType::Quad>(PolyOrder::P1),
                                                      getSubElemNum<ElemType::Quad>(P)>& subelem_connection_mat) {
  switch (P) {
    // clang-format off
  case PolyOrder::P1:
    subelem_connection_mat << 0,
                              1,
                              2,
                              3;
    break;
  case PolyOrder::P2:
    subelem_connection_mat << 0, 4, 7, 8,
                              4, 1, 8, 5,
                              8, 5, 6, 2,
                              7, 8, 3, 6;
    break;
  case PolyOrder::P3:
    subelem_connection_mat << 0,  4,  5,  11, 12, 13, 10, 15, 14,
                              4,  5,  1,  12, 13,  6, 15, 14,  7,
                              12, 13, 6,  15, 14,  7,  9,  8,  2,
                              11, 12, 13, 10, 15, 14,  3,  9,  8;
    // clang-format on
  }
}

template <PolyOrder P, MeshType MeshT, EquModel EquModelT>
inline void getSubElemConnectionMat(View<2, P, MeshT, EquModelT>& view) {
  if constexpr (HasTri<MeshT>) {
    getElemSubElemConnectionMat<P, ElemType::Tri>(view.tri_.subelem_connection_mat_);
  }
  if constexpr (HasQuad<MeshT>) {
    getElemSubElemConnectionMat<P, ElemType::Quad>(view.quad_.subelem_connection_mat_);
  }
}

}  // namespace SubrosaDG
#endif  // SUBROSA_DG_GET_CONNECTION_MAT_HPP_
