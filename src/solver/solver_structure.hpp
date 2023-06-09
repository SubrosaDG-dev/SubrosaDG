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
#include <array>

#include "basic/config.hpp"
#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "integral/cal_basisfun_num.hpp"
#include "integral/get_integral_num.hpp"
#include "mesh/elem_type.hpp"

namespace SubrosaDG {

template <EquModel EquModelT>
struct ThermoModel;
template <TimeDiscrete TimeDiscreteT>
struct TimeSolver;

template <>
struct TimeSolver<TimeDiscrete::ExplicitEuler> : TimeVar {
  inline static Real delta_t;
  inline static constexpr std::array<Real, 1> kStepCoeffs{0.0};

  inline constexpr TimeSolver(const TimeVar& time_var) : TimeVar(time_var) {}
};

template <>
struct TimeSolver<TimeDiscrete::RungeKutta3> : TimeVar {
  inline static Real delta_t;
  inline static constexpr std::array<Real, 3> kStepCoeffs{0.0, 3.0 / 4.0, 1.0 / 3.0};

  inline constexpr TimeSolver(const TimeVar& time_var) : TimeVar(time_var) {}
};

template <int Dim, int PolyOrder, ElemInfo ElemT>
struct PerElemSolverBase {
  Eigen::Vector<Eigen::Matrix<Real, Dim + 2, calBasisFunNum<ElemT>(PolyOrder)>, 2> basis_fun_coeff_;
  Eigen::Matrix<Real, Dim + 2, getElemAdjacencyIntegralNum<ElemT>(PolyOrder)> adjacency_integral_;
  Eigen::Matrix<Real, Dim + 2, getElemIntegralNum<ElemT>(PolyOrder) * Dim> elem_integral_;
  Eigen::Matrix<Real, Dim + 2, calBasisFunNum<ElemT>(PolyOrder)> residual_;
};

template <int Dim, int PolyOrder, ElemInfo ElemT, EquModel EquModelT>
struct PerElemSolver;

template <int Dim, int PolyOrder, ElemInfo ElemT>
struct PerElemSolver<Dim, PolyOrder, ElemT, EquModel::Euler> : PerElemSolverBase<Dim, PolyOrder, ElemT> {};

template <int Dim, int PolyOrder, ElemInfo ElemT>
struct PerElemSolver<Dim, PolyOrder, ElemT, EquModel::NS> : PerElemSolverBase<Dim, PolyOrder, ElemT> {};

template <int Dim, int PolyOrder, MeshType MeshT, EquModel EquModelT>
struct ElemSolver;

template <int PolyOrder, EquModel EquModelT>
struct ElemSolver<2, PolyOrder, MeshType::Tri, EquModelT> {
  Eigen::Vector<PerElemSolver<2, PolyOrder, kTri, EquModelT>, Eigen::Dynamic> tri_;
};

template <int PolyOrder, EquModel EquModelT>
struct ElemSolver<2, PolyOrder, MeshType::Quad, EquModelT> {
  Eigen::Vector<PerElemSolver<2, PolyOrder, kQuad, EquModelT>, Eigen::Dynamic> quad_;
};

template <int PolyOrder, EquModel EquModelT>
struct ElemSolver<2, PolyOrder, MeshType::TriQuad, EquModelT> {
  Eigen::Vector<PerElemSolver<2, PolyOrder, kTri, EquModelT>, Eigen::Dynamic> tri_;
  Eigen::Vector<PerElemSolver<2, PolyOrder, kQuad, EquModelT>, Eigen::Dynamic> quad_;
};

template <EquModel EquModelT, TimeDiscrete TimeDiscreteT>
struct SolverBase {
  TimeSolver<TimeDiscreteT> time_solver_;
  ThermoModel<EquModelT> thermo_model_;

  inline constexpr SolverBase(const TimeVar& time_var, const ThermoModel<EquModelT>& thermo_model)
      : time_solver_(time_var), thermo_model_(thermo_model) {}
};

template <int Dim, int PolyOrder, MeshType MeshT, ConvectiveFlux ConvectiveFluxT, TimeDiscrete TimeDiscreteT>
struct SolverEuler : SolverBase<EquModel::Euler, TimeDiscreteT> {
  ElemSolver<Dim, PolyOrder, MeshT, EquModel::Euler> elem_;

  using SolverBase<EquModel::Euler, TimeDiscreteT>::SolverBase;
};

template <int Dim, int PolyOrder, MeshType MeshT, ConvectiveFlux ConvectiveFluxT, ViscousFlux ViscousFluxT,
          TimeDiscrete TimeDiscreteT>
struct SolverNS {};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_SOLVER_STRUCTURE_HPP_
