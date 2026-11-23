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

#include <oneapi/tbb.h>

#include <Eigen/Core>
#include <algorithm>
#include <array>
#include <ios>
#include <sycl/sycl.hpp>

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

struct TimeIntegrationBase {
  int iteration_start_{0};
  int iteration_end_;
  int iteration_{0};
  Real courant_friedrichs_lewy_number_;
  Real delta_time_{0.0};
};

template <typename SimulationControl>
struct TimeIntegration<SimulationControl, TimeIntegrationEnum::ForwardEuler> : TimeIntegrationBase {
  static constexpr int kStep = 1;
  static constexpr std::array<std::array<Real, 3>, kStep> kStepCoefficients{{{1.0_r, 0.0_r, 1.0_r}}};
  static constexpr std::array<Real, kStep> kButcherCoefficients{0.0_r};
};

template <typename SimulationControl>
struct TimeIntegration<SimulationControl, TimeIntegrationEnum::HeunRK2> : TimeIntegrationBase {
  static constexpr int kStep = 2;
  static constexpr std::array<std::array<Real, 3>, kStep> kStepCoefficients{
      {{1.0_r, 0.0_r, 1.0_r}, {0.5_r, 0.5_r, 0.5_r}}};
  static constexpr std::array<Real, kStep> kButcherCoefficients{0.0_r, 1.0_r};
};

template <typename SimulationControl>
struct TimeIntegration<SimulationControl, TimeIntegrationEnum::SSPRK3> : TimeIntegrationBase {
  static constexpr int kStep = 3;
  static constexpr std::array<std::array<Real, 3>, kStep> kStepCoefficients{
      {{1.0_r, 0.0_r, 1.0_r},
       {3.0_r / 4.0_r, 1.0_r / 4.0_r, 1.0_r / 4.0_r},
       {1.0_r / 3.0_r, 2.0_r / 3.0_r, 2.0_r / 3.0_r}}};
  static constexpr std::array<Real, kStep> kButcherCoefficients{0.0_r, 1.0_r, 0.5_r};
};

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
      const Device::View<Device::Matrix<Real, SimulationControl::kConservedVariableNumber,
                                        VolumeElementTrait::kAllBasisFunctionNumber>>
          variable_basis_function_coefficient = this->variable_basis_function_coefficient_.view(i, this->number_);
      Device::View<Device::Matrix<Real, SimulationControl::kConservedVariableNumber,
                                  VolumeElementTrait::kAllBasisFunctionNumber>>
          variable_basis_function_coefficient_last =
              this->variable_basis_function_coefficient_last_.view(i, this->number_);
      for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
        for (Isize n = 0; n < VolumeElementTrait::kAllBasisFunctionNumber; n++) {
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
  tbb::combinable<Real> delta_time_combinable(kRealMax);
  tbb::parallel_for(tbb::blocked_range<Isize>(0, this->number_), [&](const tbb::blocked_range<Isize>& range) -> void {
    for (Isize i = range.begin(); i != range.end(); i++) {
      const Real inverse_convective_scale = (SimulationControl::kPolynomialOrder + 1) *
                                            (SimulationControl::kPolynomialOrder + 1) /
                                            volume_element_mesh.characteristic_length_(i);
      Real max_spectral_radius = 0.0_r;
      Real max_diffusivity = 0.0_r;
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> quadrature_node_conserved_variable;
      Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber> quadrature_node_computational_variable;
      Eigen::Vector<Real, SimulationControl::kDimension> quadrature_node_rotation_velocity;
      for (Isize j = 0; j < VolumeElementTrait::kQuadratureNumber; j++) {
        VolumeElementVariable<VolumeElementTrait, SimulationControl>::getRotationVelocity(
            volume_element_mesh, *this, quadrature_node_rotation_velocity, i, j);
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
            (Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
                 quadrature_node_computational_variable) -
             quadrature_node_rotation_velocity)
                .norm() +
            sound_speed;
        max_spectral_radius = std::max(max_spectral_radius, spectral_radius);
        if constexpr (IsNS<SimulationControl::kEquationModel>) {
          const Real density = Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
              quadrature_node_computational_variable);
          const Real temperature =
              PhysicalModel<SimulationControl, PhysicalModelData>::computeTemperatureFromInternalEnergy(
                  Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
                      quadrature_node_computational_variable));
          const Real momentum_diffusivity =
              4.0_r / 3.0_r *
              PhysicalModel<SimulationControl, PhysicalModelData>::computeDynamicViscosity(temperature) / density;
          // The energy variable uses e = c_v*T, so its thermal diffusion scale is kappa/(rho*c_v).
          const Real thermal_diffusivity =
              PhysicalModel<SimulationControl, PhysicalModelData>::computeThermalConductivity(temperature) /
              (density * ThermodynamicModel<PhysicalModelData,
                                            SimulationControl::kThermodynamicModel>::kSpecificHeatConstantVolume);
          max_diffusivity = std::max({max_diffusivity, momentum_diffusivity, thermal_diffusivity});
        }
      }
      // Kloeckner et al., arXiv:1102.3190, Eq. (2.1): dt ~ 1/(lambda*N^2/h + nu*N^4/h^2), N = p.
      const Real inverse_delta_time = max_spectral_radius * inverse_convective_scale +
                                      max_diffusivity * inverse_convective_scale * inverse_convective_scale;
      delta_time_combinable.local() =
          std::min(delta_time_combinable.local(), courant_friedrichs_lewy_number / inverse_delta_time);
    }
  });
  delta_time =
      std::min(delta_time, delta_time_combinable.combine([](const Real a, const Real b) { return std::min(a, b); }));
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::computeDeltaTime(const Mesh<SimulationControl>& mesh,
                                                        TimeIntegration<SimulationControl>& time_integration) {
  if constexpr (SimulationControl::kInitialCondition == InitialConditionEnum::LastStep) {
    this->raw_binary_ss_.seekg(0, std::ios::beg);
    this->raw_binary_ss_.read(reinterpret_cast<char*>(&time_integration.delta_time_),
                              static_cast<std::streamsize>(sizeof(Real)));
    time_integration.delta_time_ /= static_cast<Real>(time_integration.iteration_start_);
  } else {
    time_integration.delta_time_ = kRealMax;
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
  constexpr std::array<int, VolumeElementTrait::kAdjacencyNumber> kAdjacencyNodeNumber{
      getVolumeElementPerAdjacencyNodeNumber<VolumeElementTrait::kElementType, VolumeElementTrait::kPolynomialOrder>()};
  constexpr std::array<int, VolumeElementTrait::kAdjacencyNumber> kAdjacencyQuadratureNumber{
      getVolumeElementPerAdjacencyQuadratureNumber<VolumeElementTrait::kElementType,
                                                   VolumeElementTrait::kPolynomialOrder>()};
  tbb::parallel_for(tbb::blocked_range<Isize>(0, this->number_), [&](const tbb::blocked_range<Isize>& range) -> void {
    for (Isize i = range.begin(); i != range.end(); i++) {
      Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, VolumeElementTrait::kAllBasisFunctionNumber>&
          variable_residual = this->variable_residual_(i);
      variable_residual.noalias() = this->variable_quadrature_(i) * volume_element_mesh.nodal_gradient_basis_function_;
      static_for<VolumeElementTrait::kAdjacencyNumber>([&]<Usize I>() {
        const Eigen::Map<const Eigen::Matrix<Real, kAdjacencyQuadratureNumber[I], kAdjacencyNodeNumber[I]>>
            nodal_adjacency_basis_function(volume_element_mesh.nodal_adjacency_basis_function_.data() +
                                           VolumeElementTrait::kAdjacencyBasisValueSequence[I]);
        Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, kAdjacencyNodeNumber[I]> projection;
        projection.noalias() =
            this->variable_adjacency_quadrature_(i)(
                Eigen::placeholders::all, Eigen::seqN(Eigen::fix<VolumeElementTrait::kAdjacencyQuadratureSequence[I]>,
                                                      Eigen::fix<kAdjacencyQuadratureNumber[I]>)) *
            nodal_adjacency_basis_function;
        for (Isize a = 0; a < kAdjacencyNodeNumber[I]; a++) {
          const Isize n = VolumeElementTrait::kPerAdjacencyNodeIndex[static_cast<Usize>(
              VolumeElementTrait::kAdjacencyNodeSequence[I] + a)];
          variable_residual.col(n) -= projection.col(a);
        }
      });
      if constexpr (SimulationControl::kSourceTerm != SourceTermEnum::None) {
        variable_residual.noalias() += this->variable_source_quadrature_(i) * volume_element_mesh.nodal_basis_function_;
      }
      // NOTE: Here we split the calculation to trigger eigen's noalias to avoid intermediate variables.
      this->variable_basis_function_coefficient_(i) *=
          TimeIntegration<SimulationControl>::kStepCoefficients[static_cast<Usize>(rk_step)][1];
      this->variable_basis_function_coefficient_(i).noalias() +=
          TimeIntegration<SimulationControl>::kStepCoefficients[static_cast<Usize>(rk_step)][0] *
          this->variable_basis_function_coefficient_last_(i);
      this->variable_basis_function_coefficient_(i).noalias() +=
          TimeIntegration<SimulationControl>::kStepCoefficients[static_cast<Usize>(rk_step)][2] *
          time_integration.delta_time_ * variable_residual * volume_element_mesh.local_mass_matrix_inverse_(i);
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
  constexpr std::array<int, VolumeElementTrait::kAdjacencyNumber> kAdjacencyNodeNumber{
      getVolumeElementPerAdjacencyNodeNumber<VolumeElementTrait::kElementType, VolumeElementTrait::kPolynomialOrder>()};
  constexpr std::array<int, VolumeElementTrait::kAdjacencyNumber> kAdjacencyQuadratureNumber{
      getVolumeElementPerAdjacencyQuadratureNumber<VolumeElementTrait::kElementType,
                                                   VolumeElementTrait::kPolynomialOrder>()};
  queue.submit([&](sycl::handler& cgh) -> void {
    cgh.parallel_for(getNdRange(this->number_), [=, this](sycl::nd_item<1> index) -> void {
      const auto i = static_cast<Isize>(index.get_global_id(0));
      if (i >= this->number_) {
        return;
      }
      const VolumeElementSolverDevice<VolumeElementTrait, SimulationControl>* self = this;
      const Device::View<const Device::Matrix<Real, SimulationControl::kConservedVariableNumber,
                                              VolumeElementTrait::kQuadratureNumber * SimulationControl::kDimension>>
          variable_quadrature = self->variable_quadrature_.view(i, this->number_);
      const Device::View<const Device::Matrix<Real, SimulationControl::kConservedVariableNumber,
                                              VolumeElementTrait::kAllAdjacencyQuadratureNumber>>
          variable_adjacency_quadrature = self->variable_adjacency_quadrature_.view(i, this->number_);
      Device::View<Device::Matrix<Real, SimulationControl::kConservedVariableNumber,
                                  VolumeElementTrait::kAllBasisFunctionNumber>>
          variable_residual = this->variable_residual_.view(i, this->number_);
      for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
        for (Isize n = 0; n < VolumeElementTrait::kAllBasisFunctionNumber; n++) {
          Real sum = 0.0_r;
          for (Isize k = 0; k < VolumeElementTrait::kQuadratureNumber * SimulationControl::kDimension; k++) {
            sum += variable_quadrature(m, k) * volume_element_mesh.nodal_gradient_basis_function_(k, n);
          }
          variable_residual(m, n) = sum;
        }
      }
      for (Usize face = 0; face < VolumeElementTrait::kAdjacencyNumber; face++) {
        const Isize quadrature_start = VolumeElementTrait::kAdjacencyQuadratureSequence[face];
        const Isize quadrature_number = kAdjacencyQuadratureNumber[face];
        const Isize basis_start = VolumeElementTrait::kAdjacencyBasisValueSequence[face];
        const Isize node_start = VolumeElementTrait::kAdjacencyNodeSequence[face];
        for (Isize a = 0; a < kAdjacencyNodeNumber[face]; a++) {
          const Isize n = VolumeElementTrait::kPerAdjacencyNodeIndex[static_cast<Usize>(node_start + a)];
          const Isize value_start = basis_start + a * quadrature_number;
          for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
            Real sum = 0.0_r;
            for (Isize q = 0; q < quadrature_number; q++) {
              sum += variable_adjacency_quadrature(m, quadrature_start + q) *
                     volume_element_mesh.nodal_adjacency_basis_function_(value_start + q);
            }
            variable_residual(m, n) -= sum;
          }
        }
      }
      if constexpr (SimulationControl::kSourceTerm != SourceTermEnum::None) {
        const Device::View<const Device::Matrix<Real, SimulationControl::kConservedVariableNumber,
                                                VolumeElementTrait::kQuadratureNumber>>
            variable_source_quadrature = self->variable_source_quadrature_.view(i, this->number_);
        for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
          for (Isize n = 0; n < VolumeElementTrait::kAllBasisFunctionNumber; n++) {
            Real sum = 0.0_r;
            for (Isize k = 0; k < VolumeElementTrait::kQuadratureNumber; k++) {
              sum += variable_source_quadrature(m, k) * volume_element_mesh.nodal_basis_function_(k, n);
            }
            variable_residual(m, n) += sum;
          }
        }
      }
      Device::View<Device::Matrix<Real, SimulationControl::kConservedVariableNumber,
                                  VolumeElementTrait::kAllBasisFunctionNumber>>
          variable_basis_function_coefficient = this->variable_basis_function_coefficient_.view(i, this->number_);
      for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
        for (Isize n = 0; n < VolumeElementTrait::kAllBasisFunctionNumber; n++) {
          variable_basis_function_coefficient(m, n) *=
              TimeIntegration<SimulationControl>::kStepCoefficients[static_cast<Usize>(rk_step)][1];
        }
      }
      const Device::View<const Device::Matrix<Real, SimulationControl::kConservedVariableNumber,
                                              VolumeElementTrait::kAllBasisFunctionNumber>>
          variable_basis_function_coefficient_last =
              self->variable_basis_function_coefficient_last_.view(i, this->number_);
      for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
        for (Isize n = 0; n < VolumeElementTrait::kAllBasisFunctionNumber; n++) {
          variable_basis_function_coefficient(m, n) +=
              TimeIntegration<SimulationControl>::kStepCoefficients[static_cast<Usize>(rk_step)][0] *
              variable_basis_function_coefficient_last(m, n);
        }
      }
      const Device::View<const Device::Matrix<Real, VolumeElementTrait::kAllBasisFunctionNumber,
                                              VolumeElementTrait::kAllBasisFunctionNumber>>
          local_mass_matrix_inverse = volume_element_mesh.local_mass_matrix_inverse_.view(i, this->number_);
      for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
        for (Isize n = 0; n < VolumeElementTrait::kAllBasisFunctionNumber; n++) {
          Real sum = 0.0_r;
          for (Isize k = 0; k < VolumeElementTrait::kAllBasisFunctionNumber; k++) {
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
  constexpr std::array<int, VolumeElementTrait::kAdjacencyNumber> kAdjacencyNodeNumber{
      getVolumeElementPerAdjacencyNodeNumber<VolumeElementTrait::kElementType, VolumeElementTrait::kPolynomialOrder>()};
  constexpr std::array<int, VolumeElementTrait::kAdjacencyNumber> kAdjacencyQuadratureNumber{
      getVolumeElementPerAdjacencyQuadratureNumber<VolumeElementTrait::kElementType,
                                                   VolumeElementTrait::kPolynomialOrder>()};
  tbb::parallel_for(tbb::blocked_range<Isize>(0, this->number_), [&](const tbb::blocked_range<Isize>& range) -> void {
    for (Isize i = range.begin(); i != range.end(); i++) {
      const Eigen::Matrix<Real, VolumeElementTrait::kAllBasisFunctionNumber,
                          VolumeElementTrait::kAllBasisFunctionNumber>& mass_inverse =
          volume_element_mesh.local_mass_matrix_inverse_(i);
      // Project and lift while the residual is local to the element.
      Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                    VolumeElementTrait::kAllBasisFunctionNumber>
          variable_gradient_residual;
      variable_gradient_residual.setZero();
      static_for<VolumeElementTrait::kAdjacencyNumber>([&]<Usize I>() {
        const Eigen::Map<const Eigen::Matrix<Real, kAdjacencyQuadratureNumber[I], kAdjacencyNodeNumber[I]>>
            nodal_adjacency_basis_function(volume_element_mesh.nodal_adjacency_basis_function_.data() +
                                           VolumeElementTrait::kAdjacencyBasisValueSequence[I]);
        Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                      kAdjacencyNodeNumber[I]>
            projection;
        projection.noalias() =
            this->variable_volume_gradient_adjacency_quadrature_(i)(
                Eigen::placeholders::all, Eigen::seqN(Eigen::fix<VolumeElementTrait::kAdjacencyQuadratureSequence[I]>,
                                                      Eigen::fix<kAdjacencyQuadratureNumber[I]>)) *
            nodal_adjacency_basis_function;
        for (Isize a = 0; a < kAdjacencyNodeNumber[I]; a++) {
          const Isize n = VolumeElementTrait::kPerAdjacencyNodeIndex[static_cast<Usize>(
              VolumeElementTrait::kAdjacencyNodeSequence[I] + a)];
          variable_gradient_residual.col(n) += projection.col(a);
        }
      });
      variable_gradient_residual.noalias() -=
          this->variable_volume_gradient_quadrature_(i) * volume_element_mesh.nodal_gradient_basis_function_;
      this->variable_volume_gradient_basis_function_coefficient_(i).noalias() =
          variable_gradient_residual * mass_inverse;
      if constexpr (IsNS<SimulationControl::kEquationModel>) {
        this->variable_gradient_basis_function_coefficient_(i).noalias() =
            this->variable_volume_gradient_basis_function_coefficient_(i);
        if constexpr (SimulationControl::kViscousFlux == ViscousFluxEnum::BR1) {
          variable_gradient_residual.setZero();
          static_for<VolumeElementTrait::kAdjacencyNumber>([&]<Usize I>() {
            const Eigen::Map<const Eigen::Matrix<Real, kAdjacencyQuadratureNumber[I], kAdjacencyNodeNumber[I]>>
                nodal_adjacency_basis_function(volume_element_mesh.nodal_adjacency_basis_function_.data() +
                                               VolumeElementTrait::kAdjacencyBasisValueSequence[I]);
            Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                          kAdjacencyNodeNumber[I]>
                projection;
            projection.noalias() = this->variable_interface_gradient_adjacency_quadrature_(i)(
                                       Eigen::placeholders::all,
                                       Eigen::seqN(Eigen::fix<VolumeElementTrait::kAdjacencyQuadratureSequence[I]>,
                                                   Eigen::fix<kAdjacencyQuadratureNumber[I]>)) *
                                   nodal_adjacency_basis_function;
            for (Isize a = 0; a < kAdjacencyNodeNumber[I]; a++) {
              const Isize n = VolumeElementTrait::kPerAdjacencyNodeIndex[static_cast<Usize>(
                  VolumeElementTrait::kAdjacencyNodeSequence[I] + a)];
              variable_gradient_residual.col(n) += projection.col(a);
            }
          });
          this->variable_interface_gradient_basis_function_coefficient_(i).noalias() =
              variable_gradient_residual * mass_inverse;
          this->variable_gradient_basis_function_coefficient_(i).noalias() +=
              this->variable_interface_gradient_basis_function_coefficient_(i);
        } else if constexpr (SimulationControl::kViscousFlux == ViscousFluxEnum::BR2) {
          static_for<VolumeElementTrait::kAdjacencyNumber>([&]<Usize I>() {
            const Eigen::Map<const Eigen::Matrix<Real, kAdjacencyQuadratureNumber[I], kAdjacencyNodeNumber[I]>>
                nodal_adjacency_basis_function(volume_element_mesh.nodal_adjacency_basis_function_.data() +
                                               VolumeElementTrait::kAdjacencyBasisValueSequence[I]);
            Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                          kAdjacencyNodeNumber[I]>
                projection;
            projection.noalias() = this->variable_interface_gradient_adjacency_quadrature_(i)(
                                       Eigen::placeholders::all,
                                       Eigen::seqN(Eigen::fix<VolumeElementTrait::kAdjacencyQuadratureSequence[I]>,
                                                   Eigen::fix<kAdjacencyQuadratureNumber[I]>)) *
                                   nodal_adjacency_basis_function;
            const Eigen::Map<const Eigen::Vector<Isize, kAdjacencyNodeNumber[I]>> node_index(
                VolumeElementTrait::kPerAdjacencyNodeIndex.data() + VolumeElementTrait::kAdjacencyNodeSequence[I]);
            Eigen::Ref<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                                     VolumeElementTrait::kAllBasisFunctionNumber>>
                variable_interface_gradient_basis_function_coefficient =
                    this->variable_interface_gradient_basis_function_coefficient_(i)(
                        Eigen::placeholders::all,
                        Eigen::seqN(Eigen::fix<I * VolumeElementTrait::kAllBasisFunctionNumber>,
                                    Eigen::fix<VolumeElementTrait::kAllBasisFunctionNumber>));
            variable_interface_gradient_basis_function_coefficient.noalias() =
                projection * mass_inverse(node_index, Eigen::placeholders::all);
            this->variable_gradient_basis_function_coefficient_(i) +=
                variable_interface_gradient_basis_function_coefficient;
          });
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
  constexpr std::array<int, VolumeElementTrait::kAdjacencyNumber> kAdjacencyNodeNumber{
      getVolumeElementPerAdjacencyNodeNumber<VolumeElementTrait::kElementType, VolumeElementTrait::kPolynomialOrder>()};
  constexpr std::array<int, VolumeElementTrait::kAdjacencyNumber> kAdjacencyQuadratureNumber{
      getVolumeElementPerAdjacencyQuadratureNumber<VolumeElementTrait::kElementType,
                                                   VolumeElementTrait::kPolynomialOrder>()};
  constexpr int kMaxAdjacencyNodeNumber = *std::max_element(kAdjacencyNodeNumber.begin(), kAdjacencyNodeNumber.end());
  queue.submit([&](sycl::handler& cgh) -> void {
    cgh.parallel_for(getNdRange(this->number_), [=, this](sycl::nd_item<1> index) -> void {
      const auto i = static_cast<Isize>(index.get_global_id(0));
      if (i >= this->number_) {
        return;
      }
      const VolumeElementSolverDevice<VolumeElementTrait, SimulationControl>* self = this;
      const Device::View<
          const Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                               VolumeElementTrait::kAllAdjacencyQuadratureNumber>>
          volume_gradient_adjacency_quadrature =
              self->variable_volume_gradient_adjacency_quadrature_.view(i, this->number_);
      const Device::View<
          const Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                               VolumeElementTrait::kQuadratureNumber * SimulationControl::kDimension>>
          volume_gradient_quadrature = self->variable_volume_gradient_quadrature_.view(i, this->number_);
      const Device::View<const Device::Matrix<Real, VolumeElementTrait::kAllBasisFunctionNumber,
                                              VolumeElementTrait::kAllBasisFunctionNumber>>
          local_mass_matrix_inverse = volume_element_mesh.local_mass_matrix_inverse_.view(i, this->number_);
      Device::View<Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                                  VolumeElementTrait::kAllBasisFunctionNumber>>
          variable_volume_gradient_basis_function_coefficient =
              this->variable_volume_gradient_basis_function_coefficient_.view(i, this->number_);
      // A volume gradient or BR1 lifting still needs a full residual row.
      Device::StaticVector<Real, VolumeElementTrait::kAllBasisFunctionNumber> variable_gradient_residual;
      for (Isize m = 0; m < SimulationControl::kConservedVariableNumber * SimulationControl::kDimension; m++) {
        for (Isize n = 0; n < VolumeElementTrait::kAllBasisFunctionNumber; n++) {
          variable_gradient_residual(n) = 0.0_r;
        }
        for (Usize face = 0; face < VolumeElementTrait::kAdjacencyNumber; face++) {
          const Isize quadrature_start = VolumeElementTrait::kAdjacencyQuadratureSequence[face];
          const Isize quadrature_number = kAdjacencyQuadratureNumber[face];
          const Isize basis_start = VolumeElementTrait::kAdjacencyBasisValueSequence[face];
          const Isize node_start = VolumeElementTrait::kAdjacencyNodeSequence[face];
          for (Isize a = 0; a < kAdjacencyNodeNumber[face]; a++) {
            const Isize n = VolumeElementTrait::kPerAdjacencyNodeIndex[static_cast<Usize>(node_start + a)];
            const Isize value_start = basis_start + a * quadrature_number;
            Real sum = 0.0_r;
            for (Isize q = 0; q < quadrature_number; q++) {
              sum += volume_gradient_adjacency_quadrature(m, quadrature_start + q) *
                     volume_element_mesh.nodal_adjacency_basis_function_(value_start + q);
            }
            variable_gradient_residual(n) += sum;
          }
        }
        for (Isize n = 0; n < VolumeElementTrait::kAllBasisFunctionNumber; n++) {
          Real volume_sum = 0.0_r;
          for (Isize k = 0; k < VolumeElementTrait::kQuadratureNumber * SimulationControl::kDimension; k++) {
            volume_sum += volume_gradient_quadrature(m, k) * volume_element_mesh.nodal_gradient_basis_function_(k, n);
          }
          variable_gradient_residual(n) -= volume_sum;
        }
        for (Isize n = 0; n < VolumeElementTrait::kAllBasisFunctionNumber; n++) {
          Real sum = 0.0_r;
          for (Isize k = 0; k < VolumeElementTrait::kAllBasisFunctionNumber; k++) {
            sum += variable_gradient_residual(k) * local_mass_matrix_inverse(k, n);
          }
          variable_volume_gradient_basis_function_coefficient(m, n) = sum;
        }
      }
      if constexpr (IsNS<SimulationControl::kEquationModel>) {
        Device::View<Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                                    VolumeElementTrait::kAllBasisFunctionNumber>>
            variable_gradient_basis_function_coefficient =
                this->variable_gradient_basis_function_coefficient_.view(i, this->number_);
        for (Isize m = 0; m < SimulationControl::kConservedVariableNumber * SimulationControl::kDimension; m++) {
          for (Isize n = 0; n < VolumeElementTrait::kAllBasisFunctionNumber; n++) {
            variable_gradient_basis_function_coefficient(m, n) =
                variable_volume_gradient_basis_function_coefficient(m, n);
          }
        }
        if constexpr (SimulationControl::kViscousFlux == ViscousFluxEnum::BR1) {
          const Device::View<
              const Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                                   VolumeElementTrait::kAllAdjacencyQuadratureNumber>>
              variable_interface_gradient_adjacency_quadrature =
                  self->variable_interface_gradient_adjacency_quadrature_.view(i, this->number_);
          Device::View<Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                                      VolumeElementTrait::kAllBasisFunctionNumber>>
              variable_interface_gradient_basis_function_coefficient =
                  this->variable_interface_gradient_basis_function_coefficient_.view(i, this->number_);
          for (Isize m = 0; m < SimulationControl::kConservedVariableNumber * SimulationControl::kDimension; m++) {
            for (Isize n = 0; n < VolumeElementTrait::kAllBasisFunctionNumber; n++) {
              variable_gradient_residual(n) = 0.0_r;
            }
            for (Usize face = 0; face < VolumeElementTrait::kAdjacencyNumber; face++) {
              const Isize quadrature_start = VolumeElementTrait::kAdjacencyQuadratureSequence[face];
              const Isize quadrature_number = kAdjacencyQuadratureNumber[face];
              const Isize basis_start = VolumeElementTrait::kAdjacencyBasisValueSequence[face];
              const Isize node_start = VolumeElementTrait::kAdjacencyNodeSequence[face];
              for (Isize a = 0; a < kAdjacencyNodeNumber[face]; a++) {
                const Isize n = VolumeElementTrait::kPerAdjacencyNodeIndex[static_cast<Usize>(node_start + a)];
                const Isize value_start = basis_start + a * quadrature_number;
                Real sum = 0.0_r;
                for (Isize q = 0; q < quadrature_number; q++) {
                  sum += variable_interface_gradient_adjacency_quadrature(m, quadrature_start + q) *
                         volume_element_mesh.nodal_adjacency_basis_function_(value_start + q);
                }
                variable_gradient_residual(n) += sum;
              }
            }
            for (Isize n = 0; n < VolumeElementTrait::kAllBasisFunctionNumber; n++) {
              Real sum = 0.0_r;
              for (Isize k = 0; k < VolumeElementTrait::kAllBasisFunctionNumber; k++) {
                sum += variable_gradient_residual(k) * local_mass_matrix_inverse(k, n);
              }
              variable_interface_gradient_basis_function_coefficient(m, n) = sum;
              variable_gradient_basis_function_coefficient(m, n) += sum;
            }
          }
        } else if constexpr (SimulationControl::kViscousFlux == ViscousFluxEnum::BR2) {
          const Device::View<
              const Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                                   VolumeElementTrait::kAllAdjacencyQuadratureNumber>>
              variable_interface_gradient_adjacency_quadrature =
                  self->variable_interface_gradient_adjacency_quadrature_.view(i, this->number_);
          for (Usize face = 0; face < VolumeElementTrait::kAdjacencyNumber; face++) {
            const Isize quadrature_start = VolumeElementTrait::kAdjacencyQuadratureSequence[face];
            const Isize quadrature_number = kAdjacencyQuadratureNumber[face];
            const Isize basis_start = VolumeElementTrait::kAdjacencyBasisValueSequence[face];
            const Isize node_start = VolumeElementTrait::kAdjacencyNodeSequence[face];
            const Isize node_number = kAdjacencyNodeNumber[face];
            Device::View<
                Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                               VolumeElementTrait::kAllBasisFunctionNumber>>
                variable_interface_gradient_basis_function_coefficient =
                    this->variable_interface_gradient_basis_function_coefficient_.slice(
                        i, this->number_,
                        Device::Slice<SimulationControl::kConservedVariableNumber *
                                      SimulationControl::kDimension>::all(),
                        Device::Slice<VolumeElementTrait::kAllBasisFunctionNumber>::seqN(
                            static_cast<Isize>(face) * VolumeElementTrait::kAllBasisFunctionNumber));
            for (Isize m = 0; m < SimulationControl::kConservedVariableNumber * SimulationControl::kDimension; m++) {
              Device::StaticVector<Real, kMaxAdjacencyNodeNumber> projection;
              for (Isize a = 0; a < node_number; a++) {
                const Isize value_start = basis_start + a * quadrature_number;
                Real sum = 0.0_r;
                for (Isize q = 0; q < quadrature_number; q++) {
                  sum += variable_interface_gradient_adjacency_quadrature(m, quadrature_start + q) *
                         volume_element_mesh.nodal_adjacency_basis_function_(value_start + q);
                }
                projection(a) = sum;
              }
              for (Isize n = 0; n < VolumeElementTrait::kAllBasisFunctionNumber; n++) {
                Real sum = 0.0_r;
                for (Isize a = 0; a < node_number; a++) {
                  const Isize row = VolumeElementTrait::kPerAdjacencyNodeIndex[static_cast<Usize>(node_start + a)];
                  sum += projection(a) * local_mass_matrix_inverse(row, n);
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
inline void VolumeElementSolver<VolumeElementTrait, SimulationControl>::computeVolumeElementMeanAbsoluteResidual(
    const VolumeElementMesh<VolumeElementTrait>& volume_element_mesh,
    Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& residual) {
  tbb::combinable<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>> residual_combinable(
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>::Zero());
  tbb::parallel_for(tbb::blocked_range<Isize>(0, this->number_), [&](const tbb::blocked_range<Isize>& range) -> void {
    for (Isize i = range.begin(); i != range.end(); i++) {
      const Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber,
                          VolumeElementTrait::kAllBasisFunctionNumber>
          derivative = this->variable_residual_(i) * volume_element_mesh.local_mass_matrix_inverse_(i);
      const Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, VolumeElementTrait::kQuadratureNumber>
          quadrature_node_residual = derivative * volume_element_mesh.nodal_basis_function_.transpose();
      const Eigen::Vector<Real, VolumeElementTrait::kQuadratureNumber>& weights =
          volume_element_mesh.jacobian_determinant_multiply_weight_(i);
      residual_combinable.local().noalias() += (quadrature_node_residual.cwiseAbs() * weights) / weights.sum();
    }
  });
  residual += residual_combinable.combine(
      [](const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& a,
         const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& b) { return a + b; });
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::computeMeanAbsoluteResidual(const Mesh<SimulationControl>& mesh) {
  this->residual_.setZero();
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.computeVolumeElementMeanAbsoluteResidual(mesh.line_, this->residual_);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.computeVolumeElementMeanAbsoluteResidual(mesh.triangle_, this->residual_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.computeVolumeElementMeanAbsoluteResidual(mesh.quadrangle_, this->residual_);
    }
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.computeVolumeElementMeanAbsoluteResidual(mesh.tetrahedron_, this->residual_);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.computeVolumeElementMeanAbsoluteResidual(mesh.pyramid_, this->residual_);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.computeVolumeElementMeanAbsoluteResidual(mesh.hexahedron_, this->residual_);
    }
  }
  this->residual_ = this->residual_ / static_cast<Real>(mesh.volume_element_number_);
}

template <typename VolumeElementTrait, typename SimulationControl>
inline void VolumeElementSolverDevice<VolumeElementTrait, SimulationControl>::computeVolumeElementMeanAbsoluteResidual(
    const VolumeElementMeshDevice<VolumeElementTrait>& volume_element_mesh,
    Device::Vector<Real, SimulationControl::kConservedVariableNumber>& residual) {
  queue
      .submit([&](sycl::handler& cgh) -> void {
        cgh.parallel_for(
            getNdRange(this->number_),
            sycl::reduction(sycl::span<Real, SimulationControl::kConservedVariableNumber>(
                                residual.data(), SimulationControl::kConservedVariableNumber),
                            sycl::plus<Real>{}),
            [=, this](sycl::nd_item<1> index, auto& sum) -> void {
              const auto i = static_cast<Isize>(index.get_global_id(0));
              if (i >= this->number_) {
                return;
              }
              const VolumeElementSolverDevice<VolumeElementTrait, SimulationControl>* self = this;
              const Device::View<const Device::Matrix<Real, SimulationControl::kConservedVariableNumber,
                                                      VolumeElementTrait::kAllBasisFunctionNumber>>
                  variable_residual = self->variable_residual_.view(i, this->number_);
              const Device::View<const Device::Matrix<Real, VolumeElementTrait::kAllBasisFunctionNumber,
                                                      VolumeElementTrait::kAllBasisFunctionNumber>>
                  local_mass_matrix_inverse = volume_element_mesh.local_mass_matrix_inverse_.view(i, this->number_);
              const Device::View<const Device::Vector<Real, VolumeElementTrait::kQuadratureNumber>> weights =
                  volume_element_mesh.jacobian_determinant_multiply_weight_.view(i, this->number_);
              Real element_volume = 0.0_r;
              for (Isize q = 0; q < VolumeElementTrait::kQuadratureNumber; q++) {
                element_volume += weights(q);
              }
              for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
                Device::StaticVector<Real, VolumeElementTrait::kAllBasisFunctionNumber> derivative;
                for (Isize n = 0; n < VolumeElementTrait::kAllBasisFunctionNumber; n++) {
                  Real coefficient = 0.0_r;
                  for (Isize k = 0; k < VolumeElementTrait::kAllBasisFunctionNumber; k++) {
                    coefficient += variable_residual(m, k) * local_mass_matrix_inverse(k, n);
                  }
                  derivative(n) = coefficient;
                }
                Real integral = 0.0_r;
                for (Isize q = 0; q < VolumeElementTrait::kQuadratureNumber; q++) {
                  Real value = 0.0_r;
                  for (Isize k = 0; k < VolumeElementTrait::kAllBasisFunctionNumber; k++) {
                    value += derivative(k) * volume_element_mesh.nodal_basis_function_(q, k);
                  }
                  integral += weights(q) * sycl::fabs(value);
                }
                sum[static_cast<Usize>(m)].combine(integral / element_volume);
              }
            });
      })
      .wait();
}

template <typename SimulationControl>
inline void SolverDevice<SimulationControl>::computeMeanAbsoluteResidual(const MeshDevice<SimulationControl>& mesh,
                                                                         Solver<SimulationControl>& solver) {
  queue
      .submit([&](sycl::handler& cgh) -> void {
        cgh.parallel_for(getNdRange(SimulationControl::kConservedVariableNumber),
                         [=, this](sycl::nd_item<1> index) -> void {
                           const auto i = static_cast<Isize>(index.get_global_id(0));
                           if (i >= SimulationControl::kConservedVariableNumber) {
                             return;
                           }
                           this->residual_(i) = 0.0_r;
                         });
      })
      .wait();
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.computeVolumeElementMeanAbsoluteResidual(mesh.line_, this->residual_);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.computeVolumeElementMeanAbsoluteResidual(mesh.triangle_, this->residual_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.computeVolumeElementMeanAbsoluteResidual(mesh.quadrangle_, this->residual_);
    }
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.computeVolumeElementMeanAbsoluteResidual(mesh.tetrahedron_, this->residual_);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.computeVolumeElementMeanAbsoluteResidual(mesh.pyramid_, this->residual_);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.computeVolumeElementMeanAbsoluteResidual(mesh.hexahedron_, this->residual_);
    }
  }
  queue
      .submit([&](sycl::handler& cgh) -> void {
        cgh.parallel_for(getNdRange(SimulationControl::kConservedVariableNumber),
                         [=, this](sycl::nd_item<1> index) -> void {
                           const auto i = static_cast<Isize>(index.get_global_id(0));
                           if (i >= SimulationControl::kConservedVariableNumber) {
                             return;
                           }
                           this->residual_(i) = this->residual_(i) / static_cast<Real>(mesh.volume_element_number_);
                         });
      })
      .wait();
  Utils::transferToHost<Real, SimulationControl::kConservedVariableNumber>(this->residual_, solver.residual_);
}

template <typename VolumeElementTrait, typename SimulationControl>
inline void VolumeElementSolverDevice<VolumeElementTrait, SimulationControl>::transferVolumeElementCoefficientToHost(
    VolumeElementSolver<VolumeElementTrait, SimulationControl>& volume_element_solver) {
  Utils::transferToHost<Real, SimulationControl::kConservedVariableNumber, VolumeElementTrait::kAllBasisFunctionNumber>(
      this->variable_basis_function_coefficient_, volume_element_solver.variable_basis_function_coefficient_);
}

template <typename SimulationControl>
inline void SolverDevice<SimulationControl>::transferCoefficientToHost(Solver<SimulationControl>& solver) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.transferVolumeElementCoefficientToHost(solver.line_);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.transferVolumeElementCoefficientToHost(solver.triangle_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.transferVolumeElementCoefficientToHost(solver.quadrangle_);
    }
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.transferVolumeElementCoefficientToHost(solver.tetrahedron_);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.transferVolumeElementCoefficientToHost(solver.pyramid_);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.transferVolumeElementCoefficientToHost(solver.hexahedron_);
    }
  }
}

template <typename SimulationControl>
[[nodiscard]] inline Real getStageTime(const TimeIntegration<SimulationControl>& time_integration, const int rk_step) {
  return (static_cast<Real>(time_integration.iteration_) +
          TimeIntegration<SimulationControl>::kButcherCoefficients[static_cast<Usize>(rk_step)]) *
         time_integration.delta_time_;
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::stepSolver(Mesh<SimulationControl>& mesh,
                                                  const SourceTerm<SimulationControl>& source_term,
                                                  const TimeIntegration<SimulationControl>& time_integration) {
  this->copyBasisFunctionCoefficient();
  for (int i = 0; i < TimeIntegration<SimulationControl>::kStep; i++) {
    const Real stage_time = getStageTime(time_integration, i);
    if constexpr (SimulationControl::kBoundaryTime == BoundaryTimeEnum::TimeVarying) {
      this->updateBoundaryVariable(mesh, stage_time);
    }
    this->rotateMesh(mesh, stage_time);
    this->updateInterfaceVariable(mesh);
    this->computeVolumeGradientQuadrature(mesh);
    this->computeAdjacencyGradientQuadrature(mesh);
    this->updateGradientBasisFunctionCoefficient(mesh);
    if constexpr (IsNS<SimulationControl::kEquationModel>) {
      this->updateInterfaceVariableGradient(mesh);
    }
    this->computeVolumeQuadrature(mesh, source_term);
    this->computeAdjacencyQuadrature(mesh);
    this->updateBasisFunctionCoefficient(mesh, time_integration, i);
  }
}

template <typename SimulationControl>
inline void SolverDevice<SimulationControl>::stepSolver(MeshDevice<SimulationControl>& mesh,
                                                        const SourceTermDevice<SimulationControl>& source_term,
                                                        const TimeIntegration<SimulationControl>& time_integration) {
  this->copyBasisFunctionCoefficient();
  for (int i = 0; i < TimeIntegration<SimulationControl>::kStep; i++) {
    const Real stage_time = getStageTime(time_integration, i);
    this->rotateMesh(mesh, stage_time);
    this->updateInterfaceVariable(mesh);
    this->computeVolumeGradientQuadrature(mesh);
    this->computeAdjacencyGradientQuadrature(mesh);
    this->updateGradientBasisFunctionCoefficient(mesh);
    if constexpr (IsNS<SimulationControl::kEquationModel>) {
      this->updateInterfaceVariableGradient(mesh);
    }
    this->computeVolumeQuadrature(mesh, source_term);
    this->computeAdjacencyQuadrature(mesh);
    this->updateBasisFunctionCoefficient(mesh, time_integration, i);
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_TIME_INTEGRATION_CPP_
