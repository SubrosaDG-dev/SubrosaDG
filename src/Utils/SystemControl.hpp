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
#include <format>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
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
  InitialCondition<SimulationControl> initial_condition_;
  TimeIntegration<SimulationControl> time_integration_;
  Solver<SimulationControl> solver_;
  View<SimulationControl> view_;

  inline void setMesh(const std::filesystem::path& mesh_file_path,
                      const std::function<void(const std::filesystem::path& mesh_file_path)>& generate_mesh_function) {
    this->mesh_.initializeMesh(mesh_file_path, generate_mesh_function);
  }

  template <BoundaryConditionEnum BoundaryConditionType>
    requires(BoundaryConditionType == BoundaryConditionEnum::RiemannFarfield)
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
      [[maybe_unused]] const std::string& initial_condition_name,
      const std::function<Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber>(
          const Eigen::Vector<Real, SimulationControl::kDimension>& coordinate)>& initial_condition_function) {
    this->initial_condition_.function_ = initial_condition_function;
    this->initial_condition_.type_ = InitialConditionEnum::Function;
  }

  inline void addInitialCondition([[maybe_unused]] const std::string& initial_condition_name,
                                  const std::filesystem::path& initial_condition_file) {
    this->initial_condition_.file_path_ = initial_condition_file;
    this->initial_condition_.type_ = InitialConditionEnum::SpecificFile;
  }

  inline void setTransportModel(const Real dynamic_viscosity) {
    this->thermal_model_.transport_model_.dynamic_viscosity = dynamic_viscosity;
    this->thermal_model_.calculateThermalConductivityFromDynamicViscosity();
  }

  inline void setTimeIntegration(const Real courant_friedrichs_lewy_number, const int iteration_start = 0,
                                 const int iteration_end = 0) {
    if (iteration_start == 0 && iteration_end == 0) {
      std::cout << "\nSet time integration start and number: ";
      std::cin >> this->time_integration_.iteration_start_ >> this->time_integration_.iteration_end_;
    } else {
      this->time_integration_.iteration_start_ = iteration_start;
      this->time_integration_.iteration_end_ = iteration_end;
    }
    this->time_integration_.courant_friedrichs_lewy_number_ = courant_friedrichs_lewy_number;
  }

  inline void setViewConfig(const std::filesystem::path& output_directory,
                            const std::string_view output_file_name_prefix, const int io_interval = 0) {
    if (io_interval == 0) {
      std::cout << "Set view interval: ";
      std::cin >> this->view_.io_interval_;
      if (this->view_.io_interval_ == -1) {
        this->view_.io_interval_ = this->time_integration_.iteration_end_;
      }
    } else if (io_interval == -1) {
      this->view_.io_interval_ = this->time_integration_.iteration_end_;
    } else {
      this->view_.io_interval_ = io_interval;
    }
    this->view_.iteration_order_ = static_cast<int>(log10(this->time_integration_.iteration_end_) + 1);
    this->view_.output_directory_ = output_directory;
    this->view_.output_file_name_prefix_ = output_file_name_prefix;
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

  inline void synchronize() {
    this->mesh_.readMeshElement();
    if (this->time_integration_.iteration_start_ != 0) {
      this->initial_condition_.file_path_ =
          this->view_.output_directory_ /
          std::format("raw/{}_{}.raw", this->view_.output_file_name_prefix_, this->time_integration_.iteration_start_);
      this->initial_condition_.type_ = InitialConditionEnum::LastFile;
      this->solver_.error_fout_.seekg(0, std::ios::beg);
      std::string line;
      for (int i = 0; i < 3; i++) {
        std::getline(this->solver_.error_fout_, line);
      }
      std::stringstream ss(line);
      ss.ignore(2) >> this->time_integration_.delta_time_;
      this->solver_.error_fout_.seekg(0, std::ios::end);
    }
  }

  inline void solve(const bool delete_dir = true) {
    this->view_.initializeSolverFout(delete_dir, this->time_integration_.iteration_start_, this->solver_.error_fout_);
    this->command_line_.initializeSolver(this->time_integration_.iteration_start_,
                                         this->time_integration_.iteration_end_);
    this->solver_.initializeSolver(this->mesh_, this->thermal_model_, this->boundary_condition_,
                                   this->initial_condition_);
    if (this->time_integration_.iteration_start_ == 0) {
      this->view_.setSolverRawBinaryFout(0, this->solver_.raw_binary_fout_);
      this->solver_.writeRawBinary();
      this->view_.finalizeSolverRawBinaryFout(this->solver_.raw_binary_fout_);
      this->solver_.error_fout_ << this->command_line_.getVariableList() << '\n'
                                << this->command_line_.getLineInformation(0.0, this->solver_.relative_error_) << '\n';
      this->solver_.calculateDeltaTime(this->mesh_, this->thermal_model_, this->time_integration_);
    }
    for (int i = this->time_integration_.iteration_start_ + 1; i <= this->time_integration_.iteration_end_; i++) {
      this->solver_.copyBasisFunctionCoefficient();
      for (int j = 0; j < this->time_integration_.kStep; j++) {
        this->solver_.stepSolver(j, this->mesh_, this->thermal_model_, this->boundary_condition_,
                                 this->time_integration_);
      }
      if (i % this->view_.io_interval_ == 0) {
        this->view_.setSolverRawBinaryFout(i, this->solver_.raw_binary_fout_);
        this->solver_.writeRawBinary();
        this->view_.finalizeSolverRawBinaryFout(this->solver_.raw_binary_fout_);
      }
      this->solver_.calculateRelativeError(this->mesh_);
      this->command_line_.updateSolver(i, this->time_integration_.delta_time_, this->solver_.relative_error_,
                                       this->solver_.error_fout_);
    }
    this->view_.finalizeSolverFout(this->solver_.error_fout_);
  }

  inline void view(const bool delete_dir = true) {
    this->command_line_.initializeView(this->time_integration_.iteration_end_ / this->view_.io_interval_ -
                                       this->time_integration_.iteration_start_ / this->view_.io_interval_);
    this->view_.initializeViewFin(delete_dir, this->time_integration_.iteration_start_,
                                  this->time_integration_.iteration_end_ + 1);
    ViewData<SimulationControl> view_data(this->mesh_);
    if (this->time_integration_.iteration_start_ == 0) {
      this->view_.setViewRawBinaryFin(0, view_data.raw_binary_fin_);
      this->view_.stepView(0, this->mesh_, this->thermal_model_, view_data);
      this->view_.finalizeViewRawBinaryFin(view_data.raw_binary_fin_);
    }
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto) firstprivate(view_data)
#endif  // SUBROSA_DG_DEVELOP
    for (int i = this->time_integration_.iteration_start_ + 1; i <= this->time_integration_.iteration_end_; i++) {
      if (i % this->view_.io_interval_ == 0) {
        this->view_.setViewRawBinaryFin(i, view_data.raw_binary_fin_);
        this->view_.stepView(i, this->mesh_, this->thermal_model_, view_data);
        this->view_.finalizeViewRawBinaryFin(view_data.raw_binary_fin_);
#ifndef SUBROSA_DG_DEVELOP
#pragma omp critical
#endif  // SUBROSA_DG_DEVELOP
        { this->command_line_.updateView(); }
      }
    }

    this->view_.finalizeViewFin();
  }

  explicit inline System() : command_line_(true) {}

  explicit inline System(const bool open_command_line) : command_line_(open_command_line) {}
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_SYSTEM_CONTROL_HPP_
