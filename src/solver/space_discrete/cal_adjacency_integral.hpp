/**
 * @file cal_adjacency_integral.hpp
 * @brief The header file to calculate adjacency element integral.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-17
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CAL_ADJACENCY_INTEGRAL_HPP_
#define SUBROSA_DG_CAL_ADJACENCY_INTEGRAL_HPP_

#include <Eigen/Core>

#include "basic/concept.hpp"
#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "config/thermo_model.hpp"
#include "integral/integral_structure.hpp"
#include "mesh/mesh_structure.hpp"
#include "solver/convective_flux/cal_roe_flux.hpp"
#include "solver/solver_structure.hpp"
#include "solver/space_discrete/store_to_elem.hpp"
#include "solver/variable/cal_primitive_var.hpp"
#include "solver/variable/cal_wall_var.hpp"
#include "solver/variable/get_parent_var.hpp"
#include "solver/variable/get_var_num.hpp"

namespace SubrosaDG {

template <int Dim, PolyOrder P, ElemType ElemT, MeshType MeshT, EquModel EquModelT, ConvectiveFlux ConvectiveFluxT>
inline void calInternalAdjacencyElemIntegral(const AdjacencyElemIntegral<P, ElemT, MeshT>& adjacency_elem_integral,
                                             const AdjacencyElemMesh<Dim, P, ElemT, MeshT>& adjacency_elem_mesh,
                                             const ThermoModel<EquModelT>& thermo_model,
                                             Solver<Dim, P, MeshT, EquModelT>& solver) {
  Eigen::Vector<Real, getConservedVarNum<EquModelT>(Dim)> l_conserved_var;
  Eigen::Vector<Real, getPrimitiveVarNum<EquModelT>(Dim)> l_primitive_var;
  Eigen::Vector<Real, getConservedVarNum<EquModelT>(Dim)> r_conserved_var;
  Eigen::Vector<Real, getPrimitiveVarNum<EquModelT>(Dim)> r_primitive_var;
  Eigen::Vector<Real, getConservedVarNum<EquModelT>(Dim)> convective_flux;
  Eigen::Vector<Real, getConservedVarNum<EquModelT>(Dim)> adjancy_integral;
  Isize l_elem_tag;
  Isize l_adjacency_order;
  Isize r_elem_tag;
  Isize r_adjacency_order;
#ifdef SUBROSA_DG_WITH_OPENMP
#pragma omp parallel for default(none)                                                                          \
    schedule(auto) private(l_conserved_var, l_primitive_var, r_conserved_var, r_primitive_var, convective_flux, \
                           adjancy_integral, l_elem_tag, l_adjacency_order, r_elem_tag, r_adjacency_order)      \
        shared(adjacency_elem_mesh, adjacency_elem_integral, thermo_model, solver)
#endif
  for (Isize i = 0; i < adjacency_elem_mesh.internal_.num_; i++) {
    l_elem_tag = adjacency_elem_mesh.internal_.elem_(i).parent_index_(0);
    r_elem_tag = adjacency_elem_mesh.internal_.elem_(i).parent_index_(1);
    l_adjacency_order = adjacency_elem_mesh.internal_.elem_(i).adjacency_index_(0);
    r_adjacency_order = adjacency_elem_mesh.internal_.elem_(i).adjacency_index_(1);
    for (Isize j = 0; j < adjacency_elem_integral.kIntegralNum; j++) {
      if constexpr (IsMixed<MeshT>) {
        getParentVar(adjacency_elem_mesh.internal_.elem_(i).typology_index_(0), l_elem_tag,
                     l_adjacency_order * adjacency_elem_integral.kIntegralNum + j, adjacency_elem_integral, solver,
                     l_conserved_var);
        getParentVar(adjacency_elem_mesh.internal_.elem_(i).typology_index_(1), r_elem_tag,
                     r_adjacency_order * adjacency_elem_integral.kIntegralNum + j, adjacency_elem_integral, solver,
                     r_conserved_var);
      } else {
        getParentVar(l_elem_tag, l_adjacency_order * adjacency_elem_integral.kIntegralNum + j, adjacency_elem_integral,
                     solver, l_conserved_var);
        getParentVar(r_elem_tag, r_adjacency_order * adjacency_elem_integral.kIntegralNum + j, adjacency_elem_integral,
                     solver, r_conserved_var);
      }
      calPrimitiveVar(thermo_model, l_conserved_var, l_primitive_var);
      calPrimitiveVar(thermo_model, r_conserved_var, r_primitive_var);
      if constexpr (ConvectiveFluxT == ConvectiveFlux::Roe) {
        calRoeFlux(thermo_model, adjacency_elem_mesh.internal_.elem_(i).norm_vec_, l_primitive_var, r_primitive_var,
                   convective_flux);
      }
      adjancy_integral.noalias() = convective_flux * adjacency_elem_mesh.internal_.elem_(i).jacobian_det_(j) *
                                   adjacency_elem_integral.weight_(j);
      if constexpr (IsMixed<MeshT>) {
        storeAdjacencyIntegralToElem(adjacency_elem_mesh.internal_.elem_(i).typology_index_(0), l_elem_tag,
                                     l_adjacency_order * adjacency_elem_integral.kIntegralNum + j, adjancy_integral,
                                     solver);
        storeAdjacencyIntegralToElem(adjacency_elem_mesh.internal_.elem_(i).typology_index_(1), r_elem_tag,
                                     r_adjacency_order * adjacency_elem_integral.kIntegralNum + j, -adjancy_integral,
                                     solver);
      } else {
        storeAdjacencyIntegralToElem(l_elem_tag, l_adjacency_order * adjacency_elem_integral.kIntegralNum + j,
                                     adjancy_integral, solver);
        storeAdjacencyIntegralToElem(r_elem_tag, r_adjacency_order * adjacency_elem_integral.kIntegralNum + j,
                                     -adjancy_integral, solver);
      }
    }
  }
}

template <int Dim, PolyOrder P, ElemType ElemT, MeshType MeshT, EquModel EquModelT, ConvectiveFlux ConvectiveFluxT>
inline void calBoundaryAdjacencyElemIntegral(
    const AdjacencyElemIntegral<P, ElemT, MeshT>& adjacency_elem_integral,
    const AdjacencyElemMesh<Dim, P, ElemT, MeshT>& adjacency_elem_mesh, const ThermoModel<EquModelT>& thermo_model,
    const Eigen::Vector<Real, getPrimitiveVarNum<EquModelT>(Dim)> farfield_primitive_var,
    Solver<Dim, P, MeshT, EquModelT>& solver) {
  Eigen::Vector<Real, getConservedVarNum<EquModelT>(Dim)> l_conserved_var;
  Eigen::Vector<Real, getPrimitiveVarNum<EquModelT>(Dim)> l_primitive_var;
  Eigen::Vector<Real, getPrimitiveVarNum<EquModelT>(Dim)> wall_primitive_var;
  Eigen::Vector<Real, getConservedVarNum<EquModelT>(Dim)> convective_flux;
  Eigen::Vector<Real, getConservedVarNum<EquModelT>(Dim)> adjancy_integral;
  Isize l_elem_tag;
  Isize l_adjacency_order;
  Isize r_boundary_tag;
#ifdef SUBROSA_DG_WITH_OPENMP
#pragma omp parallel for default(none)                                                                              \
    schedule(auto) private(l_conserved_var, l_primitive_var, wall_primitive_var, convective_flux, adjancy_integral, \
                           l_elem_tag, l_adjacency_order, r_boundary_tag)                                           \
        shared(adjacency_elem_mesh, adjacency_elem_integral, thermo_model, farfield_primitive_var, solver)
#endif
  for (Isize i = 0; i < adjacency_elem_mesh.boundary_.num_; i++) {
    l_elem_tag = adjacency_elem_mesh.boundary_.elem_(i).parent_index_(0);
    r_boundary_tag = adjacency_elem_mesh.boundary_.elem_(i).parent_index_(1);
    l_adjacency_order = adjacency_elem_mesh.boundary_.elem_(i).adjacency_index_(0);
    for (Isize j = 0; j < adjacency_elem_integral.kIntegralNum; j++) {
      if constexpr (IsMixed<MeshT>) {
        getParentVar(adjacency_elem_mesh.boundary_.elem_(i).typology_index_(0), l_elem_tag,
                     l_adjacency_order * adjacency_elem_integral.kIntegralNum + j, adjacency_elem_integral, solver,
                     l_conserved_var);
      } else {
        getParentVar(l_elem_tag, l_adjacency_order * adjacency_elem_integral.kIntegralNum + j, adjacency_elem_integral,
                     solver, l_conserved_var);
      }
      calPrimitiveVar(thermo_model, l_conserved_var, l_primitive_var);
      switch (static_cast<Boundary>(r_boundary_tag)) {
      case Boundary::Farfield:
        if constexpr (ConvectiveFluxT == ConvectiveFlux::Roe) {
          calRoeFlux(thermo_model, adjacency_elem_mesh.boundary_.elem_(i).norm_vec_, l_primitive_var,
                     farfield_primitive_var, convective_flux);
        }
        break;
      case Boundary::Wall:
        calWallPrimitiveVar<EquModelT>(l_primitive_var, wall_primitive_var);
        if constexpr (ConvectiveFluxT == ConvectiveFlux::Roe) {
          calRoeFlux(thermo_model, adjacency_elem_mesh.boundary_.elem_(i).norm_vec_, l_primitive_var,
                     wall_primitive_var, convective_flux);
        }
        break;
      }
      adjancy_integral.noalias() = convective_flux * adjacency_elem_mesh.boundary_.elem_(i).jacobian_det_(j) *
                                   adjacency_elem_integral.weight_(j);
      if constexpr (IsMixed<MeshT>) {
        storeAdjacencyIntegralToElem(adjacency_elem_mesh.boundary_.elem_(i).typology_index_(0), l_elem_tag,
                                     l_adjacency_order * adjacency_elem_integral.kIntegralNum + j, adjancy_integral,
                                     solver);
      } else {
        storeAdjacencyIntegralToElem(l_elem_tag, l_adjacency_order * adjacency_elem_integral.kIntegralNum + j,
                                     adjancy_integral, solver);
      }
    }
  }
}

template <typename T, PolyOrder P, MeshType MeshT, EquModel EquModelT, TimeDiscrete TimeDiscreteT>
inline void calAdjacencyElemIntegral(const Integral<2, P, MeshT>& integral, const Mesh<2, P, MeshT>& mesh,
                                     const SolverSupplemental<2, EquModelT, TimeDiscreteT>& solver_supplemental,
                                     Solver<2, P, MeshT, EquModelT>& solver) {
  calInternalAdjacencyElemIntegral<2, P, ElemType::Line, MeshT, EquModelT, T::kConvectiveFlux>(
      integral.line_, mesh.line_, solver_supplemental.thermo_model_, solver);
  calBoundaryAdjacencyElemIntegral<2, P, ElemType::Line, MeshT, EquModelT, T::kConvectiveFlux>(
      integral.line_, mesh.line_, solver_supplemental.thermo_model_, solver_supplemental.farfield_primitive_var_,
      solver);
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CAL_ADJACENCY_INTEGRAL_HPP_
