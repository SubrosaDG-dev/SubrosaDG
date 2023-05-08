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
#include <toml++/toml.h>              // for array, operator!=, array_iterator, parse_error, table, node, operator<<
#include <iostream>                   // for endl, operator<<, basic_ostream, ostream, cerr
#include <string>                     // for string, basic_string, operator<<, operator==, hash
#include <Eigen/Core>                 // for DenseCoeffsBase, Vector
#include <string_view>                // for string_view, operator""sv, basic_string_view, string_view_literals
#include <cstdlib>                    // for exit, EXIT_FAILURE
#include <memory>                     // for __shared_ptr_access
#include <unordered_map>              // for unordered_map
#include <vector>                     // for vector

#include "cmake.h"                    // for kProjectSourceDir
#include "config/config_structure.h"  // for Config, FlowParameter, ThermodynamicModel, TimeIntegration
#include "basic/data_types.h"         // for Real, Usize, Isize
#include "config/config_defines.h"    // for BoundaryType (ptr only), EquationOfState (ptr only), NoVisFluxType (ptr...

// clang-format on

namespace SubrosaDG::Internal {

template <>
toml::array getValueFromToml<toml::array>(const toml::table& config_table, const std::string_view& key) {
  auto value_node_view = config_table.at_path(key);
  if (value_node_view.is_array()) {
    toml::array value_array = *value_node_view.as_array();
    if (!value_array.empty()) {
      return value_array;
    }
    throw std::out_of_range(fmt::format("Error: {} is empty in config file.", key));
  }
  throw std::out_of_range(fmt::format("Error: {} is not found in config file.", key));
}

void readConfig(const std::filesystem::path& config_file, Config& config) {
  using namespace std::string_view_literals;

  try {
    toml::table config_table = toml::parse_file(config_file.string());

    config.config_file_ = config_file;

    config.dimension_ = getValueFromToml<int>(config_table, "Dimension"sv);

    config.polynomial_order_ = getValueFromToml<int>(config_table, "PolynomialOrder"sv);

    config.simulation_type_ =
        castStringToEnum<SimulationType>(getValueFromToml<std::string_view>(config_table, "SimulationType"sv));

    config.no_vis_flux_type_ =
        castStringToEnum<NoVisFluxType>(getValueFromToml<std::string_view>(config_table, "NoVisFluxType"sv));

    config.mesh_file_ = kProjectSourceDir / getValueFromToml<std::string_view>(config_table, "MeshFile"sv);

    config.time_integration_.time_integration_type_ = castStringToEnum<TimeIntegrationType>(
        getValueFromToml<std::string_view>(config_table, "TimeIntegration.type"sv));
    config.time_integration_.iteration_ = getValueFromToml<Usize>(config_table, "TimeIntegration.iteration"sv);
    config.time_integration_.cfl_ = getValueFromToml<Real>(config_table, "TimeIntegration.CFL"sv);
    config.time_integration_.tolerance_ = getValueFromToml<int>(config_table, "TimeIntegration.tolerance"sv);

    for (const auto& boundary_iter : getValueFromToml<toml::array>(config_table, "Boundary.name"sv)) {
      std::string boundary_name = boundary_iter.as_string()->get();
      config.boundary_condition_[boundary_name] = castStringToEnum<BoundaryType>(
          getValueFromToml<std::string_view>(config_table, fmt::format("BoundaryCondition.{}.type", boundary_name)));
    }

    config.thermodynamic_model_.equation_of_state_ = castStringToEnum<EquationOfState>(
        getValueFromToml<std::string_view>(config_table, "ThermodynamicModel.EquationOfState"sv));
    config.thermodynamic_model_.gamma_ = getValueFromToml<Real>(config_table, "ThermodynamicModel.gamma"sv);
    config.thermodynamic_model_.c_p_ = getValueFromToml<Real>(config_table, "ThermodynamicModel.c_p"sv);
    config.thermodynamic_model_.r_ = getValueFromToml<Real>(config_table, "ThermodynamicModel.R"sv);
    if (config.simulation_type_ == SimulationType::NavierStokes) {
      config.thermodynamic_model_.mu_ = getValueFromToml<Real>(config_table, "ThermodynamicModel.mu"sv);
    }

    for (Usize i = 1; const auto& region_iter : getValueFromToml<toml::array>(config_table, "Region.name"sv)) {
      std::string region_name = region_iter.as_string()->get();
      config.region_name_map_[region_name] = i++;
      config.region_name_.emplace_back(region_name);
    }

    for (const auto& region_name : config.region_name_) {
      for (Isize i = 0; const auto& u_iter : getValueFromToml<toml::array>(
                            config_table, fmt::format("InitialCondition.{}.U", region_name))) {
        config.initial_condition_[region_name].u_[i++] = u_iter.as<Real>()->get();
      }
      config.initial_condition_[region_name].rho_ =
          getValueFromToml<Real>(config_table, fmt::format("InitialCondition.{}.rho", region_name));
      config.initial_condition_[region_name].p_ =
          getValueFromToml<Real>(config_table, fmt::format("InitialCondition.{}.p", region_name));
      config.initial_condition_[region_name].t_ =
          getValueFromToml<Real>(config_table, fmt::format("InitialCondition.{}.T", region_name));
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
    std::cerr << error.what() << std::endl;
    std::exit(EXIT_FAILURE);
  }
}

}  // namespace SubrosaDG::Internal
