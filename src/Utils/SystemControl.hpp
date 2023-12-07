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
#include <iostream>
#include <memory>
#include <string>
#include <tqdm.hpp>
#include <unordered_map>
#include <vector>

#include "Mesh/ReadControl.hpp"
#include "Solver/BoundaryCondition.hpp"
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
  Mesh<SimulationControl, SimulationControl::kDimension> mesh_;
  ThermalModel<SimulationControl, SimulationControl::kEquationModel> thermal_model_;
  std::unordered_map<std::string, std::unique_ptr<BoundaryConditionBase<SimulationControl>>> boundary_condition_;
  std::unordered_map<std::string, InitialCondition<SimulationControl, SimulationControl::kDimension>>
      initial_condition_;
  TimeIntegrationData<SimulationControl::kTimeIntegration> time_integration_;
  Solver<SimulationControl, SimulationControl::kDimension> solver_;
  View<SimulationControl, SimulationControl::kViewModel> view_;

  inline void setCommandLineConfig(const std::filesystem::path& output_directory);

  template <BoundaryCondition BoundaryConditionType>
  inline void addBoundaryCondition(const std::string&);

  template <BoundaryCondition BoundaryConditionType>
  inline void addBoundaryCondition(const std::string&,
                                   const Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber>&);

  template <>
  inline void addBoundaryCondition<BoundaryCondition::NormalFarfield>(
      const std::string& boundary_condition_name,
      const Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber>& boundary_condition_variable) {
    this->boundary_condition_[boundary_condition_name] =
        std::make_unique<BoundaryConditionData<SimulationControl, BoundaryCondition::NormalFarfield>>();
    this->boundary_condition_[boundary_condition_name]->variable_.primitive_ = boundary_condition_variable;
  }

  template <>
  inline void addBoundaryCondition<BoundaryCondition::RiemannFarfield>(
      const std::string& boundary_condition_name,
      const Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber>& boundary_condition_variable) {
    this->boundary_condition_[boundary_condition_name] =
        std::make_unique<BoundaryConditionData<SimulationControl, BoundaryCondition::RiemannFarfield>>();
    this->boundary_condition_[boundary_condition_name]->variable_.primitive_ = boundary_condition_variable;
  }

  template <>
  inline void addBoundaryCondition<BoundaryCondition::CharacteristicFarfield>(
      const std::string& boundary_condition_name,
      const Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber>& boundary_condition_variable) {
    this->boundary_condition_[boundary_condition_name] =
        std::make_unique<BoundaryConditionData<SimulationControl, BoundaryCondition::CharacteristicFarfield>>();
    this->boundary_condition_[boundary_condition_name]->variable_.primitive_ = boundary_condition_variable;
  }

  template <>
  inline void addBoundaryCondition<BoundaryCondition::AdiabaticNoSlipWall>(const std::string& boundary_condition_name) {
    this->boundary_condition_[boundary_condition_name] =
        std::make_unique<BoundaryConditionData<SimulationControl, BoundaryCondition::AdiabaticNoSlipWall>>();
  }

  template <>
  inline void addBoundaryCondition<BoundaryCondition::AdiabaticFreeSlipWall>(
      const std::string& boundary_condition_name) {
    this->boundary_condition_[boundary_condition_name] =
        std::make_unique<BoundaryConditionData<SimulationControl, BoundaryCondition::AdiabaticFreeSlipWall>>();
  }

  inline void addInitialCondition(const std::string& initial_condition_name,
                                  std::function<Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber>(
                                      const Eigen::Vector<Real, SimulationControl::kDimension>& coordinate)>
                                      initial_condition_function) {
    this->initial_condition_[initial_condition_name].function_ = initial_condition_function;
  }

  template <ThermodynamicModel ThermodynamicModelType>
  inline void setThermodynamicModel(Real);

  template <>
  inline void setThermodynamicModel<ThermodynamicModel::ConstantE>(const Real specific_heat_constant_volume) {
    this->thermal_model_.specific_heat_constant_volume_ = specific_heat_constant_volume;
  }

  template <>
  inline void setThermodynamicModel<ThermodynamicModel::ConstantH>(const Real specific_heat_constant_pressure) {
    this->thermal_model_.specific_heat_constant_pressure_ = specific_heat_constant_pressure;
  }

  template <EquationOfState EquationOfStateType>
  inline void setEquationOfState(Real);

  template <>
  inline void setEquationOfState<EquationOfState::IdealGas>(const Real specific_heat_ratio) {
    this->thermal_model_.specific_heat_ratio_ = specific_heat_ratio;
  }

  inline void setTimeIntegration(bool is_steady, int iteration_number, Real courant_friedrichs_lewy_number,
                                 Real tolerance);

  inline void setViewConfig(int io_interval, const std::filesystem::path& output_directory,
                            const std::string& output_file_name_prefix,
                            const std::vector<ViewElementVariable>& view_element_variable);

  inline void solve();

  explicit inline System(const std::function<void()>& generate_mesh_function,
                         const std::filesystem::path& mesh_file_path);
};

template <typename SimulationControl>
inline void System<SimulationControl>::setCommandLineConfig(const std::filesystem::path& output_directory) {
  this->command_line_.output_directory_ = output_directory;
  this->command_line_.initializeCommandLine();
}

template <typename SimulationControl>
inline void System<SimulationControl>::setTimeIntegration(const bool is_steady, const int iteration_number,
                                                          const Real courant_friedrichs_lewy_number,
                                                          const Real tolerance) {
  this->time_integration_.is_steady_ = is_steady;
  this->time_integration_.iteration_number_ = iteration_number;
  this->time_integration_.courant_friedrichs_lewy_number_ = courant_friedrichs_lewy_number;
  this->time_integration_.tolerance_ = tolerance;
}

template <typename SimulationControl>
inline void System<SimulationControl>::setViewConfig(const int io_interval,
                                                     const std::filesystem::path& output_directory,
                                                     const std::string& output_file_name_prefix,
                                                     const std::vector<ViewElementVariable>& view_element_variable) {
  this->setCommandLineConfig(output_directory);
  if (io_interval <= 0) {
    this->view_.io_interval_ = this->time_integration_.iteration_number_;
  } else {
    this->view_.io_interval_ = io_interval;
  }
  this->view_.output_directory_ = output_directory;
  this->view_.output_file_name_prefix_ = output_file_name_prefix;
  this->view_.view_element_variable_ = view_element_variable;
  this->view_.initializeViewRawBinary();
}

template <typename SimulationControl>
inline void System<SimulationControl>::solve() {
  this->solver_.initializeSolver(this->mesh_, this->thermal_model_, this->boundary_condition_,
                                 this->initial_condition_);
  Tqdm::ProgressBar solver_progress_bar(this->time_integration_.iteration_number_, 12);
  for (int i = 1; i <= this->time_integration_.iteration_number_; i++) {
    this->solver_.copyBasisFunctionCoefficient();
    this->solver_.calculateDeltaTime(this->mesh_, this->thermal_model_, this->time_integration_);
    for (int j = 0; j < this->time_integration_.kStep; j++) {
      this->solver_.stepSolver(j, this->mesh_, this->thermal_model_, this->boundary_condition_,
                               this->time_integration_);
    }
    if (i % this->view_.io_interval_ == 0) {
      this->solver_.writeRawBinary(this->view_.raw_binary_finout_);
    }
    this->solver_.calculateRelativeError(this->mesh_);
    solver_progress_bar << this->command_line_.updateError(i, this->solver_.relative_error_);
    solver_progress_bar.update();
  }
  std::cout << '\n';
  this->view_.initializeView();
  Tqdm::ProgressBar view_progress_bar(this->time_integration_.iteration_number_ / this->view_.io_interval_, 1);
  for (int i = 1; i <= this->time_integration_.iteration_number_; i++) {
    if (i % this->view_.io_interval_ == 0) {
      this->view_.stepView(i, this->mesh_, this->thermal_model_);
      view_progress_bar.update();
    }
  }
}

template <typename SimulationControl>
inline System<SimulationControl>::System(const std::function<void()>& generate_mesh_function,
                                         const std::filesystem::path& mesh_file_path)
    : mesh_(generate_mesh_function, mesh_file_path) {}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_SYSTEM_CONTROL_HPP_
