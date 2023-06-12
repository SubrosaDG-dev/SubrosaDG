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

#include <fmt/core.h>
#include <gmsh.h>

#include <Eigen/Core>
#include <vector>

#include "basic/concept.hpp"
#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "integral/cal_basisfun_num.hpp"
#include "integral/get_integral_num.hpp"
#include "integral/get_standard.hpp"
#include "integral/integral_structure.hpp"
#include "mesh/get_elem_info.hpp"

namespace SubrosaDG {

template <int Dim, int PolyOrder, MeshType MeshT>
struct Integral;
template <int PolyOrder, ElemType ElemT, MeshType MeshT>
struct AdjacencyElemIntegral;

template <int PolyOrder, ElemType ElemT>
inline std::vector<double> getElemGaussQuad(const int gauss_accuracy,
                                            ElemGaussQuad<PolyOrder, ElemT>& elem_gauss_quad) {
  using T = ElemIntegral<PolyOrder, ElemT>;
  getElemStandard<ElemT>();
  std::vector<double> local_coords;
  std::vector<double> weights;
  gmsh::model::mesh::getIntegrationPoints(getTopology<ElemT>(), fmt::format("Gauss{}", gauss_accuracy), local_coords,
                                          weights);
  for (Isize i = 0; i < T::kIntegralNum; i++) {
    elem_gauss_quad.weight_(i) = static_cast<Real>(weights[static_cast<Usize>(i)]);
  }
  return local_coords;
}

template <int PolyOrder, ElemType ElemT>
inline void getElemIntegral(ElemIntegral<PolyOrder, ElemT>& elem_integral) {
  using T = ElemIntegral<PolyOrder, ElemT>;
  std::vector<double> local_coords = getElemGaussQuad(2 * PolyOrder, elem_integral);
  int num_components;
  int num_orientations;
  std::vector<double> basis_functions;
  gmsh::model::mesh::getBasisFunctions(getTopology<ElemT>(), local_coords, fmt::format("Lagrange{}", PolyOrder),
                                       num_components, basis_functions, num_orientations);
  for (Isize i = 0; i < T::kIntegralNum; i++) {
    for (Isize j = 0; j < T::kBasisFunNum; j++) {
      elem_integral.basis_fun_(i, j) = static_cast<Real>(basis_functions[static_cast<Usize>(i * T::kBasisFunNum + j)]);
    }
  }
  elem_integral.local_mass_mat_inv_.noalias() =
      (elem_integral.basis_fun_.transpose() *
       (elem_integral.basis_fun_.array().colwise() * elem_integral.weight_.array()).matrix())
          .inverse();
  std::vector<double> grad_basis_functions;
  gmsh::model::mesh::getBasisFunctions(getTopology<ElemT>(), local_coords, fmt::format("GradLagrange{}", PolyOrder),
                                       num_components, grad_basis_functions, num_orientations);
  for (Isize i = 0; i < T::kIntegralNum; i++) {
    for (Isize j = 0; j < T::kBasisFunNum; j++) {
      for (Isize k = 0; k < getDim<ElemT>(); k++) {
        elem_integral.grad_basis_fun_(i * getDim<ElemT>() + k, j) =
            static_cast<Real>(grad_basis_functions[static_cast<Usize>((i * T::kBasisFunNum + j) * 3 + k)]);
      }
    }
  }
}

template <int PolyOrder, ElemType ElemT, ElemType ParentElemT, MeshType MeshT>
inline void getAdjacencyElemIntegralFromParent(
    const std::vector<double>& coords_basis_functions,
    Eigen::Matrix<Real, getElemAdjacencyIntegralNum<ParentElemT>(PolyOrder), calBasisFunNum<ParentElemT>(PolyOrder),
                  Eigen::RowMajor>& parent_basis_fun) {
  using T = AdjacencyElemIntegral<PolyOrder, ElemT, MeshT>;
  using ParentT = ElemIntegral<PolyOrder, ParentElemT>;
  Eigen::Matrix<double, 3, T::kIntegralNum * getAdjacencyNum<ParentElemT>()> parent_coords =
      Eigen::Matrix<double, 3, T::kIntegralNum * getAdjacencyNum<ParentElemT>()>::Zero();
  for (Isize i = 0; i < getAdjacencyNum<ParentElemT>(); i++) {
    for (Isize j = 0; j < T::kIntegralNum; j++) {
      parent_coords(Eigen::seqN(0, Eigen::fix<getDim<ParentElemT>()>), i * T::kIntegralNum + j).noalias() =
          (ElemStandard<ParentElemT>::coord.row((i + 1) % getAdjacencyNum<ParentElemT>()) *
               coords_basis_functions[static_cast<Usize>(j * getAdjacencyNum<ElemT>() + 1)] +
           ElemStandard<ParentElemT>::coord.row(i) *
               coords_basis_functions[static_cast<Usize>(j * getAdjacencyNum<ElemT>())])
              .transpose();
    }
  }
  int num_components;
  int num_orientations;
  std::vector<double> basis_functions;
  gmsh::model::mesh::getBasisFunctions(
      getTopology<ParentElemT>(), {parent_coords.data(), parent_coords.data() + parent_coords.size()},
      fmt::format("Lagrange{}", PolyOrder), num_components, basis_functions, num_orientations);
  for (Isize i = 0; i < T::kIntegralNum * getAdjacencyNum<ParentElemT>(); i++) {
    for (Isize j = 0; j < ParentT::kBasisFunNum; j++) {
      parent_basis_fun(i, j) = static_cast<Real>(basis_functions[static_cast<Usize>(i * ParentT::kBasisFunNum + j)]);
    }
  }
}

template <int PolyOrder, ElemType ElemT, MeshType MeshT>
inline void getAdjacencyElemIntegral(AdjacencyElemIntegral<PolyOrder, ElemT, MeshT>& adjacency_elem_integral) {
  std::vector<double> local_coords = getElemGaussQuad(2 * PolyOrder + 1, adjacency_elem_integral);
  int num_components;
  int num_orientations;
  std::vector<double> cooords_basis_functions;
  gmsh::model::mesh::getBasisFunctions(getTopology<ElemT>(), local_coords, "Lagrange1", num_components,
                                       cooords_basis_functions, num_orientations);
  if constexpr (HasTri<MeshT>) {
    getAdjacencyElemIntegralFromParent<PolyOrder, ElemT, ElemType::Tri, MeshT>(cooords_basis_functions,
                                                                               adjacency_elem_integral.tri_basis_fun_);
  }
  if constexpr (HasQuad<MeshT>) {
    getAdjacencyElemIntegralFromParent<PolyOrder, ElemT, ElemType::Quad, MeshT>(
        cooords_basis_functions, adjacency_elem_integral.quad_basis_fun_);
  }
}

template <int PolyOrder, MeshType MeshT>
inline void getIntegral(Integral<2, PolyOrder, MeshT>& integral) {
  if constexpr (HasTri<MeshT>) {
    getElemIntegral(integral.tri_);
  }
  if constexpr (HasQuad<MeshT>) {
    getElemIntegral(integral.quad_);
  }
  getAdjacencyElemIntegral(integral.line_);
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_INTEGRAL_HPP_
