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

// clang-format off

#include <Eigen/Core>                                // for Vector, MatrixBase::operator*, DenseBase::col

#include "basic/config.hpp"                          // for TimeDiscrete, FarfieldVar
#include "basic/data_type.hpp"                       // for Real, Isize
#include "basic/enum.hpp"                            // for MeshType, Boundary, ConvectiveFlux
#include "mesh/elem_type.hpp"                        // for ElemInfo, kLine
#include "solver/variable/cal_primitive_var.hpp"     // for calPrimitiveVar
#include "solver/convective_flux/cal_roe_flux.hpp"   // for calRoeFlux
#include "solver/convective_flux/cal_wall_flux.hpp"  // for calWallFlux
#include "solver/variable/get_parent_var.hpp"        // for getParentVar
#include "solver/elem_integral/store_to_elem.hpp"    // for storeIntegralToElem

// clang-format on

namespace SubrosaDG {

template <int Dim, ElemInfo ElemT>
struct AdjacencyElemMesh;
template <int Dim, int PolyOrder, MeshType MeshT, ConvectiveFlux ConvectiveFluxT, TimeDiscrete TimeDiscreteT>
struct SolverEuler;
template <int PolyOrder, ElemInfo ElemT, SubrosaDG::MeshType MeshT>
struct AdjacencyElemIntegral;

template <int PolyOrder, ElemInfo ElemT, MeshType MeshT, ConvectiveFlux ConvectiveFluxT, TimeDiscrete TimeDiscreteT>
inline void calInternalAdjacencyIntegral(const AdjacencyElemMesh<2, ElemT>& adjacency_elem_mesh,
                                         const AdjacencyElemIntegral<PolyOrder, ElemT, MeshT>& adjacency_elem_integral,
                                         SolverEuler<2, PolyOrder, MeshT, ConvectiveFluxT, TimeDiscreteT>& solver) {
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
    l_elem_tag = adjacency_elem_mesh.internal_.elem_(i).index_(ElemT.kNodeNum + 0);
    l_adjacency_order = adjacency_elem_mesh.internal_.elem_(i).index_(ElemT.kNodeNum + 2);
    r_elem_tag = adjacency_elem_mesh.internal_.elem_(i).index_(ElemT.kNodeNum + 3);
    r_adjacency_order = adjacency_elem_mesh.internal_.elem_(i).index_(ElemT.kNodeNum + 5);
    for (Isize j = 0; j < adjacency_elem_integral.kIntegralNum; j++) {
      if constexpr (MeshT == MeshType::TriQuad) {
        getParentVar(static_cast<int>(adjacency_elem_mesh.internal_.elem_(i).index_(ElemT.kNodeNum + 1)), l_elem_tag,
                     l_adjacency_order * adjacency_elem_integral.kIntegralNum + j, adjacency_elem_integral,
                     solver.elem_solver_, l_conserved_var);
        getParentVar(static_cast<int>(adjacency_elem_mesh.internal_.elem_(i).index_(ElemT.kNodeNum + 4)), r_elem_tag,
                     r_adjacency_order * adjacency_elem_integral.kIntegralNum + j, adjacency_elem_integral,
                     solver.elem_solver_, r_conserved_var);
      } else {
        getParentVar(l_elem_tag, l_adjacency_order * adjacency_elem_integral.kIntegralNum + j, adjacency_elem_integral,
                     solver.elem_solver_, l_conserved_var);
        getParentVar(r_elem_tag, r_adjacency_order * adjacency_elem_integral.kIntegralNum + j, adjacency_elem_integral,
                     solver.elem_solver_, r_conserved_var);
      }
      calPrimitiveVar(solver.thermo_model_, l_conserved_var, l_primitive_var);
      calPrimitiveVar(solver.thermo_model_, r_conserved_var, r_primitive_var);
      if constexpr (ConvectiveFluxT == ConvectiveFlux::Roe) {
        calRoeFlux<2>(solver.thermo_model_, adjacency_elem_mesh.internal_.elem_(i).norm_vec_, l_primitive_var,
                      r_primitive_var, convective_flux);
      }
      adjancy_integral =
          convective_flux * adjacency_elem_mesh.internal_.elem_(i).jacobian_ * adjacency_elem_integral.weight_(j);
      if constexpr (MeshT == MeshType::TriQuad) {
        storeIntegralToElem(static_cast<int>(adjacency_elem_mesh.internal_.elem_(i).index_(ElemT.kNodeNum + 1)),
                            l_elem_tag, l_adjacency_order * adjacency_elem_integral.kIntegralNum + j, adjancy_integral,
                            solver.elem_solver_);
        storeIntegralToElem(static_cast<int>(adjacency_elem_mesh.internal_.elem_(i).index_(ElemT.kNodeNum + 4)),
                            r_elem_tag, r_adjacency_order * adjacency_elem_integral.kIntegralNum + j, adjancy_integral,
                            solver.elem_solver_);
      }
      storeIntegralToElem(l_elem_tag, l_adjacency_order * adjacency_elem_integral.kIntegralNum + j, adjancy_integral,
                          solver.elem_solver_);
      storeIntegralToElem(r_elem_tag, r_adjacency_order * adjacency_elem_integral.kIntegralNum + j, adjancy_integral,
                          solver.elem_solver_);
    }
  }
}

template <int PolyOrder, ElemInfo ElemT, MeshType MeshT, ConvectiveFlux ConvectiveFluxT, TimeDiscrete TimeDiscreteT>
inline void calBoundaryAdjacencyIntegral(const AdjacencyElemMesh<2, ElemT>& adjacency_elem_mesh,
                                         const AdjacencyElemIntegral<PolyOrder, ElemT, MeshT>& adjacency_elem_integral,
                                         const FarfieldVar<2> farfield_var,
                                         SolverEuler<2, PolyOrder, MeshT, ConvectiveFluxT, TimeDiscreteT>& solver) {
  Eigen::Vector<Real, 4> l_conserved_var;
  Eigen::Vector<Real, 5> l_primitive_var;
  Eigen::Vector<Real, 5> farfield_primitive_var;
  calPrimitiveVar(solver.thermo_model_, farfield_var, farfield_primitive_var);
  Eigen::Vector<Real, 4> convective_flux;
  Eigen::Vector<Real, 4> adjancy_integral;
  Isize l_elem_tag;
  Isize l_adjacency_order;
  Isize r_boundary_tag;
  for (Isize i = 0; i < adjacency_elem_mesh.boundary_.num_; i++) {
    l_elem_tag = adjacency_elem_mesh.boundary_.elem_(i).index_(ElemT.kNodeNum + 0);
    l_adjacency_order = adjacency_elem_mesh.boundary_.elem_(i).index_(ElemT.kNodeNum + 2);
    r_boundary_tag = adjacency_elem_mesh.boundary_.elem_(i).index_(ElemT.kNodeNum + 3);
    for (Isize j = 0; j < adjacency_elem_integral.kIntegralNum; j++) {
      if constexpr (MeshT == MeshType::TriQuad) {
        getParentVar(static_cast<int>(adjacency_elem_mesh.boundary_.elem_(i).index_(ElemT.kNodeNum + 1)), l_elem_tag,
                     l_adjacency_order * adjacency_elem_integral.kIntegralNum + j, adjacency_elem_integral,
                     solver.elem_solver_, l_conserved_var);
      } else {
        getParentVar(l_elem_tag, l_adjacency_order * adjacency_elem_integral.kIntegralNum + j, adjacency_elem_integral,
                     solver.elem_solver_, l_conserved_var);
      }
      calPrimitiveVar(solver.thermo_model_, l_conserved_var, l_primitive_var);
      switch (static_cast<Boundary>(r_boundary_tag)) {
      case Boundary::Farfield:
        if constexpr (ConvectiveFluxT == ConvectiveFlux::Roe) {
          calRoeFlux<2>(solver.thermo_model_, adjacency_elem_mesh.boundary_.elem_(i).norm_vec_, l_primitive_var,
                        farfield_primitive_var, convective_flux);
        }
        break;
      case Boundary::Wall:
        calWallFlux<2>(adjacency_elem_mesh.boundary_.elem_(i).norm_vec_.col(i), l_primitive_var, convective_flux);
        break;
      }
      adjancy_integral =
          convective_flux * adjacency_elem_mesh.boundary_.elem_(i).jacobian_ * adjacency_elem_integral.weight_(j);
      if constexpr (MeshT == MeshType::TriQuad) {
        storeIntegralToElem(static_cast<int>(adjacency_elem_mesh.boundary_.elem_(i).index_(ElemT.kNodeNum + 1)),
                            l_elem_tag, l_adjacency_order * adjacency_elem_integral.kIntegralNum + j, adjancy_integral,
                            solver.elem_solver_);
      }
      storeIntegralToElem(l_elem_tag, l_adjacency_order * adjacency_elem_integral.kIntegralNum + j, adjancy_integral,
                          solver.elem_solver_);
    }
  }
}

template <int PolyOrder, ElemInfo ElemT, MeshType MeshT, ConvectiveFlux ConvectiveFluxT, TimeDiscrete TimeDiscreteT>
inline void calAdjacencyIntegral(const AdjacencyElemMesh<2, ElemT>& adjacency_elem_mesh,
                                 const AdjacencyElemIntegral<PolyOrder, ElemT, MeshT>& adjacency_elem_integral,
                                 const FarfieldVar<2> farfield_var,
                                 SolverEuler<2, PolyOrder, MeshT, ConvectiveFluxT, TimeDiscreteT>& solver) {
  calInternalAdjacencyIntegral<PolyOrder, kLine, MeshT>(adjacency_elem_mesh, adjacency_elem_integral, solver);
  calBoundaryAdjacencyIntegral<PolyOrder, kLine, MeshT>(adjacency_elem_mesh, adjacency_elem_integral, farfield_var,
                                                        solver);
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CAL_ADJACENCY_INTEGRAL_HPP_
