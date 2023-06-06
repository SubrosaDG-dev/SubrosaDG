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

// clang-format off

#include <Eigen/Core>                     // for Vector, Dynamic, Matrix
#include <array>                          // for array

#include "basic/concept.hpp"              // IWYU pragma: keep
#include "basic/data_type.hpp"            // for Real
#include "basic/enum.hpp"                 // for EquModel, MeshType, ConvectiveFlux (ptr only), ViscousFlux (ptr only)
#include "basic/config.hpp"               // for TimeDiscrete, TimeVar, kExplicitEuler, kRungeKutta3, ThermoModel
#include "mesh/elem_type.hpp"             // for ElemInfo, kQuad, kTri
#include "integral/cal_basisfun_num.hpp"  // for calBasisFunNum
#include "integral/get_integral_num.hpp"  // for getElemAdjacencyIntegralNum, getElemIntegralNum

// clang-format on

namespace SubrosaDG {

template <TimeDiscrete TimeDiscreteT>
struct TimeSolver : TimeVar<TimeDiscreteT> {};

template <>
struct TimeSolver<kExplicitEuler> : TimeVar<kExplicitEuler> {
  inline static constexpr std::array<Real, 1> kStepCoeffs{0.0};

  inline constexpr TimeSolver(const TimeVar<kExplicitEuler>& time_var) : TimeVar<kExplicitEuler>(time_var) {}
};

template <>
struct TimeSolver<kRungeKutta3> : TimeVar<kRungeKutta3> {
  inline static constexpr std::array<Real, 3> kStepCoeffs{0.0, 3.0 / 4.0, 1.0 / 3.0};

  inline constexpr TimeSolver(const TimeVar<kRungeKutta3>& time_var) : TimeVar<kRungeKutta3>(time_var) {}
};

template <int Dim, int PolyOrder, ElemInfo ElemT, TimeDiscrete TimeDiscreteT>
struct PerElemConvectiveSolver {};

template <int Dim, int PolyOrder, ElemInfo ElemT, TimeDiscrete TimeDiscreteT>
  requires IsExplicit<TimeDiscreteT>
struct PerElemConvectiveSolver<Dim, PolyOrder, ElemT, TimeDiscreteT> {
  Eigen::Vector<Eigen::Matrix<Real, Dim + 2, calBasisFunNum<ElemT>(PolyOrder)>, 2> basis_fun_coeff_;
  Eigen::Matrix<Real, Dim + 2, getElemAdjacencyIntegralNum<ElemT>(PolyOrder)> adjacency_integral_;
  Eigen::Matrix<Real, Dim + 2, getElemIntegralNum<ElemT>(PolyOrder)> element_integral_;
  Eigen::Matrix<Real, Dim + 2, calBasisFunNum<ElemT>(PolyOrder)> residual_;
};

template <int Dim, int PolyOrder, ElemInfo ElemT, TimeDiscrete TimeDiscreteT>
struct PerElemVisciousSolver {};

template <int Dim, int PolyOrder, ElemInfo ElemT, TimeDiscrete TimeDiscreteT>
  requires IsExplicit<TimeDiscreteT>
struct PerElemVisciousSolver<Dim, PolyOrder, ElemT, TimeDiscreteT>
    : PerElemConvectiveSolver<Dim, PolyOrder, ElemT, TimeDiscreteT> {};

template <int Dim, int PolyOrder, MeshType MeshT, TimeDiscrete TimeDiscreteT>
struct ElemConvectiveSolver {};

template <int PolyOrder, TimeDiscrete TimeDiscreteT>
struct ElemConvectiveSolver<2, PolyOrder, MeshType::Tri, TimeDiscreteT> {
  Eigen::Vector<PerElemConvectiveSolver<2, PolyOrder, kTri, TimeDiscreteT>, Eigen::Dynamic> tri_;
};

template <int PolyOrder, TimeDiscrete TimeDiscreteT>
struct ElemConvectiveSolver<2, PolyOrder, MeshType::Quad, TimeDiscreteT> {
  Eigen::Vector<PerElemConvectiveSolver<2, PolyOrder, kQuad, TimeDiscreteT>, Eigen::Dynamic> quad_;
};

template <int PolyOrder, TimeDiscrete TimeDiscreteT>
struct ElemConvectiveSolver<2, PolyOrder, MeshType::TriQuad, TimeDiscreteT> {
  Eigen::Vector<PerElemConvectiveSolver<2, PolyOrder, kTri, TimeDiscreteT>, Eigen::Dynamic> tri_;
  Eigen::Vector<PerElemConvectiveSolver<2, PolyOrder, kQuad, TimeDiscreteT>, Eigen::Dynamic> quad_;
};

template <int Dim, int PolyOrder, MeshType MeshT, TimeDiscrete TimeDiscreteT>
struct ElemVisciousSolver {};

template <int PolyOrder, TimeDiscrete TimeDiscreteT>
struct ElemVisciousSolver<2, PolyOrder, MeshType::Tri, TimeDiscreteT> {
  Eigen::Vector<PerElemVisciousSolver<2, PolyOrder, kTri, TimeDiscreteT>, Eigen::Dynamic> tri_;
};

template <int PolyOrder, TimeDiscrete TimeDiscreteT>
struct ElemVisciousSolver<2, PolyOrder, MeshType::Quad, TimeDiscreteT> {
  Eigen::Vector<PerElemVisciousSolver<2, PolyOrder, kQuad, TimeDiscreteT>, Eigen::Dynamic> quad_;
};

template <int PolyOrder, TimeDiscrete TimeDiscreteT>
struct ElemVisciousSolver<2, PolyOrder, MeshType::TriQuad, TimeDiscreteT> {
  Eigen::Vector<PerElemVisciousSolver<2, PolyOrder, kTri, TimeDiscreteT>, Eigen::Dynamic> tri_;
  Eigen::Vector<PerElemVisciousSolver<2, PolyOrder, kQuad, TimeDiscreteT>, Eigen::Dynamic> quad_;
};

template <EquModel EquModelT, TimeDiscrete TimeDiscreteT>
struct SolverBase {
  TimeSolver<TimeDiscreteT> time_solver_;
  ThermoModel<EquModelT> thermo_model_;

  inline constexpr SolverBase(const TimeVar<TimeDiscreteT>& time_var, const ThermoModel<EquModelT>& thermo_model)
      : time_solver_(time_var), thermo_model_(thermo_model) {}
};

template <int Dim, int PolyOrder, MeshType MeshT, ConvectiveFlux ConvectiveFluxT, TimeDiscrete TimeDiscreteT>
struct SolverEuler : SolverBase<EquModel::Euler, TimeDiscreteT> {
  ElemConvectiveSolver<Dim, PolyOrder, MeshT, TimeDiscreteT> elem_solver_;

  using SolverBase<EquModel::Euler, TimeDiscreteT>::SolverBase;
};

template <int Dim, int PolyOrder, MeshType MeshT, ConvectiveFlux ConvectiveFluxT, ViscousFlux ViscousFluxT,
          TimeDiscrete TimeDiscreteT>
struct SolverNS {};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_SOLVER_STRUCTURE_HPP_
