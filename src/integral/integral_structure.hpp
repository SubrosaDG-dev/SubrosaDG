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

#include <Eigen/Core>

#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "integral/cal_basisfun_num.hpp"
#include "integral/get_integral_num.hpp"
#include "mesh/get_elem_info.hpp"

namespace SubrosaDG {

template <ElemType ElemT>
struct ElemStandard {
  inline static Real measure;
  inline static Eigen::Matrix<Real, getNodeNum<ElemT>(), getDim<ElemT>()> coord;
};

template <int PolyOrder, ElemType ElemT>
struct ElemGaussQuad : ElemStandard<ElemT> {
  inline static constexpr int kIntegralNum{getElemIntegralNum<ElemT>(PolyOrder)};
  Eigen::Vector<Real, kIntegralNum> weight_;
};

template <int PolyOrder, ElemType ElemT>
struct ElemIntegral : ElemGaussQuad<PolyOrder, ElemT> {
  inline static constexpr int kBasisFunNum{calBasisFunNum<ElemT>(PolyOrder)};
  Eigen::Matrix<Real, ElemGaussQuad<PolyOrder, ElemT>::kIntegralNum, kBasisFunNum, Eigen::RowMajor> basis_fun_;
  Eigen::Matrix<Real, kBasisFunNum, kBasisFunNum> local_mass_mat_inv_;
  Eigen::Matrix<Real, ElemGaussQuad<PolyOrder, ElemT>::kIntegralNum * getDim<ElemT>(), kBasisFunNum> grad_basis_fun_;
};

template <int PolyOrder, ElemType ElemT, MeshType MeshT>
struct AdjacencyElemIntegral;

template <int PolyOrder, ElemType ElemT>
struct AdjacencyElemIntegral<PolyOrder, ElemT, MeshType::Tri> : ElemGaussQuad<PolyOrder, ElemT> {
  Eigen::Matrix<Real, getElemAdjacencyIntegralNum<ElemType::Tri>(PolyOrder), calBasisFunNum<ElemType::Tri>(PolyOrder),
                Eigen::RowMajor>
      tri_basis_fun_;
};

template <int PolyOrder, ElemType ElemT>
struct AdjacencyElemIntegral<PolyOrder, ElemT, MeshType::Quad> : ElemGaussQuad<PolyOrder, ElemT> {
  Eigen::Matrix<Real, getElemAdjacencyIntegralNum<ElemType::Quad>(PolyOrder), calBasisFunNum<ElemType::Quad>(PolyOrder),
                Eigen::RowMajor>
      quad_basis_fun_;
};

template <int PolyOrder, ElemType ElemT>
struct AdjacencyElemIntegral<PolyOrder, ElemT, MeshType::TriQuad> : ElemGaussQuad<PolyOrder, ElemT> {
  Eigen::Matrix<Real, getElemAdjacencyIntegralNum<ElemType::Tri>(PolyOrder), calBasisFunNum<ElemType::Tri>(PolyOrder),
                Eigen::RowMajor>
      tri_basis_fun_;
  Eigen::Matrix<Real, getElemAdjacencyIntegralNum<ElemType::Quad>(PolyOrder), calBasisFunNum<ElemType::Quad>(PolyOrder),
                Eigen::RowMajor>
      quad_basis_fun_;
};

template <int PolyOrder>
using TriElemIntegral = ElemIntegral<PolyOrder, ElemType::Tri>;

template <int PolyOrder>
using QuadElemIntegral = ElemIntegral<PolyOrder, ElemType::Quad>;

template <int PolyOrder, MeshType MeshT>
using AdjacencyLineElemIntegral = AdjacencyElemIntegral<PolyOrder, ElemType::Line, MeshT>;

template <int Dim, int PolyOrder, MeshType MeshT>
struct Integral;

template <int PolyOrder>
struct Integral<2, PolyOrder, MeshType::Tri> {
  AdjacencyLineElemIntegral<PolyOrder, MeshType::Tri> line_;
  TriElemIntegral<PolyOrder> tri_;
};

template <int PolyOrder>
struct Integral<2, PolyOrder, MeshType::Quad> {
  AdjacencyLineElemIntegral<PolyOrder, MeshType::Quad> line_;
  QuadElemIntegral<PolyOrder> quad_;
};

template <int PolyOrder>
struct Integral<2, PolyOrder, MeshType::TriQuad> {
  AdjacencyLineElemIntegral<PolyOrder, MeshType::TriQuad> line_;
  TriElemIntegral<PolyOrder> tri_;
  QuadElemIntegral<PolyOrder> quad_;
};

}  // namespace SubrosaDG

#endif
