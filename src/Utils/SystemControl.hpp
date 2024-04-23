/**
 * @file SimulationControl.hpp
 * @brief The header file of SimulationControl.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_SYSTEM_CONTROL_HPP_
#define SUBROSA_DG_SYSTEM_CONTROL_HPP_

#include <Eigen/Core>
#include <filesystem>
#include <fstream>
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
  std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>> boundary_condition_;
  std::unordered_map<Isize, InitialCondition<SimulationControl>> initial_condition_;
  TimeIntegration<SimulationControl> time_integration_;
  Solver<SimulationControl> solver_;
  View<SimulationControl> view_;

  inline void setMesh(const std::filesystem::path& mesh_file_path,
                      const std::function<void(const std::filesystem::path& mesh_file_path)>& generate_mesh_function) {
    this->mesh_.initializeMesh(mesh_file_path, generate_mesh_function);
  }

  template <BoundaryConditionEnum BoundaryConditionType>
    requires(BoundaryConditionType == BoundaryConditionEnum::RiemannFarfield ||
             BoundaryConditionType == BoundaryConditionEnum::CharacteristicInflow ||
             BoundaryConditionType == BoundaryConditionEnum::PressureOutflow)
  inline void addBoundaryCondition(
      const std::string& boundary_condition_name,
      const Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber>& boundary_condition_variable) {
    const auto boundary_condition_index =
        static_cast<Isize>(this->mesh_.information_.physical_.find_index(boundary_condition_name));
    this->boundary_condition_[boundary_condition_index] =
        std::make_unique<BoundaryCondition<SimulationControl, BoundaryConditionType>>();
    this->boundary_condition_[boundary_condition_index]->boundary_dummy_variable_.primitive_ =
        boundary_condition_variable;
  }

  template <BoundaryConditionEnum BoundaryConditionType>
    requires(BoundaryConditionType == BoundaryConditionEnum::IsothermalNoslipWall)
  inline void addBoundaryCondition(const std::string& boundary_condition_name,
                                   const Real boundary_condition_temperature) {
    const auto boundary_condition_index =
        static_cast<Isize>(this->mesh_.information_.physical_.find_index(boundary_condition_name));
    this->boundary_condition_[boundary_condition_index] =
        std::make_unique<BoundaryCondition<SimulationControl, BoundaryConditionType>>();
    this->boundary_condition_[boundary_condition_index]->boundary_dummy_variable_.primitive_.setZero();
    this->boundary_condition_[boundary_condition_index]
        ->boundary_dummy_variable_.template setScalar<PrimitiveVariableEnum::Temperature>(
            boundary_condition_temperature);
  }

  template <BoundaryConditionEnum BoundaryConditionType>
    requires(BoundaryConditionType == BoundaryConditionEnum::AdiabaticSlipWall ||
             BoundaryConditionType == BoundaryConditionEnum::AdiabaticNoSlipWall)
  inline void addBoundaryCondition(const std::string& boundary_condition_name) {
    const auto boundary_condition_index =
        static_cast<Isize>(this->mesh_.information_.physical_.find_index(boundary_condition_name));
    this->boundary_condition_[boundary_condition_index] =
        std::make_unique<BoundaryCondition<SimulationControl, BoundaryConditionType>>();
  }

  template <BoundaryConditionEnum BoundaryConditionType>
    requires(BoundaryConditionType == BoundaryConditionEnum::Periodic)
  inline void addBoundaryCondition(const std::string& boundary_condition_name) {
    this->mesh_.addPeriodicBoundary(boundary_condition_name);
  }

  inline void addInitialCondition(
      const std::string& initial_condition_name,
      const std::function<Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber>(
          const Eigen::Vector<Real, SimulationControl::kDimension>& coordinate)>& initial_condition_function) {
    const auto initial_condition_index =
        static_cast<Isize>(this->mesh_.information_.physical_.find_index(initial_condition_name));
    this->initial_condition_[initial_condition_index].function_ = initial_condition_function;
  }

  inline void setTimeIntegration(const bool is_steady, const int iteration_number,
                                 const Real courant_friedrichs_lewy_number) {
    this->time_integration_.is_steady_ = is_steady;
    this->time_integration_.iteration_number_ = iteration_number;
    this->time_integration_.courant_friedrichs_lewy_number_ = courant_friedrichs_lewy_number;
  }

  inline void setViewConfig(const int io_interval, const std::filesystem::path& output_directory,
                            const std::string_view output_file_name_prefix, const ViewConfigEnum view_config) {
    if (io_interval <= 0) {
      this->view_.io_interval_ = this->time_integration_.iteration_number_;
    } else {
      this->view_.io_interval_ = io_interval;
    }
    this->view_.iteration_order_ = static_cast<int>(log10(this->time_integration_.iteration_number_) + 1);
    this->view_.output_directory_ = output_directory;
    this->view_.output_file_name_prefix_ = output_file_name_prefix;
    this->view_.config_enum_ = view_config;
    this->view_.initializeViewRawBinary(this->view_.config_enum_);
    this->command_line_.initializeCommandLine(this->view_.error_finout_);
  }

  inline void setViewVariable(const std::vector<ViewVariableEnum>& view_variable) {
    this->view_.variable_type_.clear();
    if constexpr (SimulationControl::kViewModel == ViewModelEnum::Dat) {
      auto handle_variable = [this](ViewVariableEnum variable_x, ViewVariableEnum variable_y,
                                    ViewVariableEnum variable_z) {
        this->view_.variable_type_.emplace_back(variable_x);
        if constexpr (SimulationControl::kDimension >= 2) {
          this->view_.variable_type_.emplace_back(variable_y);
        }
        if constexpr (SimulationControl::kDimension >= 3) {
          this->view_.variable_type_.emplace_back(variable_z);
        }
      };
      for (const auto variable : view_variable) {
        if (variable == ViewVariableEnum::Velocity) {
          handle_variable(ViewVariableEnum::VelocityX, ViewVariableEnum::VelocityY, ViewVariableEnum::VelocityZ);
        } else if (variable == ViewVariableEnum::MachNumber) {
          handle_variable(ViewVariableEnum::MachNumberX, ViewVariableEnum::MachNumberY, ViewVariableEnum::MachNumberZ);
        } else if (variable == ViewVariableEnum::Vorticity && SimulationControl::kDimension == 3) {
          handle_variable(ViewVariableEnum::VorticityX, ViewVariableEnum::VorticityY, ViewVariableEnum::VorticityZ);
        } else {
          this->view_.variable_type_.emplace_back(variable);
        }
      }
    } else if constexpr (SimulationControl::kViewModel == ViewModelEnum::Vtu) {
      this->view_.variable_type_ = view_variable;
    }
  }

  inline void synchronize() { this->mesh_.readMeshElement(); }

  inline void solve() {
    this->command_line_.initializeSolver(this->time_integration_.iteration_number_);
    this->solver_.initializeSolver(this->mesh_, this->thermal_model_, this->boundary_condition_,
                                   this->initial_condition_);
    Real delta_time{0.0};
    this->solver_.writeRawBinary(this->view_.raw_binary_finout_);
    this->view_.error_finout_ << this->command_line_.getLineInformation(delta_time, this->solver_.relative_error_) << '\n';
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
      this->command_line_.updateSolver(i, delta_time, this->solver_.relative_error_, this->view_.error_finout_);
    }
  }

  inline void view(const bool delete_dir = true) {
    this->command_line_.initializeView(this->time_integration_.iteration_number_ / this->view_.io_interval_);
    this->view_.initializeView(delete_dir, this->time_integration_.iteration_number_ + 1, this->mesh_);
    this->view_.stepView(0, this->mesh_, this->thermal_model_);
    for (int i = 1; i <= this->time_integration_.iteration_number_; i++) {
      if (i % this->view_.io_interval_ == 0) {
        this->view_.stepView(i, this->mesh_, this->thermal_model_);
        this->command_line_.updateView();
      }
    }
    this->view_.finalizeView();
  }

  explicit inline System() : command_line_(true) {}

  explicit inline System(const bool open_command_line) : command_line_(open_command_line) {}
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_SYSTEM_CONTROL_HPP_
