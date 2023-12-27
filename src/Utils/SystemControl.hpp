/**
 * @file SimulationControl.hpp
 * @brief The header file of SimulationControl.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_SYSTEM_CONTROL_HPP_
#define SUBROSA_DG_SYSTEM_CONTROL_HPP_
#include <Eigen/Core>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Mesh/ReadControl.hpp"
#include "Solver/BoundaryCondition.hpp"
#include "Solver/InitialCondition.hpp"
#include "Solver/SolveControl.hpp"
#include "Solver/ThermalModel.hpp"
#include "Solver/TimeIntegration.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Enum.hpp"
#include "Utils/Environment.hpp"
#include "View/CommandLine.hpp"
#include "View/IOControl.hpp"

namespace SubrosaDG {

template <typename SimulationControl>
struct System {
  Environment environment_;
  CommandLine<SimulationControl> command_line_;
  Mesh<SimulationControl> mesh_;
  ThermalModel<SimulationControl> thermal_model_;
  std::unordered_map<std::string, std::unique_ptr<BoundaryConditionBase<SimulationControl>>> boundary_condition_;
  std::unordered_map<std::string, InitialCondition<SimulationControl>> initial_condition_;
  TimeIntegration<SimulationControl> time_integration_;
  Solver<SimulationControl> solver_;
  View<SimulationControl> view_;

  inline void setCommandLineConfig(const std::filesystem::path& output_directory) {
    this->command_line_.output_directory_ = output_directory;
    this->command_line_.initializeCommandLine();
  }

  template <BoundaryConditionEnum BoundaryConditionType>
    requires(BoundaryConditionType == BoundaryConditionEnum::AdiabaticWall)
  inline void addBoundaryCondition(const std::string&);

  template <BoundaryConditionEnum BoundaryConditionType>
    requires(BoundaryConditionType == BoundaryConditionEnum::NormalFarfield ||
             BoundaryConditionType == BoundaryConditionEnum::RiemannFarfield)
  inline void addBoundaryCondition(const std::string&,
                                   const Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber>&);

  template <>
  inline void addBoundaryCondition<BoundaryConditionEnum::NormalFarfield>(
      const std::string& boundary_condition_name,
      const Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber>& boundary_condition_variable) {
    this->boundary_condition_[boundary_condition_name] =
        std::make_unique<BoundaryCondition<SimulationControl, BoundaryConditionEnum::NormalFarfield>>();
    this->boundary_condition_[boundary_condition_name]->variable_.primitive_ = boundary_condition_variable;
  }

  template <>
  inline void addBoundaryCondition<BoundaryConditionEnum::RiemannFarfield>(
      const std::string& boundary_condition_name,
      const Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber>& boundary_condition_variable) {
    this->boundary_condition_[boundary_condition_name] =
        std::make_unique<BoundaryCondition<SimulationControl, BoundaryConditionEnum::RiemannFarfield>>();
    this->boundary_condition_[boundary_condition_name]->variable_.primitive_ = boundary_condition_variable;
  }

  template <>
  inline void addBoundaryCondition<BoundaryConditionEnum::AdiabaticWall>(const std::string& boundary_condition_name) {
    this->boundary_condition_[boundary_condition_name] =
        std::make_unique<BoundaryCondition<SimulationControl, BoundaryConditionEnum::AdiabaticWall>>();
  }

  inline void addInitialCondition(const std::string& initial_condition_name,
                                  std::function<Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber>(
                                      const Eigen::Vector<Real, SimulationControl::kDimension>& coordinate)>
                                      initial_condition_function) {
    this->initial_condition_[initial_condition_name].function_ = initial_condition_function;
  }

  template <ThermodynamicModelEnum ThermodynamicModelType>
  inline void setThermodynamicModel(Real);

  template <>
  inline void setThermodynamicModel<ThermodynamicModelEnum::ConstantE>(const Real specific_heat_constant_volume) {
    this->thermal_model_.specific_heat_constant_volume_ = specific_heat_constant_volume;
  }

  template <>
  inline void setThermodynamicModel<ThermodynamicModelEnum::ConstantH>(const Real specific_heat_constant_pressure) {
    this->thermal_model_.specific_heat_constant_pressure_ = specific_heat_constant_pressure;
  }

  template <EquationOfStateEnum EquationOfStateType>
  inline void setEquationOfState(Real);

  template <>
  inline void setEquationOfState<EquationOfStateEnum::IdealGas>(const Real specific_heat_ratio) {
    this->thermal_model_.specific_heat_ratio_ = specific_heat_ratio;
  }

  inline void setTimeIntegration(bool is_steady, int iteration_number, Real courant_friedrichs_lewy_number,
                                 Real tolerance) {
    this->time_integration_.is_steady_ = is_steady;
    this->time_integration_.iteration_number_ = iteration_number;
    this->time_integration_.courant_friedrichs_lewy_number_ = courant_friedrichs_lewy_number;
    this->time_integration_.tolerance_ = tolerance;
  }

  inline void setViewConfig(int io_interval, const std::filesystem::path& output_directory,
                            const std::string& output_file_name_prefix,
                            const ViewConfigEnum view_config = ViewConfigEnum::Default) {
    this->setCommandLineConfig(output_directory);
    if (io_interval <= 0) {
      this->view_.io_interval_ = this->time_integration_.iteration_number_;
    } else {
      this->view_.io_interval_ = io_interval;
    }
    this->view_.output_directory_ = output_directory;
    this->view_.output_file_name_prefix_ = output_file_name_prefix;
    this->view_.config_enum_ = view_config;
    this->view_.initializeViewRawBinary(this->view_.config_enum_);
  }

  inline void setViewVariable(const std::vector<ViewVariableEnum>& view_variable) {
    this->view_.variable_vector_.clear();
    if constexpr (SimulationControl::kViewModel == ViewModelEnum::Dat) {
      auto handle_variable = [this](ViewVariableEnum variable_x, ViewVariableEnum variable_y,
                                    ViewVariableEnum variable_z) {
        this->view_.variable_vector_.emplace_back(variable_x);
        if constexpr (SimulationControl::kDimension >= 2) {
          this->view_.variable_vector_.emplace_back(variable_y);
        }
        if constexpr (SimulationControl::kDimension >= 3) {
          this->view_.variable_vector_.emplace_back(variable_z);
        }
      };
      for (const auto variable : view_variable) {
        if (variable == ViewVariableEnum::Velocity) {
          handle_variable(ViewVariableEnum::VelocityX, ViewVariableEnum::VelocityY, ViewVariableEnum::VelocityZ);
        } else if (variable == ViewVariableEnum::MachNumber) {
          handle_variable(ViewVariableEnum::MachNumberX, ViewVariableEnum::MachNumberY, ViewVariableEnum::MachNumberZ);
        } else if (variable == ViewVariableEnum::Vorticity) {
          handle_variable(ViewVariableEnum::VorticityX, ViewVariableEnum::VorticityY, ViewVariableEnum::VorticityZ);
        } else {
          this->view_.variable_vector_.emplace_back(variable);
        }
      }
    } else if constexpr (SimulationControl::kViewModel == ViewModelEnum::Vtu) {
      this->view_.variable_vector_ = view_variable;
    }
  }

  inline void solve() {
    this->solver_.initializeSolver(this->mesh_, this->thermal_model_, this->boundary_condition_,
                                   this->initial_condition_);
    Real delta_time;
    for (int i = 1; i <= this->time_integration_.iteration_number_; i++) {
      this->solver_.copyBasisFunctionCoefficient();
      delta_time = this->solver_.calculateDeltaTime(this->mesh_, this->thermal_model_, this->time_integration_);
      for (int j = 0; j < this->time_integration_.kStep; j++) {
        this->solver_.stepSolver(j, this->mesh_, this->thermal_model_, this->boundary_condition_,
                                 this->time_integration_);
      }
      if (i % this->view_.io_interval_ == 0) {
        this->solver_.writeRawBinary(this->view_.raw_binary_finout_);
      }
      this->solver_.calculateRelativeError(this->mesh_);
      this->command_line_.updateSolver(this->time_integration_.iteration_number_, i, delta_time,
                                       this->solver_.relative_error_);
    }
  }

  inline void view(const bool delete_dir = true) {
    this->view_.initializeView(this->mesh_, delete_dir);
    for (int i = 1; i <= this->time_integration_.iteration_number_; i++) {
      if (i % this->view_.io_interval_ == 0) {
        this->view_.stepView(i, this->mesh_, this->thermal_model_);
        this->command_line_.updateView(this->time_integration_.iteration_number_ / this->view_.io_interval_,
                                       i / this->view_.io_interval_);
      }
    }
  }

  explicit inline System(
      const std::filesystem::path& mesh_file_path,
      const std::function<void(const std::filesystem::path& mesh_file_path)>& generate_mesh_function =
          [](const std::filesystem::path&) {},
      bool open_command_line = true)
      : command_line_(open_command_line), mesh_(mesh_file_path, generate_mesh_function) {}
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_SYSTEM_CONTROL_HPP_
