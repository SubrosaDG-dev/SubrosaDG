/**
 * @file cal_measure.hpp
 * @brief The header file to calculate element measure.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-06
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CAL_MEASURE_HPP_
#define SUBROSA_DG_CAL_MEASURE_HPP_

#include <Eigen/Core>

#include "basic/concept.hpp"
#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "mesh/get_elem_info.hpp"
#include "mesh/mesh_structure.hpp"

namespace SubrosaDG {

template <int Dim, ElemType ElemT>
  requires Is1dElem<ElemT>
inline Real calMeasure(const Eigen::Matrix<Real, Dim, getNodeNum<ElemT>(PolyOrder::P1)>& node) {
  return (node.col(1) - node.col(0)).norm();
}

template <int Dim, ElemType ElemT>
  requires(Dim == 2) && Is2dElem<ElemT>
inline Real calMeasure(const Eigen::Matrix<Real, Dim, getNodeNum<ElemT>(PolyOrder::P1)>& node) {
  Eigen::Matrix<Real, 3, getNodeNum<ElemT>(PolyOrder::P1)> node3d =
      Eigen::Matrix<Real, 3, getNodeNum<ElemT>(PolyOrder::P1)>::Zero();
  node3d(Eigen::seqN(Eigen::fix<0>, Eigen::fix<Dim>), Eigen::all) = node;
  Eigen::Vector<Real, 3> cross_product = Eigen::Vector<Real, 3>::Zero();
  for (Isize i = 0; i < getNodeNum<ElemT>(PolyOrder::P1); i++) {
    cross_product += node3d.col(i).cross(node3d.col((i + 1) % getNodeNum<ElemT>(PolyOrder::P1)));
  }
  return 0.5 * cross_product.norm();
}

template <int Dim, ElemType ElemT>
  requires(Dim == 3) && Is2dElem<ElemT>
inline Real calMeasure(const Eigen::Matrix<Real, Dim, getNodeNum<ElemT>()>& node);

template <int Dim, PolyOrder P, ElemType ElemT>
inline void calElemMeasure(ElemMesh<Dim, P, ElemT>& elem_mesh) {
  for (Isize i = 0; i < elem_mesh.num_; i++) {
    for (Isize j = 0; j < getSubElemNum<ElemT>(P); j++) {
      elem_mesh.elem_(i).subelem_measure_(j) =
          calMeasure<Dim, ElemT>(elem_mesh.elem_(i).node_(Eigen::all, elem_mesh.subelem_index_.col(j)));
    }
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CAL_MEASURE_HPP_
