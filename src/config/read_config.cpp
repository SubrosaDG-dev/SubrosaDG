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

#include <fmt/core.h>                 // for format
#include <toml++/toml.h>              // for array, operator!=, array_iterator, node, parse_error, operator<<, parse...
#include <iostream>                   // for endl, basic_ostream, ostream, cerr, operator<<
#include <string>                     // for string, basic_string, operator<<
#include <Eigen/Core>                 // for DenseCoeffsBase, Vector
#include <string_view>                // for string_view, operator""sv, basic_string_view, operator==, hash, string_...
#include <cstddef>                    // for EXIT_FAILURE
#include <cstdlib>                    // for exit
#include <memory>                     // for __shared_ptr_access
#include <unordered_map>              // for unordered_map

#include "config/config_structure.h"  // for Config, FlowParameter, ThermodynamicModel, TimeIntegration
#include "basic/data_types.h"         // for Real, Usize, Isize
#include "config/config_defines.h"    // for BoundaryType (ptr only), EquationOfState (ptr only), NoVisFluxType (ptr...
#include "config/config_map.h"        // for kConfigMap

// clang-format on

namespace SubrosaDG::Internal {

void readConfig(const std::filesystem::path& config_file, Config& config) {
  using namespace std::string_view_literals;

  try {
    toml::table config_table = toml::parse_file(config_file.string());

    config.dimension_ = getValueFromToml<int>(config_table, "Dimension"sv);

    config.polynomial_order_ = getValueFromToml<int>(config_table, "PolynomialOrder"sv);

    config.simulation_type_ =
        kConfigMap<SimulationType>.at(getValueFromToml<std::string_view>(config_table, "SimulationType"sv));

    config.no_vis_flux_type_ =
        kConfigMap<NoVisFluxType>.at(getValueFromToml<std::string_view>(config_table, "NoVisFluxType"sv));

    config.mesh_file_ = getValueFromToml<std::string_view>(config_table, "MeshFile"sv);

    config.time_integration_.time_integration_type_ =
        kConfigMap<TimeIntegrationType>.at(getValueFromToml<std::string_view>(config_table, "TimeIntegration.type"sv));
    config.time_integration_.iteration_ = getValueFromToml<Usize>(config_table, "TimeIntegration.iteration"sv);
    config.time_integration_.cfl_ = getValueFromToml<Real>(config_table, "TimeIntegration.CFL"sv);
    config.time_integration_.tolerance_ = getValueFromToml<int>(config_table, "TimeIntegration.tolerance"sv);

    for (const auto& boundary_iter : getValueFromToml<toml::array>(config_table, "Boundary.name"sv)) {
      std::string_view boundary_name = boundary_iter.as_string()->get();
      config.boundary_condition_[boundary_name] = kConfigMap<BoundaryType>.at(
          getValueFromToml<std::string_view>(config_table, fmt::format("BoundaryCondition.{}.type"sv, boundary_name)));
    }

    for (const auto& domain_iter : getValueFromToml<toml::array>(config_table, "Domain.name"sv)) {
      std::string_view domain_name = domain_iter.as_string()->get();
      config.thermodynamic_model_[domain_name].equation_of_state_ =
          kConfigMap<EquationOfState>.at(getValueFromToml<std::string_view>(
              config_table, fmt::format("ThermodynamicModel.{}.EquationOfState"sv, domain_name)));
      config.thermodynamic_model_[domain_name].gamma_ =
          getValueFromToml<Real>(config_table, fmt::format("ThermodynamicModel.{}.gamma"sv, domain_name));
      config.thermodynamic_model_[domain_name].c_p_ =
          getValueFromToml<Real>(config_table, fmt::format("ThermodynamicModel.{}.c_p"sv, domain_name));
      config.thermodynamic_model_[domain_name].r_ =
          getValueFromToml<Real>(config_table, fmt::format("ThermodynamicModel.{}.R"sv, domain_name));
      if (config.simulation_type_ == SimulationType::NavierStokes) {
        config.thermodynamic_model_[domain_name].mu_ =
            getValueFromToml<Real>(config_table, fmt::format("ThermodynamicModel.{}.mu"sv, domain_name));
      }
    }

    for (const auto& domain_iter : getValueFromToml<toml::array>(config_table, "Domain.name"sv)) {
      std::string_view domain_name = domain_iter.as_string()->get();
      for (Isize i = 0; const auto& u_iter : getValueFromToml<toml::array>(
                            config_table, fmt::format("InitialCondition.{}.U"sv, domain_name))) {
        config.initial_condition_[domain_name].u_[i++] = u_iter.as<Real>()->get();
      }
      config.initial_condition_[domain_name].rho_ =
          getValueFromToml<Real>(config_table, fmt::format("InitialCondition.{}.rho"sv, domain_name));
      config.initial_condition_[domain_name].p_ =
          getValueFromToml<Real>(config_table, fmt::format("InitialCondition.{}.p"sv, domain_name));
      config.initial_condition_[domain_name].t_ =
          getValueFromToml<Real>(config_table, fmt::format("InitialCondition.{}.T"sv, domain_name));
    }

    for (Isize i = 0; const auto& u_iter : getValueFromToml<toml::array>(config_table, "FarfieldParameter.U"sv)) {
      config.farfield_parameter_.u_[i++] = u_iter.as<Real>()->get();
    }
    config.farfield_parameter_.rho_ = getValueFromToml<Real>(config_table, "FarfieldParameter.rho"sv);
    config.farfield_parameter_.p_ = getValueFromToml<Real>(config_table, "FarfieldParameter.p"sv);
    config.farfield_parameter_.t_ = getValueFromToml<Real>(config_table, "FarfieldParameter.T"sv);

    config_table.clear();
  } catch (const toml::parse_error& error) {
    std::cerr << std::endl << fmt::format("Error parsing file '{}':", *error.source().path) << std::endl;
    std::cerr << fmt::format("{} (", error.description()) << error.source().begin << ")" << std::endl;
    std::exit(EXIT_FAILURE);
  } catch (const std::out_of_range& error) {
    std::cerr << std::endl << fmt::format("Error parsing file '{}':", config_file.string()) << std::endl;
    std::cerr << fmt::format("{}", error.what()) << std::endl;
    std::exit(EXIT_FAILURE);
  }
}

}  // namespace SubrosaDG::Internal
