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

#include <Eigen/Core>           // for Vector, DenseBase::col, Matrix, Dynamic, DenseBase::operator(), DenseBase::re...
#include <memory>               // for make_unique, unique_ptr
#include <Eigen/Geometry>       // for MatrixBase::cross

#include "basic/data_type.hpp"  // for Real, Isize
#include "mesh/elem_type.hpp"   // for ElemInfo

// clang-format on

namespace SubrosaDG {

template <Isize Dim, ElemInfo ElemT>
struct ElemMesh;
template <Isize Dim, ElemInfo ElemT>
struct AdjacencyElemMesh;

template <Isize Dim, ElemInfo ElemT>
inline std::unique_ptr<Eigen::Vector<Real, Eigen::Dynamic>> calElemLength(
    Isize elem_num, const Eigen::Matrix<Real, Dim * ElemT.kNodeNum, Eigen::Dynamic>& elem_node) {
  auto measure = std::make_unique<Eigen::Vector<Real, Eigen::Dynamic>>(elem_num);
  Eigen::Matrix<Real, 3, ElemT.kNodeNum> node = Eigen::Matrix<Real, 3, ElemT.kNodeNum>::Zero();
  for (Isize i = 0; i < elem_num; i++) {
    node(Eigen::seqN(0, Eigen::fix<Dim>), Eigen::all) = elem_node.col(i).reshaped(Dim, ElemT.kNodeNum);
    measure->operator()(i) = (node.col(1) - node.col(0)).norm();
  }
  return measure;
}

// TODO: for 3d need to mutiply the normal vector
template <Isize Dim, ElemInfo ElemT>
inline std::unique_ptr<Eigen::Vector<Real, Eigen::Dynamic>> calElemArea(
    Isize elem_num, const Eigen::Matrix<Real, Dim * ElemT.kNodeNum, Eigen::Dynamic>& elem_node) {
  auto measure = std::make_unique<Eigen::Vector<Real, Eigen::Dynamic>>(elem_num);
  Eigen::Matrix<Real, 3, ElemT.kNodeNum> node = Eigen::Matrix<Real, 3, ElemT.kNodeNum>::Zero();
  for (Isize i = 0; i < elem_num; i++) {
    node(Eigen::seqN(0, Eigen::fix<Dim>), Eigen::all) = elem_node.col(i).reshaped(Dim, ElemT.kNodeNum);
    Eigen::Vector<Real, 3> cross_product = node.col(ElemT.kNodeNum - 1).cross(node.col(0));
    for (Isize j = 0; j < ElemT.kNodeNum - 1; j++) {
      cross_product += node.col(j).cross(node.col(j + 1));
    }
    measure->operator()(i) = 0.5 * cross_product.norm();
  }
  return measure;
}

template <Isize Dim, ElemInfo ElemT>
inline std::unique_ptr<Eigen::Vector<Real, Eigen::Dynamic>> calElemMeasure(const ElemMesh<Dim, ElemT>& elem_mesh) {
  if constexpr (ElemT.kDim == 2) {
    return calElemArea<Dim, ElemT>(elem_mesh.num_, elem_mesh.node_);
  } else if constexpr (ElemT.kDim == 3) {
    // TODO: need to be implemented
    return calElemVolume<Dim, ElemT>(elem_mesh.num_, elem_mesh.node_);
  }
}

template <Isize Dim, ElemInfo ElemT>
inline std::unique_ptr<Eigen::Vector<Real, Eigen::Dynamic>> calElemMeasure(
    const AdjacencyElemMesh<Dim, ElemT>& adjacency_elem_mesh) {
  if constexpr (ElemT.kDim == 1) {
    return calElemLength<Dim, ElemT>(adjacency_elem_mesh.num_tag_.second, adjacency_elem_mesh.node_);
  } else if constexpr (ElemT.kDim == 2) {
    return calElemArea<Dim, ElemT>(adjacency_elem_mesh.num_tag_.second, adjacency_elem_mesh.node_);
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CAL_MEASURE_HPP_
