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

#include <Eigen/Core>
#include <array>
#include <string_view>
#include <unordered_map>

#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "mesh/elem_type.hpp"
#include "mesh/get_mesh_supplemental.hpp"
#include "mesh/mesh_structure.hpp"
#include "solver/variable/cal_conserved_var.hpp"

namespace SubrosaDG {

template <EquModel EquModelT>
struct ThermoModel;
template <int Dim>
struct InitVar;
template <int Dim, int PolyOrder, ElemInfo ElemT, EquModel EquModelT>
struct PerElemSolver;
template <int Dim, int PolyOrder, MeshType MeshT, ConvectiveFlux ConvectiveFluxT, TimeDiscrete TimeDiscreteT>
struct SolverEuler;

template <int PolyOrder, Usize RegionNum, ElemInfo ElemT>
inline void initElemSolver(
    const Isize elem_num, const std::unordered_map<std::string_view, int>& region_id_map,
    const std::array<InitVar<2>, RegionNum>& init_var_vec, const ThermoModel<EquModel::Euler>& thermo_model,
    Eigen::Vector<PerElemSolver<2, PolyOrder, ElemT, EquModel::Euler>, Eigen::Dynamic>& elem_solver) {
  elem_solver.resize(elem_num);
  MeshSupplemental<ElemT> internal_supplemental;
  getMeshSupplemental<int, ElemT>(region_id_map, internal_supplemental);
  std::array<Eigen::Vector<Real, 4>, RegionNum> init_conserved_var;
  for (Usize i = 0; i < RegionNum; i++) {
    calConservedVar(thermo_model, init_var_vec[i], init_conserved_var[i]);
  }
  for (Isize i = 0; i < elem_num; i++) {
    elem_solver(i).basis_fun_coeff_(0).colwise() =
        init_conserved_var[static_cast<Usize>(internal_supplemental.index_(i))];
    elem_solver(i).basis_fun_coeff_(1).colwise() =
        init_conserved_var[static_cast<Usize>(internal_supplemental.index_(i))];
  }
}

template <int PolyOrder, Usize RegionNum, ConvectiveFlux ConvectiveFluxT, TimeDiscrete TimeDiscreteT>
inline void initSolver(const Mesh<2, MeshType::Tri>& mesh,
                       const std::unordered_map<std::string_view, int>& region_id_map,
                       const std::array<InitVar<2>, RegionNum>& init_var_vec,
                       SolverEuler<2, PolyOrder, MeshType::Tri, ConvectiveFluxT, TimeDiscreteT>& solver) {
  initElemSolver<PolyOrder, RegionNum, kTri>(mesh.tri_.num_, region_id_map, init_var_vec, solver.thermo_model_,
                                             solver.elem_.tri_);
}

template <int PolyOrder, Usize RegionNum, ConvectiveFlux ConvectiveFluxT, TimeDiscrete TimeDiscreteT>
inline void initSolver(const Mesh<2, MeshType::Quad>& mesh,
                       const std::unordered_map<std::string_view, int>& region_id_map,
                       const std::array<InitVar<2>, RegionNum>& init_var_vec,
                       SolverEuler<2, PolyOrder, MeshType::Quad, ConvectiveFluxT, TimeDiscreteT>& solver) {
  initElemSolver<PolyOrder, RegionNum, kQuad>(mesh.quad_.num_, region_id_map, init_var_vec, solver.thermo_model_,
                                              solver.elem_.quad_);
}

template <int PolyOrder, Usize RegionNum, ConvectiveFlux ConvectiveFluxT, TimeDiscrete TimeDiscreteT>
inline void initSolver(const Mesh<2, MeshType::TriQuad>& mesh,
                       const std::unordered_map<std::string_view, int>& region_id_map,
                       const std::array<InitVar<2>, RegionNum>& init_var_vec,
                       SolverEuler<2, PolyOrder, MeshType::TriQuad, ConvectiveFluxT, TimeDiscreteT>& solver) {
  initElemSolver<PolyOrder, RegionNum, kTri>(mesh.tri_.num_, region_id_map, init_var_vec, solver.thermo_model_,
                                             solver.elem_.tri_);
  initElemSolver<PolyOrder, RegionNum, kQuad>(mesh.quad_.num_, region_id_map, init_var_vec, solver.thermo_model_,
                                              solver.elem_.quad_);
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_INIT_SOLVER_HPP_
