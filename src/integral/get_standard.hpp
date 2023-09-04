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

#include <gmsh.h>

#include <Eigen/Core>

#include "basic/enum.hpp"
#include "integral/integral_structure.hpp"
#include "mesh/element/cal_measure.hpp"

namespace SubrosaDG {

template <PolyOrder P, ElemType ElemT>
inline void getElemStandard() {
  std::string name;
  int d;
  int order;
  int numv;
  int numpv;
  std::vector<double> param;
  gmsh::model::mesh::getElementProperties(getTopology<ElemT>(P), name, d, order, numv, param, numpv);
  for (Isize i = 0; i < getNodeNum<ElemT>(P); i++) {
    for (Isize j = 0; j < getDim<ElemT>(); j++) {
      ElemStandard<P, ElemT>::node(j, i) = static_cast<Real>(param[static_cast<Usize>(i * getDim<ElemT>() + j)]);
    }
  }
  ElemStandard<P, ElemT>::measure = calMeasure<getDim<ElemT>(), ElemT>(
      ElemStandard<P, ElemT>::node(Eigen::all, Eigen::seqN(Eigen::fix<0>, Eigen::fix<getNodeNum<ElemT>(PolyOrder::P1)>)));
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_STANDARD_HPP_
