/**
 * @file SimulationControl.cpp
 * @brief The header file of SimulationControl.
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

  void setMesh(const std::filesystem::path& mesh_file_path) { this->mesh_.initializeMesh(mesh_file_path); }

  void setMesh(const std::filesystem::path& mesh_file_path,
               const std::function<void(const std::filesystem::path& mesh_file_path)>& generate_mesh_function) {
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

  template <BoundaryConditionEnum BoundaryConditionType>
  void addBoundaryCondition(const Isize physical_index) {
    this->mesh_.physical_.information_[static_cast<Usize>(physical_index) - 1].boundary_condition_type_ =
        BoundaryConditionType;
  }

  void setTimeIntegration(const Real courant_friedrichs_lewy_number, const std::pair<int, int> iteration_range = {0, 0},
                          const Real delta_time = 0.0_r) {
    if (iteration_range.first == 0 && iteration_range.second == 0) {
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
    this->view_.iteration_order_ = static_cast<int>(std::log10(this->time_integration_.iteration_end_) + 1);
    this->view_.output_directory_ = output_directory;
    this->view_.output_file_name_prefix_ = output_file_name_prefix;
  }

  void addViewVariable(const std::vector<ViewVariableEnum>& view_variable) {
    this->view_.variable_type_ = view_variable;
  }

  void synchronize() {
    this->mesh_.readMeshElement();
#ifdef SUBROSA_DG_GPU
    this->mesh_device_.transferMeshToDevice(this->mesh_);
#endif  // SUBROSA_DG_GPU
    if constexpr (SimulationControl::kInitialCondition == InitialConditionEnum::LowOrder) {
      RawBinaryCompress::read(this->solver_.raw_binary_path_, this->solver_.raw_binary_ss_);
    }
    if constexpr (SimulationControl::kInitialCondition == InitialConditionEnum::LastStep) {
      this->solver_.raw_binary_path_ =
          this->view_.output_directory_ /
          std::format("raw/{}_{}.zst", this->view_.output_file_name_prefix_, this->time_integration_.iteration_start_);
      RawBinaryCompress::read(this->solver_.raw_binary_path_, this->solver_.raw_binary_ss_);
    }
    this->command_line_.printInformation();
  }

  void solve(const bool delete_dir = true) {
    this->view_.initializeSolverFinout(this->solver_.error_finout_, delete_dir);
    this->solver_.initializeSolver(this->mesh_);
#ifdef SUBROSA_DG_GPU
    this->solver_device_.transferSolverToDevice(this->solver_);
#endif  // SUBROSA_DG_GPU
    if (this->time_integration_.delta_time_ == 0.0_r) {
      this->solver_.computeDeltaTime(this->mesh_, this->time_integration_);
    }
    if constexpr (SimulationControl::kInitialCondition != InitialConditionEnum::LastStep) {
      this->solver_.writeRawBinary(
          this->mesh_,
          this->view_.output_directory_ / std::format("raw/{}_{}.zst", this->view_.output_file_name_prefix_, 0));
    } else {
      this->solver_.write_raw_binary_future_ = std::async(std::launch::async, []() {});
    }
    this->command_line_.initializeSolver(this->time_integration_, this->solver_.error_finout_,
                                         this->solver_.error_output_interval_);
    for (int i = this->time_integration_.iteration_start_ + 1; i <= this->time_integration_.iteration_end_; i++) {
#ifndef SUBROSA_DG_GPU
      this->solver_.stepSolver(this->mesh_, this->source_term_, this->time_integration_);
#else   // SUBROSA_DG_GPU
      this->solver_device_.stepSolver(this->mesh_device_, this->source_term_device_, this->time_integration_);
#endif  // SUBROSA_DG_GPU
      this->time_integration_.iteration_ = i;
      if (i % this->solver_.error_output_interval_ == 0) {
#ifndef SUBROSA_DG_GPU
        this->solver_.computeRelativeError(this->mesh_);
#else   // SUBROSA_DG_GPU
        this->solver_device_.computeRelativeError(this->mesh_device_, this->solver_);
#endif  // SUBROSA_DG_GPU
        this->command_line_.updateSolver(this->solver_.relative_error_, this->solver_.error_finout_, i);
      }
      if (i % this->view_.io_interval_ == 0) [[unlikely]] {
        this->solver_.write_raw_binary_future_.get();
#ifdef SUBROSA_DG_GPU
        this->solver_device_.transferSolverToHost(this->solver_);
#endif  // SUBROSA_DG_GPU
        this->solver_.writeRawBinary(
            this->mesh_,
            this->view_.output_directory_ / std::format("raw/{}_{}.zst", this->view_.output_file_name_prefix_, i));
      }
      if (this->solver_.relative_error_.array().isNaN().all()) [[unlikely]] {
        if (this->view_.io_interval_ == this->time_integration_.iteration_end_) {
          this->view_.io_interval_ = i;
        }
        this->time_integration_.iteration_end_ = i;
        break;
      }
    }
    this->solver_.write_raw_binary_future_.get();
    this->view_.finalizeSolverFinout(this->solver_.error_finout_);
  }

  void view(const bool delete_dir = true) {
    this->command_line_.initializeView(
        (this->time_integration_.iteration_end_ - this->time_integration_.iteration_start_) / this->view_.io_interval_ +
        1);
    this->view_.initializeViewFin(this->time_integration_.iteration_end_, this->solver_.error_output_interval_,
                                  delete_dir);
#ifndef SUBROSA_DG_DEVELOP
    oneapi::tbb::task_arena arena(kNumberOfPhysicalCores / 2);
#else   // SUBROSA_DG_DEVELOP
    oneapi::tbb::task_arena arena(1);
#endif  // SUBROSA_DG_DEVELOP
    arena.execute([&] {
      tbb::spin_mutex mtx;
      tbb::enumerable_thread_specific<ViewSolver<SimulationControl>> thread_view_solver(
          [&] { return ViewSolver<SimulationControl>(this->mesh_); });
      tbb::parallel_for(tbb::blocked_range<Isize>(this->time_integration_.iteration_start_,
                                                  this->time_integration_.iteration_end_ + 1),
                        [&](const tbb::blocked_range<Isize>& range) -> void {
                          ViewSolver<SimulationControl>& view_solver = thread_view_solver.local();
                          for (Isize i = range.begin(); i != range.end(); i++) {
                            if (i % this->view_.io_interval_ == 0) {
                              view_solver.raw_binary_path_ =
                                  this->view_.output_directory_ /
                                  std::format("raw/{}_{}.zst", this->view_.output_file_name_prefix_, i);
                              this->view_.stepView(this->mesh_, view_solver, i);
                              {
                                tbb::spin_mutex::scoped_lock lock(mtx);
                                this->command_line_.updateView();
                              }
                            }
                          }
                        });
    });
    this->view_.finalizeViewFin();
  }

  explicit System() : command_line_(CommandLineEnum::Open) {}

  explicit System(const CommandLineEnum command_line_type) : command_line_(command_line_type) {}
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_SYSTEM_CONTROL_CPP_
