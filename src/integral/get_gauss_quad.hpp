/**
 * @file get_gauss_quad.hpp
 * @brief The head file to get gauss quadrature.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-07-17
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_GET_GAUSS_QUAD_HPP_
#define SUBROSA_DG_GET_GAUSS_QUAD_HPP_

#include <fmt/core.h>
#include <gmsh.h>

#include <vector>

#include "basic/constant.hpp"
#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "integral/get_standard.hpp"
#include "integral/integral_structure.hpp"
#include "mesh/get_elem_info.hpp"

namespace SubrosaDG {

template <int IntegralNum, PolyOrder P, ElemType ElemT>
inline std::vector<double> getElemGaussQuad(const int gauss_accuracy,
                                            ElemGaussQuad<IntegralNum, P, ElemT>& elem_gauss_quad) {
  getElemStandard<P, ElemT>();
  std::vector<double> local_coords;
  std::vector<double> weights;
  gmsh::model::mesh::getIntegrationPoints(getTopology<ElemT>(P), fmt::format("Gauss{}", gauss_accuracy), local_coords,
                                          weights);
  for (Isize i = 0; i < IntegralNum; i++) {
    for (Isize j = 0; j < getDim<ElemT>(); j++) {
      elem_gauss_quad.integral_point_(j, i) = static_cast<Real>(local_coords[static_cast<Usize>(i * 3 + j)]);
    }
    elem_gauss_quad.weight_(i) = static_cast<Real>(weights[static_cast<Usize>(i)]);
  }
  return local_coords;
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_GAUSS_QUAD_HPP_
