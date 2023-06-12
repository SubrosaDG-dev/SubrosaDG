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

#include "basic/enum.hpp"
#include "integral/integral_structure.hpp"

namespace SubrosaDG {

template <ElemType ElemT>
inline void getElemStandard();

template <>
inline void getElemStandard<ElemType::Line>() {
  ElemStandard<ElemType::Line>::measure = 2.0;
  ElemStandard<ElemType::Line>::coord << -1.0, 1.0;
}

template <>
inline void getElemStandard<ElemType::Tri>() {
  ElemStandard<ElemType::Tri>::measure = 0.5;
  ElemStandard<ElemType::Tri>::coord << 0.0, 0.0, 1.0, 0.0, 0.0, 1.0;
}

template <>
inline void getElemStandard<ElemType::Quad>() {
  ElemStandard<ElemType::Quad>::measure = 4.0;
  ElemStandard<ElemType::Quad>::coord << -1.0, -1.0, 1.0, -1.0, 1.0, 1.0, -1.0, 1.0;
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_STANDARD_HPP_
