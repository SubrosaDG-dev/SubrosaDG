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
#include <memory>
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

struct TimeIntegrationDataBase {
  bool is_steady_;
  int iteration_number_;
  Real courant_friedrichs_lewy_number_;
  Real tolerance_;
};

template <TimeIntegrationEnum TimeIntegrationType>
struct TimeIntegrationData : TimeIntegrationDataBase {};

template <>
struct TimeIntegrationData<TimeIntegrationEnum::TestInitialization> : TimeIntegrationDataBase {
  inline static constexpr int kStep = 0;
  inline static constexpr std::array<std::array<Real, 3>, kStep> kStepCoefficients{};
};

template <>
struct TimeIntegrationData<TimeIntegrationEnum::ForwardEuler> : TimeIntegrationDataBase {
  inline static constexpr int kStep = 1;
  inline static constexpr std::array<std::array<Real, 3>, kStep> kStepCoefficients{{{1.0, 0.0, 1.0}}};
};

template <>
struct TimeIntegrationData<TimeIntegrationEnum::HeunRK2> : TimeIntegrationDataBase {
  inline static constexpr int kStep = 2;
  inline static constexpr std::array<std::array<Real, 3>, kStep> kStepCoefficients{{{1.0, 0.0, 1.0}, {0.5, 0.0, 0.5}}};
};

template <>
struct TimeIntegrationData<TimeIntegrationEnum::SSPRK3> : TimeIntegrationDataBase {
  inline static constexpr int kStep = 3;
  inline static constexpr std::array<std::array<Real, 3>, kStep> kStepCoefficients{
      {{1.0, 0.0, 1.0}, {3.0 / 4.0, 1.0 / 4.0, 1.0 / 4.0}, {1.0 / 3.0, 2.0 / 3.0, 2.0 / 3.0}}};
};

template <typename SimulationControl>
struct TimeIntegration : TimeIntegrationData<SimulationControl::kTimeIntegration> {};

template <typename ElementTrait, typename SimulationControl, EquationModelEnum EquationModelType>
inline void ElementSolver<ElementTrait, SimulationControl, EquationModelType>::copyElementBasisFunctionCoefficient() {
#if defined(SUBROSA_DG_WITH_OPENMP) && !defined(SUBROSA_DG_DEVELOP)
#pragma omp parallel for default(none) schedule(nonmonotonic : auto)
#endif  // SUBROSA_DG_WITH_OPENMP && !SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < this->number_; i++) {
    this->element_(i).conserved_variable_basis_function_coefficient_(0).noalias() =
        this->element_(i).conserved_variable_basis_function_coefficient_(1);
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

template <typename ElementTrait, typename SimulationControl, EquationModelEnum EquationModelType>
inline void ElementSolver<ElementTrait, SimulationControl, EquationModelType>::calculateElementDeltaTime(
    const ElementMesh<ElementTrait>& element_mesh, const ThermalModel<SimulationControl>& thermal_model,
    const TimeIntegrationData<SimulationControl::kTimeIntegration>& time_integration) {
#if defined(SUBROSA_DG_WITH_OPENMP) && !defined(SUBROSA_DG_DEVELOP)
#pragma omp parallel for default(none) schedule(nonmonotonic : auto) \
    shared(element_mesh, thermal_model, time_integration)
#endif  // SUBROSA_DG_WITH_OPENMP && !SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < element_mesh.number_; i++) {
    Eigen::Vector<Real, ElementTrait::kQuadratureNumber> delta_time;
    for (Isize j = 0; j < ElementTrait::kQuadratureNumber; j++) {
      Variable<SimulationControl> variable;
      variable.template getFromSelf<ElementTrait>(i, j, element_mesh, thermal_model, *this);
      const Real spectral_radius =
          std::sqrt(variable.template get<ComputationalVariableEnum::VelocitySquareSummation>()) +
          thermal_model.calculateSoundSpeedFromInternalEnergy(
              variable.template get<ComputationalVariableEnum::InternalEnergy>());
      delta_time(j) =
          time_integration.courant_friedrichs_lewy_number_ * element_mesh.element_(i).size_ / spectral_radius;
    }
    this->delta_time_(i) = delta_time.minCoeff();
  }
}

template <typename SimulationControl>
inline Real Solver<SimulationControl>::setDeltaTime(
    const TimeIntegrationData<SimulationControl::kTimeIntegration>& time_integration) {
  Real delta_time = kRealMax;
  if constexpr (SimulationControl::kDimension == 1) {
    delta_time = std::min(delta_time, this->line_.delta_time_.minCoeff());
    if (!time_integration.is_steady_) {
      this->line_.delta_time_.setConstant(delta_time);
    }
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      delta_time = std::min(delta_time, this->triangle_.delta_time_.minCoeff());
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      delta_time = std::min(delta_time, this->quadrangle_.delta_time_.minCoeff());
    }
    if (!time_integration.is_steady_) {
      if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
        this->triangle_.delta_time_.setConstant(delta_time);
      }
      if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
        this->quadrangle_.delta_time_.setConstant(delta_time);
      }
    }
  }
  return delta_time;
}

template <typename SimulationControl>
inline Real Solver<SimulationControl>::calculateDeltaTime(
    const Mesh<SimulationControl>& mesh, const ThermalModel<SimulationControl>& thermal_model,
    const TimeIntegrationData<SimulationControl::kTimeIntegration>& time_integration) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.calculateElementDeltaTime(mesh.line_, thermal_model, time_integration);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.calculateElementDeltaTime(mesh.triangle_, thermal_model, time_integration);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.calculateElementDeltaTime(mesh.quadrangle_, thermal_model, time_integration);
    }
  }
  return this->setDeltaTime(time_integration);
}

template <typename ElementTrait, typename SimulationControl, EquationModelEnum EquationModelType>
inline void ElementSolver<ElementTrait, SimulationControl, EquationModelType>::updateElementBasisFunctionCoefficient(
    const int step, const ElementMesh<ElementTrait>& element_mesh,
    const TimeIntegrationData<SimulationControl::kTimeIntegration>& time_integration) {
#if defined(SUBROSA_DG_WITH_OPENMP) && !defined(SUBROSA_DG_DEVELOP)
#pragma omp parallel for default(none) schedule(nonmonotonic : auto) \
    shared(Eigen::Dynamic, step, element_mesh, time_integration)
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

template <typename ElementTrait, typename SimulationControl, EquationModelEnum EquationModelType>
inline void ElementSolver<ElementTrait, SimulationControl, EquationModelType>::calculateElementRelativeError(
    const ElementMesh<ElementTrait>& element_mesh,
    Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& relative_error) {
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> local_error =
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>::Zero();
#if defined(SUBROSA_DG_WITH_OPENMP) && !defined(SUBROSA_DG_DEVELOP)
#pragma omp parallel for reduction(+ : local_error) default(none) schedule(nonmonotonic : auto) \
    shared(Eigen::Dynamic, element_mesh)
#endif  // SUBROSA_DG_WITH_OPENMP && !SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < this->number_; i++) {
    local_error.array() += ((this->element_(i).residual_ * element_mesh.basis_function_.value_.transpose()).array() /
                            ((this->element_(i).conserved_variable_basis_function_coefficient_(1) *
                              element_mesh.basis_function_.value_.transpose())
                                 .array()))
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
  this->calculateGaussianQuadrature(mesh, thermal_model);
  this->calculateAdjacencyGaussianQuadrature(mesh, thermal_model, boundary_condition);
  this->calculateResidual(mesh);
  this->updateBasisFunctionCoefficient(step, mesh, time_integration);
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_TIME_INTEGRATION_HPP_
