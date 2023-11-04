/**
 * @file view_structure.hpp
 * @brief The head file to define the view structure.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-08-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_VIEW_VIEW_STRUCTURE_HPP_
#define SUBROSA_DG_VIEW_VIEW_STRUCTURE_HPP_

#include <Eigen/Core>
#include <basic/enum.hpp>

#include "basic/data_type.hpp"
#include "integral/cal_basisfun_num.hpp"
#include "solver/variable/get_var_num.hpp"
#include "view/variable/get_output_var_num.hpp"

namespace SubrosaDG {

template <int Dim, EquModel EquModelT>
struct NodeSolverView {
  Eigen::Matrix<Real, getOutputVarNum<EquModelT>(Dim), Eigen::Dynamic> output_var_;
};

template <int Dim, PolyOrder P, ElemType ElemT, EquModel EquModelT>
struct PerElemSolverView {
  Eigen::Matrix<Real, getConservedVarNum<EquModelT>(Dim), calBasisFunNum<ElemT>(P)> basis_fun_coeff_;
};

template <int Dim, PolyOrder P, ElemType ElemT, EquModel EquModelT>
struct ElemSolverView {
  Eigen::Array<PerElemSolverView<Dim, P, ElemT, EquModelT>, Eigen::Dynamic, 1> elem_;
};

template <int Dim, PolyOrder P, MeshType MeshT, EquModel EquModelT>
struct View;

template <int Dim, PolyOrder P, EquModel EquModelT>
struct View<Dim, P, MeshType::Tri, EquModelT> {
  NodeSolverView<Dim, EquModelT> node_;
  ElemSolverView<Dim, P, ElemType::Tri, EquModelT> tri_;
};

template <int Dim, PolyOrder P, EquModel EquModelT>
struct View<Dim, P, MeshType::Quad, EquModelT> {
  NodeSolverView<Dim, EquModelT> node_;
  ElemSolverView<Dim, P, ElemType::Quad, EquModelT> quad_;
};

template <int Dim, PolyOrder P, EquModel EquModelT>
struct View<Dim, P, MeshType::TriQuad, EquModelT> {
  NodeSolverView<Dim, EquModelT> node_;
  ElemSolverView<Dim, P, ElemType::Tri, EquModelT> tri_;
  ElemSolverView<Dim, P, ElemType::Quad, EquModelT> quad_;
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_VIEW_VIEW_STRUCTURE_HPP_
