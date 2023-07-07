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
#include <vector>

#include "basic/concept.hpp"
#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "config/flow_var.hpp"
#include "config/thermo_model.hpp"
#include "mesh/get_mesh_supplemental.hpp"
#include "mesh/mesh_structure.hpp"
#include "solver/solver_structure.hpp"
#include "solver/variable/cal_conserved_var.hpp"
#include "solver/variable/cal_primitive_var.hpp"

namespace SubrosaDG {

template <PolyOrder P, ElemType ElemT>
inline void initElemSolver(const Isize elem_num, const InitVar<2>& init_var,
                           const ThermoModel<EquModel::Euler>& thermo_model,
                           ElemSolver<2, P, ElemT, EquModel::Euler>& elem_solver) {
  elem_solver.elem_.resize(elem_num);
  MeshSupplemental<ElemT> internal_supplemental;
  getMeshSupplemental<int, ElemT>(init_var.region_map_, internal_supplemental);
  std::vector<Eigen::Vector<Real, 4>> init_conserved_var;
  init_conserved_var.resize(init_var.flow_var_.size());
  for (Usize i = 0; i < init_var.flow_var_.size(); i++) {
    calConservedVar(thermo_model, init_var.flow_var_[i], init_conserved_var[i]);
  }
  for (Isize i = 0; i < elem_num; i++) {
    elem_solver.elem_(i).basis_fun_coeff_(1).colwise() =
        init_conserved_var[static_cast<Usize>(internal_supplemental.index_(i))];
  }
}

template <PolyOrder P, MeshType MeshT, TimeDiscrete TimeDiscreteT>
inline void initSolver(const Mesh<2, P, MeshT>& mesh, const InitVar<2>& init_var, const FarfieldVar<2> farfield_var,
                       SolverSupplemental<2, EquModel::Euler, TimeDiscreteT>& solver_supplemental,
                       Solver<2, P, EquModel::Euler, MeshT>& solver) {
  calPrimitiveVar(solver_supplemental.thermo_model_, farfield_var, solver_supplemental.farfield_primitive_var_);
  if constexpr (HasTri<MeshT>) {
    initElemSolver(mesh.tri_.num_, init_var, solver_supplemental.thermo_model_, solver.tri_);
  }
  if constexpr (HasQuad<MeshT>) {
    initElemSolver(mesh.quad_.num_, init_var, solver_supplemental.thermo_model_, solver.quad_);
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_INIT_SOLVER_HPP_
