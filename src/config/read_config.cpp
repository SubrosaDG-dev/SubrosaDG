/**
 * @file read_config.cpp
 * @brief The source file for reading config.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-04-27
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

// clang-format off

#include "config/read_config.h"

#include <libconfig.h++>              // for Config, Setting
#include <map>                        // for map
#include <string>                     // for string, allocator, operator+, char_traits, operator==, basic_string
#include <Eigen/Core>                 // for DenseCoeffsBase, Vector

#include "cmake.h"                    // for kProjectSourceDir
#include "config/config_structure.h"  // for Config, FlowParameter, ThermodynamicModel, TimeIntegration
#include "basic/data_types.h"         // for Isize, Real, Usize
#include "config/config_defines.h"    // for NoVisFluxType, SimulationType, TimeIntegrationType, BoundaryType, Equat...

// clang-format on

namespace SubrosaDG::Internal {

void readConfig(const std::filesystem::path& config_file, Config& config) {
  libconfig::Config cfg;
  cfg.readFile(config_file.string().c_str());

  cfg.lookupValue("Dimension", config.dimension_);

  cfg.lookupValue("PolynomialOrder", config.polynomial_order_);

  std::string simulation_type;
  cfg.lookupValue("SimulationType", simulation_type);
  if (simulation_type == "Euler") {
    config.simulation_type_ = Internal::SimulationType::Euler;
  } else if (simulation_type == "NavierStokes") {
    config.simulation_type_ = Internal::SimulationType::NavierStokes;
  }

  std::string no_vis_flux_type;
  cfg.lookupValue("NoVisFluxType", no_vis_flux_type);
  if (no_vis_flux_type == "Central") {
    config.no_vis_flux_type_ = Internal::NoVisFluxType::Central;
  } else if (no_vis_flux_type == "Roe") {
    config.no_vis_flux_type_ = Internal::NoVisFluxType::Roe;
  } else if (no_vis_flux_type == "HLLC") {
    config.no_vis_flux_type_ = Internal::NoVisFluxType::HLLC;
  }

  std::string mesh_file;
  cfg.lookupValue("MeshFile", mesh_file);
  config.mesh_file_ = kProjectSourceDir / mesh_file;

  std::string time_integration_type;
  cfg.lookupValue("TimeIntegration.type", time_integration_type);
  if (time_integration_type == "ExplicitEuler") {
    config.time_integration_.time_integration_type_ = Internal::TimeIntegrationType::ExplicitEuler;
  } else if (time_integration_type == "ImplicitEuler") {
    config.time_integration_.time_integration_type_ = Internal::TimeIntegrationType::ImplicitEuler;
  } else if (time_integration_type == "RungeKutta3") {
    config.time_integration_.time_integration_type_ = Internal::TimeIntegrationType::RungeKutta3;
  }
  int iteration;
  cfg.lookupValue("TimeIntegration.iteration", iteration);
  config.time_integration_.iteration_ = static_cast<Usize>(iteration);
  cfg.lookupValue("TimeIntegration.CFL", config.time_integration_.cfl_);
  cfg.lookupValue("TimeIntegration.tolerance", config.time_integration_.tolerance_);

  int boundary_condition_size = cfg.lookup("BoundaryCondition").getLength();
  for (int i = 0; i < boundary_condition_size; i++) {
    std::string boundary_name = cfg.lookup("BoundaryCondition")[i].getName();
    std::string boundary_type;
    cfg.lookupValue("BoundaryCondition." + boundary_name + ".type", boundary_type);
    if (boundary_type == "Farfield") {
      config.boundary_condition_[boundary_name] = Internal::BoundaryType::Farfield;
    } else if (boundary_type == "Wall") {
      config.boundary_condition_[boundary_name] = Internal::BoundaryType::Wall;
    }
  }

  int thermodynamic_model_size = cfg.lookup("ThermodynamicModel").getLength();
  for (int i = 0; i < thermodynamic_model_size; i++) {
    std::string thermodynamic_model_name = cfg.lookup("ThermodynamicModel")[i].getName();
    std::string equation_of_state;
    cfg.lookupValue("ThermodynamicModel." + thermodynamic_model_name + ".EquationOfState", equation_of_state);
    if (equation_of_state == "IdealGas") {
      config.thermodynamic_model_[thermodynamic_model_name].equation_of_state_ = Internal::EquationOfState::IdealGas;
    }
    cfg.lookupValue("ThermodynamicModel." + thermodynamic_model_name + ".gamma",
                    config.thermodynamic_model_[thermodynamic_model_name].gamma_);
    cfg.lookupValue("ThermodynamicModel." + thermodynamic_model_name + ".c_p",
                    config.thermodynamic_model_[thermodynamic_model_name].c_p_);
    cfg.lookupValue("ThermodynamicModel." + thermodynamic_model_name + ".R",
                    config.thermodynamic_model_[thermodynamic_model_name].r_);
    if (config.simulation_type_ == Internal::SimulationType::NavierStokes) {
      cfg.lookupValue("ThermodynamicModel." + thermodynamic_model_name + ".mu",
                      config.thermodynamic_model_[thermodynamic_model_name].mu_);
    }
  }

  int initial_condition_size = cfg.lookup("InitialCondition").getLength();
  for (int i = 0; i < initial_condition_size; i++) {
    std::string initial_condition_name = cfg.lookup("InitialCondition")[i].getName();
    for (Isize j = 0; j < 3; j++) {
      config.initial_condition_[initial_condition_name].u_(j) =
          static_cast<Real>(cfg.lookup("InitialCondition." + initial_condition_name + ".U")[static_cast<int>(j)]);
    }
    cfg.lookupValue("InitialCondition." + initial_condition_name + ".rho",
                    config.initial_condition_[initial_condition_name].rho_);
    cfg.lookupValue("InitialCondition." + initial_condition_name + ".p",
                    config.initial_condition_[initial_condition_name].p_);
    cfg.lookupValue("InitialCondition." + initial_condition_name + ".T",
                    config.initial_condition_[initial_condition_name].t_);
  }

  for (Isize i = 0; i < 3; i++) {
    config.farfield_parameter_.u_(i) = static_cast<Real>(cfg.lookup("FarfieldParameter.U")[static_cast<int>(i)]);
  }
  cfg.lookupValue("FarfieldParameter.rho", config.farfield_parameter_.rho_);
  cfg.lookupValue("FarfieldParameter.p", config.farfield_parameter_.p_);
  cfg.lookupValue("FarfieldParameter.T", config.farfield_parameter_.t_);

  cfg.clear();
}

}  // namespace SubrosaDG
