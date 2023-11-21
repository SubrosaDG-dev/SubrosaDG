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

#ifdef SUBROSA_DG_WITH_OPENMP
#include <omp.h>
#endif  // SUBROSA_DG_WITH_OPENMP

#include <fmt/core.h>
#include <gmsh.h>

#include <Eigen/Core>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <tqdm.hpp>
#include <unordered_map>

#include "Cmake.hpp"
#include "Mesh/ReadControl.hpp"
#include "Solver/BoundaryCondition.hpp"
#include "Solver/SolveControl.hpp"
#include "Solver/ThermalModel.hpp"
#include "Solver/TimeIntegration.hpp"
#include "Solver/VariableConvertor.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Enum.hpp"
#include "View/IOControl.hpp"

namespace SubrosaDG {

template <typename SimulationControl>
struct System {
  Mesh<SimulationControl, SimulationControl::kDimension> mesh_;
  ThermalModel<SimulationControl, SimulationControl::kEquationModel> thermal_model_;
  std::unordered_map<std::string, std::unique_ptr<BoundaryConditionBase<SimulationControl>>> boundary_condition_;
  std::unordered_map<std::string, Variable<SimulationControl, SimulationControl::kDimension>> initial_condition_;
  TimeIntegrationData<SimulationControl::kTimeIntegration> time_integration_;
  Solver<SimulationControl, SimulationControl::kDimension> solver_;
  View<SimulationControl, SimulationControl::kViewModel> view_;

  template <BoundaryCondition BoundaryConditionType>
  inline void addBoundaryCondition(const std::string&);

  template <BoundaryCondition BoundaryConditionType>
  inline void addBoundaryCondition(const std::string&,
                                   const Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber>&);

  template <>
  inline void addBoundaryCondition<BoundaryCondition::NoSlipWall>(const std::string& boundary_condition_name) {
    this->boundary_condition_[boundary_condition_name] =
        std::make_unique<BoundaryConditionData<SimulationControl, BoundaryCondition::NoSlipWall>>();
  }

  template <>
  inline void addBoundaryCondition<BoundaryCondition::NormalFarfield>(
      const std::string& boundary_condition_name,
      const Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber>& boundary_condition_variable) {
    this->boundary_condition_[boundary_condition_name] =
        std::make_unique<BoundaryConditionData<SimulationControl, BoundaryCondition::NormalFarfield>>();
    this->boundary_condition_[boundary_condition_name]->variable_.human_readable_primitive_ =
        boundary_condition_variable;
  }

  template <>
  inline void addBoundaryCondition<BoundaryCondition::RiemannFarfield>(
      const std::string& boundary_condition_name,
      const Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber>& boundary_condition_variable) {
    this->boundary_condition_[boundary_condition_name] =
        std::make_unique<BoundaryConditionData<SimulationControl, BoundaryCondition::RiemannFarfield>>();
    this->boundary_condition_[boundary_condition_name]->variable_.human_readable_primitive_ =
        boundary_condition_variable;
  }

  inline void addInitialCondition(
      const std::string& initial_condition_name,
      const Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber>& initial_condition_variable) {
    this->initial_condition_[initial_condition_name].human_readable_primitive_ = initial_condition_variable;
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
                            const std::string& output_file_name_prefix);

  [[nodiscard]] inline std::string getProgressBarInfo() const;

  inline void solve();

  inline System(const std::function<void()>& generate_mesh_function, const std::filesystem::path& mesh_file_path);

  inline ~System();
};

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
                                                     const std::string& output_file_name_prefix) {
  if (io_interval <= 0) {
    this->view_.io_interval_ = this->time_integration_.iteration_number_;
  } else {
    this->view_.io_interval_ = io_interval;
  }
  this->view_.output_directory_ = output_directory;
  this->view_.output_file_name_prefix_ = output_file_name_prefix;
  this->view_.initializeViewRawBinary();
}

template <typename SimulationControl>
[[nodiscard]] inline std::string System<SimulationControl>::getProgressBarInfo() const {
  if constexpr (SimulationControl::kDimension == 2) {
    return fmt::format("Residual: rho: {:.4e}, rhou: {:.4e}, rhov: {:.4e}, rhoE: {:.4e}",
                       this->solver_.absolute_error_(0), this->solver_.absolute_error_(1),
                       this->solver_.absolute_error_(2), this->solver_.absolute_error_(3));
  }
}

template <typename SimulationControl>
inline void System<SimulationControl>::solve() {
  this->solver_.initializeSolver(this->mesh_, this->thermal_model_, this->boundary_condition_,
                                 this->initial_condition_);
  Tqdm::ProgressBar progress_bar(this->time_integration_.iteration_number_);
  for (int i = 1; i <= this->time_integration_.iteration_number_; i++) {
    this->solver_.copyBasisFunctionCoefficient();
    this->solver_.calculateDeltaTime(this->mesh_, this->thermal_model_, this->time_integration_);
    for (int j = 0; j < this->time_integration_.kStep; j++) {
      this->solver_.stepTime(j, this->mesh_, this->thermal_model_, this->boundary_condition_, this->time_integration_);
    }
    if (i % this->view_.io_interval_ == 0) {
      this->solver_.writeRawBinary(this->view_.raw_binary_finout_);
    }
    this->solver_.calculateAbsoluteError(this->mesh_);
    progress_bar << this->getProgressBarInfo();
    progress_bar.update();
  }
  this->view_.write(this->time_integration_.iteration_number_, this->mesh_, this->thermal_model_);
}

template <typename SimulationControl>
inline System<SimulationControl>::System(const std::function<void()>& generate_mesh_function,
                                         const std::filesystem::path& mesh_file_path)
    : mesh_(generate_mesh_function, mesh_file_path) {
  std::cout << '\n';
  std::cout << "SubrosaDG Info:" << '\n';
  std::cout << fmt::format("Version: {}", kSubrosaDGVersionString) << '\n';
#ifdef SUBROSA_DG_DEVELOP
  std::cout << "Build type: Debug" << '\n';
#else
  std::cout << "Build type: Release" << '\n';
#endif  // SUBROSA_DG_DEVELOP
#ifdef SUBROSA_DG_WITH_OPENMP
  omp_set_num_threads(kNumberOfPhysicalCores);
  std::cout << fmt::format("Number of physical cores: {}", kNumberOfPhysicalCores) << "\n\n";
#else
  std::cout << "Number of physical cores: 1"
            << "\n\n";
#endif  // SUBROSA_DG_WITH_OPENMP
}

template <typename SimulationControl>
inline System<SimulationControl>::~System() {
  gmsh::finalize();
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_SYSTEM_CONTROL_HPP_
