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
#include "basic/config.hpp"
#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "integral/integral_structure.hpp"
#include "mesh/mesh_structure.hpp"
#include "solver/convective_flux/cal_roe_flux.hpp"
#include "solver/convective_flux/cal_wall_flux.hpp"
#include "solver/solver_structure.hpp"
#include "solver/space_discrete/store_to_elem.hpp"
#include "solver/variable/cal_primitive_var.hpp"
#include "solver/variable/get_parent_var.hpp"

namespace SubrosaDG {

template <PolyOrder P, ElemType ElemT, MeshType MeshT, EquModel EquModelT, ConvectiveFlux ConvectiveFluxT>
inline void calInternalAdjacencyElemIntegral(const AdjacencyElemMesh<2, ElemT, MeshT>& adjacency_elem_mesh,
                                             const AdjacencyElemIntegral<P, ElemT, MeshT>& adjacency_elem_integral,
                                             const ThermoModel<EquModel::Euler>& thermo_model,
                                             Solver<2, P, EquModelT, MeshT>& solver) {
  Eigen::Vector<Real, 4> l_conserved_var;
  Eigen::Vector<Real, 5> l_primitive_var;
  Eigen::Vector<Real, 4> r_conserved_var;
  Eigen::Vector<Real, 5> r_primitive_var;
  Eigen::Vector<Real, 4> convective_flux;
  Eigen::Vector<Real, 4> adjancy_integral;
  Isize l_elem_tag;
  Isize l_adjacency_order;
  Isize r_elem_tag;
  Isize r_adjacency_order;
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
      adjancy_integral.noalias() =
          convective_flux * adjacency_elem_mesh.internal_.elem_(i).jacobian_ * adjacency_elem_integral.weight_(j);
      if constexpr (IsMixed<MeshT>) {
        storeAdjacencyIntegralToElem(adjacency_elem_mesh.internal_.elem_(i).typology_index_(0), l_elem_tag,
                                     l_adjacency_order * adjacency_elem_integral.kIntegralNum + j, adjancy_integral,
                                     solver);
        storeAdjacencyIntegralToElem(adjacency_elem_mesh.internal_.elem_(i).typology_index_(1), r_elem_tag,
                                     r_adjacency_order * adjacency_elem_integral.kIntegralNum + j, adjancy_integral,
                                     solver);
      } else {
        storeAdjacencyIntegralToElem(l_elem_tag, l_adjacency_order * adjacency_elem_integral.kIntegralNum + j,
                                     adjancy_integral, solver);
        storeAdjacencyIntegralToElem(r_elem_tag, r_adjacency_order * adjacency_elem_integral.kIntegralNum + j,
                                     adjancy_integral, solver);
      }
    }
  }
}

template <PolyOrder P, ElemType ElemT, MeshType MeshT, EquModel EquModelT, ConvectiveFlux ConvectiveFluxT>
inline void calBoundaryAdjacencyElemIntegral(const AdjacencyElemMesh<2, ElemT, MeshT>& adjacency_elem_mesh,
                                             const AdjacencyElemIntegral<P, ElemT, MeshT>& adjacency_elem_integral,
                                             const ThermoModel<EquModel::Euler>& thermo_model,
                                             const Eigen::Vector<Real, 5> farfield_primitive_var,
                                             Solver<2, P, EquModelT, MeshT>& solver) {
  Eigen::Vector<Real, 4> l_conserved_var;
  Eigen::Vector<Real, 5> l_primitive_var;
  Eigen::Vector<Real, 4> convective_flux;
  Eigen::Vector<Real, 4> adjancy_integral;
  Isize l_elem_tag;
  Isize l_adjacency_order;
  Isize r_boundary_tag;
  for (Isize i = 0; i < adjacency_elem_mesh.boundary_.num_; i++) {
    l_elem_tag = adjacency_elem_mesh.internal_.elem_(i).parent_index_(0);
    r_boundary_tag = adjacency_elem_mesh.internal_.elem_(i).parent_index_(1);
    l_adjacency_order = adjacency_elem_mesh.internal_.elem_(i).adjacency_index_(0);
    for (Isize j = 0; j < adjacency_elem_integral.kIntegralNum; j++) {
      if constexpr (IsMixed<MeshT>) {
        getParentVar(adjacency_elem_mesh.internal_.elem_(i).typology_index_(0), l_elem_tag,
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
        calWallFlux(adjacency_elem_mesh.boundary_.elem_(i).norm_vec_, l_primitive_var, convective_flux);
        break;
      }
      adjancy_integral.noalias() =
          convective_flux * adjacency_elem_mesh.boundary_.elem_(i).jacobian_ * adjacency_elem_integral.weight_(j);
      if constexpr (IsMixed<MeshT>) {
        storeAdjacencyIntegralToElem(adjacency_elem_mesh.internal_.elem_(i).typology_index_(0), l_elem_tag,
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
  requires DerivedFromSpatialDiscrete<T, EquModelT>
inline void calAdjacencyElemIntegral(const Mesh<2, MeshT>& mesh, const Integral<2, P, MeshT>& integral,
                                     const SolverSupplemental<2, EquModelT, TimeDiscreteT>& solver_supplemental,
                                     Solver<2, P, EquModelT, MeshT>& solver) {
  calInternalAdjacencyElemIntegral<P, ElemType::Line, MeshT, EquModelT, T::kConvectiveFlux>(
      mesh.line_, integral.line_, solver_supplemental.thermo_model_, solver);
  calBoundaryAdjacencyElemIntegral<P, ElemType::Line, MeshT, EquModelT, T::kConvectiveFlux>(
      mesh.line_, integral.line_, solver_supplemental.thermo_model_, solver_supplemental.farfield_primitive_var_,
      solver);
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CAL_ADJACENCY_INTEGRAL_HPP_
