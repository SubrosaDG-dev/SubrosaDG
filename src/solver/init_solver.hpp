/**
 * @file init_solver.hpp
 * @brief The initialize solver header file.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-24
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_INIT_SOLVER_HPP_
#define SUBROSA_DG_INIT_SOLVER_HPP_

// clang-format off

#include <Eigen/Core>                             // for Vector, Dynamic
#include <array>                                  // for array
#include <string_view>                            // for string_view
#include <unordered_map>                          // for unordered_map

#include "basic/config.hpp"                       // for TimeDiscrete, InitVar (ptr only), ThermoModel (ptr only)
#include "basic/concept.hpp"                      // for IsExplicit
#include "basic/data_type.hpp"                    // for Usize, Isize, Real
#include "basic/enum.hpp"                         // for MeshType, ConvectiveFlux (ptr only), EquModel
#include "mesh/mesh_structure.hpp"                // for Mesh, QuadElemMesh, TriElemMesh, MeshSupplemental
#include "mesh/elem_type.hpp"                     // for ElemInfo, kQuad, kTri
#include "mesh/get_mesh_supplemental.hpp"         // for getMeshSupplemental
#include "solver/variable/cal_conserved_var.hpp"  // for calConservedVar

// clang-format on

namespace SubrosaDG {

template <int Dim, int PolyOrder, MeshType MeshT, ConvectiveFlux ConvectiveFluxT, TimeDiscrete TimeDiscreteT>
struct SolverEuler;
template <int Dim, int PolyOrder, ElemInfo ElemT, TimeDiscrete TimeDiscreteT>
struct PerElemConvectiveSolver;

template <int PolyOrder, Usize RegionNum, ElemInfo ElemT, TimeDiscrete TimeDiscreteT>
  requires IsExplicit<TimeDiscreteT>
inline void initElemSolver(
    const Isize elem_num, const std::unordered_map<std::string_view, int>& region_id_map,
    const std::array<InitVar<2>, RegionNum>& init_var_vec, const ThermoModel<EquModel::Euler>& thermo_model,
    Eigen::Vector<PerElemConvectiveSolver<2, PolyOrder, ElemT, TimeDiscreteT>, Eigen::Dynamic>& elem_t_solver) {
  elem_t_solver.resize(elem_num);
  MeshSupplemental<ElemT> internal_supplemental;
  getMeshSupplemental<int, ElemT>(region_id_map, internal_supplemental);
  std::array<Eigen::Vector<Real, 4>, RegionNum> init_conserved_var;
  for (Usize i = 0; i < RegionNum; i++) {
    calConservedVar(thermo_model, init_var_vec[i], init_conserved_var[i]);
  }
  for (Isize i = 0; i < elem_num; i++) {
    elem_t_solver(i).basis_fun_coeff_(0).colwise() =
        init_conserved_var[static_cast<Usize>(internal_supplemental.index_(i)) - 1];
  }
}

template <int PolyOrder, Usize RegionNum, ConvectiveFlux ConvectiveFluxT, TimeDiscrete TimeDiscreteT>
  requires IsExplicit<TimeDiscreteT>
inline void initSolver(const Mesh<2, MeshType::Tri>& mesh,
                       const std::unordered_map<std::string_view, int>& region_id_map,
                       const std::array<InitVar<2>, RegionNum>& init_var_vec,
                       SolverEuler<2, PolyOrder, MeshType::Tri, ConvectiveFluxT, TimeDiscreteT>& solver) {
  initElemSolver<PolyOrder, RegionNum, kTri, TimeDiscreteT>(mesh.tri_.num_, region_id_map, init_var_vec,
                                                            solver.thermo_model_, solver.elem_solver_.tri_);
}

template <int PolyOrder, Usize RegionNum, ConvectiveFlux ConvectiveFluxT, TimeDiscrete TimeDiscreteT>
  requires IsExplicit<TimeDiscreteT>
inline void initSolver(const Mesh<2, MeshType::Quad>& mesh,
                       const std::unordered_map<std::string_view, int>& region_id_map,
                       const std::array<InitVar<2>, RegionNum>& init_var_vec,
                       SolverEuler<2, PolyOrder, MeshType::Quad, ConvectiveFluxT, TimeDiscreteT>& solver) {
  initElemSolver<PolyOrder, RegionNum, kQuad, TimeDiscreteT>(mesh.quad_.num_, region_id_map, init_var_vec,
                                                             solver.thermo_model_, solver.elem_solver_.quad_);
}

template <int PolyOrder, Usize RegionNum, ConvectiveFlux ConvectiveFluxT, TimeDiscrete TimeDiscreteT>
  requires IsExplicit<TimeDiscreteT>
inline void initSolver(const Mesh<2, MeshType::TriQuad>& mesh,
                       const std::unordered_map<std::string_view, int>& region_id_map,
                       const std::array<InitVar<2>, RegionNum>& init_var_vec,
                       SolverEuler<2, PolyOrder, MeshType::TriQuad, ConvectiveFluxT, TimeDiscreteT>& solver) {
  initElemSolver<PolyOrder, RegionNum, kTri, TimeDiscreteT>(mesh.tri_.num_, region_id_map, init_var_vec,
                                                            solver.thermo_model_, solver.elem_solver_.tri_);
  initElemSolver<PolyOrder, RegionNum, kQuad, TimeDiscreteT>(mesh.quad_.num_, region_id_map, init_var_vec,
                                                             solver.thermo_model_, solver.elem_solver_.quad_);
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_INIT_SOLVER_HPP_
