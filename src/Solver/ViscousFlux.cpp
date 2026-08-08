/**
 * @file ViscousFlux.cpp
 * @brief The header file of SubrosaDG viscous flux.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-12-02
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_VISCOUS_FLUX_HPP
#define SUBROSA_DG_VISCOUS_FLUX_HPP

#include <Eigen/Core>

#include "Solver/BoundaryCondition.cpp"
#include "Solver/PhysicalModel.cpp"
#include "Solver/VariableConvertor.cpp"
#include "Utils/BasicDataType.cpp"
#include "Utils/Enum.cpp"

namespace SubrosaDG {

template <typename SimulationControl>
struct ViscousFlux {
  static void computeRawFlux(
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          quadrature_node_computational_variable,
      const Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          quadrature_node_primitive_variable_gradient,
      Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kConservedVariableNumber>&
          viscous_raw_flux) {
    if constexpr (SimulationControl::kEquationModel == EquationModelEnum::CompressibleNS) {
      RawFlux<SimulationControl>::template getScalarDimension<ConservedVariableEnum::Density>(viscous_raw_flux) =
          Eigen::Vector<Real, SimulationControl::kDimension>::Zero();
      const Eigen::Ref<const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>>
          velocity_gradient =
              VariableGradient<SimulationControl>::template getVectorGradient<PrimitiveVariableEnum::Velocity>(
                  quadrature_node_primitive_variable_gradient);
      const Real temperature =
          PhysicalModel<SimulationControl, PhysicalModelData>::computeTemperatureFromInternalEnergy(
              Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
                  quadrature_node_computational_variable));
      const Real dynamic_viscosity =
          PhysicalModel<SimulationControl, PhysicalModelData>::computeDynamicViscosity(temperature);
      Eigen::Ref<Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>> viscous_stress =
          RawFlux<SimulationControl>::template getVectorDimension<ConservedVariableEnum::Momentum>(viscous_raw_flux);
      viscous_stress =
          dynamic_viscosity * (velocity_gradient + velocity_gradient.transpose()) -
          2.0_r / 3.0_r * dynamic_viscosity * velocity_gradient.trace() *
              Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>::Identity();
      RawFlux<SimulationControl>::template getScalarDimension<ConservedVariableEnum::DensityTotalEnergy>(
          viscous_raw_flux) =
          viscous_stress * Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
                               quadrature_node_computational_variable) +
          PhysicalModel<SimulationControl, PhysicalModelData>::computeThermalConductivity(temperature) *
              VariableGradient<SimulationControl>::template getScalarGradient<PrimitiveVariableEnum::Temperature>(
                  quadrature_node_primitive_variable_gradient);
    }
    if constexpr (SimulationControl::kEquationModel == EquationModelEnum::IncompressibleNS) {
      RawFlux<SimulationControl>::template getScalarDimension<ConservedVariableEnum::Density>(viscous_raw_flux) =
          Eigen::Vector<Real, SimulationControl::kDimension>::Zero();
      const Eigen::Ref<const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>>
          velocity_gradient =
              VariableGradient<SimulationControl>::template getVectorGradient<PrimitiveVariableEnum::Velocity>(
                  quadrature_node_primitive_variable_gradient);
      const Real temperature =
          PhysicalModel<SimulationControl, PhysicalModelData>::computeTemperatureFromInternalEnergy(
              Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
                  quadrature_node_computational_variable));
      const Real dynamic_viscosity =
          PhysicalModel<SimulationControl, PhysicalModelData>::computeDynamicViscosity(temperature);
      RawFlux<SimulationControl>::template getVectorDimension<ConservedVariableEnum::Momentum>(viscous_raw_flux) =
          dynamic_viscosity * (velocity_gradient + velocity_gradient.transpose()) -
          2.0_r / 3.0_r * dynamic_viscosity * velocity_gradient.trace() *
              Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>::Identity();
      RawFlux<SimulationControl>::template getScalarDimension<ConservedVariableEnum::DensityInternalEnergy>(
          viscous_raw_flux) =
          PhysicalModel<SimulationControl, PhysicalModelData>::computeThermalConductivity(temperature) *
          VariableGradient<SimulationControl>::template getScalarGradient<PrimitiveVariableEnum::Temperature>(
              quadrature_node_primitive_variable_gradient);
    }
  }

  static void addNormalFlux(
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          quadrature_node_computational_variable,
      const Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          quadrature_node_primitive_variable_gradient,
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& viscous_normal_flux) {
    Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kConservedVariableNumber> viscous_raw_flux;
    ViscousFlux<SimulationControl>::computeRawFlux(quadrature_node_computational_variable,
                                                   quadrature_node_primitive_variable_gradient, viscous_raw_flux);
    viscous_normal_flux.noalias() += viscous_raw_flux.transpose() * normal_vector;
  }

  static void minusVariableQuadratureRawFlux(
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber> quadrature_node_computational_variable,
      const Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          quadrature_node_primitive_variable_gradient,
      const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>&
          jacobian_transpose_inverse_multiply_determinate_and_weight,
      Eigen::Ref<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, SimulationControl::kDimension>>
          quadrature_node_variable_quadrature) {
    Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kConservedVariableNumber> viscous_raw_flux;
    ViscousFlux<SimulationControl>::computeRawFlux(quadrature_node_computational_variable,
                                                   quadrature_node_primitive_variable_gradient, viscous_raw_flux);
    quadrature_node_variable_quadrature.noalias() -=
        viscous_raw_flux.transpose() * jacobian_transpose_inverse_multiply_determinate_and_weight;
  }

  static void minusVariableInteriorAdjacencyQuadratureNormalFlux(
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      const Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          left_quadrature_node_primitive_variable_gradient,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      const Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          right_quadrature_node_primitive_variable_gradient,
      Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>>
          left_quadrature_node_variable_adjacency_quadrature,
      Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>>
          right_quadrature_node_variable_adjacency_quadrature,
      const Real jacobian_determinant_multiply_weight) {
    Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> viscous_normal_flux;
    viscous_normal_flux.setZero();
    ViscousFlux<SimulationControl>::addNormalFlux(normal_vector, left_quadrature_node_computational_variable,
                                                  left_quadrature_node_primitive_variable_gradient,
                                                  viscous_normal_flux);
    ViscousFlux<SimulationControl>::addNormalFlux(normal_vector, right_quadrature_node_computational_variable,
                                                  right_quadrature_node_primitive_variable_gradient,
                                                  viscous_normal_flux);
    viscous_normal_flux /= 2.0_r;
    left_quadrature_node_variable_adjacency_quadrature.noalias() -=
        viscous_normal_flux * jacobian_determinant_multiply_weight;
    right_quadrature_node_variable_adjacency_quadrature.noalias() +=
        viscous_normal_flux * jacobian_determinant_multiply_weight;
  }

  static void minusVariableBoundaryAdjacencyQuadratureNormalFlux(
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>& left_quadrature_node_computational_variable,
      const Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          left_quadrature_node_primitive_variable_gradient,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          boundary_quadrature_node_computational_variable,
      Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>>
          left_quadrature_node_variable_adjacency_quadrature,
      const BoundaryConditionEnum boundary_condition_type, const Real jacobian_determinant_multiply_weight) {
    Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>
        boundary_quadrature_node_primitive_variable_gradient;
    BoundaryCondition<SimulationControl>::modifyBoundaryVariableForViscousFlux(
        left_quadrature_node_computational_variable, left_quadrature_node_primitive_variable_gradient,
        boundary_quadrature_node_computational_variable, boundary_quadrature_node_primitive_variable_gradient,
        boundary_condition_type);
    Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> viscous_normal_flux;
    viscous_normal_flux.setZero();
    ViscousFlux<SimulationControl>::addNormalFlux(normal_vector, left_quadrature_node_computational_variable,
                                                  left_quadrature_node_primitive_variable_gradient,
                                                  viscous_normal_flux);
    ViscousFlux<SimulationControl>::addNormalFlux(normal_vector, boundary_quadrature_node_computational_variable,
                                                  boundary_quadrature_node_primitive_variable_gradient,
                                                  viscous_normal_flux);
    viscous_normal_flux /= 2.0_r;
    left_quadrature_node_variable_adjacency_quadrature -= viscous_normal_flux * jacobian_determinant_multiply_weight;
  }

  static void minusVariableInterfaceAdjacencyQuadratureNormalFlux(
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      const Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          left_quadrature_node_primitive_variable_gradient,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      const Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          right_quadrature_node_primitive_variable_gradient,
      Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>>
          left_quadrature_node_variable_adjacency_quadrature,
      const Real jacobian_determinant_multiply_weight) {
    Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> viscous_normal_flux;
    viscous_normal_flux.setZero();
    ViscousFlux<SimulationControl>::addNormalFlux(normal_vector, left_quadrature_node_computational_variable,
                                                  left_quadrature_node_primitive_variable_gradient,
                                                  viscous_normal_flux);
    ViscousFlux<SimulationControl>::addNormalFlux(normal_vector, right_quadrature_node_computational_variable,
                                                  right_quadrature_node_primitive_variable_gradient,
                                                  viscous_normal_flux);
    viscous_normal_flux /= 2.0_r;
    left_quadrature_node_variable_adjacency_quadrature.noalias() -=
        viscous_normal_flux * jacobian_determinant_multiply_weight;
  }
};

template <typename SimulationControl>
struct ViscousFluxDevice {
  static void computeRawFlux(
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          quadrature_node_computational_variable,
      const Device::StaticVector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          quadrature_node_primitive_variable_gradient,
      Device::StaticMatrix<Real, SimulationControl::kDimension, SimulationControl::kConservedVariableNumber>&
          viscous_raw_flux) {
    if constexpr (SimulationControl::kEquationModel == EquationModelEnum::CompressibleNS) {
      Device::View<Device::StaticVector<Real, SimulationControl::kDimension>> density_flux =
          RawFluxDevice<SimulationControl>::template getScalarDimension<ConservedVariableEnum::Density>(
              viscous_raw_flux);
      density_flux.setZero();
      const Device::View<const Device::StaticMatrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>>
          velocity_gradient =
              VariableGradientDevice<SimulationControl>::template getVectorGradient<PrimitiveVariableEnum::Velocity>(
                  quadrature_node_primitive_variable_gradient);
      const Real temperature =
          PhysicalModel<SimulationControl, PhysicalModelData>::computeTemperatureFromInternalEnergy(
              VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
                  quadrature_node_computational_variable));
      const Real dynamic_viscosity =
          PhysicalModel<SimulationControl, PhysicalModelData>::computeDynamicViscosity(temperature);
      Device::View<Device::StaticMatrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>>
          viscous_stress =
              RawFluxDevice<SimulationControl>::template getVectorDimension<ConservedVariableEnum::Momentum>(
                  viscous_raw_flux);
      Real velocity_gradient_trace = 0.0_r;
      for (Isize m = 0; m < SimulationControl::kDimension; m++) {
        velocity_gradient_trace += velocity_gradient(m, m);
      }
      for (Isize m = 0; m < SimulationControl::kDimension; m++) {
        for (Isize n = 0; n < SimulationControl::kDimension; n++) {
          viscous_stress(m, n) = dynamic_viscosity * (velocity_gradient(m, n) + velocity_gradient(n, m)) -
                                 2.0_r / 3.0_r * dynamic_viscosity * velocity_gradient_trace * (m == n ? 1.0_r : 0.0_r);
        }
      }
      const Device::View<const Device::StaticVector<Real, SimulationControl::kDimension>> velocity =
          VariableDevice<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
              quadrature_node_computational_variable);
      const Device::View<const Device::StaticVector<Real, SimulationControl::kDimension>> temperature_gradient =
          VariableGradientDevice<SimulationControl>::template getScalarGradient<PrimitiveVariableEnum::Temperature>(
              quadrature_node_primitive_variable_gradient);
      const Real thermal_conductivity =
          PhysicalModel<SimulationControl, PhysicalModelData>::computeThermalConductivity(temperature);
      Device::View<Device::StaticVector<Real, SimulationControl::kDimension>> energy_flux =
          RawFluxDevice<SimulationControl>::template getScalarDimension<ConservedVariableEnum::DensityTotalEnergy>(
              viscous_raw_flux);
      for (Isize m = 0; m < SimulationControl::kDimension; m++) {
        energy_flux(m) = 0.0_r;
        for (Isize n = 0; n < SimulationControl::kDimension; n++) {
          energy_flux(m) += viscous_stress(m, n) * velocity(n);
        }
        energy_flux(m) += thermal_conductivity * temperature_gradient(m);
      }
    }
    if constexpr (SimulationControl::kEquationModel == EquationModelEnum::IncompressibleNS) {
      Device::View<Device::StaticVector<Real, SimulationControl::kDimension>> density_flux =
          RawFluxDevice<SimulationControl>::template getScalarDimension<ConservedVariableEnum::Density>(
              viscous_raw_flux);
      density_flux.setZero();
      const Device::View<const Device::StaticMatrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>>
          velocity_gradient =
              VariableGradientDevice<SimulationControl>::template getVectorGradient<PrimitiveVariableEnum::Velocity>(
                  quadrature_node_primitive_variable_gradient);
      const Real temperature =
          PhysicalModel<SimulationControl, PhysicalModelData>::computeTemperatureFromInternalEnergy(
              VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
                  quadrature_node_computational_variable));
      const Real dynamic_viscosity =
          PhysicalModel<SimulationControl, PhysicalModelData>::computeDynamicViscosity(temperature);
      Device::View<Device::StaticMatrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>>
          viscous_stress =
              RawFluxDevice<SimulationControl>::template getVectorDimension<ConservedVariableEnum::Momentum>(
                  viscous_raw_flux);
      Real velocity_gradient_trace = 0.0_r;
      for (Isize m = 0; m < SimulationControl::kDimension; m++) {
        velocity_gradient_trace += velocity_gradient(m, m);
      }
      for (Isize m = 0; m < SimulationControl::kDimension; m++) {
        for (Isize n = 0; n < SimulationControl::kDimension; n++) {
          viscous_stress(m, n) = dynamic_viscosity * (velocity_gradient(m, n) + velocity_gradient(n, m)) -
                                 2.0_r / 3.0_r * dynamic_viscosity * velocity_gradient_trace * (m == n ? 1.0_r : 0.0_r);
        }
      }
      const Device::View<const Device::StaticVector<Real, SimulationControl::kDimension>> temperature_gradient =
          VariableGradientDevice<SimulationControl>::template getScalarGradient<PrimitiveVariableEnum::Temperature>(
              quadrature_node_primitive_variable_gradient);
      const Real thermal_conductivity =
          PhysicalModel<SimulationControl, PhysicalModelData>::computeThermalConductivity(temperature);
      Device::View<Device::StaticVector<Real, SimulationControl::kDimension>> energy_flux =
          RawFluxDevice<SimulationControl>::template getScalarDimension<ConservedVariableEnum::DensityInternalEnergy>(
              viscous_raw_flux);
      for (Isize m = 0; m < SimulationControl::kDimension; m++) {
        energy_flux(m) = thermal_conductivity * temperature_gradient(m);
      }
    }
  }

  static void addNormalFlux(
      const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          quadrature_node_computational_variable,
      const Device::StaticVector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          quadrature_node_primitive_variable_gradient,
      Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>& viscous_normal_flux) {
    Device::StaticMatrix<Real, SimulationControl::kDimension, SimulationControl::kConservedVariableNumber>
        viscous_raw_flux;
    ViscousFluxDevice<SimulationControl>::computeRawFlux(quadrature_node_computational_variable,
                                                         quadrature_node_primitive_variable_gradient, viscous_raw_flux);
    for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
      for (Isize n = 0; n < SimulationControl::kDimension; n++) {
        viscous_normal_flux(m) += viscous_raw_flux(n, m) * normal_vector(n);
      }
    }
  }

  static void minusVariableQuadratureRawFlux(
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          quadrature_node_computational_variable,
      const Device::StaticVector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          quadrature_node_primitive_variable_gradient,
      const Device::View<const Device::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>>
          jacobian_transpose_inverse_multiply_determinate_and_weight,
      Device::View<Device::Matrix<Real, SimulationControl::kConservedVariableNumber, SimulationControl::kDimension>>
          quadrature_node_variable_quadrature) {
    Device::StaticMatrix<Real, SimulationControl::kDimension, SimulationControl::kConservedVariableNumber>
        viscous_raw_flux;
    ViscousFluxDevice<SimulationControl>::computeRawFlux(quadrature_node_computational_variable,
                                                         quadrature_node_primitive_variable_gradient, viscous_raw_flux);
    for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
      for (Isize n = 0; n < SimulationControl::kDimension; n++) {
        Real sum = 0.0_r;
        for (Isize k = 0; k < SimulationControl::kDimension; k++) {
          sum += viscous_raw_flux(k, m) * jacobian_transpose_inverse_multiply_determinate_and_weight(k, n);
        }
        quadrature_node_variable_quadrature(m, n) -= sum;
      }
    }
  }

  static void minusVariableInteriorAdjacencyQuadratureNormalFlux(
      const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      const Device::StaticVector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          left_quadrature_node_primitive_variable_gradient,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      const Device::StaticVector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          right_quadrature_node_primitive_variable_gradient,
      Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber>>
          left_quadrature_node_variable_adjacency_quadrature,
      Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber>>
          right_quadrature_node_variable_adjacency_quadrature,
      const Real jacobian_determinant_multiply_weight) {
    Device::StaticVector<Real, SimulationControl::kConservedVariableNumber> viscous_normal_flux;
    viscous_normal_flux.setZero();
    ViscousFluxDevice<SimulationControl>::addNormalFlux(normal_vector, left_quadrature_node_computational_variable,
                                                        left_quadrature_node_primitive_variable_gradient,
                                                        viscous_normal_flux);
    ViscousFluxDevice<SimulationControl>::addNormalFlux(normal_vector, right_quadrature_node_computational_variable,
                                                        right_quadrature_node_primitive_variable_gradient,
                                                        viscous_normal_flux);
    for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
      viscous_normal_flux(m) /= 2.0_r;
      left_quadrature_node_variable_adjacency_quadrature(m) -=
          viscous_normal_flux(m) * jacobian_determinant_multiply_weight;
      right_quadrature_node_variable_adjacency_quadrature(m) +=
          viscous_normal_flux(m) * jacobian_determinant_multiply_weight;
    }
  }

  static void minusVariableBoundaryAdjacencyQuadratureNormalFlux(
      const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector,
      Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      const Device::StaticVector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          left_quadrature_node_primitive_variable_gradient,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          boundary_quadrature_node_computational_variable,
      Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber>>
          left_quadrature_node_variable_adjacency_quadrature,
      const BoundaryConditionEnum boundary_condition_type, const Real jacobian_determinant_multiply_weight) {
    Device::StaticVector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>
        boundary_quadrature_node_primitive_variable_gradient;
    BoundaryConditionDevice<SimulationControl>::modifyBoundaryVariableForViscousFlux(
        left_quadrature_node_computational_variable, left_quadrature_node_primitive_variable_gradient,
        boundary_quadrature_node_computational_variable, boundary_quadrature_node_primitive_variable_gradient,
        boundary_condition_type);
    Device::StaticVector<Real, SimulationControl::kConservedVariableNumber> viscous_normal_flux;
    viscous_normal_flux.setZero();
    ViscousFluxDevice<SimulationControl>::addNormalFlux(normal_vector, left_quadrature_node_computational_variable,
                                                        left_quadrature_node_primitive_variable_gradient,
                                                        viscous_normal_flux);
    ViscousFluxDevice<SimulationControl>::addNormalFlux(normal_vector, boundary_quadrature_node_computational_variable,
                                                        boundary_quadrature_node_primitive_variable_gradient,
                                                        viscous_normal_flux);
    for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
      viscous_normal_flux(m) /= 2.0_r;
      left_quadrature_node_variable_adjacency_quadrature(m) -=
          viscous_normal_flux(m) * jacobian_determinant_multiply_weight;
    }
  }

  static void minusVariableInterfaceAdjacencyQuadratureNormalFlux(
      const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      const Device::StaticVector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          left_quadrature_node_primitive_variable_gradient,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      const Device::StaticVector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          right_quadrature_node_primitive_variable_gradient,
      Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber>>
          left_quadrature_node_variable_adjacency_quadrature,
      const Real jacobian_determinant_multiply_weight) {
    Device::StaticVector<Real, SimulationControl::kConservedVariableNumber> viscous_normal_flux;
    viscous_normal_flux.setZero();
    ViscousFluxDevice<SimulationControl>::addNormalFlux(normal_vector, left_quadrature_node_computational_variable,
                                                        left_quadrature_node_primitive_variable_gradient,
                                                        viscous_normal_flux);
    ViscousFluxDevice<SimulationControl>::addNormalFlux(normal_vector, right_quadrature_node_computational_variable,
                                                        right_quadrature_node_primitive_variable_gradient,
                                                        viscous_normal_flux);
    for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
      viscous_normal_flux(m) /= 2.0_r;
      left_quadrature_node_variable_adjacency_quadrature(m) -=
          viscous_normal_flux(m) * jacobian_determinant_multiply_weight;
    }
  }
};

template <typename SimulationControl>
struct GradientFlux {
  static void computeVolumeGradientRawFlux(
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& left_quadrature_node_conserved_variable,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& right_quadrature_node_conserved_variable,
      Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kConservedVariableNumber>&
          gradient_raw_flux) {
    gradient_raw_flux.noalias() =
        normal_vector *
        (left_quadrature_node_conserved_variable + right_quadrature_node_conserved_variable).transpose() / 2.0_r;
  }

  static void computeInterfaceGradientRawFlux(
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& left_quadrature_node_conserved_variable,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& right_quadrature_node_conserved_variable,
      Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kConservedVariableNumber>&
          gradient_raw_flux) {
    gradient_raw_flux.noalias() =
        normal_vector *
        (right_quadrature_node_conserved_variable - left_quadrature_node_conserved_variable).transpose() / 2.0_r;
  }

  static void computeVariableVolumeGradientInteriorAdjacencyQuadratureNormalFlux(
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& left_quadrature_node_conserved_variable,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& right_quadrature_node_conserved_variable,
      Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
          left_quadrature_node_variable_volume_gradient_adjacency_quadrature,
      Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
          right_quadrature_node_variable_volume_gradient_adjacency_quadrature,
      const Real jacobian_determinant_multiply_weight) {
    Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kConservedVariableNumber> gradient_raw_flux;
    GradientFlux<SimulationControl>::computeVolumeGradientRawFlux(
        normal_vector, left_quadrature_node_conserved_variable, right_quadrature_node_conserved_variable,
        gradient_raw_flux);
    left_quadrature_node_variable_volume_gradient_adjacency_quadrature =
        gradient_raw_flux.reshaped() * jacobian_determinant_multiply_weight;
    right_quadrature_node_variable_volume_gradient_adjacency_quadrature =
        -gradient_raw_flux.reshaped() * jacobian_determinant_multiply_weight;
  }

  static void computeVariableVolumeGradientBoundaryAdjacencyQuadratureNormalFlux(
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Eigen::Vector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& left_quadrature_node_conserved_variable,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>&
          boundary_quadrature_node_interface_gradient_variable,
      Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
          left_quadrature_node_variable_volume_gradient_adjacency_quadrature,
      const BoundaryConditionEnum boundary_condition_type, const Real jacobian_determinant_multiply_weight) {
    Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> boundary_quadrature_node_volume_gradient_variable;
    BoundaryCondition<SimulationControl>::computeBoundaryGradientVariable(
        normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_conserved_variable,
        right_quadrature_node_computational_variable, boundary_quadrature_node_volume_gradient_variable,
        boundary_quadrature_node_interface_gradient_variable, boundary_condition_type);
    left_quadrature_node_variable_volume_gradient_adjacency_quadrature =
        (normal_vector * boundary_quadrature_node_volume_gradient_variable.transpose()).reshaped() *
        jacobian_determinant_multiply_weight;
  }

  static void computeVariableVolumeGradientInterfaceAdjacencyQuadratureNormalFlux(
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& left_quadrature_node_conserved_variable,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& right_quadrature_node_conserved_variable,
      Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
          left_quadrature_node_variable_volume_gradient_adjacency_quadrature,
      const Real jacobian_determinant_multiply_weight) {
    Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kConservedVariableNumber> gradient_raw_flux;
    GradientFlux<SimulationControl>::computeVolumeGradientRawFlux(
        normal_vector, left_quadrature_node_conserved_variable, right_quadrature_node_conserved_variable,
        gradient_raw_flux);
    left_quadrature_node_variable_volume_gradient_adjacency_quadrature =
        gradient_raw_flux.reshaped() * jacobian_determinant_multiply_weight;
  }

  static void computeVariableInterfaceGradientInteriorAdjacencyQuadratureNormalFlux(
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& left_quadrature_node_conserved_variable,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& right_quadrature_node_conserved_variable,
      Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
          left_quadrature_node_variable_interface_gradient_adjacency_quadrature,
      Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
          right_quadrature_node_variable_interface_gradient_adjacency_quadrature,
      const Real jacobian_determinant_multiply_weight) {
    Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kConservedVariableNumber> gradient_raw_flux;
    GradientFlux<SimulationControl>::computeInterfaceGradientRawFlux(
        normal_vector, left_quadrature_node_conserved_variable, right_quadrature_node_conserved_variable,
        gradient_raw_flux);
    // NOTE: Here h_int(u^-,u^+;n)=(u^+ - u^-)n/2, so the gradient flux on the left and right have the same sign
    left_quadrature_node_variable_interface_gradient_adjacency_quadrature =
        gradient_raw_flux.reshaped() * jacobian_determinant_multiply_weight;
    right_quadrature_node_variable_interface_gradient_adjacency_quadrature =
        gradient_raw_flux.reshaped() * jacobian_determinant_multiply_weight;
  }

  static void computeVariableInterfaceGradientBoundaryAdjacencyQuadratureNormalFlux(
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>&
          boundary_quadrature_node_interface_gradient_variable,
      Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
          left_quadrature_node_variable_interface_gradient_adjacency_quadrature,
      const Real jacobian_determinant_multiply_weight) {
    left_quadrature_node_variable_interface_gradient_adjacency_quadrature =
        (normal_vector * boundary_quadrature_node_interface_gradient_variable.transpose()).reshaped() *
        jacobian_determinant_multiply_weight;
  }

  static void computeVariableInterfaceGradientInterfaceAdjacencyQuadratureNormalFlux(
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& left_quadrature_node_conserved_variable,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& right_quadrature_node_conserved_variable,
      Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
          left_quadrature_node_variable_interface_gradient_adjacency_quadrature,
      const Real jacobian_determinant_multiply_weight) {
    Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kConservedVariableNumber> gradient_raw_flux;
    GradientFlux<SimulationControl>::computeInterfaceGradientRawFlux(
        normal_vector, left_quadrature_node_conserved_variable, right_quadrature_node_conserved_variable,
        gradient_raw_flux);
    left_quadrature_node_variable_interface_gradient_adjacency_quadrature =
        gradient_raw_flux.reshaped() * jacobian_determinant_multiply_weight;
  }
};

template <typename SimulationControl>
struct GradientFluxDevice {
  static void computeVolumeGradientRawFlux(
      const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector,
      const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          left_quadrature_node_conserved_variable,
      const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          right_quadrature_node_conserved_variable,
      Device::StaticMatrix<Real, SimulationControl::kDimension, SimulationControl::kConservedVariableNumber>&
          gradient_raw_flux) {
    for (Isize m = 0; m < SimulationControl::kDimension; m++) {
      for (Isize n = 0; n < SimulationControl::kConservedVariableNumber; n++) {
        gradient_raw_flux(m, n) =
            normal_vector(m) *
            (left_quadrature_node_conserved_variable(n) + right_quadrature_node_conserved_variable(n)) / 2.0_r;
      }
    }
  }

  static void computeInterfaceGradientRawFlux(
      const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector,
      const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          left_quadrature_node_conserved_variable,
      const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          right_quadrature_node_conserved_variable,
      Device::StaticMatrix<Real, SimulationControl::kDimension, SimulationControl::kConservedVariableNumber>&
          gradient_raw_flux) {
    for (Isize m = 0; m < SimulationControl::kDimension; m++) {
      for (Isize n = 0; n < SimulationControl::kConservedVariableNumber; n++) {
        gradient_raw_flux(m, n) =
            normal_vector(m) *
            (right_quadrature_node_conserved_variable(n) - left_quadrature_node_conserved_variable(n)) / 2.0_r;
      }
    }
  }

  static void computeVariableVolumeGradientInteriorAdjacencyQuadratureNormalFlux(
      const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector,
      const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          left_quadrature_node_conserved_variable,
      const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          right_quadrature_node_conserved_variable,
      Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
          left_quadrature_node_variable_volume_gradient_adjacency_quadrature,
      Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
          right_quadrature_node_variable_volume_gradient_adjacency_quadrature,
      const Real jacobian_determinant_multiply_weight) {
    Device::StaticMatrix<Real, SimulationControl::kDimension, SimulationControl::kConservedVariableNumber>
        gradient_raw_flux;
    GradientFluxDevice<SimulationControl>::computeVolumeGradientRawFlux(
        normal_vector, left_quadrature_node_conserved_variable, right_quadrature_node_conserved_variable,
        gradient_raw_flux);
    for (Isize m = 0; m < SimulationControl::kDimension; m++) {
      for (Isize n = 0; n < SimulationControl::kConservedVariableNumber; n++) {
        left_quadrature_node_variable_volume_gradient_adjacency_quadrature(m + n * SimulationControl::kDimension) =
            gradient_raw_flux(m, n) * jacobian_determinant_multiply_weight;
        right_quadrature_node_variable_volume_gradient_adjacency_quadrature(m + n * SimulationControl::kDimension) =
            -gradient_raw_flux(m, n) * jacobian_determinant_multiply_weight;
      }
    }
  }

  static void computeVariableVolumeGradientBoundaryAdjacencyQuadratureNormalFlux(
      const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector,
      const Device::StaticVector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          left_quadrature_node_conserved_variable,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          boundary_quadrature_node_interface_gradient_variable,
      Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
          left_quadrature_node_variable_volume_gradient_adjacency_quadrature,
      const BoundaryConditionEnum boundary_condition_type, const Real jacobian_determinant_multiply_weight) {
    Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>
        boundary_quadrature_node_volume_gradient_variable;
    BoundaryConditionDevice<SimulationControl>::computeBoundaryGradientVariable(
        normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_conserved_variable,
        right_quadrature_node_computational_variable, boundary_quadrature_node_volume_gradient_variable,
        boundary_quadrature_node_interface_gradient_variable, boundary_condition_type);
    for (Isize m = 0; m < SimulationControl::kDimension; m++) {
      for (Isize n = 0; n < SimulationControl::kConservedVariableNumber; n++) {
        left_quadrature_node_variable_volume_gradient_adjacency_quadrature(m + n * SimulationControl::kDimension) =
            normal_vector(m) * boundary_quadrature_node_volume_gradient_variable(n) *
            jacobian_determinant_multiply_weight;
      }
    }
  }

  static void computeVariableVolumeGradientInterfaceAdjacencyQuadratureNormalFlux(
      const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector,
      const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          left_quadrature_node_conserved_variable,
      const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          right_quadrature_node_conserved_variable,
      Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
          left_quadrature_node_variable_volume_gradient_adjacency_quadrature,
      const Real jacobian_determinant_multiply_weight) {
    Device::StaticMatrix<Real, SimulationControl::kDimension, SimulationControl::kConservedVariableNumber>
        gradient_raw_flux;
    GradientFluxDevice<SimulationControl>::computeVolumeGradientRawFlux(
        normal_vector, left_quadrature_node_conserved_variable, right_quadrature_node_conserved_variable,
        gradient_raw_flux);
    for (Isize m = 0; m < SimulationControl::kDimension; m++) {
      for (Isize n = 0; n < SimulationControl::kConservedVariableNumber; n++) {
        left_quadrature_node_variable_volume_gradient_adjacency_quadrature(m + n * SimulationControl::kDimension) =
            gradient_raw_flux(m, n) * jacobian_determinant_multiply_weight;
      }
    }
  }

  static void computeVariableInterfaceGradientInteriorAdjacencyQuadratureNormalFlux(
      const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector,
      const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          left_quadrature_node_conserved_variable,
      const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          right_quadrature_node_conserved_variable,
      Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
          left_quadrature_node_variable_interface_gradient_adjacency_quadrature,
      Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
          right_quadrature_node_variable_interface_gradient_adjacency_quadrature,
      const Real jacobian_determinant_multiply_weight) {
    Device::StaticMatrix<Real, SimulationControl::kDimension, SimulationControl::kConservedVariableNumber>
        gradient_raw_flux;
    GradientFluxDevice<SimulationControl>::computeInterfaceGradientRawFlux(
        normal_vector, left_quadrature_node_conserved_variable, right_quadrature_node_conserved_variable,
        gradient_raw_flux);
    for (Isize m = 0; m < SimulationControl::kDimension; m++) {
      for (Isize n = 0; n < SimulationControl::kConservedVariableNumber; n++) {
        left_quadrature_node_variable_interface_gradient_adjacency_quadrature(m + n * SimulationControl::kDimension) =
            gradient_raw_flux(m, n) * jacobian_determinant_multiply_weight;
        right_quadrature_node_variable_interface_gradient_adjacency_quadrature(m + n * SimulationControl::kDimension) =
            gradient_raw_flux(m, n) * jacobian_determinant_multiply_weight;
      }
    }
  }

  static void computeVariableInterfaceGradientBoundaryAdjacencyQuadratureNormalFlux(
      const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector,
      const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          boundary_quadrature_node_interface_gradient_variable,
      Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
          left_quadrature_node_variable_interface_gradient_adjacency_quadrature,
      const Real jacobian_determinant_multiply_weight) {
    for (Isize m = 0; m < SimulationControl::kDimension; m++) {
      for (Isize n = 0; n < SimulationControl::kConservedVariableNumber; n++) {
        left_quadrature_node_variable_interface_gradient_adjacency_quadrature(m + n * SimulationControl::kDimension) =
            normal_vector(m) * boundary_quadrature_node_interface_gradient_variable(n) *
            jacobian_determinant_multiply_weight;
      }
    }
  }

  static void computeVariableInterfaceGradientInterfaceAdjacencyQuadratureNormalFlux(
      const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector,
      const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          left_quadrature_node_conserved_variable,
      const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          right_quadrature_node_conserved_variable,
      Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
          left_quadrature_node_variable_interface_gradient_adjacency_quadrature,
      const Real jacobian_determinant_multiply_weight) {
    Device::StaticMatrix<Real, SimulationControl::kDimension, SimulationControl::kConservedVariableNumber>
        gradient_raw_flux;
    GradientFluxDevice<SimulationControl>::computeInterfaceGradientRawFlux(
        normal_vector, left_quadrature_node_conserved_variable, right_quadrature_node_conserved_variable,
        gradient_raw_flux);
    for (Isize m = 0; m < SimulationControl::kDimension; m++) {
      for (Isize n = 0; n < SimulationControl::kConservedVariableNumber; n++) {
        left_quadrature_node_variable_interface_gradient_adjacency_quadrature(m + n * SimulationControl::kDimension) =
            gradient_raw_flux(m, n) * jacobian_determinant_multiply_weight;
      }
    }
  }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_VISCOUS_FLUX_HPP
