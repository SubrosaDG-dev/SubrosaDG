/**
 * @file TimeIntegration.cpp
 * @brief The header file of TimeIntegration.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2025 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_TIME_INTEGRATION_CPP_
#define SUBROSA_DG_TIME_INTEGRATION_CPP_

#include <Eigen/Core>
#include <array>
#include <sstream>
#include <string>

#include "Mesh/ReadControl.cpp"
#include "Solver/BoundaryCondition.cpp"
#include "Solver/PhysicalModel.cpp"
#include "Solver/SolveControl.cpp"
#include "Solver/SourceTerm.cpp"
#include "Solver/VariableConvertor.cpp"
#include "Utils/BasicDataType.cpp"
#include "Utils/Concept.cpp"
#include "Utils/Constant.cpp"
#include "Utils/Enum.cpp"

namespace SubrosaDG {

struct TimeIntegrationBase {
  int iteration_start_{0};
  int iteration_end_;
  int iteration_{0};
  Real courant_friedrichs_lewy_number_;
  Real delta_time_{0.0};
};

template <TimeIntegrationEnum TimeIntegrationType>
struct TimeIntegrationData;

template <>
struct TimeIntegrationData<TimeIntegrationEnum::ForwardEuler> : TimeIntegrationBase {
  inline static constexpr int kStep = 1;
  inline static constexpr std::array<std::array<Real, 3>, kStep> kStepCoefficients{{{1.0_r, 0.0_r, 1.0_r}}};
};

template <>
struct TimeIntegrationData<TimeIntegrationEnum::HeunRK2> : TimeIntegrationBase {
  inline static constexpr int kStep = 2;
  inline static constexpr std::array<std::array<Real, 3>, kStep> kStepCoefficients{
      {{1.0_r, 0.0_r, 1.0_r}, {0.5_r, 0.5_r, 0.5_r}}};
};

template <>
struct TimeIntegrationData<TimeIntegrationEnum::SSPRK3> : TimeIntegrationBase {
  inline static constexpr int kStep = 3;
  inline static constexpr std::array<std::array<Real, 3>, kStep> kStepCoefficients{
      {{1.0_r, 0.0_r, 1.0_r},
       {3.0_r / 4.0_r, 1.0_r / 4.0_r, 1.0_r / 4.0_r},
       {1.0_r / 3.0_r, 2.0_r / 3.0_r, 2.0_r / 3.0_r}}};
};

template <typename SimulationControl>
struct TimeIntegration : TimeIntegrationData<SimulationControl::kTimeIntegration> {};

template <typename ElementTrait, typename SimulationControl>
inline void ElementSolver<ElementTrait, SimulationControl>::copyElementBasisFunctionCoefficient() {
  tbb::parallel_for(tbb::blocked_range<Isize>(0, this->number_), [&](const tbb::blocked_range<Isize>& range) {
    for (Isize i = range.begin(); i != range.end(); i++) {
      this->element_(i).variable_basis_function_coefficient_last_.noalias() =
          this->element_(i).variable_basis_function_coefficient_;
    }
  });
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
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.copyElementBasisFunctionCoefficient();
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.copyElementBasisFunctionCoefficient();
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.copyElementBasisFunctionCoefficient();
    }
  }
}

template <typename ElementTrait, typename SimulationControl>
inline void ElementSolver<ElementTrait, SimulationControl>::calculateElementDeltaTime(
    const ElementMesh<ElementTrait>& element_mesh, const PhysicalModel<SimulationControl>& physical_model,
    const Real courant_friedrichs_lewy_number, Real& delta_time) {
  tbb::combinable<Real> min_delta_time_combinable(kRealMax);
  tbb::parallel_for(tbb::blocked_range<Isize>(0, this->number_), [&](const tbb::blocked_range<Isize>& range) {
    for (Isize i = range.begin(); i != range.end(); i++) {
      ElementVariable<ElementTrait, SimulationControl> quadrature_node_variable;
      Eigen::Vector<Real, ElementTrait::kQuadratureNumber> local_delta_time;
      quadrature_node_variable.get(element_mesh, *this, i);
      quadrature_node_variable.calculateComputationalFromConserved(physical_model);
      for (Isize j = 0; j < ElementTrait::kQuadratureNumber; j++) {
        const Real sound_speed = physical_model.calculateSoundSpeedFromDensityPressure(
            quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(j),
            quadrature_node_variable.template getScalar<ComputationalVariableEnum::Pressure>(j));
        const Real spectral_radius =
            std::sqrt(quadrature_node_variable.template getScalar<ComputationalVariableEnum::VelocitySquaredNorm>(j)) +
            sound_speed;
        // NOTE: https://arxiv.org/pdf/2008.12044
        local_delta_time(j) = courant_friedrichs_lewy_number * element_mesh.element_(i).minimum_edge_ /
                              (spectral_radius * (SimulationControl::kPolynomialOrder + 1.0_r) *
                               (SimulationControl::kPolynomialOrder + 1.0_r));
      }
      min_delta_time_combinable.local() = std::min(min_delta_time_combinable.local(), local_delta_time.minCoeff());
    }
  });
  delta_time = min_delta_time_combinable.combine([](const Real a, const Real b) { return std::ranges::min(a, b); });
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::calculateDeltaTime(const Mesh<SimulationControl>& mesh,
                                                          const PhysicalModel<SimulationControl>& physical_model,
                                                          TimeIntegration<SimulationControl>& time_integration) {
  time_integration.delta_time_ = kRealMax;
  if constexpr (SimulationControl::kInitialCondition == InitialConditionEnum::LastStep) {
    this->error_finout_.seekg(0, std::ios::beg);
    std::string line;
    for (int i = 0; i < 3; i++) {
      std::getline(this->error_finout_, line);
    }
    std::stringstream ss(line);
    ss.ignore(2) >> time_integration.delta_time_;
  } else {
    if constexpr (SimulationControl::kDimension == 1) {
      this->line_.calculateElementDeltaTime(
          mesh.line_, physical_model, time_integration.courant_friedrichs_lewy_number_, time_integration.delta_time_);
    } else if constexpr (SimulationControl::kDimension == 2) {
      if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
        this->triangle_.calculateElementDeltaTime(mesh.triangle_, physical_model,
                                                  time_integration.courant_friedrichs_lewy_number_,
                                                  time_integration.delta_time_);
      }
      if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
        this->quadrangle_.calculateElementDeltaTime(mesh.quadrangle_, physical_model,
                                                    time_integration.courant_friedrichs_lewy_number_,
                                                    time_integration.delta_time_);
      }
    } else if constexpr (SimulationControl::kDimension == 3) {
      if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
        this->tetrahedron_.calculateElementDeltaTime(mesh.tetrahedron_, physical_model,
                                                     time_integration.courant_friedrichs_lewy_number_,
                                                     time_integration.delta_time_);
      }
      if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
        this->pyramid_.calculateElementDeltaTime(mesh.pyramid_, physical_model,
                                                 time_integration.courant_friedrichs_lewy_number_,
                                                 time_integration.delta_time_);
      }
      if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
        this->hexahedron_.calculateElementDeltaTime(mesh.hexahedron_, physical_model,
                                                    time_integration.courant_friedrichs_lewy_number_,
                                                    time_integration.delta_time_);
      }
    }
  }
}

template <typename ElementTrait, typename SimulationControl>
inline void ElementSolver<ElementTrait, SimulationControl>::updateElementBasisFunctionCoefficient(
    const int rk_step, const ElementMesh<ElementTrait>& element_mesh,
    const TimeIntegration<SimulationControl>& time_integration) {
  tbb::parallel_for(tbb::blocked_range<Isize>(0, this->number_), [&](const tbb::blocked_range<Isize>& range) {
    for (Isize i = range.begin(); i != range.end(); i++) {
      // NOTE: Here we split the calculation to trigger eigen's noalias to avoid intermediate variables.
      this->element_(i).variable_basis_function_coefficient_ *=
          time_integration.kStepCoefficients[static_cast<Usize>(rk_step)][1];
      this->element_(i).variable_basis_function_coefficient_.noalias() +=
          time_integration.kStepCoefficients[static_cast<Usize>(rk_step)][0] *
          this->element_(i).variable_basis_function_coefficient_last_;
      this->element_(i).variable_basis_function_coefficient_.noalias() +=
          time_integration.kStepCoefficients[static_cast<Usize>(rk_step)][2] * time_integration.delta_time_ *
          this->element_(i).variable_residual_ * element_mesh.element_(i).local_mass_matrix_inverse_;
    }
  });
}

template <typename ElementTrait, typename SimulationControl>
inline void ElementSolver<ElementTrait, SimulationControl>::updateElementGardientBasisFunctionCoefficient(
    const ElementMesh<ElementTrait>& element_mesh) {
  tbb::parallel_for(tbb::blocked_range<Isize>(0, this->number_), [&](const tbb::blocked_range<Isize>& range) {
    for (Isize i = range.begin(); i != range.end(); i++) {
      this->element_(i).variable_volume_gradient_basis_function_coefficient_.noalias() =
          this->element_(i).variable_volume_gradient_residual_ * element_mesh.element_(i).local_mass_matrix_inverse_;
      if constexpr (IsNS<SimulationControl::kEquationModel>) {
        this->element_(i).variable_gradient_basis_function_coefficient_.noalias() =
            this->element_(i).variable_volume_gradient_basis_function_coefficient_;
        if constexpr (SimulationControl::kViscousFlux == ViscousFluxEnum::BR1) {
          this->element_(i).variable_interface_gradient_basis_function_coefficient_.noalias() =
              this->element_(i).variable_interface_gradient_residual_ *
              element_mesh.element_(i).local_mass_matrix_inverse_;
          this->element_(i).variable_gradient_basis_function_coefficient_.noalias() +=
              this->element_(i).variable_interface_gradient_basis_function_coefficient_;
        } else if constexpr (SimulationControl::kViscousFlux == ViscousFluxEnum::BR2) {
          for (Isize j = 0; j < ElementTrait::kAdjacencyNumber; j++) {
            this->element_(i).variable_interface_gradient_basis_function_coefficient_(j).noalias() =
                this->element_(i).variable_interface_gradient_residual_(j) *
                element_mesh.element_(i).local_mass_matrix_inverse_;
            this->element_(i).variable_gradient_basis_function_coefficient_.noalias() +=
                this->element_(i).variable_interface_gradient_basis_function_coefficient_(j);
          }
        }
      }
    }
  });
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::updateBasisFunctionCoefficient(
    int rk_step, const Mesh<SimulationControl>& mesh, const TimeIntegration<SimulationControl>& time_integration) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.updateElementBasisFunctionCoefficient(rk_step, mesh.line_, time_integration);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.updateElementBasisFunctionCoefficient(rk_step, mesh.triangle_, time_integration);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.updateElementBasisFunctionCoefficient(rk_step, mesh.quadrangle_, time_integration);
    }
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.updateElementBasisFunctionCoefficient(rk_step, mesh.tetrahedron_, time_integration);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.updateElementBasisFunctionCoefficient(rk_step, mesh.pyramid_, time_integration);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.updateElementBasisFunctionCoefficient(rk_step, mesh.hexahedron_, time_integration);
    }
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::updateGardientBasisFunctionCoefficient(const Mesh<SimulationControl>& mesh) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.updateElementGardientBasisFunctionCoefficient(mesh.line_);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.updateElementGardientBasisFunctionCoefficient(mesh.triangle_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.updateElementGardientBasisFunctionCoefficient(mesh.quadrangle_);
    }
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.updateElementGardientBasisFunctionCoefficient(mesh.tetrahedron_);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.updateElementGardientBasisFunctionCoefficient(mesh.pyramid_);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.updateElementGardientBasisFunctionCoefficient(mesh.hexahedron_);
    }
  }
}

template <typename ElementTrait, typename SimulationControl>
inline void ElementSolver<ElementTrait, SimulationControl>::calculateElementRelativeError(
    const ElementMesh<ElementTrait>& element_mesh,
    Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& relative_error) {
  tbb::combinable<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>> relative_error_combinable(
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>::Zero());
  tbb::parallel_for(tbb::blocked_range<Isize>(0, this->number_), [&](const tbb::blocked_range<Isize>& range) {
    for (Isize i = range.begin(); i != range.end(); i++) {
      relative_error_combinable.local().array() +=
          (this->element_(i).variable_residual_ * element_mesh.basis_function_.modal_value_.transpose())
              .array()
              .abs()
              .rowwise()
              .mean();
    }
  });
  relative_error = relative_error_combinable.combine(
      [](const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& a,
         const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& b) { return a + b; });
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
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.calculateElementRelativeError(mesh.tetrahedron_, this->relative_error_);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.calculateElementRelativeError(mesh.pyramid_, this->relative_error_);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.calculateElementRelativeError(mesh.hexahedron_, this->relative_error_);
    }
  }
  this->relative_error_ = this->relative_error_ / static_cast<Real>(mesh.element_number_);
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::stepSolver(const Mesh<SimulationControl>& mesh,
                                                  [[maybe_unused]] const SourceTerm<SimulationControl>& source_term,
                                                  const PhysicalModel<SimulationControl>& physical_model,
                                                  const BoundaryCondition<SimulationControl>& boundary_condition,
                                                  const TimeIntegration<SimulationControl>& time_integration) {
  this->copyBasisFunctionCoefficient();
  if constexpr (SimulationControl::kBoundaryTime == BoundaryTimeEnum::TimeVarying) {
    this->updateBoundaryVariable(mesh, physical_model, boundary_condition, time_integration);
  }
  if constexpr (SimulationControl::kShockCapturing == ShockCapturingEnum::ArtificialViscosity) {
    this->calculateArtificialViscosity(mesh);
  }
  for (int i = 0; i < time_integration.kStep; i++) {
    this->calculateGardientQuadrature(mesh);
    this->calculateAdjacencyGardientQuadrature(mesh, physical_model, boundary_condition);
    this->calculateGardientResidual(mesh);
    this->updateGardientBasisFunctionCoefficient(mesh);
    this->calculateQuadrature(mesh, source_term, physical_model);
    this->calculateAdjacencyQuadrature(mesh, physical_model, boundary_condition);
    this->calculateResidual(mesh);
    this->updateBasisFunctionCoefficient(i, mesh, time_integration);
  }
  this->calculateRelativeError(mesh);
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_TIME_INTEGRATION_CPP_
