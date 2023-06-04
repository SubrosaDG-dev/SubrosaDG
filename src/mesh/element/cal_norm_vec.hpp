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
#include <Eigen/Core>           // for DenseBase<>::ConstColXpr, Matrix, Vector, MatrixBase::operator-, CwiseBinaryOp
#include <Eigen/Geometry>       // for Rotation2D

#include "basic/data_type.hpp"  // for Real, Isize
#include "mesh/elem_type.hpp"   // for ElemInfo, kLine

// clang-format on

namespace SubrosaDG {

template <int Dim, ElemInfo ElemT>
struct AdjacencyElemMesh;

template <ElemInfo ElemT>
inline void calNormVec2d(const Eigen::Matrix<Real, 2, ElemT.kNodeNum>& node, Eigen::Vector<Real, 2>& norm_vec);

template <>
inline void calNormVec2d<kLine>(const Eigen::Matrix<Real, 2, kLine.kNodeNum>& node, Eigen::Vector<Real, 2>& norm_vec) {
  Eigen::Rotation2D<Real> rotation{-kPi / 2.0};
  norm_vec = rotation * (node.col(1) - node.col(0)).normalized();
}

template <ElemInfo ElemT>
inline void calNormVec3d(const Eigen::Matrix<Real, 3, ElemT.kNodeNum>& node, Eigen::Vector<Real, 3>& norm_vec);

template <int Dim, ElemInfo ElemT>
inline void calAdjacencyElemNormVec(AdjacencyElemMesh<Dim, ElemT>& adjacency_elem_mesh) {
  if constexpr (Dim == 2) {
    for (Isize i = 0; i < adjacency_elem_mesh.internal_.num_; i++) {
      calNormVec2d<ElemT>(adjacency_elem_mesh.internal_.elem_(i).node_,
                          adjacency_elem_mesh.internal_.elem_(i).norm_vec_);
    }
    for (Isize i = 0; i < adjacency_elem_mesh.boundary_.num_; i++) {
      calNormVec2d<ElemT>(adjacency_elem_mesh.boundary_.elem_(i).node_,
                          adjacency_elem_mesh.boundary_.elem_(i).norm_vec_);
    }
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CAL_NORM_VEC_HPP_
