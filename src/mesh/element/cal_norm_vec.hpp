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

#include <Eigen/Core>           // for DenseBase::col, Matrix, MatrixBase::operator-, Vector
#include <Eigen/Geometry>       // for Rotation2D

#include "basic/constant.hpp"   // for kPi
#include "basic/data_type.hpp"  // for Real, Isize
#include "mesh/elem_type.hpp"   // for ElemInfo

// clang-format on

namespace SubrosaDG {

enum class MeshType;
template <int Dim, ElemInfo ElemT, MeshType MeshT>
struct AdjacencyElemMesh;

template <ElemInfo ElemT>
inline void calNormVec(const Eigen::Matrix<Real, 2, ElemT.kNodeNum>& node, Eigen::Vector<Real, 2>& norm_vec) {
  Eigen::Rotation2D<Real> rotation{-kPi / 2.0};
  norm_vec = rotation * (node.col(1) - node.col(0)).normalized();
}

template <ElemInfo ElemT>
inline void calNormVec(const Eigen::Matrix<Real, 3, ElemT.kNodeNum>& node, Eigen::Vector<Real, 3>& norm_vec);

template <int Dim, ElemInfo ElemT, MeshType MeshT>
inline void calAdjacencyElemNormVec(AdjacencyElemMesh<Dim, ElemT, MeshT>& adjacency_elem_mesh) {
  for (Isize i = 0; i < adjacency_elem_mesh.internal_.num_; i++) {
    calNormVec<ElemT>(adjacency_elem_mesh.internal_.elem_(i).node_, adjacency_elem_mesh.internal_.elem_(i).norm_vec_);
  }
  for (Isize i = 0; i < adjacency_elem_mesh.boundary_.num_; i++) {
    calNormVec<ElemT>(adjacency_elem_mesh.boundary_.elem_(i).node_, adjacency_elem_mesh.boundary_.elem_(i).norm_vec_);
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CAL_NORM_VEC_HPP_
