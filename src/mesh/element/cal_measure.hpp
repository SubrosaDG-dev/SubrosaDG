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

// clang-format off

#include <Eigen/Core>           // for Vector, DenseBase::col, DenseBase::operator(), Dynamic, Matrix, MatrixBase::o...
#include <memory>               // for make_unique, unique_ptr
#include <Eigen/Geometry>       // for MatrixBase::cross

#include "basic/data_type.hpp"  // for Real, Isize
#include "mesh/elem_type.hpp"   // for ElemInfo

// clang-format on

namespace SubrosaDG {

template <int Dim, ElemInfo ElemT>
struct ElemMesh;
template <int Dim, ElemInfo ElemT>
struct AdjacencyElemMesh;

template <int Dim, ElemInfo ElemT>
inline Real calElemLength(const Eigen::Matrix<Real, Dim, ElemT.kNodeNum>& node) {
  return (node.col(1) - node.col(0)).norm();
}

template <int Dim, ElemInfo ElemT>
inline Real calElemArea(const Eigen::Matrix<Real, Dim, ElemT.kNodeNum>& node) {
  Eigen::Matrix<Real, 3, ElemT.kNodeNum> node3d = Eigen::Matrix<Real, 3, ElemT.kNodeNum>::Zero();
  node3d(Eigen::seqN(0, Eigen::fix<Dim>), Eigen::all) = node;
  Eigen::Vector<Real, 3> cross_product = Eigen::Vector<Real, 3>::Zero();
  for (Isize i = 0; i < ElemT.kNodeNum; i++) {
    cross_product += node3d.col(i).cross(node3d.col((i + 1) % ElemT.kNodeNum));
  }
  return 0.5 * cross_product.norm();
}

// TODO: for Dim == 3 need to mutiply the normal vector
template <int Dim, ElemInfo ElemT>
inline std::unique_ptr<Eigen::Vector<Real, Eigen::Dynamic>> calElemMeasure(const ElemMesh<Dim, ElemT>& elem_mesh) {
  auto measure = std::make_unique<Eigen::Vector<Real, Eigen::Dynamic>>(elem_mesh.num_);
  for (Isize i = 0; i < elem_mesh.num_; i++) {
    if constexpr (ElemT.kDim == 2) {
      measure->operator()(i) = calElemArea<Dim, ElemT>(elem_mesh.elem_(i).node_);
    }
  }
  return measure;
}

template <int Dim, ElemInfo ElemT>
inline std::unique_ptr<Eigen::Vector<Real, Eigen::Dynamic>> calElemMeasure(
    const AdjacencyElemMesh<Dim, ElemT>& adjacency_elem_mesh) {
  auto measure = std::make_unique<Eigen::Vector<Real, Eigen::Dynamic>>(adjacency_elem_mesh.internal_.num_ +
                                                                       adjacency_elem_mesh.boundary_.num_);
  for (Isize i = 0; i < adjacency_elem_mesh.internal_.num_; i++) {
    if constexpr (ElemT.kDim == 1) {
      measure->operator()(i) = calElemLength<Dim, ElemT>(adjacency_elem_mesh.internal_.elem_(i).node_);
    } else if constexpr (ElemT.kDim == 2) {
      measure->operator()(i) = calElemArea<Dim, ElemT>(adjacency_elem_mesh.internal_.elem_(i).node_);
    }
  }
  for (Isize i = 0; i < adjacency_elem_mesh.boundary_.num_; i++) {
    if constexpr (ElemT.kDim == 1) {
      measure->operator()(i + adjacency_elem_mesh.internal_.num_) =
          calElemLength<Dim, ElemT>(adjacency_elem_mesh.boundary_.elem_(i).node_);
    } else if constexpr (ElemT.kDim == 2) {
      measure->operator()(i + adjacency_elem_mesh.internal_.num_) =
          calElemArea<Dim, ElemT>(adjacency_elem_mesh.boundary_.elem_(i).node_);
    }
  }
  return measure;
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CAL_MEASURE_HPP_