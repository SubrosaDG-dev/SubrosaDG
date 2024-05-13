/**
 * @file TimeIntegration.hpp
 * @brief The header file of TimeIntegration.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_TIME_INTEGRATION_HPP_
#define SUBROSA_DG_TIME_INTEGRATION_HPP_

#include <Eigen/Core>
#include <algorithm>
#include <array>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

#include "Mesh/ReadControl.hpp"
#include "Solver/BoundaryCondition.hpp"
#include "Solver/SolveControl.hpp"
#include "Solver/ThermalModel.hpp"
#include "Solver/VariableConvertor.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Concept.hpp"
#include "Utils/Constant.hpp"
#include "Utils/Enum.hpp"

namespace SubrosaDG {

struct TimeIntegrationBase {
  int iteration_start_{0};
  int iteration_end_;
  Real courant_friedrichs_lewy_number_;
  Real delta_time_{kRealMax};
};

template <>
struct TimeIntegrationData<TimeIntegrationEnum::ForwardEuler> : TimeIntegrationBase {
  inline static constexpr int kStep = 1;
  inline static constexpr std::array<std::array<Real, 3>, kStep> kStepCoefficients{{{1.0, 0.0, 1.0}}};
};

template <>
struct TimeIntegrationData<TimeIntegrationEnum::HeunRK2> : TimeIntegrationBase {
  inline static constexpr int kStep = 2;
  inline static constexpr std::array<std::array<Real, 3>, kStep> kStepCoefficients{{{1.0, 0.0, 1.0}, {0.5, 0.0, 0.5}}};
};

template <>
struct TimeIntegrationData<TimeIntegrationEnum::SSPRK3> : TimeIntegrationBase {
  inline static constexpr int kStep = 3;
  inline static constexpr std::array<std::array<Real, 3>, kStep> kStepCoefficients{
      {{1.0, 0.0, 1.0}, {3.0 / 4.0, 1.0 / 4.0, 1.0 / 4.0}, {1.0 / 3.0, 2.0 / 3.0, 2.0 / 3.0}}};
};

template <typename SimulationControl>
struct TimeIntegration : TimeIntegrationData<SimulationControl::kTimeIntegration> {};

template <typename ElementTrait, typename SimulationControl>
inline void ElementSolverBase<ElementTrait, SimulationControl>::copyElementBasisFunctionCoefficient() {
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < this->number_; i++) {
    this->element_(i).variable_basis_function_coefficient_(0).noalias() =
        this->element_(i).variable_basis_function_coefficient_(1);
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::copyBasisFunctionCoefficient() {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.copyElementBasisFunctionCoefficient();
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.copyElementBasisFunctionCoefficient();
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.copyElementBasisFunctionCoefficient();
    }
  }
}

template <typename ElementTrait, typename SimulationControl>
inline Real ElementSolverBase<ElementTrait, SimulationControl>::calculateElementDeltaTime(
    const ElementMesh<ElementTrait>& element_mesh, const ThermalModel<SimulationControl>& thermal_model,
    const Real courant_friedrichs_lewy_number) {
  Variable<SimulationControl> quadrature_node_variable;
  Eigen::Vector<Real, ElementTrait::kQuadratureNumber> delta_time;
  Real min_delta_time{kRealMax};
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto)                                                  \
    shared(element_mesh, thermal_model, courant_friedrichs_lewy_number) private(quadrature_node_variable, delta_time) \
    reduction(min : min_delta_time)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < element_mesh.number_; i++) {
    for (Isize j = 0; j < ElementTrait::kQuadratureNumber; j++) {
      quadrature_node_variable.template getFromSelf<ElementTrait>(element_mesh, *this, i, j);
      quadrature_node_variable.calculateComputationalFromConserved(thermal_model);
      const Real spectral_radius =
          std::sqrt(quadrature_node_variable.template getScalar<ComputationalVariableEnum::VelocitySquareSummation>()) +
          thermal_model.calculateSoundSpeedFromInternalEnergy(
              quadrature_node_variable.template getScalar<ComputationalVariableEnum::InternalEnergy>());
      delta_time(j) = courant_friedrichs_lewy_number * element_mesh.element_(i).size_ / spectral_radius;
    }
    min_delta_time = std::min(min_delta_time, delta_time.minCoeff());
  }
  return min_delta_time;
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::calculateDeltaTime(
    const Mesh<SimulationControl>& mesh, const ThermalModel<SimulationControl>& thermal_model,
    TimeIntegrationData<SimulationControl::kTimeIntegration>& time_integration) {
  if constexpr (SimulationControl::kInitialCondition != InitialConditionEnum::LastStep) {
    if constexpr (SimulationControl::kDimension == 1) {
      time_integration.delta_time_ =
          std::min(time_integration.delta_time_,
                   this->line_.calculateElementDeltaTime(mesh.line_, thermal_model,
                                                         time_integration.courant_friedrichs_lewy_number_));
    } else if constexpr (SimulationControl::kDimension == 2) {
      if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
        time_integration.delta_time_ =
            std::min(time_integration.delta_time_,
                     this->triangle_.calculateElementDeltaTime(mesh.triangle_, thermal_model,
                                                               time_integration.courant_friedrichs_lewy_number_));
      }
      if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
        time_integration.delta_time_ =
            std::min(time_integration.delta_time_,
                     this->quadrangle_.calculateElementDeltaTime(mesh.quadrangle_, thermal_model,
                                                                 time_integration.courant_friedrichs_lewy_number_));
      }
    }
  } else {
    this->error_finout_.seekg(0, std::ios::beg);
    std::string line;
    for (int i = 0; i < 3; i++) {
      std::getline(this->error_finout_, line);
    }
    std::stringstream ss(line);
    ss.ignore(2) >> time_integration.delta_time_;
  }
}

template <typename ElementTrait, typename SimulationControl>
inline void ElementSolverBase<ElementTrait, SimulationControl>::updateElementBasisFunctionCoefficient(
    const int step, const ElementMesh<ElementTrait>& element_mesh,
    const TimeIntegrationData<SimulationControl::kTimeIntegration>& time_integration) {
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto) \
    shared(Eigen::Dynamic, step, element_mesh, time_integration)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < this->number_; i++) {
    // NOTE: Here we split the calculation to trigger eigen's noalias to avoid intermediate variables.
    this->element_(i).variable_basis_function_coefficient_(1) *=
        time_integration.kStepCoefficients[static_cast<Usize>(step)][1];
    this->element_(i).variable_basis_function_coefficient_(1).noalias() +=
        time_integration.kStepCoefficients[static_cast<Usize>(step)][0] *
        this->element_(i).variable_basis_function_coefficient_(0);
    this->element_(i).variable_basis_function_coefficient_(1).noalias() +=
        time_integration.kStepCoefficients[static_cast<Usize>(step)][2] * time_integration.delta_time_ *
        this->element_(i).variable_residual_ * element_mesh.element_(i).local_mass_matrix_inverse_;
  }
}

template <typename ElementTrait, typename SimulationControl>
inline void ElementSolver<ElementTrait, SimulationControl, EquationModelEnum::NavierStokes>::
    updateElementGardientBasisFunctionCoefficient(const ElementMesh<ElementTrait>& element_mesh) {
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto) shared(Eigen::Dynamic, element_mesh)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < this->number_; i++) {
    this->element_(i).variable_gradient_volume_basis_function_coefficient_.noalias() =
        this->element_(i).variable_gradient_volume_residual_ * element_mesh.element_(i).local_mass_matrix_inverse_;
    this->element_(i).variable_gradient_basis_function_coefficient_.noalias() =
        this->element_(i).variable_gradient_volume_basis_function_coefficient_;
    for (Isize j = 0; j < ElementTrait::kAdjacencyNumber; j++) {
      this->element_(i).variable_gradient_interface_basis_function_coefficient_(j).noalias() =
          this->element_(i).variable_gradient_interface_residual_(j) *
          element_mesh.element_(i).local_mass_matrix_inverse_;
      this->element_(i).variable_gradient_basis_function_coefficient_.noalias() +=
          this->element_(i).variable_gradient_interface_basis_function_coefficient_(j);
    }
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::updateBasisFunctionCoefficient(
    int step, const Mesh<SimulationControl>& mesh,
    const TimeIntegrationData<SimulationControl::kTimeIntegration>& time_integration) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.updateElementBasisFunctionCoefficient(step, mesh.line_, time_integration);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.updateElementBasisFunctionCoefficient(step, mesh.triangle_, time_integration);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.updateElementBasisFunctionCoefficient(step, mesh.quadrangle_, time_integration);
    }
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::updateGardientBasisFunctionCoefficient(const Mesh<SimulationControl>& mesh) {
  if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.updateElementGardientBasisFunctionCoefficient(mesh.triangle_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.updateElementGardientBasisFunctionCoefficient(mesh.quadrangle_);
    }
  }
}

template <typename ElementTrait, typename SimulationControl>
inline void ElementSolverBase<ElementTrait, SimulationControl>::calculateElementRelativeError(
    const ElementMesh<ElementTrait>& element_mesh,
    Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& relative_error) {
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> local_error =
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>::Zero();
#ifndef SUBROSA_DG_DEVELOP
#pragma omp declare reduction(+ : Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> : omp_out += \
                                  omp_in) initializer(omp_priv = decltype(omp_orig)::Zero())
#pragma omp parallel for reduction(+ : local_error) default(none) schedule(nonmonotonic : auto) \
    shared(Eigen::Dynamic, element_mesh)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < this->number_; i++) {
    local_error.array() +=
        ((this->element_(i).variable_residual_ * element_mesh.basis_function_.value_.transpose()).array() /
         ((this->element_(i).variable_basis_function_coefficient_(1) * element_mesh.basis_function_.value_.transpose())
              .array() +
          1e-8))
            .square()
            .rowwise()
            .mean();
  }
  relative_error.array() += local_error.array();
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::calculateRelativeError(const Mesh<SimulationControl>& mesh) {
  this->relative_error_.setZero();
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.calculateElementRelativeError(mesh.line_, this->relative_error_);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.calculateElementRelativeError(mesh.triangle_, this->relative_error_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.calculateElementRelativeError(mesh.quadrangle_, this->relative_error_);
    }
  }
  this->relative_error_ = (this->relative_error_ / static_cast<Real>(mesh.element_number_)).array().sqrt();
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::stepSolver(
    const int step, const Mesh<SimulationControl>& mesh, const ThermalModel<SimulationControl>& thermal_model,
    const std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition,
    const TimeIntegrationData<SimulationControl::kTimeIntegration>& time_integration) {
  if constexpr (SimulationControl::kEquationModel == EquationModelEnum::NavierStokes) {
    this->calculateGardientQuadrature(mesh);
    this->calculateAdjacencyGardientQuadrature(mesh, thermal_model, boundary_condition);
    this->calculateGardientResidual(mesh);
    this->updateGardientBasisFunctionCoefficient(mesh);
  }
  this->calculateQuadrature(mesh, thermal_model);
  this->calculateAdjacencyQuadrature(mesh, thermal_model, boundary_condition);
  this->calculateResidual(mesh);
  this->updateBasisFunctionCoefficient(step, mesh, time_integration);
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_TIME_INTEGRATION_HPP_
