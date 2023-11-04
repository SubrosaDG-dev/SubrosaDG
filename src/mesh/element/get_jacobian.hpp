/**
 * @file get_jacobian.hpp
 * @brief The get element jacobian header file.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-22
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_GET_JACOBIAN_HPP_
#define SUBROSA_DG_GET_JACOBIAN_HPP_

#include <gmsh.h>

#include <Eigen/Core>
#include <vector>

#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "integral/get_integral_num.hpp"
#include "integral/integral_structure.hpp"
#include "mesh/get_elem_info.hpp"
#include "mesh/mesh_structure.hpp"

namespace SubrosaDG {

template <int Dim, PolyOrder P, ElemType ElemT>
inline void getElemJacobian(const ElemIntegral<P, ElemT>& elem_integral, ElemMesh<Dim, P, ElemT>& elem_mesh) {
  std::vector<double> jacobians;
  std::vector<double> determinants;
  std::vector<double> coord;
  Eigen::Matrix<Real, 3, getElemIntegralNum<ElemT>(P)> local_coord;
  Eigen::Matrix<Real, Dim, Dim> jacobian_trans;
  local_coord(Eigen::seqN(Eigen::fix<0>, Eigen::fix<getDim<ElemT>()>), Eigen::all) = elem_integral.integral_point_;
  for (Isize i = 0; i < elem_mesh.num_; i++) {
    gmsh::model::mesh::getJacobian(static_cast<Usize>(i + elem_mesh.range_.first),
                                   {local_coord.data(), local_coord.data() + local_coord.size()}, jacobians,
                                   determinants, coord);
    for (Isize j = 0; j < elem_integral.kIntegralNum; j++) {
      for (Isize k = 0; k < Dim; k++) {
        for (Isize l = 0; l < Dim; l++) {
          jacobian_trans(k, l) = static_cast<Real>(jacobians[static_cast<Usize>(j * 9 + k * 3 + l)]);
        }
      }
      elem_mesh.elem_(i).jacobian_trans_inv_(Eigen::all, Eigen::seqN(j * Dim, Eigen::fix<Dim>)) =
          jacobian_trans.inverse();
      elem_mesh.elem_(i).jacobian_det_(j) = static_cast<Real>(determinants[static_cast<Usize>(j)]);
    }
  }
}

template <int Dim, PolyOrder P, ElemType ElemT, MeshType MeshT>
inline void getElemJacobian(const AdjacencyElemIntegral<P, ElemT, MeshT>& adjacency_elem_integral,
                            AdjacencyElemMesh<Dim, P, ElemT, MeshT>& adjacency_elem_mesh) {
  std::vector<double> jacobians;
  std::vector<double> determinants;
  std::vector<double> coord;
  Eigen::Matrix<Real, 3, getAdjacencyElemIntegralNum<ElemT>(P)> local_coord;
  local_coord(Eigen::seqN(Eigen::fix<0>, Eigen::fix<getDim<ElemT>()>), Eigen::all) =
      adjacency_elem_integral.integral_point_;
  for (Isize i = 0; i < adjacency_elem_mesh.internal_.num_; i++) {
    gmsh::model::mesh::getJacobian(static_cast<Usize>(i + adjacency_elem_mesh.internal_.range_.first),
                                   {local_coord.data(), local_coord.data() + local_coord.size()}, jacobians,
                                   determinants, coord);
    for (Isize j = 0; j < adjacency_elem_integral.kIntegralNum; j++) {
      adjacency_elem_mesh.internal_.elem_(i).jacobian_det_(j) = static_cast<Real>(determinants[static_cast<Usize>(j)]);
    }
  }
  for (Isize i = 0; i < adjacency_elem_mesh.boundary_.num_; i++) {
    gmsh::model::mesh::getJacobian(static_cast<Usize>(i + adjacency_elem_mesh.boundary_.range_.first),
                                   {local_coord.data(), local_coord.data() + local_coord.size()}, jacobians,
                                   determinants, coord);
    for (Isize j = 0; j < adjacency_elem_integral.kIntegralNum; j++) {
      adjacency_elem_mesh.boundary_.elem_(i).jacobian_det_(j) = static_cast<Real>(determinants[static_cast<Usize>(j)]);
    }
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_JACOBIAN_HPP_
