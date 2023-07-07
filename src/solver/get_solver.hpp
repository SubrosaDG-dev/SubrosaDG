/**
 * @file get_solver.hpp
 * @brief The head file to get the solver.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-06-13
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_GET_SOLVER_HPP_
#define SUBROSA_DG_GET_SOLVER_HPP_

#include <fmt/core.h>

#include <Eigen/Core>
#include <string>
#include <tqdm.hpp>

#include "basic/concept.hpp"
#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "config/flow_var.hpp"
#include "config/thermo_model.hpp"
#include "config/time_var.hpp"
#include "integral/integral_structure.hpp"
#include "mesh/mesh_structure.hpp"
#include "solver/equation/cal_absolute_error.hpp"
#include "solver/equation/copy_fun_coeff.hpp"
#include "solver/init_solver.hpp"
#include "solver/solver_structure.hpp"
#include "solver/time_discrete/cal_delta_time.hpp"
#include "solver/time_discrete/step_time.hpp"

namespace SubrosaDG {

template <typename T, PolyOrder P, MeshType MeshT, TimeDiscrete TimeDiscreteT>
  requires DerivedFromSpatialDiscrete<T, EquModel::Euler>
inline void getSolver(const Mesh<2, P, MeshT>& mesh, const Integral<2, P, MeshT>& integral,
                      const ThermoModel<EquModel::Euler>& thermo_model, const TimeVar<TimeDiscreteT>& time_var,
                      const InitVar<2>& init_var, const FarfieldVar<2> farfield_var,
                      Solver<2, P, EquModel::Euler, MeshT>& solver) {
  SolverSupplemental<2, EquModel::Euler, TimeDiscreteT> solver_supplemental{thermo_model, time_var};
  initSolver(mesh, init_var, farfield_var, solver_supplemental, solver);
  Tqdm::ProgressBar bar(solver_supplemental.time_solver_.iter_);
  Eigen::Vector<Real, 4> absolute_error;
  for (int i = 0; i < solver_supplemental.time_solver_.iter_; i++) {
    copyFunCoeff(mesh, solver);
    calDeltaTime(mesh, integral, solver, solver_supplemental);
    for (Usize j = 0; j < solver_supplemental.time_solver_.kStepCoeffs.size(); j++) {
      stepTime<T>(mesh, integral, solver_supplemental, solver_supplemental.time_solver_.kStepCoeffs[j], solver);
    }
    calAbsoluteError(mesh, integral, solver, absolute_error);
    bar << fmt::format("Residual: rho: {:.4e}, rhou: {:.4e}, rhov: {:.4e}, rhoE: {:.4e}", absolute_error(0),
                       absolute_error(1), absolute_error(2), absolute_error(3));
    bar.update();
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_SOLVER_HPP_
