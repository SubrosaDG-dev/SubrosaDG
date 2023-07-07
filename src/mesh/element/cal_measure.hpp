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
#include <memory>

#include "basic/concept.hpp"
#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "mesh/get_elem_info.hpp"
#include "mesh/mesh_structure.hpp"

namespace SubrosaDG {

template <int Dim, ElemType ElemT>
  requires Is1dElem<ElemT>
inline Real calElemMeasure(const Eigen::Matrix<Real, Dim, getNodeNum<ElemT>()>& node) {
  return (node.col(1) - node.col(0)).norm();
}

template <int Dim, ElemType ElemT>
  requires(Dim == 2) && Is2dElem<ElemT>
inline Real calElemMeasure(const Eigen::Matrix<Real, Dim, getNodeNum<ElemT>()>& node) {
  Eigen::Matrix<Real, 3, getNodeNum<ElemT>()> node3d = Eigen::Matrix<Real, 3, getNodeNum<ElemT>()>::Zero();
  node3d(Eigen::seqN(0, Eigen::fix<Dim>), Eigen::all) = node;
  Eigen::Vector<Real, 3> cross_product = Eigen::Vector<Real, 3>::Zero();
  for (Isize i = 0; i < getNodeNum<ElemT>(); i++) {
    cross_product += node3d.col(i).cross(node3d.col((i + 1) % getNodeNum<ElemT>()));
  }
  return 0.5 * cross_product.norm();
}

template <int Dim, ElemType ElemT>
  requires(Dim == 3) && Is2dElem<ElemT>
inline Real calElemMeasure(const Eigen::Matrix<Real, Dim, getNodeNum<ElemT>()>& node);

template <int Dim, ElemType ElemT>
inline std::unique_ptr<Eigen::Vector<Real, Eigen::Dynamic>> calElemMeasure(const ElemMesh<Dim, ElemT>& elem_mesh) {
  auto measure = std::make_unique<Eigen::Vector<Real, Eigen::Dynamic>>(elem_mesh.num_);
  for (Isize i = 0; i < elem_mesh.num_; i++) {
    measure->operator()(i) = calElemMeasure<Dim, ElemT>(elem_mesh.elem_(i).node_);
  }
  return measure;
}

template <int Dim, ElemType ElemT, MeshType MeshT>
inline std::unique_ptr<Eigen::Vector<Real, Eigen::Dynamic>> calElemMeasure(
    const AdjacencyElemMesh<Dim, ElemT, MeshT>& adjacency_elem_mesh) {
  auto measure = std::make_unique<Eigen::Vector<Real, Eigen::Dynamic>>(adjacency_elem_mesh.internal_.num_ +
                                                                       adjacency_elem_mesh.boundary_.num_);
  for (Isize i = 0; i < adjacency_elem_mesh.internal_.num_; i++) {
    measure->operator()(i) = calElemMeasure<Dim, ElemT>(adjacency_elem_mesh.internal_.elem_(i).node_);
  }
  for (Isize i = 0; i < adjacency_elem_mesh.boundary_.num_; i++) {
    measure->operator()(i + adjacency_elem_mesh.internal_.num_) =
        calElemMeasure<Dim, ElemT>(adjacency_elem_mesh.boundary_.elem_(i).node_);
  }
  return measure;
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CAL_MEASURE_HPP_
