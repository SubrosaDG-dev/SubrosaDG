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

#include <fmt/core.h>                       // for format
#include <gmsh.h>                           // for getBasisFunctions, getIntegrationPoints
#include <vector>                           // for vector
#include <Eigen/Core>                       // for MatrixBase::operator*, DenseBase::operator(), DenseBase::row, Mat...
#include <Eigen/LU>                         // for MatrixBase::inverse
#include <algorithm>                        // for copy

#include "basic/data_type.hpp"              // for Isize, Usize, Real
#include "mesh/elem_type.hpp"               // for ElemInfo, kQuad, kTri
#include "integral/cal_basisfun_num.hpp"    // for calBasisFunNum
#include "integral/integral_structure.hpp"  // for ElemIntegral, Integral (ptr only), ElemStandard, AdjacencyElemInt...
#include "integral/get_standard_coord.hpp"  // for getElemStandardCoord
#include "integral/get_integral_num.hpp"    // for getElemAdjacencyIntegralNum
#include "basic/enum.hpp"                   // for MeshType

// clang-format on

namespace SubrosaDG {

template <int PolyOrder, ElemInfo ElemT>
inline std::vector<double> getElemGaussQuad(ElemGaussQuad<PolyOrder, ElemT>& elem_gauss_quad) {
  using T = ElemIntegral<PolyOrder, ElemT>;
  getElemStandardCoord<ElemT>();
  std::vector<double> local_coords;
  std::vector<double> weights;
  gmsh::model::mesh::getIntegrationPoints(ElemT.kTopology, fmt::format("Gauss{}", 2 * PolyOrder + 1), local_coords,
                                          weights);
  for (Isize i = 0; i < T::kIntegralNum; i++) {
    elem_gauss_quad.weight_(i) = static_cast<Real>(weights[static_cast<Usize>(i)]);
  }
  return local_coords;
}

template <int PolyOrder, ElemInfo ElemT>
inline void getElemIntegral(ElemIntegral<PolyOrder, ElemT>& elem_integral) {
  using T = ElemIntegral<PolyOrder, ElemT>;
  std::vector<double> local_coords = getElemGaussQuad(elem_integral);
  int num_components;
  int num_orientations;
  std::vector<double> basis_functions;
  gmsh::model::mesh::getBasisFunctions(ElemT.kTopology, local_coords, fmt::format("Lagrange{}", PolyOrder),
                                       num_components, basis_functions, num_orientations);
  for (Isize i = 0; i < T::kIntegralNum; i++) {
    for (Isize j = 0; j < T::kBasisFunNum; j++) {
      elem_integral.basis_fun_(j, i) = static_cast<Real>(basis_functions[static_cast<Usize>(i * T::kBasisFunNum + j)]);
    }
  }
  elem_integral.local_mass_mat_inv_.noalias() =
      ((elem_integral.basis_fun_.array().rowwise() * elem_integral.weight_.transpose().array()).matrix() *
       elem_integral.basis_fun_.transpose())
          .inverse();
  std::vector<double> grad_basis_functions;
  gmsh::model::mesh::getBasisFunctions(ElemT.kTopology, local_coords, fmt::format("GradLagrange{}", PolyOrder),
                                       num_components, grad_basis_functions, num_orientations);
  for (Isize i = 0; i < T::kIntegralNum; i++) {
    for (Isize j = 0; j < T::kBasisFunNum; j++) {
      for (Isize k = 0; k < ElemT.kDim; k++) {
        elem_integral.grad_basis_fun_(i * ElemT.kDim + k, j) =
            static_cast<Real>(grad_basis_functions[static_cast<Usize>((i * T::kBasisFunNum + j) * 3 + k)]);
      }
    }
  }
}

template <int PolyOrder, ElemInfo ElemT, ElemInfo ParentElemT, MeshType MeshT>
inline void getAdjacencyElemIntegralFromParent(
    const std::vector<double>& coords_basis_functions,
    Eigen::Matrix<Real, getElemAdjacencyIntegralNum<ParentElemT>(PolyOrder), calBasisFunNum<ParentElemT>(PolyOrder),
                  Eigen::RowMajor>& parent_basis_fun) {
  using T = AdjacencyElemIntegral<PolyOrder, ElemT, MeshT>;
  using ParentT = ElemIntegral<PolyOrder, ParentElemT>;
  Eigen::Matrix<double, 3, T::kIntegralNum* ParentElemT.kAdjacencyNum> parent_coords =
      Eigen::Matrix<double, 3, T::kIntegralNum * ParentElemT.kAdjacencyNum>::Zero();
  for (Isize i = 0; i < ParentElemT.kAdjacencyNum; i++) {
    for (Isize j = 0; j < T::kIntegralNum; j++) {
      parent_coords(Eigen::seqN(0, Eigen::fix<ParentElemT.kDim>), i * T::kIntegralNum + j) =
          (ElemStandard<ParentElemT>::coord.row((i + 1) % ParentElemT.kAdjacencyNum) *
               coords_basis_functions[static_cast<Usize>(j * ElemT.kAdjacencyNum + 1)] +
           ElemStandard<ParentElemT>::coord.row(i) *
               coords_basis_functions[static_cast<Usize>(j * ElemT.kAdjacencyNum)])
              .transpose();
    }
  }
  int num_components;
  int num_orientations;
  std::vector<double> basis_functions;
  gmsh::model::mesh::getBasisFunctions(
      ParentElemT.kTopology, {parent_coords.data(), parent_coords.data() + parent_coords.size()},
      fmt::format("Lagrange{}", PolyOrder), num_components, basis_functions, num_orientations);
  for (Isize i = 0; i < T::kIntegralNum * ParentElemT.kAdjacencyNum; i++) {
    for (Isize j = 0; j < ParentT::kBasisFunNum; j++) {
      parent_basis_fun(i, j) = static_cast<Real>(basis_functions[static_cast<Usize>(i * ParentT::kBasisFunNum + j)]);
    }
  }
}

template <int PolyOrder, ElemInfo ElemT, MeshType MeshT>
inline void getAdjacencyElemIntegral(AdjacencyElemIntegral<PolyOrder, ElemT, MeshT>& adjacency_elem_integral) {
  std::vector<double> local_coords = getElemGaussQuad(adjacency_elem_integral);
  int num_components;
  int num_orientations;
  std::vector<double> cooords_basis_functions;
  gmsh::model::mesh::getBasisFunctions(ElemT.kTopology, local_coords, "Lagrange1", num_components,
                                       cooords_basis_functions, num_orientations);
  if constexpr (MeshT == MeshType::Tri) {
    getAdjacencyElemIntegralFromParent<PolyOrder, ElemT, kTri, MeshT>(cooords_basis_functions,
                                                                      adjacency_elem_integral.tri_basis_fun_);
  } else if constexpr (MeshT == MeshType::Quad) {
    getAdjacencyElemIntegralFromParent<PolyOrder, ElemT, kQuad, MeshT>(cooords_basis_functions,
                                                                       adjacency_elem_integral.quad_basis_fun_);
  } else if constexpr (MeshT == MeshType::TriQuad) {
    getAdjacencyElemIntegralFromParent<PolyOrder, ElemT, kTri, MeshT>(cooords_basis_functions,
                                                                      adjacency_elem_integral.tri_basis_fun_);
    getAdjacencyElemIntegralFromParent<PolyOrder, ElemT, kQuad, MeshT>(cooords_basis_functions,
                                                                       adjacency_elem_integral.quad_basis_fun_);
  }
}

template <int PolyOrder>
inline void getIntegral(Integral<2, PolyOrder, MeshType::Tri>& integral) {
  getElemIntegral(integral.tri_);
  getAdjacencyElemIntegral(integral.line_);
}

template <int PolyOrder>
inline void getIntegral(Integral<2, PolyOrder, MeshType::Quad>& integral) {
  getElemIntegral(integral.quad_);
  getAdjacencyElemIntegral(integral.line_);
}

template <int PolyOrder>
inline void getIntegral(Integral<2, PolyOrder, MeshType::TriQuad>& integral) {
  getElemIntegral(integral.tri_);
  getElemIntegral(integral.quad_);
  getAdjacencyElemIntegral(integral.line_);
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_INTEGRAL_HPP_
