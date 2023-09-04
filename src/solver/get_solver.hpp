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
#include <filesystem>
#include <fstream>
#include <string>
#include <tqdm.hpp>

#include "basic/concept.hpp"
#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "config/flow_var.hpp"
#include "config/thermo_model.hpp"
#include "config/time_var.hpp"
#include "config/view_config.hpp"
#include "integral/integral_structure.hpp"
#include "mesh/mesh_structure.hpp"
#include "solver/equation/cal_absolute_error.hpp"
#include "solver/equation/copy_fun_coeff.hpp"
#include "solver/init_solver.hpp"
#include "solver/solver_structure.hpp"
#include "solver/time_discrete/cal_delta_time.hpp"
#include "solver/time_discrete/step_time.hpp"
#include "solver/variable/get_var_num.hpp"
#include "view/writer/write_raw_buffer.hpp"

namespace SubrosaDG {

template <typename T, int Dim, PolyOrder P, MeshType MeshT, TimeDiscrete TimeDiscreteT, EquModel EquModelT>
  requires DerivedFromSpatialDiscrete<T, EquModelT>
inline void getSolver(const Integral<Dim, P, MeshT>& integral, const Mesh<Dim, P, MeshT>& mesh,
                      const ThermoModel<EquModelT>& thermo_model, const TimeVar<TimeDiscreteT>& time_var,
                      const InitVar<Dim, EquModelT>& init_var, const FarfieldVar<Dim, EquModelT> farfield_var,
                      const ViewConfig& view_config, Solver<Dim, P, MeshT, EquModelT>& solver) {
  std::ofstream fout;
#ifndef SUBROSA_DG_DEVELOP
  fout.open((view_config.dir_ / "cache.raw").string(), std::ios::out | std::ios::trunc | std::ios::binary);
#else
  fout.open((view_config.dir_ / "cache.raw").string(), std::ios::out | std::ios::trunc);
#endif
  // fout.precision(kSignificantDigits);
  SolverSupplemental<Dim, EquModelT, TimeDiscreteT> solver_supplemental{thermo_model, time_var};
  initSolver(mesh, init_var, farfield_var, solver_supplemental, solver);
  Tqdm::ProgressBar bar(solver_supplemental.time_solver_.iter_);
  Eigen::Vector<Real, getConservedVarNum<EquModelT>(Dim)> absolute_error;
  for (int i = 1; i <= solver_supplemental.time_solver_.iter_; i++) {
    copyFunCoeff(mesh, solver);
    calDeltaTime(integral, mesh, solver, solver_supplemental);
    for (Usize j = 0; j < solver_supplemental.time_solver_.kStep; j++) {
      stepTime<T>(integral, mesh, solver_supplemental, solver_supplemental.time_solver_.kStepCoeffs[j], solver);
    }
    if (i % view_config.write_interval_ == 0) {
      writeRawBuffer(mesh, solver, fout);
    }
    bar << calAbsoluteError(integral, mesh, solver, absolute_error);
    bar.update();
  }
  fout.close();
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_SOLVER_HPP_
