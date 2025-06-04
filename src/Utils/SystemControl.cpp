/**
 * @file SimulationControl.cpp
 * @brief The header file of SimulationControl.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2025 by SubrosaDG developers. All rights reserved.
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
#include "Solver/BoundaryCondition.cpp"
#include "Solver/InitialCondition.cpp"
#include "Solver/PhysicalModel.cpp"
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
  PhysicalModel<SimulationControl> physical_model_;
  BoundaryCondition<SimulationControl> boundary_condition_;
  InitialCondition<SimulationControl> initial_condition_;
  TimeIntegration<SimulationControl> time_integration_;
  Solver<SimulationControl> solver_;
  View<SimulationControl> view_;

  inline void setMesh(const std::filesystem::path& mesh_file_path,
                      const std::function<void(const std::filesystem::path& mesh_file_path)>& generate_mesh_function) {
    if constexpr (SimulationControl::kInitialCondition != InitialConditionEnum::LastStep) {
      generate_mesh_function(mesh_file_path);
    }
    this->mesh_.initializeMesh(mesh_file_path);
  }

  template <SourceTermEnum SourceTermType>
    requires(SourceTermType == SourceTermEnum::Boussinesq)
  inline void setSourceTerm(const Real thermal_expansion_coefficient, const Real reference_temperature) {
    this->source_term_.thermal_expansion_coefficient = thermal_expansion_coefficient;
    this->source_term_.reference_temperature = reference_temperature;
  }

  template <InitialConditionEnum InitialConditionType>
    requires(InitialConditionType == InitialConditionEnum::SpecificFile)
  inline void addInitialCondition(const std::filesystem::path& initial_condition_file) {
    this->initial_condition_.raw_binary_path_ = initial_condition_file;
  }

  template <BoundaryConditionEnum BoundaryConditionType>
  inline void addBoundaryCondition(const Isize physical_index) {
    this->mesh_.information_.physical_[static_cast<Usize>(physical_index) - 1].boundary_condition_type_ =
        BoundaryConditionType;
  }

  template <ThermodynamicModelEnum ThermodynamicModelType>
    requires(ThermodynamicModelType == ThermodynamicModelEnum::Constant)
  inline void setThermodynamicModel(const Real specific_heat_constant_pressure,
                                    const Real specific_heat_constant_volume) {
    this->physical_model_.thermodynamic_model_.specific_heat_constant_pressure = specific_heat_constant_pressure;
    this->physical_model_.thermodynamic_model_.specific_heat_constant_volume = specific_heat_constant_volume;
  }

  template <EquationOfStateEnum EquationOfStateType>
    requires(EquationOfStateType == EquationOfStateEnum::WeakCompressibleFluid)
  inline void setEquationOfState(const Real reference_sound_speed, const Real reference_density) {
    this->physical_model_.equation_of_state_.reference_sound_speed = reference_sound_speed;
    this->physical_model_.equation_of_state_.reference_density = reference_density;
    this->physical_model_.equation_of_state_.calculatePressureAdditionFromSoundSpeedDensity();
  }

  template <TransportModelEnum TransportModelType>
    requires(TransportModelType == TransportModelEnum::Constant || TransportModelType == TransportModelEnum::Sutherland)
  inline void setTransportModel(const Real dynamic_viscosity) {
    this->physical_model_.transport_model_.dynamic_viscosity = dynamic_viscosity;
    this->physical_model_.calculateThermalConductivityFromDynamicViscosity();
  }

  inline void setArtificialViscosity(const Real empirical_tolerance, const Real artificial_viscosity_factor = 1.0_r) {
    this->solver_.empirical_tolerance_ = empirical_tolerance;
    this->solver_.artificial_viscosity_factor_ = artificial_viscosity_factor;
  }

  inline void setTimeIntegration(const Real courant_friedrichs_lewy_number,
                                 const std::pair<int, int> iteration_range = {0, 0}) {
    if (iteration_range.first == 0 && iteration_range.second == 0) {
      std::cout << "\nSet time integration end number: ";
      std::cin >> this->time_integration_.iteration_end_;
    } else {
      this->time_integration_.iteration_start_ = iteration_range.first;
      this->time_integration_.iteration_end_ = iteration_range.second;
    }
    this->time_integration_.courant_friedrichs_lewy_number_ = courant_friedrichs_lewy_number;
  }

  inline void setDeltaTime(const Real delta_time) { this->time_integration_.delta_time_ = delta_time; }

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
    this->view_.iteration_order_ = static_cast<int>(std::log10(this->time_integration_.iteration_end_) + 1);
    this->view_.output_directory_ = output_directory;
    this->view_.output_file_name_prefix_ = output_file_name_prefix;
  }

  inline void addViewVariable(const std::vector<ViewVariableEnum>& view_variable) {
    this->view_.variable_type_ = view_variable;
  }

  inline void synchronize() {
    this->mesh_.readMeshElement();
    if constexpr (SimulationControl::kInitialCondition == InitialConditionEnum::SpecificFile) {
      RawBinaryCompress::read(this->initial_condition_.raw_binary_path_, this->initial_condition_.raw_binary_ss_);
    } else if constexpr (SimulationControl::kInitialCondition == InitialConditionEnum::LastStep) {
      this->initial_condition_.raw_binary_path_ =
          this->view_.output_directory_ /
          std::format("raw/{}_{}.zst", this->view_.output_file_name_prefix_, this->time_integration_.iteration_start_);
      RawBinaryCompress::read(this->initial_condition_.raw_binary_path_, this->initial_condition_.raw_binary_ss_);
    }
    this->command_line_.printInformation();
  }

  inline void solve(const bool delete_dir = true) {
    this->view_.initializeSolverFinout(delete_dir, this->solver_.error_finout_);
    this->solver_.initializeSolver(this->mesh_, this->physical_model_, this->boundary_condition_,
                                   this->initial_condition_);
    if (this->time_integration_.delta_time_ == 0.0_r) {
      this->solver_.calculateDeltaTime(this->mesh_, this->physical_model_, this->time_integration_);
    }
    if constexpr (SimulationControl::kInitialCondition != InitialConditionEnum::LastStep) {
      this->solver_.writeRawBinary(
          this->mesh_,
          this->view_.output_directory_ / std::format("raw/{}_{}.zst", this->view_.output_file_name_prefix_, 0));
    } else {
      this->solver_.write_raw_binary_future_ = std::async(std::launch::async, []() {});
    }
    this->command_line_.initializeSolver(this->time_integration_, this->solver_.error_finout_);
    for (int i = this->time_integration_.iteration_start_ + 1; i <= this->time_integration_.iteration_end_; i++) {
      this->solver_.stepSolver(this->mesh_, this->source_term_, this->physical_model_, this->boundary_condition_,
                               this->time_integration_);
      this->time_integration_.iteration_ = i;
      if (i % this->view_.io_interval_ == 0) [[unlikely]] {
        this->solver_.write_raw_binary_future_.get();
        this->solver_.writeRawBinary(
            this->mesh_,
            this->view_.output_directory_ / std::format("raw/{}_{}.zst", this->view_.output_file_name_prefix_, i));
      }
      this->command_line_.updateSolver(i, this->solver_.relative_error_, this->solver_.error_finout_);
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

  inline void view(const bool delete_dir = true) {
    this->command_line_.initializeView(
        (this->time_integration_.iteration_end_ - this->time_integration_.iteration_start_) / this->view_.io_interval_ +
        1);
    this->view_.initializeViewFin(delete_dir, this->time_integration_.iteration_end_);
#ifndef SUBROSA_DG_DEVELOP
    oneapi::tbb::task_arena arena(kNumberOfPhysicalCores / 2);
#else   // SUBROSA_DG_DEVELOP
    oneapi::tbb::task_arena arena(1);
#endif  // SUBROSA_DG_DEVELOP
    arena.execute([&] {
      tbb::spin_mutex mtx;
      tbb::enumerable_thread_specific<ViewData<SimulationControl>> thread_view_data([&] {
        return ViewData<SimulationControl>(this->mesh_);
      });
      tbb::parallel_for(tbb::blocked_range<Isize>(this->time_integration_.iteration_start_,
                                                  this->time_integration_.iteration_end_ + 1),
                        [&](const tbb::blocked_range<Isize>& range) {
                          ViewData<SimulationControl>& view_data = thread_view_data.local();
                          for (Isize i = range.begin(); i != range.end(); i++) {
                            if (i % this->view_.io_interval_ == 0) {
                              view_data.raw_binary_path_ =
                                  this->view_.output_directory_ /
                                  std::format("raw/{}_{}.zst", this->view_.output_file_name_prefix_, i);
                              this->view_.stepView(i, this->mesh_, this->physical_model_, view_data);
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

  explicit inline System() : command_line_(true) {}

  explicit inline System(const bool open_command_line) : command_line_(open_command_line) {}
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_SYSTEM_CONTROL_CPP_
