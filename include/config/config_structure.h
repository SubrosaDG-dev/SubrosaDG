/**
 * @file config_structure.h
 * @brief The config structure head file.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-04-27
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CONFIG_STRUCTURE_H_
#define SUBROSA_DG_CONFIG_STRUCTURE_H_

// clang-format off

#include <Eigen/Core>          // for Vector
#include <map>                 // for map
#include <filesystem>          // for path
#include <string>              // for string

#include "basic/data_types.h"  // for Real, Usize

// clang-format on

namespace SubrosaDG::Internal {

enum class BoundaryType : SubrosaDG::Usize;
enum class EquationOfState : SubrosaDG::Usize;
enum class NoVisFluxType : SubrosaDG::Usize;
enum class SimulationType : SubrosaDG::Usize;
enum class TimeIntegrationType : SubrosaDG::Usize;

struct TimeIntegration {
  TimeIntegrationType time_integration_type_;
  Usize iteration_;
  Real cfl_;
  int tolerance_;
};

struct ThermodynamicModel {
  EquationOfState equation_of_state_;
  Real gamma_;
  Real c_p_;
  Real r_;
  Real mu_;
};

struct FlowParameter {
  Eigen::Vector<Real, 3> u_;
  Real rho_;
  Real p_;
  Real t_;
};

struct Config {
  int dimension_;
  int polynomial_order_;
  Internal::SimulationType simulation_type_;
  Internal::NoVisFluxType no_vis_flux_type_;
  std::filesystem::path mesh_file_;
  Internal::TimeIntegration time_integration_;
  std::map<std::string, Internal::BoundaryType> boundary_condition_;
  std::map<std::string, Internal::ThermodynamicModel> thermodynamic_model_;
  std::map<std::string, Internal::FlowParameter> initial_condition_;
  Internal::FlowParameter farfield_parameter_;
};

}  // namespace SubrosaDG::Internal

#endif  // SUBROSA_DG_CONFIG_STRUCTURE_H_