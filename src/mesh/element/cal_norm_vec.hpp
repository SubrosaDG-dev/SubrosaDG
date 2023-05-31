/**
 * @file cal_norm_vec.hpp
 * @brief The get element normal header file.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-24
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CAL_NORM_VEC_HPP_
#define SUBROSA_DG_CAL_NORM_VEC_HPP_

// clang-format off

#include <basic/constant.hpp>   // for kPi
#include <Eigen/Core>           // for DenseBase::col, MatrixBase::operator-, DenseBase::reshaped, Matrix
#include <Eigen/Geometry>       // for Rotation2D

#include "basic/data_type.hpp"  // for Real, Isize
#include "mesh/elem_type.hpp"   // for ElemInfo

// clang-format on

namespace SubrosaDG {

template <Isize Dim, ElemInfo ElemT>
struct AdjacencyElemMesh;

template <Isize Dim, ElemInfo ElemT>
inline void calAdjacencyElemNormVec(AdjacencyElemMesh<Dim, ElemT>& adjacency_elem_mesh) {
  adjacency_elem_mesh.norm_vec_.resize(Dim, adjacency_elem_mesh.num_tag_.second);
  Eigen::Matrix<Real, Dim, ElemT.kNodeNum> node;
  if constexpr (ElemT.kDim == 1) {
    Eigen::Rotation2D<Real> rotation{-kPi / 2.0};
    for (Isize i = 0; i < adjacency_elem_mesh.num_tag_.second; i++) {
      node = adjacency_elem_mesh.node_.col(i).reshaped(Dim, ElemT.kNodeNum);
      adjacency_elem_mesh.norm_vec_.col(i) = rotation * (node.col(1) - node.col(0)).normalized();
    }
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CAL_NORM_VEC_HPP_
