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

template <int IntegralNum, ElemType ElemT>
struct ElemGaussQuad : ElemStandard<ElemT> {
  inline static constexpr int kIntegralNum{IntegralNum};
  Eigen::Matrix<Real, getDim<ElemT>(), IntegralNum> integral_point_;
  Eigen::Vector<Real, IntegralNum> weight_;
};

template <PolyOrder P, ElemType ElemT>
struct ElemIntegral : ElemGaussQuad<getElemIntegralNum<ElemT>(P), ElemT> {
  inline static constexpr int kBasisFunNum{calBasisFunNum<ElemT>(P)};
  Eigen::Matrix<Real, getElemIntegralNum<ElemT>(P), kBasisFunNum, Eigen::RowMajor> basis_fun_;
  Eigen::Matrix<Real, getElemIntegralNum<ElemT>(P) * getDim<ElemT>(), kBasisFunNum> grad_basis_fun_;
};

template <PolyOrder P, ElemType ElemT>
struct ElemAdjacencyIntegral {
  Eigen::Matrix<Real, getElemAdjacencyIntegralNum<ElemT>(P), calBasisFunNum<ElemT>(P), Eigen::RowMajor> basis_fun_;
};

template <PolyOrder P>
using TriElemAdjacencyIntegral = ElemAdjacencyIntegral<P, ElemType::Tri>;

template <PolyOrder P>
using QuadElemAdjacencyIntegral = ElemAdjacencyIntegral<P, ElemType::Quad>;

template <PolyOrder P, ElemType ElemT, MeshType MeshT>
struct AdjacencyElemIntegral;

template <PolyOrder P, ElemType ElemT>
struct AdjacencyElemIntegral<P, ElemT, MeshType::Tri> : ElemGaussQuad<getAdjacencyElemIntegralNum<ElemT>(P), ElemT> {
  TriElemAdjacencyIntegral<P> tri_;
};

template <PolyOrder P, ElemType ElemT>
struct AdjacencyElemIntegral<P, ElemT, MeshType::Quad> : ElemGaussQuad<getAdjacencyElemIntegralNum<ElemT>(P), ElemT> {
  QuadElemAdjacencyIntegral<P> quad_;
};

template <PolyOrder P, ElemType ElemT>
struct AdjacencyElemIntegral<P, ElemT, MeshType::TriQuad>
    : ElemGaussQuad<getAdjacencyElemIntegralNum<ElemT>(P), ElemT> {
  TriElemAdjacencyIntegral<P> tri_;
  QuadElemAdjacencyIntegral<P> quad_;
};

template <PolyOrder P>
using TriElemIntegral = ElemIntegral<P, ElemType::Tri>;

template <PolyOrder P>
using QuadElemIntegral = ElemIntegral<P, ElemType::Quad>;

template <PolyOrder P, MeshType MeshT>
using AdjacencyLineElemIntegral = AdjacencyElemIntegral<P, ElemType::Line, MeshT>;

template <int Dim, PolyOrder P, MeshType MeshT>
struct Integral;

template <PolyOrder P>
struct Integral<2, P, MeshType::Tri> {
  AdjacencyLineElemIntegral<P, MeshType::Tri> line_;
  TriElemIntegral<P> tri_;
};

template <PolyOrder P>
struct Integral<2, P, MeshType::Quad> {
  AdjacencyLineElemIntegral<P, MeshType::Quad> line_;
  QuadElemIntegral<P> quad_;
};

template <PolyOrder P>
struct Integral<2, P, MeshType::TriQuad> {
  AdjacencyLineElemIntegral<P, MeshType::TriQuad> line_;
  TriElemIntegral<P> tri_;
  QuadElemIntegral<P> quad_;
};

}  // namespace SubrosaDG

#endif
