/**
 * @file solver_structure.hpp
 * @brief The solver structure header file.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-16
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_SOLVER_STRUCTURE_HPP_
#define SUBROSA_DG_SOLVER_STRUCTURE_HPP_

#include <Eigen/Core>

#include "basic/config.hpp"
#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "integral/cal_basisfun_num.hpp"
#include "integral/get_integral_num.hpp"
#include "solver/time_discrete/time_solver.hpp"

namespace SubrosaDG {

template <int Dim, PolyOrder P, ElemType ElemT>
struct PerElemSolverBase {
  Eigen::Vector<Eigen::Matrix<Real, Dim + 2, calBasisFunNum<ElemT>(P)>, 2> basis_fun_coeff_;
  Eigen::Matrix<Real, Dim + 2, getElemAdjacencyIntegralNum<ElemT>(P)> adjacency_integral_;
  Eigen::Matrix<Real, Dim + 2, getElemIntegralNum<ElemT>(P) * Dim> elem_integral_;
  Eigen::Matrix<Real, Dim + 2, calBasisFunNum<ElemT>(P)> residual_;
};

template <int Dim, PolyOrder P, ElemType ElemT, EquModel EquModelT>
struct PerElemSolver;

template <int Dim, PolyOrder P, ElemType ElemT>
struct PerElemSolver<Dim, P, ElemT, EquModel::Euler> : PerElemSolverBase<Dim, P, ElemT> {};

template <int Dim, PolyOrder P, ElemType ElemT>
struct PerElemSolver<Dim, P, ElemT, EquModel::NS> : PerElemSolverBase<Dim, P, ElemT> {};

template <int Dim, PolyOrder P, ElemType ElemT, EquModel EquModelT>
struct ElemSolver {
  Eigen::Vector<PerElemSolver<Dim, P, ElemT, EquModelT>, Eigen::Dynamic> elem_;
};

template <PolyOrder P, EquModel EquModelT>
using TriElemSolver = ElemSolver<2, P, ElemType::Tri, EquModelT>;

template <PolyOrder P, EquModel EquModelT>
using QuadElemSolver = ElemSolver<2, P, ElemType::Quad, EquModelT>;

template <int Dim, PolyOrder P, EquModel EquModelT, MeshType MeshT>
struct Solver;

template <PolyOrder P, EquModel EquModelT>
struct Solver<2, P, EquModelT, MeshType::Tri> {
  TriElemSolver<P, EquModelT> tri_;
};

template <PolyOrder P, EquModel EquModelT>
struct Solver<2, P, EquModelT, MeshType::Quad> {
  QuadElemSolver<P, EquModelT> quad_;
};

template <PolyOrder P, EquModel EquModelT>
struct Solver<2, P, EquModelT, MeshType::TriQuad> {
  TriElemSolver<P, EquModelT> tri_;
  QuadElemSolver<P, EquModelT> quad_;
};

template <int Dim, EquModel EquModelT, TimeDiscrete TimeDiscreteT>
struct SolverSupplementalBase {
  const ThermoModel<EquModelT> thermo_model_;
  const TimeSolver<TimeDiscreteT> time_solver_;
  Real delta_t_;
  Eigen::Vector<Real, Dim + 3> farfield_primitive_var_;

  inline constexpr SolverSupplementalBase(const ThermoModel<EquModelT>& thermo_model, const TimeVar& time_var)
      : thermo_model_(thermo_model), time_solver_(time_var) {}
};

template <int Dim, EquModel EquModelT, TimeDiscrete TimeDiscreteT>
struct SolverSupplemental;

template <int Dim, TimeDiscrete TimeDiscreteT>
struct SolverSupplemental<Dim, EquModel::Euler, TimeDiscreteT>
    : SolverSupplementalBase<Dim, EquModel::Euler, TimeDiscreteT> {
  using SolverSupplementalBase<Dim, EquModel::Euler, TimeDiscreteT>::SolverSupplementalBase;
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_SOLVER_STRUCTURE_HPP_
