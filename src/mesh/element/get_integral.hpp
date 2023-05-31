/**
 * @file get_integral.hpp
 * @brief The header file to get element integral.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-12
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_GET_INTEGRAL_HPP_
#define SUBROSA_DG_GET_INTEGRAL_HPP_

// clang-format off

#include <fmt/core.h>                 // for format
#include <gmsh.h>                     // for getBasisFunctions, getIntegrationPoints
#include <vector>                     // for vector
#include <Eigen/Core>                 // for Matrix, CommaInitializer, MatrixBase::operator*, DenseBase::operator()
#include <Eigen/LU>                   // for MatrixBase::inverse
#include <algorithm>                  // for copy

#include "basic/data_type.hpp"        // for Usize, Isize, Real
#include "mesh/elem_type.hpp"         // for ElemInfo, kQuad, kTri, kLine
#include "mesh/cal_basisfun_num.hpp"  // for getBasisFunNum
#include "mesh/mesh_structure.hpp"    // for ElemStandard, AdjacencyElemIntegral, ElemGaussQuad, ElemIntegral

// clang-format on

namespace SubrosaDG {

template <ElemInfo ElemT>
inline void getElemStandardCoords();

template <>
inline void getElemStandardCoords<kLine>() {
  ElemStandard<kLine>::local_coord << -1.0, 1.0;
}

template <>
inline void getElemStandardCoords<kTri>() {
  ElemStandard<kTri>::local_coord << 0.0, 0.0, 1.0, 0.0, 0.0, 1.0;
}

template <>
inline void getElemStandardCoords<kQuad>() {
  ElemStandard<kQuad>::local_coord << -1.0, -1.0, 1.0, -1.0, 1.0, 1.0, -1.0, 1.0;
}

template <ElemInfo ElemT, Isize PolyOrder>
inline std::vector<double> getElemGaussQuad() {
  using T = ElemGaussQuad<ElemT, PolyOrder>;
  getElemStandardCoords<ElemT>();
  std::vector<double> local_coord;
  std::vector<double> weights;
  gmsh::model::mesh::getIntegrationPoints(ElemT.kTag, fmt::format("Gauss{}", 2 * PolyOrder + 1), local_coord, weights);
  T::integral_num = static_cast<Isize>(weights.size());
  // T::integral_node.resize(ElemT.kDim, T::integral_num);
  T::weight.resize(T::integral_num);
  for (Usize i = 0; i < weights.size(); i++) {
    // for (Usize j = 0; j < static_cast<Usize>(ElemT.kDim); j++) {
    //   T::integral_node(static_cast<Isize>(j), static_cast<Isize>(i)) = static_cast<Real>(local_coord[3 * i + j]);
    // }
    T::weight(static_cast<Isize>(i)) = static_cast<Real>(weights[i]);
  }
  return local_coord;
}

template <ElemInfo ElemT, Isize PolyOrder>
inline void getElemIntegral() {
  using T = ElemIntegral<ElemT, PolyOrder>;
  std::vector<double> local_coords = getElemGaussQuad<ElemT, PolyOrder>();
  int num_components;
  int num_orientations;
  std::vector<double> basis_functions;
  gmsh::model::mesh::getBasisFunctions(ElemT.kTag, local_coords, fmt::format("Lagrange{}", PolyOrder), num_components,
                                       basis_functions, num_orientations);
  T::basis_funs.resize(getBasisFunNum<ElemT>(PolyOrder), T::integral_num);
  for (Usize i = 0; i < static_cast<Usize>(T::integral_num); i++) {
    for (Usize j = 0; j < static_cast<Usize>(getBasisFunNum<ElemT>(PolyOrder)); j++) {
      T::basis_funs(static_cast<Isize>(j), static_cast<Isize>(i)) =
          static_cast<Real>(basis_functions[i * static_cast<Usize>(getBasisFunNum<ElemT>(PolyOrder)) + j]);
    }
  }
  T::local_mass_mat_inv.noalias() =
      ((T::basis_funs.array().rowwise() * T::weight.transpose().array()).matrix() * T::basis_funs.transpose())
          .inverse();
  std::vector<double> grad_basis_functions;
  gmsh::model::mesh::getBasisFunctions(ElemT.kTag, local_coords, fmt::format("GradLagrange{}", PolyOrder),
                                       num_components, grad_basis_functions, num_orientations);
  T::grad_basis_funs.resize(getBasisFunNum<ElemT>(PolyOrder), ElemT.kDim * T::integral_num);
  for (Usize i = 0; i < static_cast<Usize>(T::integral_num); i++) {
    for (Usize j = 0; j < static_cast<Usize>(getBasisFunNum<ElemT>(PolyOrder)); j++) {
      for (Usize k = 0; k < static_cast<Usize>(ElemT.kDim); k++) {
        T::grad_basis_funs(static_cast<Isize>(j), static_cast<Isize>(i * ElemT.kDim + k)) = static_cast<Real>(
            grad_basis_functions[i * static_cast<Usize>(getBasisFunNum<ElemT>(PolyOrder) * 3) + j * 3 + k]);
      }
    }
  }
}

template <ElemInfo ElemT, ElemInfo ParentElemT, Isize PolyOrder>
inline void getAdjacencyElemIntegralFromParent(
    const Isize integral_nodes_num, const std::vector<double>& coords_basis_functions,
    Eigen::Matrix<Real, getBasisFunNum<ParentElemT>(PolyOrder), Eigen::Dynamic>& parent_basis_functions) {
  parent_basis_functions.resize(getBasisFunNum<ParentElemT>(PolyOrder), integral_nodes_num * ParentElemT.kAdjacencyNum);
  Eigen::Matrix<double, 3, Eigen::Dynamic> parent_coords{3, integral_nodes_num * ParentElemT.kAdjacencyNum};
  parent_coords.setZero();
  for (Isize i = 0; i < ParentElemT.kAdjacencyNum; i++) {
    for (Isize j = 0; j < integral_nodes_num; j++) {
      parent_coords(Eigen::seqN(0, Eigen::fix<ParentElemT.kDim>), i * integral_nodes_num + j) =
          (ElemStandard<ParentElemT>::local_coord.row((i + 1) % ParentElemT.kAdjacencyNum) *
               coords_basis_functions[static_cast<Usize>(j * ElemT.kAdjacencyNum + 1)] +
           ElemStandard<ParentElemT>::local_coord.row(i) *
               coords_basis_functions[static_cast<Usize>(j * ElemT.kAdjacencyNum)])
              .transpose();
    }
  }
  int num_components;
  int num_orientations;
  std::vector<double> basis_functions;
  gmsh::model::mesh::getBasisFunctions(
      ParentElemT.kTag, {parent_coords.data(), parent_coords.data() + parent_coords.size()},
      fmt::format("Lagrange{}", PolyOrder), num_components, basis_functions, num_orientations);
  for (Usize i = 0; i < static_cast<Usize>(integral_nodes_num * ParentElemT.kAdjacencyNum); i++) {
    for (Usize j = 0; j < static_cast<Usize>(getBasisFunNum<ParentElemT>(PolyOrder)); j++) {
      parent_basis_functions(static_cast<Isize>(j), static_cast<Isize>(i)) =
          static_cast<Real>(basis_functions[i * static_cast<Usize>(getBasisFunNum<ParentElemT>(PolyOrder)) + j]);
    }
  }
}

template <ElemInfo ElemT, Isize PolyOrder>
inline void getAdjacencyElemIntegral() {
  using T = AdjacencyElemIntegral<ElemT, PolyOrder>;
  std::vector<double> local_coords = getElemGaussQuad<ElemT, PolyOrder>();
  int num_components;
  int num_orientations;
  std::vector<double> cooords_basis_functions;
  gmsh::model::mesh::getBasisFunctions(ElemT.kTag, local_coords, "Lagrange1", num_components, cooords_basis_functions,
                                       num_orientations);
  if constexpr (ElemT.kDim == 1) {
    getAdjacencyElemIntegralFromParent<ElemT, kTri, PolyOrder>(T::integral_num, cooords_basis_functions,
                                                               T::tri_basis_funs);
    getAdjacencyElemIntegralFromParent<ElemT, kQuad, PolyOrder>(T::integral_num, cooords_basis_functions,
                                                                T::quad_basis_funs);
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_INTEGRAL_HPP_
