/**
 * @file TimeIntegration.cpp
 * @brief The header file of TimeIntegration.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_TIME_INTEGRATION_CPP_
#define SUBROSA_DG_TIME_INTEGRATION_CPP_

#include <Eigen/Core>
#include <array>
#include <sstream>
#include <string>

#include "Mesh/ReadControl.cpp"
#include "Solver/PhysicalModel.cpp"
#include "Solver/SolveControl.cpp"
#include "Solver/SourceTerm.cpp"
#include "Solver/VariableConvertor.cpp"
#include "Utils/BasicDataType.cpp"
#include "Utils/Concept.cpp"
#include "Utils/Constant.cpp"
#include "Utils/Enum.cpp"

namespace SubrosaDG {
using sycl::plus;

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
  static constexpr int kStep = 1;
  static constexpr std::array<std::array<Real, 3>, kStep> kStepCoefficients{{{1.0_r, 0.0_r, 1.0_r}}};
};

template <>
struct TimeIntegrationData<TimeIntegrationEnum::HeunRK2> : TimeIntegrationBase {
  static constexpr int kStep = 2;
  static constexpr std::array<std::array<Real, 3>, kStep> kStepCoefficients{
      {{1.0_r, 0.0_r, 1.0_r}, {0.5_r, 0.5_r, 0.5_r}}};
};

template <>
struct TimeIntegrationData<TimeIntegrationEnum::SSPRK3> : TimeIntegrationBase {
  static constexpr int kStep = 3;
  static constexpr std::array<std::array<Real, 3>, kStep> kStepCoefficients{
      {{1.0_r, 0.0_r, 1.0_r},
       {3.0_r / 4.0_r, 1.0_r / 4.0_r, 1.0_r / 4.0_r},
       {1.0_r / 3.0_r, 2.0_r / 3.0_r, 2.0_r / 3.0_r}}};
};

template <typename SimulationControl>
struct TimeIntegration : TimeIntegrationData<SimulationControl::kTimeIntegration> {};

template <typename VolumeElementTrait, typename SimulationControl>
inline void VolumeElementSolver<VolumeElementTrait, SimulationControl>::copyVolumeElementBasisFunctionCoefficient() {
  tbb::parallel_for(tbb::blocked_range<Isize>(0, this->number_), [&](const tbb::blocked_range<Isize>& range) -> void {
    for (Isize i = range.begin(); i != range.end(); i++) {
      this->variable_basis_function_coefficient_last_(i).noalias() = this->variable_basis_function_coefficient_(i);
    }
  });
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::copyBasisFunctionCoefficient() {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.copyVolumeElementBasisFunctionCoefficient();
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.copyVolumeElementBasisFunctionCoefficient();
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.copyVolumeElementBasisFunctionCoefficient();
    }
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.copyVolumeElementBasisFunctionCoefficient();
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.copyVolumeElementBasisFunctionCoefficient();
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.copyVolumeElementBasisFunctionCoefficient();
    }
  }
}

template <typename VolumeElementTrait, typename SimulationControl>
inline void
VolumeElementSolverDevice<VolumeElementTrait, SimulationControl>::copyVolumeElementBasisFunctionCoefficient() {
  queue.submit([&](sycl::handler& cgh) -> void {
    cgh.parallel_for(getNdRange(this->number_), [=, this](sycl::nd_item<1> index) -> void {
      const auto i = static_cast<Isize>(index.get_global_id(0));
      if (i >= this->number_) {
        return;
      }
      const Device::View<
          Device::Matrix<Real, SimulationControl::kConservedVariableNumber, VolumeElementTrait::kBasisFunctionNumber>>
          variable_basis_function_coefficient = this->variable_basis_function_coefficient_.view(i, this->number_);
      Device::View<
          Device::Matrix<Real, SimulationControl::kConservedVariableNumber, VolumeElementTrait::kBasisFunctionNumber>>
          variable_basis_function_coefficient_last =
              this->variable_basis_function_coefficient_last_.view(i, this->number_);
      for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
        for (Isize n = 0; n < VolumeElementTrait::kBasisFunctionNumber; n++) {
          variable_basis_function_coefficient_last(m, n) = variable_basis_function_coefficient(m, n);
        }
      }
    });
  });
}

template <typename SimulationControl>
inline void SolverDevice<SimulationControl>::copyBasisFunctionCoefficient() {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.copyVolumeElementBasisFunctionCoefficient();
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.copyVolumeElementBasisFunctionCoefficient();
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.copyVolumeElementBasisFunctionCoefficient();
    }
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.copyVolumeElementBasisFunctionCoefficient();
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.copyVolumeElementBasisFunctionCoefficient();
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.copyVolumeElementBasisFunctionCoefficient();
    }
  }
  queue.wait();
}

template <typename VolumeElementTrait, typename SimulationControl>
inline void VolumeElementSolver<VolumeElementTrait, SimulationControl>::computeVolumeElementDeltaTime(
    const VolumeElementMesh<VolumeElementTrait>& volume_element_mesh, const Real courant_friedrichs_lewy_number,
    Real& delta_time) {
  tbb::combinable<Real> min_delta_time_combinable(kRealMax);
  tbb::parallel_for(tbb::blocked_range<Isize>(0, this->number_), [&](const tbb::blocked_range<Isize>& range) -> void {
    for (Isize i = range.begin(); i != range.end(); i++) {
      Eigen::Vector<Real, VolumeElementTrait::kQuadratureNumber> quadrature_node_delta_time;
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> quadrature_node_conserved_variable;
      Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber> quadrature_node_computational_variable;
      for (Isize j = 0; j < VolumeElementTrait::kQuadratureNumber; j++) {
        VolumeElementVariable<VolumeElementTrait, SimulationControl>::get(volume_element_mesh, *this,
                                                                          quadrature_node_conserved_variable, i, j);
        Variable<SimulationControl>::convertComputationalFromConserved(quadrature_node_conserved_variable,
                                                                       quadrature_node_computational_variable);
        const Real sound_speed =
            PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(
                Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
                    quadrature_node_computational_variable),
                Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
                    quadrature_node_computational_variable));
        const Real spectral_radius =
            Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
                quadrature_node_computational_variable)
                .norm() +
            sound_speed;
        // NOTE: https://arxiv.org/pdf/2008.12044
        quadrature_node_delta_time(j) = courant_friedrichs_lewy_number * volume_element_mesh.minimum_edge_(i) /
                                        (spectral_radius * (SimulationControl::kPolynomialOrder + 1.0_r) *
                                         (SimulationControl::kPolynomialOrder + 1.0_r));
      }
      min_delta_time_combinable.local() =
          std::min(min_delta_time_combinable.local(), quadrature_node_delta_time.minCoeff());
    }
  });
  delta_time = min_delta_time_combinable.combine([](const Real a, const Real b) { return std::ranges::min(a, b); });
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::computeDeltaTime(const Mesh<SimulationControl>& mesh,
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
    time_integration.delta_time_ /= this->error_output_interval_;
  } else {
    if constexpr (SimulationControl::kDimension == 1) {
      this->line_.computeVolumeElementDeltaTime(mesh.line_, time_integration.courant_friedrichs_lewy_number_,
                                                time_integration.delta_time_);
    } else if constexpr (SimulationControl::kDimension == 2) {
      if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
        this->triangle_.computeVolumeElementDeltaTime(mesh.triangle_, time_integration.courant_friedrichs_lewy_number_,
                                                      time_integration.delta_time_);
      }
      if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
        this->quadrangle_.computeVolumeElementDeltaTime(
            mesh.quadrangle_, time_integration.courant_friedrichs_lewy_number_, time_integration.delta_time_);
      }
    } else if constexpr (SimulationControl::kDimension == 3) {
      if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
        this->tetrahedron_.computeVolumeElementDeltaTime(
            mesh.tetrahedron_, time_integration.courant_friedrichs_lewy_number_, time_integration.delta_time_);
      }
      if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
        this->pyramid_.computeVolumeElementDeltaTime(mesh.pyramid_, time_integration.courant_friedrichs_lewy_number_,
                                                     time_integration.delta_time_);
      }
      if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
        this->hexahedron_.computeVolumeElementDeltaTime(
            mesh.hexahedron_, time_integration.courant_friedrichs_lewy_number_, time_integration.delta_time_);
      }
    }
  }
}

template <typename VolumeElementTrait, typename SimulationControl>
inline void VolumeElementSolver<VolumeElementTrait, SimulationControl>::updateVolumeElementBasisFunctionCoefficient(
    const VolumeElementMesh<VolumeElementTrait>& volume_element_mesh,
    const TimeIntegration<SimulationControl>& time_integration, const int rk_step) {
  tbb::parallel_for(tbb::blocked_range<Isize>(0, this->number_), [&](const tbb::blocked_range<Isize>& range) -> void {
    for (Isize i = range.begin(); i != range.end(); i++) {
      // NOTE: Here we split the calculation to trigger eigen's noalias to avoid intermediate variables.
      this->variable_basis_function_coefficient_(i) *=
          TimeIntegration<SimulationControl>::kStepCoefficients[static_cast<Usize>(rk_step)][1];
      this->variable_basis_function_coefficient_(i).noalias() +=
          TimeIntegration<SimulationControl>::kStepCoefficients[static_cast<Usize>(rk_step)][0] *
          this->variable_basis_function_coefficient_last_(i);
      this->variable_basis_function_coefficient_(i).noalias() +=
          TimeIntegration<SimulationControl>::kStepCoefficients[static_cast<Usize>(rk_step)][2] *
          time_integration.delta_time_ * this->variable_residual_(i) *
          volume_element_mesh.local_mass_matrix_inverse_(i);
    }
  });
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::updateBasisFunctionCoefficient(
    const Mesh<SimulationControl>& mesh, const TimeIntegration<SimulationControl>& time_integration,
    const int rk_step) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.updateVolumeElementBasisFunctionCoefficient(mesh.line_, time_integration, rk_step);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.updateVolumeElementBasisFunctionCoefficient(mesh.triangle_, time_integration, rk_step);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.updateVolumeElementBasisFunctionCoefficient(mesh.quadrangle_, time_integration, rk_step);
    }
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.updateVolumeElementBasisFunctionCoefficient(mesh.tetrahedron_, time_integration, rk_step);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.updateVolumeElementBasisFunctionCoefficient(mesh.pyramid_, time_integration, rk_step);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.updateVolumeElementBasisFunctionCoefficient(mesh.hexahedron_, time_integration, rk_step);
    }
  }
}

template <typename VolumeElementTrait, typename SimulationControl>
inline void
VolumeElementSolverDevice<VolumeElementTrait, SimulationControl>::updateVolumeElementBasisFunctionCoefficient(
    const VolumeElementMeshDevice<VolumeElementTrait>& volume_element_mesh,
    const TimeIntegration<SimulationControl>& time_integration, const int rk_step) {
  queue.submit([&](sycl::handler& cgh) -> void {
    cgh.parallel_for(getNdRange(this->number_), [=, this](sycl::nd_item<1> index) -> void {
      const auto i = static_cast<Isize>(index.get_global_id(0));
      if (i >= this->number_) {
        return;
      }
      const VolumeElementSolverDevice<VolumeElementTrait, SimulationControl>* self = this;
      Device::View<
          Device::Matrix<Real, SimulationControl::kConservedVariableNumber, VolumeElementTrait::kBasisFunctionNumber>>
          variable_basis_function_coefficient = this->variable_basis_function_coefficient_.view(i, this->number_);
      for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
        for (Isize n = 0; n < VolumeElementTrait::kBasisFunctionNumber; n++) {
          variable_basis_function_coefficient(m, n) *=
              TimeIntegration<SimulationControl>::kStepCoefficients[static_cast<Usize>(rk_step)][1];
        }
      }
      const Device::View<const Device::Matrix<Real, SimulationControl::kConservedVariableNumber,
                                              VolumeElementTrait::kBasisFunctionNumber>>
          variable_basis_function_coefficient_last =
              self->variable_basis_function_coefficient_last_.view(i, this->number_);
      for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
        for (Isize n = 0; n < VolumeElementTrait::kBasisFunctionNumber; n++) {
          variable_basis_function_coefficient(m, n) +=
              TimeIntegration<SimulationControl>::kStepCoefficients[static_cast<Usize>(rk_step)][0] *
              variable_basis_function_coefficient_last(m, n);
        }
      }
      const Device::View<const Device::Matrix<Real, SimulationControl::kConservedVariableNumber,
                                              VolumeElementTrait::kBasisFunctionNumber>>
          variable_residual = self->variable_residual_.view(i, this->number_);
      const Device::View<const Device::Matrix<Real, VolumeElementTrait::kBasisFunctionNumber,
                                              VolumeElementTrait::kBasisFunctionNumber>>
          local_mass_matrix_inverse = volume_element_mesh.local_mass_matrix_inverse_.view(i, this->number_);
      for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
        for (Isize n = 0; n < VolumeElementTrait::kBasisFunctionNumber; n++) {
          Real sum = 0.0_r;
          for (Isize k = 0; k < VolumeElementTrait::kBasisFunctionNumber; k++) {
            sum += variable_residual(m, k) * local_mass_matrix_inverse(k, n);
          }
          variable_basis_function_coefficient(m, n) +=
              TimeIntegration<SimulationControl>::kStepCoefficients[static_cast<Usize>(rk_step)][2] *
              time_integration.delta_time_ * sum;
        }
      }
    });
  });
}

template <typename SimulationControl>
inline void SolverDevice<SimulationControl>::updateBasisFunctionCoefficient(
    const MeshDevice<SimulationControl>& mesh, const TimeIntegration<SimulationControl>& time_integration,
    const int rk_step) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.updateVolumeElementBasisFunctionCoefficient(mesh.line_, time_integration, rk_step);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.updateVolumeElementBasisFunctionCoefficient(mesh.triangle_, time_integration, rk_step);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.updateVolumeElementBasisFunctionCoefficient(mesh.quadrangle_, time_integration, rk_step);
    }
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.updateVolumeElementBasisFunctionCoefficient(mesh.tetrahedron_, time_integration, rk_step);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.updateVolumeElementBasisFunctionCoefficient(mesh.pyramid_, time_integration, rk_step);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.updateVolumeElementBasisFunctionCoefficient(mesh.hexahedron_, time_integration, rk_step);
    }
  }
  queue.wait();
}

template <typename VolumeElementTrait, typename SimulationControl>
inline void
VolumeElementSolver<VolumeElementTrait, SimulationControl>::updateVolumeElementGradientBasisFunctionCoefficient(
    const VolumeElementMesh<VolumeElementTrait>& volume_element_mesh) {
  tbb::parallel_for(tbb::blocked_range<Isize>(0, this->number_), [&](const tbb::blocked_range<Isize>& range) -> void {
    for (Isize i = range.begin(); i != range.end(); i++) {
      this->variable_volume_gradient_basis_function_coefficient_(i).noalias() =
          this->variable_volume_gradient_residual_(i) * volume_element_mesh.local_mass_matrix_inverse_(i);
      if constexpr (IsNS<SimulationControl::kEquationModel>) {
        this->variable_gradient_basis_function_coefficient_(i).noalias() =
            this->variable_volume_gradient_basis_function_coefficient_(i);
        if constexpr (SimulationControl::kViscousFlux == ViscousFluxEnum::BR1) {
          this->variable_interface_gradient_basis_function_coefficient_(i).noalias() =
              this->variable_interface_gradient_residual_(i) * volume_element_mesh.local_mass_matrix_inverse_(i);
          this->variable_gradient_basis_function_coefficient_(i).noalias() +=
              this->variable_interface_gradient_basis_function_coefficient_(i);
        } else if constexpr (SimulationControl::kViscousFlux == ViscousFluxEnum::BR2) {
          for (Isize j = 0; j < VolumeElementTrait::kAdjacencyNumber; j++) {
            Eigen::Ref<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                                     VolumeElementTrait::kBasisFunctionNumber>>
                variable_interface_gradient_basis_function_coefficient =
                    this->variable_interface_gradient_basis_function_coefficient_(i)(
                        Eigen::placeholders::all, Eigen::seqN(j * VolumeElementTrait::kBasisFunctionNumber,
                                                              Eigen::fix<VolumeElementTrait::kBasisFunctionNumber>));
            variable_interface_gradient_basis_function_coefficient.noalias() =
                this->variable_interface_gradient_residual_(i)(
                    Eigen::placeholders::all, Eigen::seqN(j * VolumeElementTrait::kBasisFunctionNumber,
                                                          Eigen::fix<VolumeElementTrait::kBasisFunctionNumber>)) *
                volume_element_mesh.local_mass_matrix_inverse_(i);
            this->variable_gradient_basis_function_coefficient_(i).noalias() +=
                variable_interface_gradient_basis_function_coefficient;
          }
        }
      }
    }
  });
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::updateGradientBasisFunctionCoefficient(const Mesh<SimulationControl>& mesh) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.updateVolumeElementGradientBasisFunctionCoefficient(mesh.line_);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.updateVolumeElementGradientBasisFunctionCoefficient(mesh.triangle_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.updateVolumeElementGradientBasisFunctionCoefficient(mesh.quadrangle_);
    }
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.updateVolumeElementGradientBasisFunctionCoefficient(mesh.tetrahedron_);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.updateVolumeElementGradientBasisFunctionCoefficient(mesh.pyramid_);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.updateVolumeElementGradientBasisFunctionCoefficient(mesh.hexahedron_);
    }
  }
}

template <typename VolumeElementTrait, typename SimulationControl>
inline void
VolumeElementSolverDevice<VolumeElementTrait, SimulationControl>::updateVolumeElementGradientBasisFunctionCoefficient(
    const VolumeElementMeshDevice<VolumeElementTrait>& volume_element_mesh) {
  queue.submit([&](sycl::handler& cgh) -> void {
    cgh.parallel_for(getNdRange(this->number_), [=, this](sycl::nd_item<1> index) -> void {
      const auto i = static_cast<Isize>(index.get_global_id(0));
      if (i >= this->number_) {
        return;
      }
      const VolumeElementSolverDevice<VolumeElementTrait, SimulationControl>* self = this;
      const Device::View<
          const Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                               VolumeElementTrait::kBasisFunctionNumber>>
          variable_volume_gradient_residual = self->variable_volume_gradient_residual_.view(i, this->number_);
      const Device::View<const Device::Matrix<Real, VolumeElementTrait::kBasisFunctionNumber,
                                              VolumeElementTrait::kBasisFunctionNumber>>
          local_mass_matrix_inverse = volume_element_mesh.local_mass_matrix_inverse_.view(i, this->number_);
      Device::View<Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                                  VolumeElementTrait::kBasisFunctionNumber>>
          variable_volume_gradient_basis_function_coefficient =
              this->variable_volume_gradient_basis_function_coefficient_.view(i, this->number_);
      for (Isize m = 0; m < SimulationControl::kConservedVariableNumber * SimulationControl::kDimension; m++) {
        for (Isize n = 0; n < VolumeElementTrait::kBasisFunctionNumber; n++) {
          Real sum = 0.0_r;
          for (Isize k = 0; k < VolumeElementTrait::kBasisFunctionNumber; k++) {
            sum += variable_volume_gradient_residual(m, k) * local_mass_matrix_inverse(k, n);
          }
          variable_volume_gradient_basis_function_coefficient(m, n) = sum;
        }
      }
      if constexpr (IsNS<SimulationControl::kEquationModel>) {
        Device::View<Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                                    VolumeElementTrait::kBasisFunctionNumber>>
            variable_gradient_basis_function_coefficient =
                this->variable_gradient_basis_function_coefficient_.view(i, this->number_);
        for (Isize m = 0; m < SimulationControl::kConservedVariableNumber * SimulationControl::kDimension; m++) {
          for (Isize n = 0; n < VolumeElementTrait::kBasisFunctionNumber; n++) {
            variable_gradient_basis_function_coefficient(m, n) =
                variable_volume_gradient_basis_function_coefficient(m, n);
          }
        }
        if constexpr (SimulationControl::kViscousFlux == ViscousFluxEnum::BR1) {
          const Device::View<
              const Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                                   VolumeElementTrait::kBasisFunctionNumber>>
              variable_interface_gradient_residual = self->variable_interface_gradient_residual_.view(i, this->number_);
          Device::View<Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                                      VolumeElementTrait::kBasisFunctionNumber>>
              variable_interface_gradient_basis_function_coefficient =
                  this->variable_interface_gradient_basis_function_coefficient_.view(i, this->number_);
          for (Isize m = 0; m < SimulationControl::kConservedVariableNumber * SimulationControl::kDimension; m++) {
            for (Isize n = 0; n < VolumeElementTrait::kBasisFunctionNumber; n++) {
              Real sum = 0.0_r;
              for (Isize k = 0; k < VolumeElementTrait::kBasisFunctionNumber; k++) {
                sum += variable_interface_gradient_residual(m, k) * local_mass_matrix_inverse(k, n);
              }
              variable_interface_gradient_basis_function_coefficient(m, n) = sum;
              variable_gradient_basis_function_coefficient(m, n) += sum;
            }
          }
        } else if constexpr (SimulationControl::kViscousFlux == ViscousFluxEnum::BR2) {
          for (Isize j = 0; j < VolumeElementTrait::kAdjacencyNumber; j++) {
            const Device::View<
                const Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                                     VolumeElementTrait::kBasisFunctionNumber>>
                variable_interface_gradient_residual = self->variable_interface_gradient_residual_.slice(
                    i, this->number_,
                    Device::Slice<SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>::all(),
                    Device::Slice<VolumeElementTrait::kBasisFunctionNumber>::seqN(
                        j * VolumeElementTrait::kBasisFunctionNumber));
            Device::View<
                Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                               VolumeElementTrait::kBasisFunctionNumber>>
                variable_interface_gradient_basis_function_coefficient =
                    this->variable_interface_gradient_basis_function_coefficient_.slice(
                        i, this->number_,
                        Device::Slice<SimulationControl::kConservedVariableNumber *
                                      SimulationControl::kDimension>::all(),
                        Device::Slice<VolumeElementTrait::kBasisFunctionNumber>::seqN(
                            j * VolumeElementTrait::kBasisFunctionNumber));
            for (Isize m = 0; m < SimulationControl::kConservedVariableNumber * SimulationControl::kDimension; m++) {
              for (Isize n = 0; n < VolumeElementTrait::kBasisFunctionNumber; n++) {
                Real sum = 0.0_r;
                for (Isize k = 0; k < VolumeElementTrait::kBasisFunctionNumber; k++) {
                  sum += variable_interface_gradient_residual(m, k) * local_mass_matrix_inverse(k, n);
                }
                variable_interface_gradient_basis_function_coefficient(m, n) = sum;
                variable_gradient_basis_function_coefficient(m, n) += sum;
              }
            }
          }
        }
      }
    });
  });
}

template <typename SimulationControl>
inline void SolverDevice<SimulationControl>::updateGradientBasisFunctionCoefficient(
    const MeshDevice<SimulationControl>& mesh) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.updateVolumeElementGradientBasisFunctionCoefficient(mesh.line_);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.updateVolumeElementGradientBasisFunctionCoefficient(mesh.triangle_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.updateVolumeElementGradientBasisFunctionCoefficient(mesh.quadrangle_);
    }
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.updateVolumeElementGradientBasisFunctionCoefficient(mesh.tetrahedron_);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.updateVolumeElementGradientBasisFunctionCoefficient(mesh.pyramid_);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.updateVolumeElementGradientBasisFunctionCoefficient(mesh.hexahedron_);
    }
  }
  queue.wait();
}

template <typename VolumeElementTrait, typename SimulationControl>
inline void VolumeElementSolver<VolumeElementTrait, SimulationControl>::computeVolumeElementRelativeError(
    const VolumeElementMesh<VolumeElementTrait>& volume_element_mesh,
    Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& relative_error) {
  tbb::combinable<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>> relative_error_combinable(
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>::Zero());
  tbb::parallel_for(tbb::blocked_range<Isize>(0, this->number_), [&](const tbb::blocked_range<Isize>& range) -> void {
    for (Isize i = range.begin(); i != range.end(); i++) {
      const Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, VolumeElementTrait::kQuadratureNumber>
          quadrature_node_residual =
              this->variable_residual_(i) * volume_element_mesh.nodal_basis_function_.transpose();
      relative_error_combinable.local().array() += quadrature_node_residual.array().abs().rowwise().mean();
    }
  });
  relative_error = relative_error_combinable.combine(
      [](const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& a,
         const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& b) { return a + b; });
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::computeRelativeError(const Mesh<SimulationControl>& mesh) {
  this->relative_error_.setZero();
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.computeElementRelativeError(mesh.line_, this->relative_error_);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.computeVolumeElementRelativeError(mesh.triangle_, this->relative_error_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.computeVolumeElementRelativeError(mesh.quadrangle_, this->relative_error_);
    }
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.computeVolumeElementRelativeError(mesh.tetrahedron_, this->relative_error_);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.computeVolumeElementRelativeError(mesh.pyramid_, this->relative_error_);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.computeVolumeElementRelativeError(mesh.hexahedron_, this->relative_error_);
    }
  }
  this->relative_error_ = this->relative_error_ / static_cast<Real>(mesh.volume_element_number_);
}

template <typename VolumeElementTrait, typename SimulationControl>
inline void VolumeElementSolverDevice<VolumeElementTrait, SimulationControl>::computeVolumeElementRelativeError(
    const VolumeElementMeshDevice<VolumeElementTrait>& volume_element_mesh,
    Device::Vector<Real, SimulationControl::kConservedVariableNumber> relative_error) {
  queue
      .submit([&](sycl::handler& cgh) -> void {
        cgh.parallel_for(getNdRange(this->number_),
                         sycl::reduction(sycl::span<Real, SimulationControl::kConservedVariableNumber>(
                                             relative_error.data(), SimulationControl::kConservedVariableNumber),
                                         sycl::plus<Real>{}),
                         [=, this](sycl::nd_item<1> index, auto& sum) -> void {
                           const auto i = static_cast<Isize>(index.get_global_id(0));
                           if (i >= this->number_) {
                             return;
                           }
                           const VolumeElementSolverDevice<VolumeElementTrait, SimulationControl>* self = this;
                           const Device::View<const Device::Matrix<Real, SimulationControl::kConservedVariableNumber,
                                                                   VolumeElementTrait::kBasisFunctionNumber>>
                               variable_residual = self->variable_residual_.view(i, this->number_);
                           for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
                             Real mean_absolute_residual = 0.0_r;
                             for (Isize n = 0; n < VolumeElementTrait::kQuadratureNumber; n++) {
                               Real inner_sum = 0.0_r;
                               for (Isize k = 0; k < VolumeElementTrait::kBasisFunctionNumber; k++) {
                                 inner_sum += variable_residual(m, k) * volume_element_mesh.nodal_basis_function_(n, k);
                               }
                               mean_absolute_residual += sycl::fabs(inner_sum);
                             }
                             mean_absolute_residual /= static_cast<Real>(VolumeElementTrait::kQuadratureNumber);
                             sum[static_cast<Usize>(m)].combine(mean_absolute_residual);
                           }
                         });
      })
      .wait();
}

template <typename SimulationControl>
inline void SolverDevice<SimulationControl>::computeRelativeError(const MeshDevice<SimulationControl>& mesh,
                                                                  Solver<SimulationControl>& solver) {
  queue
      .submit([&](sycl::handler& cgh) -> void {
        cgh.parallel_for(getNdRange(SimulationControl::kConservedVariableNumber),
                         [=, this](sycl::nd_item<1> index) -> void {
                           const auto i = static_cast<Isize>(index.get_global_id(0));
                           if (i >= SimulationControl::kConservedVariableNumber) {
                             return;
                           }
                           this->relative_error_(i) = 0.0_r;
                         });
      })
      .wait();
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.computeElementRelativeError(mesh.line_, this->relative_error_);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.computeVolumeElementRelativeError(mesh.triangle_, this->relative_error_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.computeVolumeElementRelativeError(mesh.quadrangle_, this->relative_error_);
    }
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.computeVolumeElementRelativeError(mesh.tetrahedron_, this->relative_error_);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.computeVolumeElementRelativeError(mesh.pyramid_, this->relative_error_);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.computeVolumeElementRelativeError(mesh.hexahedron_, this->relative_error_);
    }
  }
  queue
      .submit([&](sycl::handler& cgh) -> void {
        cgh.parallel_for(
            getNdRange(SimulationControl::kConservedVariableNumber), [=, this](sycl::nd_item<1> index) -> void {
              const auto i = static_cast<Isize>(index.get_global_id(0));
              if (i >= SimulationControl::kConservedVariableNumber) {
                return;
              }
              this->relative_error_(i) = this->relative_error_(i) / static_cast<Real>(mesh.volume_element_number_);
            });
      })
      .wait();
  Utils::transferToHost<Real, SimulationControl::kConservedVariableNumber>(this->relative_error_,
                                                                           solver.relative_error_);
}

template <typename VolumeElementTrait, typename SimulationControl>
inline void VolumeElementSolverDevice<VolumeElementTrait, SimulationControl>::transferVolumeElementSolverToHost(
    VolumeElementSolver<VolumeElementTrait, SimulationControl>& volume_element_solver) {
  Utils::transferToHost<Real, SimulationControl::kConservedVariableNumber, VolumeElementTrait::kBasisFunctionNumber>(
      this->variable_basis_function_coefficient_, volume_element_solver.variable_basis_function_coefficient_);
  if constexpr (IsEuler<SimulationControl::kEquationModel>) {
    Utils::transferToHost<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                          VolumeElementTrait::kBasisFunctionNumber>(
        this->variable_volume_gradient_basis_function_coefficient_,
        volume_element_solver.variable_volume_gradient_basis_function_coefficient_);
  }
  if constexpr (IsNS<SimulationControl::kEquationModel>) {
    if constexpr (SimulationControl::kViscousFlux == ViscousFluxEnum::BR1) {
      Utils::transferToHost<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                            VolumeElementTrait::kBasisFunctionNumber>(
          this->variable_gradient_basis_function_coefficient_,
          volume_element_solver.variable_gradient_basis_function_coefficient_);
    } else if constexpr (SimulationControl::kViscousFlux == ViscousFluxEnum::BR2) {
      Utils::transferToHost<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                            VolumeElementTrait::kBasisFunctionNumber>(
          this->variable_volume_gradient_basis_function_coefficient_,
          volume_element_solver.variable_volume_gradient_basis_function_coefficient_);
      Utils::transferToHost<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                            VolumeElementTrait::kBasisFunctionNumber * VolumeElementTrait::kAdjacencyNumber>(
          this->variable_interface_gradient_basis_function_coefficient_,
          volume_element_solver.variable_interface_gradient_basis_function_coefficient_);
    }
  }
}

template <typename SimulationControl>
inline void SolverDevice<SimulationControl>::transferSolverToHost(Solver<SimulationControl>& solver) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.transferVolumeElementSolverToHost(solver.line_);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.transferVolumeElementSolverToHost(solver.triangle_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.transferVolumeElementSolverToHost(solver.quadrangle_);
    }
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.transferVolumeElementSolverToHost(solver.tetrahedron_);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.transferVolumeElementSolverToHost(solver.pyramid_);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.transferVolumeElementSolverToHost(solver.hexahedron_);
    }
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::stepSolver(const Mesh<SimulationControl>& mesh,
                                                  const SourceTerm<SimulationControl>& source_term,
                                                  const TimeIntegration<SimulationControl>& time_integration) {
  this->copyBasisFunctionCoefficient();
  if constexpr (SimulationControl::kBoundaryTime == BoundaryTimeEnum::TimeVarying) {
    this->updateBoundaryVariable(mesh, time_integration);
  }
  for (int i = 0; i < TimeIntegration<SimulationControl>::kStep; i++) {
    this->computeGradientQuadrature(mesh);
    this->computeAdjacencyGradientQuadrature(mesh);
    this->computeGradientResidual(mesh);
    this->updateGradientBasisFunctionCoefficient(mesh);
    this->computeQuadrature(mesh, source_term);
    this->computeAdjacencyQuadrature(mesh);
    this->computeResidual(mesh);
    this->updateBasisFunctionCoefficient(mesh, time_integration, i);
  }
}

template <typename SimulationControl>
inline void SolverDevice<SimulationControl>::stepSolver(const MeshDevice<SimulationControl>& mesh,
                                                        const SourceTermDevice<SimulationControl>& source_term,
                                                        const TimeIntegration<SimulationControl>& time_integration) {
  this->copyBasisFunctionCoefficient();
  if constexpr (SimulationControl::kBoundaryTime == BoundaryTimeEnum::TimeVarying) {
    this->updateBoundaryVariable(mesh, time_integration);
  }
  for (int i = 0; i < TimeIntegration<SimulationControl>::kStep; i++) {
    this->computeGradientQuadrature(mesh);
    this->computeAdjacencyGradientQuadrature(mesh);
    this->computeGradientResidual(mesh);
    this->updateGradientBasisFunctionCoefficient(mesh);
    this->computeQuadrature(mesh, source_term);
    this->computeAdjacencyQuadrature(mesh);
    this->computeResidual(mesh);
    this->updateBasisFunctionCoefficient(mesh, time_integration, i);
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_TIME_INTEGRATION_CPP_
