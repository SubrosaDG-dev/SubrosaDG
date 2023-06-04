/**
 * @file integral_structure.hpp
 * @brief The integral structure head file.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-06-03
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_INTEGRAL_STRUCTURE_HPP_
#define SUBROSA_DG_INTEGRAL_STRUCTURE_HPP_

// clang-format off

#include <Eigen/Core>                     // for Matrix, Vector

#include "basic/concept.hpp"              // IWYU pragma: keep
#include "basic/data_type.hpp"            // for Real
#include "mesh/elem_type.hpp"             // for ElemInfo, kQuad, kTri, kLine
#include "integral/cal_basisfun_num.hpp"  // for calBasisFunNum
#include "integral/get_integral_num.hpp"  // for getElemAdjacencyIntegralNum, getElemIntegralNum
#include "basic/enum.hpp"                 // for MeshType

// clang-format on

namespace SubrosaDG {

template <ElemInfo ElemT>
struct ElemStandard {
  inline static Eigen::Matrix<Real, ElemT.kNodeNum, ElemT.kDim> coord;
};

template <int PolyOrder, ElemInfo ElemT>
struct ElemGaussQuad : ElemStandard<ElemT> {
  inline static constexpr int kIntegralNum{getElemIntegralNum<ElemT>(PolyOrder)};
  Eigen::Vector<Real, kIntegralNum> weight_;
};

template <int PolyOrder, ElemInfo ElemT>
struct ElemIntegral : ElemGaussQuad<PolyOrder, ElemT> {
  inline static constexpr int kBasisFunNum{calBasisFunNum<ElemT>(PolyOrder)};
  Eigen::Matrix<Real, kBasisFunNum, ElemGaussQuad<PolyOrder, ElemT>::kIntegralNum> basis_fun_;
  Eigen::Matrix<Real, kBasisFunNum, kBasisFunNum> local_mass_mat_inv_;
  Eigen::Matrix<Real, ElemGaussQuad<PolyOrder, ElemT>::kIntegralNum * ElemT.kDim, kBasisFunNum> grad_basis_fun_;
};

template <int PolyOrder, ElemInfo ElemT>
struct AdjacencyElemIntegral : ElemGaussQuad<PolyOrder, ElemT> {};

template <int PolyOrder, ElemInfo ElemT>
  requires Is1dElem<ElemT>
struct AdjacencyElemIntegral<PolyOrder, ElemT> : ElemGaussQuad<PolyOrder, ElemT> {
  Eigen::Matrix<Real, getElemAdjacencyIntegralNum<kTri>(PolyOrder), calBasisFunNum<kTri>(PolyOrder)> tri_basis_fun_;
  Eigen::Matrix<Real, getElemAdjacencyIntegralNum<kQuad>(PolyOrder), calBasisFunNum<kQuad>(PolyOrder)> quad_basis_fun_;
};

template <int PolyOrder>
using TriElemIntegral = ElemIntegral<PolyOrder, kTri>;

template <int PolyOrder>
using QuadElemIntegral = ElemIntegral<PolyOrder, kQuad>;

template <int PolyOrder>
using AdjacencyLineElemIntegral = AdjacencyElemIntegral<PolyOrder, kLine>;

template <int Dim, int PolyOrder, MeshType MeshT>
struct Integral {};

template <int PolyOrder>
struct Integral<2, PolyOrder, MeshType::Tri> {
  AdjacencyLineElemIntegral<PolyOrder> line_;
  TriElemIntegral<PolyOrder> tri_;
};

template <int PolyOrder>
struct Integral<2, PolyOrder, MeshType::Quad> {
  AdjacencyLineElemIntegral<PolyOrder> line_;
  QuadElemIntegral<PolyOrder> quad_;
};

template <int PolyOrder>
struct Integral<2, PolyOrder, MeshType::TriQuad> {
  AdjacencyLineElemIntegral<PolyOrder> line_;
  TriElemIntegral<PolyOrder> tri_;
  QuadElemIntegral<PolyOrder> quad_;
};

}  // namespace SubrosaDG

#endif
