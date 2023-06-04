/**
 * @file get_standard_coord.hpp
 * @brief The get standard coordinate header file.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-06-03
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_GET_STANDARD_COORD_HPP_
#define SUBROSA_DG_GET_STANDARD_COORD_HPP_

// clang-format off

#include <Eigen/Core>                       // for Matrix, CommaInitializer, DenseBase

#include "mesh/elem_type.hpp"               // for kLine, kQuad, kTri, ElemInfo
#include "integral/integral_structure.hpp"  // for ElemStandard

// clang-format on

namespace SubrosaDG {

template <ElemInfo ElemT>
inline void getElemStandardCoord();

template <>
inline void getElemStandardCoord<kLine>() {
  ElemStandard<kLine>::coord << -1.0, 1.0;
}

template <>
inline void getElemStandardCoord<kTri>() {
  ElemStandard<kTri>::coord << 0.0, 0.0, 1.0, 0.0, 0.0, 1.0;
}

template <>
inline void getElemStandardCoord<kQuad>() {
  ElemStandard<kQuad>::coord << -1.0, -1.0, 1.0, -1.0, 1.0, 1.0, -1.0, 1.0;
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_STANDARD_COORD_HPP_
