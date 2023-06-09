/**
 * @file get_standard.hpp
 * @brief The get standard coordinate and measure header file.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-06-03
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_GET_STANDARD_HPP_
#define SUBROSA_DG_GET_STANDARD_HPP_

#include <Eigen/Core>

#include "integral/integral_structure.hpp"
#include "mesh/elem_type.hpp"

namespace SubrosaDG {

template <ElemInfo ElemT>
inline void getElemStandard();

template <>
inline void getElemStandard<kLine>() {
  ElemStandard<kLine>::measure = 2.0;
  ElemStandard<kLine>::coord << -1.0, 1.0;
}

template <>
inline void getElemStandard<kTri>() {
  ElemStandard<kTri>::measure = 0.5;
  ElemStandard<kTri>::coord << 0.0, 0.0, 1.0, 0.0, 0.0, 1.0;
}

template <>
inline void getElemStandard<kQuad>() {
  ElemStandard<kQuad>::measure = 4.0;
  ElemStandard<kQuad>::coord << -1.0, -1.0, 1.0, -1.0, 1.0, 1.0, -1.0, 1.0;
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_STANDARD_HPP_
