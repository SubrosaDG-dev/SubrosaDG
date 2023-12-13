/**
 * @file TimeIntegration.hpp
 * @brief The header file of TimeIntegration.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_TIME_INTEGRATION_HPP_
#define SUBROSA_DG_TIME_INTEGRATION_HPP_

#include <Eigen/Core>
#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <string>
#include <unordered_map>

#include "Mesh/ReadControl.hpp"
#include "Solver/BoundaryCondition.hpp"
#include "Solver/SimulationControl.hpp"
#include "Solver/SolveControl.hpp"
#include "Solver/ThermalModel.hpp"
#include "Solver/VariableConvertor.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Concept.hpp"
#include "Utils/Constant.hpp"
#include "Utils/Enum.hpp"

namespace SubrosaDG {

struct TimeIntegrationBase {
  bool is_steady_;
  int iteration_number_;
  Real courant_friedrichs_lewy_number_;
  Real tolerance_;
};

template <TimeIntegration TimeIntegrationType>
struct TimeIntegrationData : TimeIntegrationBase {};

template <>
struct TimeIntegrationData<TimeIntegration::ForwardEuler> : TimeIntegrationBase {
  inline static constexpr int kStep = 1;
  inline static constexpr std::array<std::array<Real, 3>, kStep> kStepCoefficients{{{1.0, 0.0, 1.0}}};
};

template <>
struct TimeIntegrationData<TimeIntegration::HeunRK2> : TimeIntegrationBase {
  inline static constexpr int kStep = 2;
  inline static constexpr std::array<std::array<Real, 3>, kStep> kStepCoefficients{{{1.0, 0.0, 1.0}, {0.5, 0.0, 0.5}}};
};

template <>
struct TimeIntegrationData<TimeIntegration::SSPRK3> : TimeIntegrationBase {
  inline static constexpr int kStep = 3;
  inline static constexpr std::array<std::array<Real, 3>, kStep> kStepCoefficients{
      {{1.0, 0.0, 1.0}, {3.0 / 4.0, 1.0 / 4.0, 1.0 / 4.0}, {1.0 / 3.0, 2.0 / 3.0, 2.0 / 3.0}}};
};

template <typename ElementTrait, typename SimulationControl, EquationModel EquationModelType>
inline void ElementSolver<ElementTrait, SimulationControl, EquationModelType>::copyElementBasisFunctionCoefficient() {
  for (Isize i = 0; i < this->number_; i++) {
    this->element_(i).conserved_variable_basis_function_coefficient_(0).noalias() =
        this->element_(i).conserved_variable_basis_function_coefficient_(1);
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl, 2>::copyBasisFunctionCoefficient() {
  if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
    this->triangle_.copyElementBasisFunctionCoefficient();
  }
  if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
    this->quadrangle_.copyElementBasisFunctionCoefficient();
  }
}

template <typename ElementTrait, typename SimulationControl, EquationModel EquationModelType>
inline void ElementSolver<ElementTrait, SimulationControl, EquationModelType>::calculateElementDeltaTime(
    const ElementMesh<ElementTrait>& element_mesh,
    const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
    const TimeIntegrationData<SimulationControl::kTimeIntegration>& time_integration) {
#if defined(SUBROSA_DG_WITH_OPENMP) && !defined(SUBROSA_DG_DEVELOP)
#pragma omp parallel for default(none) schedule(auto) shared(element_mesh, thermal_model, time_integration)
#endif  // SUBROSA_DG_WITH_OPENMP && !SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < element_mesh.number_; i++) {
    Eigen::Vector<Real, ElementTrait::kQuadratureNumber> delta_time;
    for (Isize j = 0; j < ElementTrait::kQuadratureNumber; j++) {
      Variable<SimulationControl> variable;
      variable.template getFromSelf<ElementTrait>(i, j, element_mesh, thermal_model, *this);
      const Real velocity_x = variable.template get<ComputationalVariable::VelocityX>();
      const Real velocity_y = variable.template get<ComputationalVariable::VelocityY>();
      const Real sound_speed = thermal_model.calculateSoundSpeedFromInternalEnergy(
          variable.template get<ComputationalVariable::InternalEnergy>());
      const Real lambda_x = std::fabs(velocity_x) *
                            (1 + sound_speed / (velocity_x * velocity_x + velocity_y * velocity_y)) *
                            element_mesh.element_(i).projection_measure_.x();
      const Real lambda_y = std::fabs(velocity_y) *
                            (1 + sound_speed / (velocity_x * velocity_x + velocity_y * velocity_y)) *
                            element_mesh.element_(i).projection_measure_.y();
      delta_time(j) = time_integration.courant_friedrichs_lewy_number_ *
                      element_mesh.element_(i).jacobian_determinant_(j) *
                      getElementMeasure<ElementTrait::kElementType>() / (lambda_x + lambda_y);
    }
    this->delta_time_(i) = delta_time.minCoeff();
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl, 2>::setDeltaTime() {
  Real delta_time = kRealMax;
  if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
    delta_time = std::min(delta_time, this->triangle_.delta_time_.minCoeff());
  }
  if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
    delta_time = std::min(delta_time, this->quadrangle_.delta_time_.minCoeff());
  }
  if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
    this->triangle_.delta_time_.setConstant(delta_time);
  }
  if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
    this->quadrangle_.delta_time_.setConstant(delta_time);
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl, 2>::calculateDeltaTime(
    const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
    const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
    const TimeIntegrationData<SimulationControl::kTimeIntegration>& time_integration) {
  if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
    this->triangle_.calculateElementDeltaTime(mesh.triangle_, thermal_model, time_integration);
  }
  if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
    this->quadrangle_.calculateElementDeltaTime(mesh.quadrangle_, thermal_model, time_integration);
  }
  if (!time_integration.is_steady_) {
    this->setDeltaTime();
  }
}

template <typename ElementTrait, typename SimulationControl, EquationModel EquationModelType>
inline void ElementSolver<ElementTrait, SimulationControl, EquationModelType>::updateElementBasisFunctionCoefficient(
    const int step, const ElementMesh<ElementTrait>& element_mesh,
    const TimeIntegrationData<SimulationControl::kTimeIntegration>& time_integration) {
#if defined(SUBROSA_DG_WITH_OPENMP) && !defined(SUBROSA_DG_DEVELOP)
#pragma omp parallel for default(none) schedule(auto) shared(Eigen::Dynamic, step, element_mesh, time_integration)
#endif  // SUBROSA_DG_WITH_OPENMP && !SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < this->number_; i++) {
    this->element_(i).conserved_variable_basis_function_coefficient_(1) =
        time_integration.kStepCoefficients[static_cast<Usize>(step)][0] *
            this->element_(i).conserved_variable_basis_function_coefficient_(0) +
        time_integration.kStepCoefficients[static_cast<Usize>(step)][1] *
            this->element_(i).conserved_variable_basis_function_coefficient_(1) +
        time_integration.kStepCoefficients[static_cast<Usize>(step)][2] * this->delta_time_(i) *
            this->element_(i).residual_ * element_mesh.element_(i).local_mass_matrix_inverse_;
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl, 2>::updateBasisFunctionCoefficient(
    int step, const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
    const TimeIntegrationData<SimulationControl::kTimeIntegration>& time_integration) {
  if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
    this->triangle_.updateElementBasisFunctionCoefficient(step, mesh.triangle_, time_integration);
  }
  if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
    this->quadrangle_.updateElementBasisFunctionCoefficient(step, mesh.quadrangle_, time_integration);
  }
}

template <typename ElementTrait, typename SimulationControl, EquationModel EquationModelType>
inline void ElementSolver<ElementTrait, SimulationControl, EquationModelType>::calculateElementRelativeError(
    const ElementMesh<ElementTrait>& element_mesh,
    Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& relative_error) {
  for (Isize i = 0; i < this->number_; i++) {
    relative_error.array() += ((this->element_(i).residual_ * element_mesh.basis_function_.value_.transpose()).array() /
                               ((this->element_(i).conserved_variable_basis_function_coefficient_(1) *
                                 element_mesh.basis_function_.value_.transpose())
                                    .array() +
                                1e-10))
                                  .square()
                                  .rowwise()
                                  .mean();
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl, 2>::calculateRelativeError(
    const Mesh<SimulationControl, SimulationControl::kDimension>& mesh) {
  this->relative_error_.setZero();
  if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
    this->triangle_.calculateElementRelativeError(mesh.triangle_, this->relative_error_);
  }
  if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
    this->quadrangle_.calculateElementRelativeError(mesh.quadrangle_, this->relative_error_);
  }
  this->relative_error_ = (this->relative_error_ / static_cast<Real>(mesh.element_number_)).array().sqrt();
}

template <typename SimulationControl>
inline void Solver<SimulationControl, 2>::stepSolver(
    const int step, const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
    const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
    const std::unordered_map<std::string, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>&
        boundary_condition,
    const TimeIntegrationData<SimulationControl::kTimeIntegration>& time_integration) {
  this->calculateGaussianQuadrature(mesh, thermal_model);
  this->calculateAdjacencyGaussianQuadrature(mesh, thermal_model, boundary_condition);
  this->calculateResidual(mesh);
  this->updateBasisFunctionCoefficient(step, mesh, time_integration);
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_TIME_INTEGRATION_HPP_
