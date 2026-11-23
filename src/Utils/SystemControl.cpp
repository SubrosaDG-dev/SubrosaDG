/**
 * @file SystemControl.cpp
 * @brief The header file of SystemControl.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_SYSTEM_CONTROL_CPP_
#define SUBROSA_DG_SYSTEM_CONTROL_CPP_

#include <Eigen/Core>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <functional>
#include <future>
#include <iostream>
#include <string_view>
#include <utility>
#include <vector>

#include "Mesh/ReadControl.cpp"
#include "Solver/SolveControl.cpp"
#include "Solver/SourceTerm.cpp"
#include "Solver/TimeIntegration.cpp"
#include "Utils/BasicDataType.cpp"
#include "Utils/Enum.cpp"
#include "Utils/Environment.cpp"
#include "View/CommandLine.cpp"
#include "View/IOControl.cpp"
#include "View/RawBinary.cpp"

namespace SubrosaDG {

template <typename SimulationControl>
struct System {
  Environment environment_;
  CommandLine<SimulationControl> command_line_;
  Mesh<SimulationControl> mesh_;
  SourceTerm<SimulationControl> source_term_;
  TimeIntegration<SimulationControl> time_integration_;
  Solver<SimulationControl> solver_;
  View<SimulationControl> view_;

#ifdef SUBROSA_DG_GPU
  MeshDevice<SimulationControl> mesh_device_;
  SourceTermDevice<SimulationControl> source_term_device_;
  SolverDevice<SimulationControl> solver_device_;
#endif  // SUBROSA_DG_GPU

  void setMesh(const std::filesystem::path& mesh_file_path) {
    this->command_line_.initializeMesh(mesh_file_path);
    this->mesh_.initializeMesh(mesh_file_path);
  }

  void setMesh(const std::filesystem::path& mesh_file_path,
               const std::function<void(const std::filesystem::path& mesh_file_path)>& generate_mesh_function) {
    this->command_line_.initializeMesh(mesh_file_path);
    if constexpr (SimulationControl::kInitialCondition != InitialConditionEnum::LastStep) {
      generate_mesh_function(mesh_file_path);
    }
    this->mesh_.initializeMesh(mesh_file_path);
  }

  template <SourceTermEnum SourceTermType = SimulationControl::kSourceTerm>
    requires(SourceTermType == SourceTermEnum::Boussinesq)
  void setSourceTerm(const Real thermal_expansion_coefficient, const Real reference_temperature) {
#ifndef SUBROSA_DG_GPU
    this->source_term_.thermal_expansion_coefficient_ = thermal_expansion_coefficient;
    this->source_term_.reference_temperature_ = reference_temperature;
#else   // SUBROSA_DG_GPU
    this->source_term_device_.thermal_expansion_coefficient_ = thermal_expansion_coefficient;
    this->source_term_device_.reference_temperature_ = reference_temperature;
#endif  // SUBROSA_DG_GPU
  }

  template <InitialConditionEnum InitialConditionType = SimulationControl::kInitialCondition>
    requires(InitialConditionType == InitialConditionEnum::LowOrder)
  void addInitialCondition(const std::filesystem::path& initial_condition_file) {
    this->solver_.raw_binary_path_ = initial_condition_file;
  }

  template <int Dimension = SimulationControl::kDimension>
    requires(Dimension >= 2)
  void setRotation(const Eigen::Vector<Real, SimulationControl::kDimension>& rotation_center,
                   const Eigen::Vector<Real, 3>& angular_velocity) {
    this->solver_.rotation_center_ = rotation_center;
    this->solver_.rotation_axis_ = angular_velocity.normalized();
    this->solver_.rotation_angular_velocity_ = angular_velocity.norm();
  }

  template <InteriorConditionEnum InteriorConditionType>
  void addVolumeCondition(const Isize physical_index) {
    this->mesh_.physical_.information_[static_cast<Usize>(physical_index) - 1].interior_condition_type_ =
        InteriorConditionType;
  }

  template <BoundaryConditionEnum BoundaryConditionType>
  void addBoundaryCondition(const Isize physical_index) {
    this->mesh_.physical_.information_[static_cast<Usize>(physical_index) - 1].boundary_condition_type_ =
        BoundaryConditionType;
  }

  void setTimeIntegration(const Real courant_friedrichs_lewy_number, const std::pair<int, int> iteration_range = {0, 0},
                          const Real delta_time = 0.0_r) {
    if (iteration_range.first == 0 && iteration_range.second == 0) {
      if (this->command_line_.command_line_type_ == CommandLineEnum::Agent) {
        this->command_line_.diagnostic("error", "configuration", "missing_parameter",
                                       "Agent mode requires an explicit iteration_range in setTimeIntegration().");
        std::exit(EXIT_FAILURE);
      }
      std::cout << "\nSet time integration end number: ";
      std::cin >> this->time_integration_.iteration_end_;
    } else {
      this->time_integration_.iteration_start_ = iteration_range.first;
      this->time_integration_.iteration_end_ = iteration_range.second;
    }
    this->time_integration_.iteration_ = this->time_integration_.iteration_start_;
    this->time_integration_.courant_friedrichs_lewy_number_ = courant_friedrichs_lewy_number;
    this->time_integration_.delta_time_ = delta_time;
  }

  void setViewConfig(const std::filesystem::path& output_directory, const std::string_view output_file_name_prefix,
                     const int io_interval = 0) {
    if (io_interval == 0) {
      if (this->command_line_.command_line_type_ == CommandLineEnum::Agent) {
        this->command_line_.diagnostic("error", "configuration", "missing_parameter",
                                       "Agent mode requires an explicit io_interval in setViewConfig().");
        std::exit(EXIT_FAILURE);
      }
      std::cout << "\nSet view interval: ";
      std::cin >> this->view_.io_interval_;
      if (this->view_.io_interval_ == -1) {
        this->view_.io_interval_ = this->time_integration_.iteration_end_;
      }
    } else if (io_interval == -1) {
      this->view_.io_interval_ = this->time_integration_.iteration_end_;
    } else {
      this->view_.io_interval_ = io_interval;
    }
    this->view_.iteration_order_ = static_cast<int>(std::log10(this->time_integration_.iteration_end_) + 1);
    this->view_.output_directory_ = output_directory;
    this->view_.output_file_name_prefix_ = output_file_name_prefix;
  }

  void addViewVariable(const std::vector<ViewVariableEnum>& view_variable) {
    this->view_.variable_type_ = view_variable;
  }

  void synchronize() {
    this->mesh_.readMeshElement(this->solver_.rotation_center_, this->solver_.rotation_axis_);
#ifdef SUBROSA_DG_GPU
    this->mesh_device_.transferMeshToDevice(this->mesh_);
#endif  // SUBROSA_DG_GPU
    if constexpr (SimulationControl::kInitialCondition == InitialConditionEnum::LastStep) {
      this->solver_.raw_binary_path_ =
          this->view_.output_directory_ /
          std::format("raw/{}_{}.zst", this->view_.output_file_name_prefix_, this->time_integration_.iteration_start_);
      RawBinaryCompress::read(this->solver_.raw_binary_path_, this->solver_.raw_binary_ss_);
    } else if constexpr (SimulationControl::kInitialCondition == InitialConditionEnum::LowOrder) {
      RawBinaryCompress::read(this->solver_.raw_binary_path_, this->solver_.raw_binary_ss_);
    }
    this->command_line_.finalizeMesh(this->mesh_.node_number_, this->mesh_.volume_element_number_);
    this->command_line_.printInformation();
  }

  void solve(const bool delete_dir = true) {
    bool diverged = false;
    this->solver_.initializeSolver(this->mesh_);
    this->solver_.initializeBasisFunctionCoefficient(this->mesh_);
    this->view_.initializeSolverFinout(this->solver_.error_finout_, delete_dir);
#ifdef SUBROSA_DG_GPU
    this->solver_device_.transferSolverToDevice(this->solver_);
#endif  // SUBROSA_DG_GPU
    if (this->time_integration_.delta_time_ == 0.0_r) {
      this->solver_.rotateMesh(this->mesh_, 0.0_r);
      this->solver_.computeDeltaTime(this->mesh_, this->time_integration_);
    }
    this->solver_.raw_binary_ss_.str("");
    this->solver_.write_raw_binary_future_ = std::async(std::launch::async, []() {});
    if constexpr (SimulationControl::kInitialCondition != InitialConditionEnum::LastStep) {
      this->solver_.writeRawBinary(
          this->view_.output_directory_ / std::format("raw/{}_{}.zst", this->view_.output_file_name_prefix_, 0), 0.0_r);
    }
    this->command_line_.initializeSolver(this->time_integration_, this->solver_.error_finout_,
                                         this->solver_.error_output_interval_, this->view_.output_directory_);
    for (int i = this->time_integration_.iteration_start_ + 1; i <= this->time_integration_.iteration_end_; i++) {
#ifndef SUBROSA_DG_GPU
      this->solver_.stepSolver(this->mesh_, this->source_term_, this->time_integration_);
#else   // SUBROSA_DG_GPU
      this->solver_device_.stepSolver(this->mesh_device_, this->source_term_device_, this->time_integration_);
#endif  // SUBROSA_DG_GPU
      this->time_integration_.iteration_ = i;
      if (i % this->solver_.error_output_interval_ == 0) {
#ifndef SUBROSA_DG_GPU
        this->solver_.computeMeanAbsoluteResidual(this->mesh_);
#else   // SUBROSA_DG_GPU
        this->solver_device_.computeMeanAbsoluteResidual(this->mesh_device_, this->solver_);
#endif  // SUBROSA_DG_GPU
        this->command_line_.updateSolver(this->solver_.residual_, this->solver_.error_finout_, i);
        // Check only a freshly computed sample; any NaN or infinity stops this run.
        if (!this->solver_.residual_.allFinite()) [[unlikely]] {
          this->command_line_.reportNonfiniteResidual(this->solver_.residual_, i);
          diverged = true;
          break;
        }
      }
      if (i % this->view_.io_interval_ == 0) [[unlikely]] {
#ifdef SUBROSA_DG_GPU
        this->solver_device_.transferCoefficientToHost(this->solver_);
#endif  // SUBROSA_DG_GPU
        const Real time = static_cast<Real>(i) * this->time_integration_.delta_time_;
        this->solver_.writeRawBinary(
            this->view_.output_directory_ / std::format("raw/{}_{}.zst", this->view_.output_file_name_prefix_, i),
            time);
      }
    }
    this->solver_.write_raw_binary_future_.get();
    this->view_.finalizeSolverFinout(this->solver_.error_finout_);
    this->command_line_.finalizeSolver(this->time_integration_.iteration_, !diverged);
    if (diverged) {
      std::exit(EXIT_FAILURE);
    }
  }

  void view(const bool delete_dir = true) {
    const int start = this->time_integration_.iteration_start_;
    const int end = this->time_integration_.iteration_end_;
    const int interval = this->view_.io_interval_;
    // Count the exact multiples visited by the loop, including zero when present.
    const int snapshots = end / interval - (start == 0 ? -1 : (start - 1) / interval);
    this->command_line_.initializeView(snapshots, this->view_.output_directory_ / "vtu");
    this->view_.initializeViewFout(delete_dir);
    this->solver_.initializeSolver(this->mesh_);
    ViewSolver<SimulationControl> view_solver(this->mesh_);
    for (int i = start; i <= end; i++) {
      if (i % interval == 0) {
        this->view_.stepView(this->mesh_, this->solver_, view_solver, i);
        this->command_line_.updateView();
      }
    }
    this->command_line_.finalizeView();
  }

  explicit System() : command_line_(CommandLineEnum::Open) {}

  explicit System(const CommandLineEnum command_line_type) : command_line_(command_line_type) {}
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_SYSTEM_CONTROL_CPP_
