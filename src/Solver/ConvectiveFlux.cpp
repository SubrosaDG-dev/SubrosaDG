/**
 * @file ConvectiveFlux.cpp
 * @brief The header file of ConvectiveFlux.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CONVECTIVE_FLUX_CPP_
#define SUBROSA_DG_CONVECTIVE_FLUX_CPP_

#include <Eigen/Core>
#include <algorithm>
#include <cmath>

#include "Solver/BoundaryCondition.cpp"
#include "Solver/PhysicalModel.cpp"
#include "Solver/VariableConvertor.cpp"
#include "Utils/BasicDataType.cpp"
#include "Utils/Enum.cpp"

namespace SubrosaDG {

template <typename SimulationControl>
struct ConvectiveFlux {
  static void computeRawFlux(const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
                                 quadrature_node_computational_variable,
                             Eigen::Matrix<Real, SimulationControl::kDimension,
                                           SimulationControl::kConservedVariableNumber>& convective_raw_flux) {
    if constexpr (IsCompressible<SimulationControl::kEquationModel>) {
      const Real density = Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
          quadrature_node_computational_variable);
      const Eigen::Ref<const Eigen::Vector<Real, SimulationControl::kDimension>> velocity =
          Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
              quadrature_node_computational_variable);
      RawFlux<SimulationControl>::template getScalarDimension<ConservedVariableEnum::Density>(convective_raw_flux) =
          density * velocity;
      const Real pressure = Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
          quadrature_node_computational_variable);
      RawFlux<SimulationControl>::template getVectorDimension<ConservedVariableEnum::Momentum>(convective_raw_flux) =
          density * velocity * velocity.transpose() +
          pressure * Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>::Identity();
      const Real total_energy =
          Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
              quadrature_node_computational_variable) +
          velocity.squaredNorm() / 2.0_r;
      RawFlux<SimulationControl>::template getScalarDimension<ConservedVariableEnum::DensityTotalEnergy>(
          convective_raw_flux) = (density * total_energy + pressure) * velocity;
    }
    if constexpr (IsIncompressible<SimulationControl::kEquationModel>) {
      const Real density = Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
          quadrature_node_computational_variable);
      const Eigen::Ref<const Eigen::Vector<Real, SimulationControl::kDimension>> velocity =
          Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
              quadrature_node_computational_variable);
      RawFlux<SimulationControl>::template getScalarDimension<ConservedVariableEnum::Density>(convective_raw_flux) =
          density * velocity;
      const Real pressure = Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
          quadrature_node_computational_variable);
      RawFlux<SimulationControl>::template getVectorDimension<ConservedVariableEnum::Momentum>(convective_raw_flux) =
          density * velocity * velocity.transpose() +
          pressure * Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>::Identity();
      RawFlux<SimulationControl>::template getScalarDimension<ConservedVariableEnum::DensityInternalEnergy>(
          convective_raw_flux) =
          density *
          Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
              quadrature_node_computational_variable) *
          velocity;
    }
  }

  static void minusRawALEFlux(
      const Eigen::Vector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& quadrature_node_conserved_variable,
      Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kConservedVariableNumber>&
          convective_raw_flux) {
    convective_raw_flux -= quadrature_node_rotation_velocity * quadrature_node_conserved_variable.transpose();
  }

  static void addNormalFlux(const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                            const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
                                quadrature_node_computational_variable,
                            Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& convective_normal_flux) {
    if constexpr (IsCompressible<SimulationControl::kEquationModel>) {
      const Real density = Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
          quadrature_node_computational_variable);
      const Eigen::Ref<const Eigen::Vector<Real, SimulationControl::kDimension>> velocity =
          Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
              quadrature_node_computational_variable);
      const Real normal_velocity = velocity.transpose() * normal_vector;
      NormalFlux<SimulationControl>::template getScalar<ConservedVariableEnum::Density>(convective_normal_flux) +=
          density * normal_velocity;
      const Real pressure = Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
          quadrature_node_computational_variable);
      NormalFlux<SimulationControl>::template getVector<ConservedVariableEnum::Momentum>(convective_normal_flux) +=
          density * normal_velocity * velocity + pressure * normal_vector;
      const Real total_energy =
          Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
              quadrature_node_computational_variable) +
          velocity.squaredNorm() / 2.0_r;
      NormalFlux<SimulationControl>::template getScalar<ConservedVariableEnum::DensityTotalEnergy>(
          convective_normal_flux) += (density * total_energy + pressure) * normal_velocity;
    }
    if constexpr (IsIncompressible<SimulationControl::kEquationModel>) {
      const Real density = Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
          quadrature_node_computational_variable);
      const Eigen::Ref<const Eigen::Vector<Real, SimulationControl::kDimension>> velocity =
          Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
              quadrature_node_computational_variable);
      const Real normal_velocity = velocity.transpose() * normal_vector;
      NormalFlux<SimulationControl>::template getScalar<ConservedVariableEnum::Density>(convective_normal_flux) +=
          density * normal_velocity;
      const Real pressure = Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
          quadrature_node_computational_variable);
      NormalFlux<SimulationControl>::template getVector<ConservedVariableEnum::Momentum>(convective_normal_flux) +=
          density * normal_velocity * velocity + pressure * normal_vector;
      NormalFlux<SimulationControl>::template getScalar<ConservedVariableEnum::DensityInternalEnergy>(
          convective_normal_flux) +=
          density *
          Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
              quadrature_node_computational_variable) *
          normal_velocity;
    }
  }

  static void minusNormalALEFlux(
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Eigen::Vector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& quadrature_node_conserved_variable,
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& convective_normal_flux) {
    const Real rotation_normal_velocity = quadrature_node_rotation_velocity.transpose() * normal_vector;
    convective_normal_flux -= rotation_normal_velocity * quadrature_node_conserved_variable;
  }

  static void computeCentralFlux(
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Eigen::Vector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& left_quadrature_node_conserved_variable,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& right_quadrature_node_conserved_variable,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& convective_normal_flux) {
    convective_normal_flux.setZero();
    ConvectiveFlux<SimulationControl>::addNormalFlux(normal_vector, left_quadrature_node_computational_variable,
                                                     convective_normal_flux);
    ConvectiveFlux<SimulationControl>::minusNormalALEFlux(normal_vector, quadrature_node_rotation_velocity,
                                                          left_quadrature_node_conserved_variable,
                                                          convective_normal_flux);
    ConvectiveFlux<SimulationControl>::addNormalFlux(normal_vector, right_quadrature_node_computational_variable,
                                                     convective_normal_flux);
    ConvectiveFlux<SimulationControl>::minusNormalALEFlux(normal_vector, quadrature_node_rotation_velocity,
                                                          right_quadrature_node_conserved_variable,
                                                          convective_normal_flux);
    convective_normal_flux /= 2.0_r;
  }

  static void computeLaxFriedrichsFlux(
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Eigen::Vector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& left_quadrature_node_conserved_variable,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& right_quadrature_node_conserved_variable,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& convective_normal_flux) {
    convective_normal_flux.setZero();
    ConvectiveFlux<SimulationControl>::addNormalFlux(normal_vector, left_quadrature_node_computational_variable,
                                                     convective_normal_flux);
    ConvectiveFlux<SimulationControl>::minusNormalALEFlux(normal_vector, quadrature_node_rotation_velocity,
                                                          left_quadrature_node_conserved_variable,
                                                          convective_normal_flux);
    ConvectiveFlux<SimulationControl>::addNormalFlux(normal_vector, right_quadrature_node_computational_variable,
                                                     convective_normal_flux);
    ConvectiveFlux<SimulationControl>::minusNormalALEFlux(normal_vector, quadrature_node_rotation_velocity,
                                                          right_quadrature_node_conserved_variable,
                                                          convective_normal_flux);
    const Real left_normal_velocity =
        Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
            left_quadrature_node_computational_variable)
            .transpose() *
        normal_vector;
    const Real right_normal_velocity =
        Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
            right_quadrature_node_computational_variable)
            .transpose() *
        normal_vector;
    const Real left_sound_speed =
        PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(
            Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
                left_quadrature_node_computational_variable),
            Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
                left_quadrature_node_computational_variable));
    const Real right_sound_speed =
        PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(
            Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
                right_quadrature_node_computational_variable),
            Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
                right_quadrature_node_computational_variable));
    const Real rotation_normal_velocity = quadrature_node_rotation_velocity.transpose() * normal_vector;
    const Real spectral_radius =
        std::max(std::fabs(left_normal_velocity - rotation_normal_velocity) + left_sound_speed,
                 std::fabs(right_normal_velocity - rotation_normal_velocity) + right_sound_speed);
    convective_normal_flux -=
        spectral_radius * (right_quadrature_node_conserved_variable - left_quadrature_node_conserved_variable);
    convective_normal_flux /= 2.0_r;
  }

  static void computeHLLCFlux(
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Eigen::Vector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>&
          left_quadrature_node_conserved_variable,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>&
          right_quadrature_node_conserved_variable,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& convective_normal_flux) {
    convective_normal_flux.setZero();
    Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> contact_conserved_variable;
    const Real left_density = Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
        left_quadrature_node_computational_variable);
    const Real right_density = Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
        right_quadrature_node_computational_variable);
    const Real left_sqrt_density = std::sqrt(left_density);
    const Real right_sqrt_density = std::sqrt(right_density);
    const Eigen::Ref<const Eigen::Vector<Real, SimulationControl::kDimension>> left_velocity =
        Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
            left_quadrature_node_computational_variable);
    const Eigen::Ref<const Eigen::Vector<Real, SimulationControl::kDimension>> right_velocity =
        Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
            right_quadrature_node_computational_variable);
    const Real rotation_normal_velocity = quadrature_node_rotation_velocity.transpose() * normal_vector;
    const Eigen::Vector<Real, SimulationControl::kDimension> roe_velocity =
        (left_sqrt_density * left_velocity + right_sqrt_density * right_velocity) /
        (left_sqrt_density + right_sqrt_density);
    const Real roe_normal_velocity = roe_velocity.transpose() * normal_vector;
    const Real left_pressure = Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
        left_quadrature_node_computational_variable);
    const Real right_pressure = Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
        right_quadrature_node_computational_variable);
    const Real left_sound_speed =
        PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(left_density,
                                                                                                  left_pressure);
    const Real right_sound_speed =
        PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(right_density,
                                                                                                  right_pressure);
    const Real left_total_energy =
        Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
            left_quadrature_node_computational_variable) +
        left_velocity.squaredNorm() / 2.0_r;
    const Real right_total_energy =
        Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
            right_quadrature_node_computational_variable) +
        right_velocity.squaredNorm() / 2.0_r;
    const Real left_total_enthalpy = left_total_energy + left_pressure / left_density;
    const Real right_total_enthalpy = right_total_energy + right_pressure / right_density;
    const Real roe_total_enthalpy =
        (left_sqrt_density * left_total_enthalpy + right_sqrt_density * right_total_enthalpy) /
        (left_sqrt_density + right_sqrt_density);
    const Real specific_heat_ratio = PhysicalModel<SimulationControl, PhysicalModelData>::getSpecificHeatRatio();
    const Real roe_sound_speed =
        std::sqrt((specific_heat_ratio - 1.0_r) * (roe_total_enthalpy - roe_velocity.squaredNorm() / 2.0_r));
    const Eigen::Vector<Real, SimulationControl::kDimension> left_relative_velocity =
        left_velocity - quadrature_node_rotation_velocity;
    const Eigen::Vector<Real, SimulationControl::kDimension> right_relative_velocity =
        right_velocity - quadrature_node_rotation_velocity;
    const Real left_relative_normal_velocity = left_relative_velocity.transpose() * normal_vector;
    const Real right_relative_normal_velocity = right_relative_velocity.transpose() * normal_vector;
    const Real left_wave_speed = std::min(left_relative_normal_velocity - left_sound_speed,
                                          roe_normal_velocity - rotation_normal_velocity - roe_sound_speed);
    if (left_wave_speed >= 0.0_r) {
      ConvectiveFlux<SimulationControl>::addNormalFlux(normal_vector, left_quadrature_node_computational_variable,
                                                       convective_normal_flux);
      ConvectiveFlux<SimulationControl>::minusNormalALEFlux(normal_vector, quadrature_node_rotation_velocity,
                                                            left_quadrature_node_conserved_variable,
                                                            convective_normal_flux);
      return;
    }
    const Real right_wave_speed = std::max(right_relative_normal_velocity + right_sound_speed,
                                           roe_normal_velocity - rotation_normal_velocity + roe_sound_speed);
    if (right_wave_speed <= 0.0_r) {
      ConvectiveFlux<SimulationControl>::addNormalFlux(normal_vector, right_quadrature_node_computational_variable,
                                                       convective_normal_flux);
      ConvectiveFlux<SimulationControl>::minusNormalALEFlux(normal_vector, quadrature_node_rotation_velocity,
                                                            right_quadrature_node_conserved_variable,
                                                            convective_normal_flux);
      return;
    }
    const Real contact_wave_speed =
        (right_density * right_relative_normal_velocity * (right_wave_speed - right_relative_normal_velocity) -
         left_density * left_relative_normal_velocity * (left_wave_speed - left_relative_normal_velocity) +
         left_pressure - right_pressure) /
        (right_density * (right_wave_speed - right_relative_normal_velocity) -
         left_density * (left_wave_speed - left_relative_normal_velocity));
    // also can be computed by right_density * (right_relative_normal_velocity - right_wave_speed) *
    // (right_relative_normal_velocity - contact_wave_speed ) + right_pressure
    const Real contact_pressure = left_density * (left_relative_normal_velocity - left_wave_speed) *
                                      (left_relative_normal_velocity - contact_wave_speed) +
                                  left_pressure;
    if (contact_wave_speed >= 0.0_r) {
      Variable<SimulationControl>::template getScalar<ConservedVariableEnum::Density>(contact_conserved_variable) =
          (left_wave_speed - left_relative_normal_velocity) * left_density;
      Variable<SimulationControl>::template getVector<ConservedVariableEnum::Momentum>(contact_conserved_variable) =
          (left_wave_speed - left_relative_normal_velocity) * left_density * left_velocity +
          (contact_pressure - left_pressure) * normal_vector;
      Variable<SimulationControl>::template getScalar<ConservedVariableEnum::DensityTotalEnergy>(
          contact_conserved_variable) =
          (left_wave_speed - left_relative_normal_velocity) * left_density * left_total_energy -
          left_pressure * left_relative_normal_velocity + contact_pressure * contact_wave_speed;
      contact_conserved_variable /= (left_wave_speed - contact_wave_speed);
    } else {
      Variable<SimulationControl>::template getScalar<ConservedVariableEnum::Density>(contact_conserved_variable) =
          (right_wave_speed - right_relative_normal_velocity) * right_density;
      Variable<SimulationControl>::template getVector<ConservedVariableEnum::Momentum>(contact_conserved_variable) =
          (right_wave_speed - right_relative_normal_velocity) * right_density * right_velocity +
          (contact_pressure - right_pressure) * normal_vector;
      Variable<SimulationControl>::template getScalar<ConservedVariableEnum::DensityTotalEnergy>(
          contact_conserved_variable) =
          (right_wave_speed - right_relative_normal_velocity) * right_density * right_total_energy -
          right_pressure * right_relative_normal_velocity + contact_pressure * contact_wave_speed;
      contact_conserved_variable /= (right_wave_speed - contact_wave_speed);
    }
    convective_normal_flux.noalias() = contact_wave_speed * contact_conserved_variable;
    NormalFlux<SimulationControl>::template getVector<ConservedVariableEnum::Momentum>(convective_normal_flux) +=
        contact_pressure * normal_vector;
    NormalFlux<SimulationControl>::template getScalar<ConservedVariableEnum::DensityTotalEnergy>(
        convective_normal_flux) += contact_pressure * (contact_wave_speed + rotation_normal_velocity);
  }

  static void computeRoeFlux(const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                             [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>&
                                 left_quadrature_node_conserved_variable,
                             const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
                                 left_quadrature_node_computational_variable,
                             [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>&
                                 right_quadrature_node_conserved_variable,
                             const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
                                 right_quadrature_node_computational_variable,
                             Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& convective_normal_flux) {
    convective_normal_flux.setZero();
    ConvectiveFlux<SimulationControl>::addNormalFlux(normal_vector, left_quadrature_node_computational_variable,
                                                     convective_normal_flux);
    ConvectiveFlux<SimulationControl>::addNormalFlux(normal_vector, right_quadrature_node_computational_variable,
                                                     convective_normal_flux);
    Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, SimulationControl::kConservedVariableNumber>
        roe_matrix;
    roe_matrix.setZero();
    const Real left_sqrt_density =
        std::sqrt(Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
            left_quadrature_node_computational_variable));
    const Real right_sqrt_density =
        std::sqrt(Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
            right_quadrature_node_computational_variable));
    const Real roe_density =
        std::sqrt(Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
                      left_quadrature_node_computational_variable) *
                  Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
                      right_quadrature_node_computational_variable));
    const Eigen::Ref<const Eigen::Vector<Real, SimulationControl::kDimension>> left_velocity =
        Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
            left_quadrature_node_computational_variable);
    const Eigen::Ref<const Eigen::Vector<Real, SimulationControl::kDimension>> right_velocity =
        Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
            right_quadrature_node_computational_variable);
    const Eigen::Vector<Real, SimulationControl::kDimension> roe_velocity =
        (left_sqrt_density * left_velocity + right_sqrt_density * right_velocity) /
        (left_sqrt_density + right_sqrt_density);
    const Real specific_heat_ratio = PhysicalModel<SimulationControl, PhysicalModelData>::getSpecificHeatRatio();
    const Real left_total_enthalpy =
        Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
            left_quadrature_node_computational_variable) *
            specific_heat_ratio +
        left_velocity.squaredNorm() / 2.0_r;
    const Real right_total_enthalpy =
        Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
            right_quadrature_node_computational_variable) *
            specific_heat_ratio +
        right_velocity.squaredNorm() / 2.0_r;
    const Real roe_total_enthalpy =
        (left_sqrt_density * left_total_enthalpy + right_sqrt_density * right_total_enthalpy) /
        (left_sqrt_density + right_sqrt_density);
    const Real roe_internal_energy = (roe_total_enthalpy - roe_velocity.squaredNorm() / 2.0_r) / specific_heat_ratio;
    const Real roe_pressure =
        PhysicalModel<SimulationControl, PhysicalModelData>::computePressureFromDensityInternalEnergy(
            roe_density, roe_internal_energy);
    const Real roe_normal_velocity = roe_velocity.transpose() * normal_vector;
    const Real roe_sound_speed =
        PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(roe_density,
                                                                                                  roe_pressure);
    const Real delta_density = Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
                                   right_quadrature_node_computational_variable) -
                               Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
                                   left_quadrature_node_computational_variable);
    const Eigen::Vector<Real, SimulationControl::kDimension> delta_velocity = right_velocity - left_velocity;
    const Real delta_pressure = Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
                                    right_quadrature_node_computational_variable) -
                                Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
                                    left_quadrature_node_computational_variable);
    const Real delta_normal_velocity = delta_velocity.transpose() * normal_vector;
    const Real harten_delta = roe_sound_speed / 20.0_r;
    const Real lambda_velocity_subtract_sound_speed =
        std::fabs(roe_normal_velocity - roe_sound_speed) > harten_delta
            ? std::fabs(roe_normal_velocity - roe_sound_speed)
            : ((roe_normal_velocity - roe_sound_speed) * (roe_normal_velocity - roe_sound_speed) +
               harten_delta * harten_delta) /
                  (2.0_r * harten_delta);
    const Real lambda_velocity_add_sound_speed =
        std::fabs(roe_normal_velocity + roe_sound_speed) > harten_delta
            ? std::fabs(roe_normal_velocity + roe_sound_speed)
            : ((roe_normal_velocity + roe_sound_speed) * (roe_normal_velocity + roe_sound_speed) +
               harten_delta * harten_delta) /
                  (2.0_r * harten_delta);
    roe_matrix.col(0) << 1.0_r, roe_velocity - roe_sound_speed * normal_vector,
        roe_total_enthalpy - roe_sound_speed * roe_normal_velocity;
    roe_matrix.col(0) *= lambda_velocity_subtract_sound_speed *
                         (delta_pressure - roe_density * roe_sound_speed * delta_normal_velocity) /
                         (2.0_r * roe_sound_speed * roe_sound_speed);
    roe_matrix.col(1) << 1.0_r, roe_velocity, roe_velocity.squaredNorm() / 2.0_r;
    roe_matrix.col(1) *=
        std::fabs(roe_normal_velocity) * (delta_density - delta_pressure / (roe_sound_speed * roe_sound_speed));
    if constexpr (SimulationControl::kDimension == 2 || SimulationControl::kDimension == 3) {
      roe_matrix.col(2) << 0.0_r, delta_velocity - delta_normal_velocity * normal_vector,
          roe_velocity.transpose() * delta_velocity - roe_normal_velocity * delta_normal_velocity;
      roe_matrix.col(2) *= std::fabs(roe_normal_velocity) * roe_density;
    }
    roe_matrix.col(SimulationControl::kDimension + 1) << 1.0_r, roe_velocity + roe_sound_speed * normal_vector,
        roe_total_enthalpy + roe_sound_speed * roe_normal_velocity;
    roe_matrix.col(SimulationControl::kDimension + 1) *=
        lambda_velocity_add_sound_speed * (delta_pressure + roe_density * roe_sound_speed * delta_normal_velocity) /
        (2.0_r * roe_sound_speed * roe_sound_speed);
    convective_normal_flux -= roe_matrix.rowwise().sum();
    convective_normal_flux /= 2.0_r;
  }

  static void computeExactFlux(
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>&
          left_quadrature_node_conserved_variable,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>&
          right_quadrature_node_conserved_variable,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& convective_normal_flux) {
    convective_normal_flux.setZero();
    Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber> exact_computational_variable;
    const Real sound_speed =
        PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(0.0_r, 0.0_r);
    const Eigen::Ref<const Eigen::Vector<Real, SimulationControl::kDimension>> left_velocity =
        Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
            left_quadrature_node_computational_variable);
    const Eigen::Ref<const Eigen::Vector<Real, SimulationControl::kDimension>> right_velocity =
        Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
            right_quadrature_node_computational_variable);
    const Real left_normal_velocity = left_velocity.transpose() * normal_vector;
    const Real right_normal_velocity = right_velocity.transpose() * normal_vector;
    Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(exact_computational_variable) =
        std::sqrt(Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
                      left_quadrature_node_computational_variable) *
                  Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
                      right_quadrature_node_computational_variable) *
                  std::exp((left_normal_velocity - right_normal_velocity) / sound_speed));
    const Real exact_normal_velocity =
        (left_normal_velocity + right_normal_velocity) / 2.0_r +
        std::log(Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
                     left_quadrature_node_computational_variable) /
                 Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
                     right_quadrature_node_computational_variable)) *
            sound_speed / 2.0_r;
    if (exact_normal_velocity < 0.0_r) {
      Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
          exact_computational_variable) =
          right_velocity + (exact_normal_velocity - right_normal_velocity) * normal_vector;
      Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
          exact_computational_variable) =
          Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
              right_quadrature_node_computational_variable);
    } else {
      Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
          exact_computational_variable) =
          left_velocity + (exact_normal_velocity - left_normal_velocity) * normal_vector;
      Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
          exact_computational_variable) =
          Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
              left_quadrature_node_computational_variable);
    }
    Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(exact_computational_variable) =
        PhysicalModel<SimulationControl, PhysicalModelData>::computePressureFromDensityInternalEnergy(
            Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
                exact_computational_variable),
            Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
                exact_computational_variable));
    ConvectiveFlux<SimulationControl>::addNormalFlux(normal_vector, exact_computational_variable,
                                                     convective_normal_flux);
  }

  static void addVariableQuadratureRawFlux(
      const Eigen::Vector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& quadrature_node_conserved_variable,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber> quadrature_node_computational_variable,
      const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>&
          jacobian_transpose_inverse_multiply_determinate_and_weight,
      Eigen::Ref<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, SimulationControl::kDimension>>
          quadrature_node_variable_quadrature) {
    Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kConservedVariableNumber> convective_raw_flux;
    ConvectiveFlux<SimulationControl>::computeRawFlux(quadrature_node_computational_variable, convective_raw_flux);
    ConvectiveFlux<SimulationControl>::minusRawALEFlux(quadrature_node_rotation_velocity,
                                                       quadrature_node_conserved_variable, convective_raw_flux);
    quadrature_node_variable_quadrature.noalias() +=
        convective_raw_flux.transpose() * jacobian_transpose_inverse_multiply_determinate_and_weight;
  }

  static void addVariableInteriorAdjacencyQuadratureNormalFlux(
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Eigen::Vector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& left_quadrature_node_conserved_variable,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& right_quadrature_node_conserved_variable,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>>
          left_quadrature_node_variable_adjacency_quadrature,
      Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>>
          right_quadrature_node_variable_adjacency_quadrature,
      const Real jacobian_determinant_multiply_weight) {
    Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> convective_normal_flux;
    if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFluxEnum::Central) {
      ConvectiveFlux<SimulationControl>::computeCentralFlux(
          normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_conserved_variable,
          left_quadrature_node_computational_variable, right_quadrature_node_conserved_variable,
          right_quadrature_node_computational_variable, convective_normal_flux);
    } else if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFluxEnum::LaxFriedrichs) {
      ConvectiveFlux<SimulationControl>::computeLaxFriedrichsFlux(
          normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_conserved_variable,
          left_quadrature_node_computational_variable, right_quadrature_node_conserved_variable,
          right_quadrature_node_computational_variable, convective_normal_flux);
    } else if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFluxEnum::HLLC) {
      ConvectiveFlux<SimulationControl>::computeHLLCFlux(
          normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_conserved_variable,
          left_quadrature_node_computational_variable, right_quadrature_node_conserved_variable,
          right_quadrature_node_computational_variable, convective_normal_flux);
    } else if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFluxEnum::Roe) {
      // ConvectiveFlux<SimulationControl>::computeRoeFlux(
      //     normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_conserved_variable,
      //     left_quadrature_node_computational_variable, right_quadrature_node_conserved_variable,
      //     right_quadrature_node_computational_variable, convective_normal_flux);
    } else if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFluxEnum::Exact) {
      // ConvectiveFlux<SimulationControl>::computeExactFlux(
      //     normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_conserved_variable,
      //     left_quadrature_node_computational_variable, right_quadrature_node_conserved_variable,
      //     right_quadrature_node_computational_variable, convective_normal_flux);
    }
    left_quadrature_node_variable_adjacency_quadrature.noalias() +=
        convective_normal_flux * jacobian_determinant_multiply_weight;
    right_quadrature_node_variable_adjacency_quadrature.noalias() -=
        convective_normal_flux * jacobian_determinant_multiply_weight;
  }

  static void addVariableBoundaryAdjacencyQuadratureNormalFlux(
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Eigen::Vector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          boundary_quadrature_node_computational_variable,
      Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>>
          left_quadrature_node_variable_adjacency_quadrature,
      const BoundaryConditionEnum boundary_condition_type, const Real jacobian_determinant_multiply_weight) {
    Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> boundary_quadrature_node_conserved_variable;
    BoundaryCondition<SimulationControl>::computeBoundaryVariable(
        normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_computational_variable,
        right_quadrature_node_computational_variable, boundary_quadrature_node_computational_variable,
        boundary_condition_type);
    Variable<SimulationControl>::convertConservedFromComputational(boundary_quadrature_node_computational_variable,
                                                                   boundary_quadrature_node_conserved_variable);
    Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> convective_normal_flux;
    convective_normal_flux.setZero();
    ConvectiveFlux<SimulationControl>::addNormalFlux(normal_vector, boundary_quadrature_node_computational_variable,
                                                     convective_normal_flux);
    ConvectiveFlux<SimulationControl>::minusNormalALEFlux(normal_vector, quadrature_node_rotation_velocity,
                                                          boundary_quadrature_node_conserved_variable,
                                                          convective_normal_flux);
    left_quadrature_node_variable_adjacency_quadrature += convective_normal_flux * jacobian_determinant_multiply_weight;
  }

  static void addVariableInterfaceAdjacencyQuadratureNormalFlux(
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Eigen::Vector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& left_quadrature_node_conserved_variable,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& right_quadrature_node_conserved_variable,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>>
          left_quadrature_node_variable_adjacency_quadrature,
      const Real jacobian_determinant_multiply_weight) {
    Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> convective_normal_flux;
    if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFluxEnum::Central) {
      ConvectiveFlux<SimulationControl>::computeCentralFlux(
          normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_conserved_variable,
          left_quadrature_node_computational_variable, right_quadrature_node_conserved_variable,
          right_quadrature_node_computational_variable, convective_normal_flux);
    } else if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFluxEnum::LaxFriedrichs) {
      ConvectiveFlux<SimulationControl>::computeLaxFriedrichsFlux(
          normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_conserved_variable,
          left_quadrature_node_computational_variable, right_quadrature_node_conserved_variable,
          right_quadrature_node_computational_variable, convective_normal_flux);
    } else if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFluxEnum::HLLC) {
      ConvectiveFlux<SimulationControl>::computeHLLCFlux(
          normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_conserved_variable,
          left_quadrature_node_computational_variable, right_quadrature_node_conserved_variable,
          right_quadrature_node_computational_variable, convective_normal_flux);
    } else if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFluxEnum::Roe) {
      // ConvectiveFlux<SimulationControl>::computeRoeFlux(
      //     normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_conserved_variable,
      //     left_quadrature_node_computational_variable, right_quadrature_node_conserved_variable,
      //     right_quadrature_node_computational_variable, convective_normal_flux);
    } else if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFluxEnum::Exact) {
      // ConvectiveFlux<SimulationControl>::computeExactFlux(
      //     normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_conserved_variable,
      //     left_quadrature_node_computational_variable, right_quadrature_node_conserved_variable,
      //     right_quadrature_node_computational_variable, convective_normal_flux);
    }
    left_quadrature_node_variable_adjacency_quadrature.noalias() +=
        convective_normal_flux * jacobian_determinant_multiply_weight;
  }
};

template <typename SimulationControl>
struct ConvectiveFluxDevice {
  static void computeRawFlux(const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
                                 quadrature_node_computational_variable,
                             Device::StaticMatrix<Real, SimulationControl::kDimension,
                                                  SimulationControl::kConservedVariableNumber>& convective_raw_flux) {
    if constexpr (IsCompressible<SimulationControl::kEquationModel>) {
      const Real density = VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
          quadrature_node_computational_variable);
      const Device::View<const Device::StaticVector<Real, SimulationControl::kDimension>> velocity =
          VariableDevice<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
              quadrature_node_computational_variable);
      Device::View<Device::StaticVector<Real, SimulationControl::kDimension>> density_flux =
          RawFluxDevice<SimulationControl>::template getScalarDimension<ConservedVariableEnum::Density>(
              convective_raw_flux);
      for (Isize m = 0; m < SimulationControl::kDimension; m++) {
        density_flux(m) = density * velocity(m);
      }
      const Real pressure = VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
          quadrature_node_computational_variable);
      Device::View<Device::StaticMatrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>>
          momentum_flux =
              RawFluxDevice<SimulationControl>::template getVectorDimension<ConservedVariableEnum::Momentum>(
                  convective_raw_flux);
      for (Isize m = 0; m < SimulationControl::kDimension; m++) {
        for (Isize n = 0; n < SimulationControl::kDimension; n++) {
          momentum_flux(m, n) = density * velocity(m) * velocity(n) + (m == n ? pressure : 0.0_r);
        }
      }
      const Real total_energy =
          VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
              quadrature_node_computational_variable) +
          velocity.squaredNorm() / 2.0_r;
      Device::View<Device::StaticVector<Real, SimulationControl::kDimension>> energy_flux =
          RawFluxDevice<SimulationControl>::template getScalarDimension<ConservedVariableEnum::DensityTotalEnergy>(
              convective_raw_flux);
      for (Isize m = 0; m < SimulationControl::kDimension; m++) {
        energy_flux(m) = (density * total_energy + pressure) * velocity(m);
      }
    }
    if constexpr (IsIncompressible<SimulationControl::kEquationModel>) {
      const Real density = VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
          quadrature_node_computational_variable);
      const Device::View<const Device::StaticVector<Real, SimulationControl::kDimension>> velocity =
          VariableDevice<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
              quadrature_node_computational_variable);
      Device::View<Device::StaticVector<Real, SimulationControl::kDimension>> density_flux =
          RawFluxDevice<SimulationControl>::template getScalarDimension<ConservedVariableEnum::Density>(
              convective_raw_flux);
      for (Isize m = 0; m < SimulationControl::kDimension; m++) {
        density_flux(m) = density * velocity(m);
      }
      const Real pressure = VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
          quadrature_node_computational_variable);
      Device::View<Device::StaticMatrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>>
          momentum_flux =
              RawFluxDevice<SimulationControl>::template getVectorDimension<ConservedVariableEnum::Momentum>(
                  convective_raw_flux);
      for (Isize m = 0; m < SimulationControl::kDimension; m++) {
        for (Isize n = 0; n < SimulationControl::kDimension; n++) {
          momentum_flux(m, n) = density * velocity(m) * velocity(n) + (m == n ? pressure : 0.0_r);
        }
      }
      Device::View<Device::StaticVector<Real, SimulationControl::kDimension>> energy_flux =
          RawFluxDevice<SimulationControl>::template getScalarDimension<ConservedVariableEnum::DensityInternalEnergy>(
              convective_raw_flux);
      for (Isize m = 0; m < SimulationControl::kDimension; m++) {
        energy_flux(m) =
            density *
            VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
                quadrature_node_computational_variable) *
            velocity(m);
      }
    }
  }

  static void minusRawALEFlux(
      const Device::StaticVector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>& quadrature_node_conserved_variable,
      Device::StaticMatrix<Real, SimulationControl::kDimension, SimulationControl::kConservedVariableNumber>&
          convective_raw_flux) {
    for (Isize m = 0; m < SimulationControl::kDimension; m++) {
      for (Isize n = 0; n < SimulationControl::kConservedVariableNumber; n++) {
        convective_raw_flux(m, n) -= quadrature_node_rotation_velocity(m) * quadrature_node_conserved_variable(n);
      }
    }
  }

  static void addNormalFlux(
      const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          quadrature_node_computational_variable,
      Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>& convective_normal_flux) {
    if constexpr (IsCompressible<SimulationControl::kEquationModel>) {
      const Real density = VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
          quadrature_node_computational_variable);
      const Device::View<const Device::StaticVector<Real, SimulationControl::kDimension>> velocity =
          VariableDevice<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
              quadrature_node_computational_variable);
      Real normal_velocity = 0.0_r;
      for (Isize m = 0; m < SimulationControl::kDimension; m++) {
        normal_velocity += velocity(m) * normal_vector(m);
      }
      NormalFluxDevice<SimulationControl>::template getScalar<ConservedVariableEnum::Density>(convective_normal_flux) +=
          density * normal_velocity;
      const Real pressure = VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
          quadrature_node_computational_variable);
      Device::View<Device::StaticVector<Real, SimulationControl::kDimension>> momentum_flux =
          NormalFluxDevice<SimulationControl>::template getVector<ConservedVariableEnum::Momentum>(
              convective_normal_flux);
      for (Isize m = 0; m < SimulationControl::kDimension; m++) {
        momentum_flux(m) += density * normal_velocity * velocity(m) + pressure * normal_vector(m);
      }
      const Real total_energy =
          VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
              quadrature_node_computational_variable) +
          velocity.squaredNorm() / 2.0_r;
      NormalFluxDevice<SimulationControl>::template getScalar<ConservedVariableEnum::DensityTotalEnergy>(
          convective_normal_flux) += (density * total_energy + pressure) * normal_velocity;
    }
    if constexpr (IsIncompressible<SimulationControl::kEquationModel>) {
      const Real density = VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
          quadrature_node_computational_variable);
      const Device::View<const Device::StaticVector<Real, SimulationControl::kDimension>> velocity =
          VariableDevice<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
              quadrature_node_computational_variable);
      Real normal_velocity = 0.0_r;
      for (Isize m = 0; m < SimulationControl::kDimension; m++) {
        normal_velocity += velocity(m) * normal_vector(m);
      }
      NormalFluxDevice<SimulationControl>::template getScalar<ConservedVariableEnum::Density>(convective_normal_flux) +=
          density * normal_velocity;
      const Real pressure = VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
          quadrature_node_computational_variable);
      Device::View<Device::StaticVector<Real, SimulationControl::kDimension>> momentum_flux =
          NormalFluxDevice<SimulationControl>::template getVector<ConservedVariableEnum::Momentum>(
              convective_normal_flux);
      for (Isize m = 0; m < SimulationControl::kDimension; m++) {
        momentum_flux(m) += density * normal_velocity * velocity(m) + pressure * normal_vector(m);
      }
      NormalFluxDevice<SimulationControl>::template getScalar<ConservedVariableEnum::DensityInternalEnergy>(
          convective_normal_flux) +=
          density *
          VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
              quadrature_node_computational_variable) *
          normal_velocity;
    }
  }

  static void minusNormalALEFlux(
      const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector,
      const Device::StaticVector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>& quadrature_node_conserved_variable,
      Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>& convective_normal_flux) {
    Real rotation_normal_velocity = 0.0_r;
    for (Isize m = 0; m < SimulationControl::kDimension; m++) {
      rotation_normal_velocity += quadrature_node_rotation_velocity(m) * normal_vector(m);
    }
    for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
      convective_normal_flux(m) -= rotation_normal_velocity * quadrature_node_conserved_variable(m);
    }
  }

  static void computeCentralFlux(
      const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector,
      const Device::StaticVector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          left_quadrature_node_conserved_variable,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          right_quadrature_node_conserved_variable,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>& convective_normal_flux) {
    convective_normal_flux.setZero();
    ConvectiveFluxDevice<SimulationControl>::addNormalFlux(normal_vector, left_quadrature_node_computational_variable,
                                                           convective_normal_flux);
    ConvectiveFluxDevice<SimulationControl>::minusNormalALEFlux(normal_vector, quadrature_node_rotation_velocity,
                                                                left_quadrature_node_conserved_variable,
                                                                convective_normal_flux);
    ConvectiveFluxDevice<SimulationControl>::addNormalFlux(normal_vector, right_quadrature_node_computational_variable,
                                                           convective_normal_flux);
    ConvectiveFluxDevice<SimulationControl>::minusNormalALEFlux(normal_vector, quadrature_node_rotation_velocity,
                                                                right_quadrature_node_conserved_variable,
                                                                convective_normal_flux);
    for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
      convective_normal_flux(m) /= 2.0_r;
    }
  }

  static void computeLaxFriedrichsFlux(
      const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector,
      const Device::StaticVector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          left_quadrature_node_conserved_variable,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          right_quadrature_node_conserved_variable,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>& convective_normal_flux) {
    convective_normal_flux.setZero();
    ConvectiveFluxDevice<SimulationControl>::addNormalFlux(normal_vector, left_quadrature_node_computational_variable,
                                                           convective_normal_flux);
    ConvectiveFluxDevice<SimulationControl>::minusNormalALEFlux(normal_vector, quadrature_node_rotation_velocity,
                                                                left_quadrature_node_conserved_variable,
                                                                convective_normal_flux);
    ConvectiveFluxDevice<SimulationControl>::addNormalFlux(normal_vector, right_quadrature_node_computational_variable,
                                                           convective_normal_flux);
    ConvectiveFluxDevice<SimulationControl>::minusNormalALEFlux(normal_vector, quadrature_node_rotation_velocity,
                                                                right_quadrature_node_conserved_variable,
                                                                convective_normal_flux);
    const Device::View<const Device::StaticVector<Real, SimulationControl::kDimension>> left_velocity =
        VariableDevice<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
            left_quadrature_node_computational_variable);
    const Device::View<const Device::StaticVector<Real, SimulationControl::kDimension>> right_velocity =
        VariableDevice<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
            right_quadrature_node_computational_variable);
    Real left_normal_velocity = 0.0_r;
    Real right_normal_velocity = 0.0_r;
    for (Isize m = 0; m < SimulationControl::kDimension; m++) {
      left_normal_velocity += left_velocity(m) * normal_vector(m);
      right_normal_velocity += right_velocity(m) * normal_vector(m);
    }
    const Real left_sound_speed =
        PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(
            VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
                left_quadrature_node_computational_variable),
            VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
                left_quadrature_node_computational_variable));
    const Real right_sound_speed =
        PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(
            VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
                right_quadrature_node_computational_variable),
            VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
                right_quadrature_node_computational_variable));
    Real rotation_normal_velocity = 0.0_r;
    for (Isize m = 0; m < SimulationControl::kDimension; m++) {
      rotation_normal_velocity += quadrature_node_rotation_velocity(m) * normal_vector(m);
    }
    const Real spectral_radius =
        sycl::max(sycl::fabs(left_normal_velocity - rotation_normal_velocity) + left_sound_speed,
                  sycl::fabs(right_normal_velocity - rotation_normal_velocity) + right_sound_speed);
    for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
      convective_normal_flux(m) -=
          spectral_radius * (right_quadrature_node_conserved_variable(m) - left_quadrature_node_conserved_variable(m));
      convective_normal_flux(m) /= 2.0_r;
    }
  }

  static void computeHLLCFlux(
      const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector,
      const Device::StaticVector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      [[maybe_unused]] const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          left_quadrature_node_conserved_variable,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      [[maybe_unused]] const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          right_quadrature_node_conserved_variable,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>& convective_normal_flux) {
    convective_normal_flux.setZero();
    Device::StaticVector<Real, SimulationControl::kConservedVariableNumber> contact_conserved_variable;
    const Real left_density = VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
        left_quadrature_node_computational_variable);
    const Real right_density =
        VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
            right_quadrature_node_computational_variable);
    const Real left_sqrt_density = sycl::sqrt(left_density);
    const Real right_sqrt_density = sycl::sqrt(right_density);
    const Device::View<const Device::StaticVector<Real, SimulationControl::kDimension>> left_velocity =
        VariableDevice<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
            left_quadrature_node_computational_variable);
    const Device::View<const Device::StaticVector<Real, SimulationControl::kDimension>> right_velocity =
        VariableDevice<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
            right_quadrature_node_computational_variable);
    Real rotation_normal_velocity = 0.0_r;
    for (Isize m = 0; m < SimulationControl::kDimension; m++) {
      rotation_normal_velocity += quadrature_node_rotation_velocity(m) * normal_vector(m);
    }
    Device::StaticVector<Real, SimulationControl::kDimension> roe_velocity;
    for (Isize m = 0; m < SimulationControl::kDimension; m++) {
      roe_velocity(m) = (left_sqrt_density * left_velocity(m) + right_sqrt_density * right_velocity(m)) /
                        (left_sqrt_density + right_sqrt_density);
    }
    Real roe_normal_velocity = 0.0_r;
    for (Isize m = 0; m < SimulationControl::kDimension; m++) {
      roe_normal_velocity += roe_velocity(m) * normal_vector(m);
    }
    const Real left_pressure =
        VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
            left_quadrature_node_computational_variable);
    const Real right_pressure =
        VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
            right_quadrature_node_computational_variable);
    const Real left_sound_speed =
        PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(left_density,
                                                                                                  left_pressure);
    const Real right_sound_speed =
        PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(right_density,
                                                                                                  right_pressure);
    const Real left_total_energy =
        VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
            left_quadrature_node_computational_variable) +
        left_velocity.squaredNorm() / 2.0_r;
    const Real right_total_energy =
        VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
            right_quadrature_node_computational_variable) +
        right_velocity.squaredNorm() / 2.0_r;
    const Real left_total_enthalpy = left_total_energy + left_pressure / left_density;
    const Real right_total_enthalpy = right_total_energy + right_pressure / right_density;
    const Real roe_total_enthalpy =
        (left_sqrt_density * left_total_enthalpy + right_sqrt_density * right_total_enthalpy) /
        (left_sqrt_density + right_sqrt_density);
    const Real specific_heat_ratio = PhysicalModel<SimulationControl, PhysicalModelData>::getSpecificHeatRatio();
    const Real roe_sound_speed = sycl::sqrt((specific_heat_ratio - 1.0_r) *
                                            (roe_total_enthalpy - roe_normal_velocity * roe_normal_velocity / 2.0_r));
    Device::StaticVector<Real, SimulationControl::kDimension> left_relative_velocity;
    Device::StaticVector<Real, SimulationControl::kDimension> right_relative_velocity;
    for (Isize m = 0; m < SimulationControl::kDimension; m++) {
      left_relative_velocity(m) = left_velocity(m) - quadrature_node_rotation_velocity(m);
      right_relative_velocity(m) = right_velocity(m) - quadrature_node_rotation_velocity(m);
    }
    Real left_relative_normal_velocity = 0.0_r;
    Real right_relative_normal_velocity = 0.0_r;
    for (Isize m = 0; m < SimulationControl::kDimension; m++) {
      left_relative_normal_velocity += left_relative_velocity(m) * normal_vector(m);
      right_relative_normal_velocity += right_relative_velocity(m) * normal_vector(m);
    }
    const Real left_wave_speed = sycl::min(left_relative_normal_velocity - left_sound_speed,
                                           roe_normal_velocity - rotation_normal_velocity - roe_sound_speed);
    if (left_wave_speed >= 0.0_r) {
      ConvectiveFluxDevice<SimulationControl>::addNormalFlux(normal_vector, left_quadrature_node_computational_variable,
                                                             convective_normal_flux);
      ConvectiveFluxDevice<SimulationControl>::minusNormalALEFlux(normal_vector, quadrature_node_rotation_velocity,
                                                                  left_quadrature_node_conserved_variable,
                                                                  convective_normal_flux);
      return;
    }
    const Real right_wave_speed = sycl::max(right_relative_normal_velocity + right_sound_speed,
                                            roe_normal_velocity - rotation_normal_velocity + roe_sound_speed);
    if (right_wave_speed <= 0.0_r) {
      ConvectiveFluxDevice<SimulationControl>::addNormalFlux(
          normal_vector, right_quadrature_node_computational_variable, convective_normal_flux);
      ConvectiveFluxDevice<SimulationControl>::minusNormalALEFlux(normal_vector, quadrature_node_rotation_velocity,
                                                                  right_quadrature_node_conserved_variable,
                                                                  convective_normal_flux);
      return;
    }
    const Real contact_wave_speed =
        (right_density * right_relative_normal_velocity * (right_wave_speed - right_relative_normal_velocity) -
         left_density * left_relative_normal_velocity * (left_wave_speed - left_relative_normal_velocity) +
         left_pressure - right_pressure) /
        (right_density * (right_wave_speed - right_relative_normal_velocity) -
         left_density * (left_wave_speed - left_relative_normal_velocity));
    // also can be computed by right_density * (right_relative_normal_velocity - right_wave_speed) *
    // (right_relative_normal_velocity - contact_wave_speed ) + right_pressure
    const Real contact_pressure = left_density * (left_relative_normal_velocity - left_wave_speed) *
                                      (left_relative_normal_velocity - contact_wave_speed) +
                                  left_pressure;
    Device::View<Device::StaticVector<Real, SimulationControl::kDimension>> contact_momentum =
        VariableDevice<SimulationControl>::template getVector<ConservedVariableEnum::Momentum>(
            contact_conserved_variable);
    if (contact_wave_speed >= 0.0_r) {
      VariableDevice<SimulationControl>::template getScalar<ConservedVariableEnum::Density>(
          contact_conserved_variable) = (left_wave_speed - left_relative_normal_velocity) * left_density;
      for (Isize m = 0; m < SimulationControl::kDimension; m++) {
        contact_momentum(m) = (left_wave_speed - left_relative_normal_velocity) * left_density * left_velocity(m) +
                              (contact_pressure - left_pressure) * normal_vector(m);
      }
      VariableDevice<SimulationControl>::template getScalar<ConservedVariableEnum::DensityTotalEnergy>(
          contact_conserved_variable) =
          (left_wave_speed - left_relative_normal_velocity) * left_density * left_total_energy -
          left_pressure * left_relative_normal_velocity + contact_pressure * contact_wave_speed;
      for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
        contact_conserved_variable(m) /= (left_wave_speed - contact_wave_speed);
      }
    } else {
      VariableDevice<SimulationControl>::template getScalar<ConservedVariableEnum::Density>(
          contact_conserved_variable) = (right_wave_speed - right_relative_normal_velocity) * right_density;
      for (Isize m = 0; m < SimulationControl::kDimension; m++) {
        contact_momentum(m) = (right_wave_speed - right_relative_normal_velocity) * right_density * right_velocity(m) +
                              (contact_pressure - right_pressure) * normal_vector(m);
      }
      VariableDevice<SimulationControl>::template getScalar<ConservedVariableEnum::DensityTotalEnergy>(
          contact_conserved_variable) =
          (right_wave_speed - right_relative_normal_velocity) * right_density * right_total_energy -
          right_pressure * right_relative_normal_velocity + contact_pressure * contact_wave_speed;
      for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
        contact_conserved_variable(m) /= (right_wave_speed - contact_wave_speed);
      }
    }
    for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
      convective_normal_flux(m) = contact_wave_speed * contact_conserved_variable(m);
    }
    Device::View<Device::StaticVector<Real, SimulationControl::kDimension>> convective_normal_velocity_flux =
        NormalFluxDevice<SimulationControl>::template getVector<ConservedVariableEnum::Momentum>(
            convective_normal_flux);
    for (Isize m = 0; m < SimulationControl::kDimension; m++) {
      convective_normal_velocity_flux(m) += contact_pressure * normal_vector(m);
    }
    NormalFluxDevice<SimulationControl>::template getScalar<ConservedVariableEnum::DensityTotalEnergy>(
        convective_normal_flux) += contact_pressure * (contact_wave_speed + rotation_normal_velocity);
  }

  // TODO: Implement Roe flux.
  // static void computeRoeFlux(
  //     const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector,
  //     const Device::StaticVector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
  //     [[maybe_unused]] const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
  //         left_quadrature_node_conserved_variable,
  //     const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
  //         left_quadrature_node_computational_variable,
  //     [[maybe_unused]] const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
  //         right_quadrature_node_conserved_variable,
  //     const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
  //         right_quadrature_node_computational_variable,
  //     Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>& convective_normal_flux) {}

  static void computeExactFlux(
      const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector,
      [[maybe_unused]] const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          left_quadrature_node_conserved_variable,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      [[maybe_unused]] const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          right_quadrature_node_conserved_variable,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>& convective_normal_flux) {
    for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
      convective_normal_flux(m) = 0.0_r;
    }
    Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber> exact_computational_variable;
    const Real sound_speed =
        PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(0.0_r, 0.0_r);
    const Device::View<const Device::StaticVector<Real, SimulationControl::kDimension>> left_velocity =
        VariableDevice<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
            left_quadrature_node_computational_variable);
    const Device::View<const Device::StaticVector<Real, SimulationControl::kDimension>> right_velocity =
        VariableDevice<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
            right_quadrature_node_computational_variable);
    Real left_normal_velocity = 0.0_r;
    Real right_normal_velocity = 0.0_r;
    for (Isize m = 0; m < SimulationControl::kDimension; m++) {
      left_normal_velocity += left_velocity(m) * normal_vector(m);
      right_normal_velocity += right_velocity(m) * normal_vector(m);
    }
    VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
        exact_computational_variable) =
        sycl::sqrt(VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
                       left_quadrature_node_computational_variable) *
                   VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
                       right_quadrature_node_computational_variable) *
                   sycl::exp((left_normal_velocity - right_normal_velocity) / sound_speed));
    const Real exact_normal_velocity =
        (left_normal_velocity + right_normal_velocity) / 2.0_r +
        sycl::log(VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
                      left_quadrature_node_computational_variable) /
                  VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
                      right_quadrature_node_computational_variable)) *
            sound_speed / 2.0_r;
    Device::View<Device::StaticVector<Real, SimulationControl::kDimension>> exact_velocity =
        VariableDevice<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
            exact_computational_variable);
    if (exact_normal_velocity < 0.0_r) {
      for (Isize m = 0; m < SimulationControl::kDimension; m++) {
        exact_velocity(m) = right_velocity(m) + (exact_normal_velocity - right_normal_velocity) * normal_vector(m);
      }
      VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
          exact_computational_variable) =
          VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
              right_quadrature_node_computational_variable);
    } else {
      for (Isize m = 0; m < SimulationControl::kDimension; m++) {
        exact_velocity(m) = left_velocity(m) + (exact_normal_velocity - left_normal_velocity) * normal_vector(m);
      }
      VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
          exact_computational_variable) =
          VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
              left_quadrature_node_computational_variable);
    }
    VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
        exact_computational_variable) =
        PhysicalModel<SimulationControl, PhysicalModelData>::computePressureFromDensityInternalEnergy(
            VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
                exact_computational_variable),
            VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
                exact_computational_variable));
    ConvectiveFluxDevice<SimulationControl>::addNormalFlux(normal_vector, exact_computational_variable,
                                                           convective_normal_flux);
  }

  static void addVariableQuadratureRawFlux(
      const Device::StaticVector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>& quadrature_node_conserved_variable,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          quadrature_node_computational_variable,
      const Device::View<const Device::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>>
          jacobian_transpose_inverse_multiply_determinate_and_weight,
      Device::View<Device::Matrix<Real, SimulationControl::kConservedVariableNumber, SimulationControl::kDimension>>
          quadrature_node_variable_quadrature) {
    Device::StaticMatrix<Real, SimulationControl::kDimension, SimulationControl::kConservedVariableNumber>
        convective_raw_flux;
    ConvectiveFluxDevice<SimulationControl>::computeRawFlux(quadrature_node_computational_variable,
                                                            convective_raw_flux);
    ConvectiveFluxDevice<SimulationControl>::minusRawALEFlux(quadrature_node_rotation_velocity,
                                                             quadrature_node_conserved_variable, convective_raw_flux);
    for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
      for (Isize n = 0; n < SimulationControl::kDimension; n++) {
        Real sum = 0.0_r;
        for (Isize k = 0; k < SimulationControl::kDimension; k++) {
          sum += convective_raw_flux(k, m) * jacobian_transpose_inverse_multiply_determinate_and_weight(k, n);
        }
        quadrature_node_variable_quadrature(m, n) += sum;
      }
    }
  }

  static void addVariableInteriorAdjacencyQuadratureNormalFlux(
      const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector,
      const Device::StaticVector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          left_quadrature_node_conserved_variable,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          right_quadrature_node_conserved_variable,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber>>
          left_quadrature_node_variable_adjacency_quadrature,
      Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber>>
          right_quadrature_node_variable_adjacency_quadrature,
      const Real jacobian_determinant_multiply_weight) {
    Device::StaticVector<Real, SimulationControl::kConservedVariableNumber> convective_normal_flux;
    if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFluxEnum::Central) {
      ConvectiveFluxDevice<SimulationControl>::computeCentralFlux(
          normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_computational_variable,
          right_quadrature_node_computational_variable, convective_normal_flux);
    } else if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFluxEnum::LaxFriedrichs) {
      ConvectiveFluxDevice<SimulationControl>::computeLaxFriedrichsFlux(
          normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_conserved_variable,
          left_quadrature_node_computational_variable, right_quadrature_node_conserved_variable,
          right_quadrature_node_computational_variable, convective_normal_flux);
    } else if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFluxEnum::HLLC) {
      ConvectiveFluxDevice<SimulationControl>::computeHLLCFlux(
          normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_conserved_variable,
          left_quadrature_node_computational_variable, right_quadrature_node_conserved_variable,
          right_quadrature_node_computational_variable, convective_normal_flux);
    } else if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFluxEnum::Roe) {
      // ConvectiveFluxDevice<SimulationControl>::computeRoeFlux(
      //     normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_conserved_variable,
      //     left_quadrature_node_computational_variable, right_quadrature_node_conserved_variable,
      //     right_quadrature_node_computational_variable, convective_normal_flux);
    } else if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFluxEnum::Exact) {
      // ConvectiveFluxDevice<SimulationControl>::computeExactFlux(
      //     normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_conserved_variable,
      //     left_quadrature_node_computational_variable, right_quadrature_node_conserved_variable,
      //     right_quadrature_node_computational_variable, convective_normal_flux);
    }
    for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
      left_quadrature_node_variable_adjacency_quadrature(m) +=
          convective_normal_flux(m) * jacobian_determinant_multiply_weight;
      right_quadrature_node_variable_adjacency_quadrature(m) -=
          convective_normal_flux(m) * jacobian_determinant_multiply_weight;
    }
  }

  static void addVariableBoundaryAdjacencyQuadratureNormalFlux(
      const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector,
      const Device::StaticVector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          boundary_quadrature_node_computational_variable,
      Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber>>
          left_quadrature_node_variable_adjacency_quadrature,
      const BoundaryConditionEnum boundary_condition_type, const Real jacobian_determinant_multiply_weight) {
    Device::StaticVector<Real, SimulationControl::kConservedVariableNumber> boundary_quadrature_node_conserved_variable;
    BoundaryConditionDevice<SimulationControl>::computeBoundaryVariable(
        normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_computational_variable,
        right_quadrature_node_computational_variable, boundary_quadrature_node_computational_variable,
        boundary_condition_type);
    VariableDevice<SimulationControl>::convertConservedFromComputational(
        boundary_quadrature_node_computational_variable, boundary_quadrature_node_conserved_variable);
    Device::StaticVector<Real, SimulationControl::kConservedVariableNumber> convective_normal_flux;
    convective_normal_flux.setZero();
    ConvectiveFluxDevice<SimulationControl>::addNormalFlux(
        normal_vector, boundary_quadrature_node_computational_variable, convective_normal_flux);
    ConvectiveFluxDevice<SimulationControl>::minusNormalALEFlux(normal_vector, quadrature_node_rotation_velocity,
                                                                boundary_quadrature_node_conserved_variable,
                                                                convective_normal_flux);
    for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
      left_quadrature_node_variable_adjacency_quadrature(m) +=
          convective_normal_flux(m) * jacobian_determinant_multiply_weight;
    }
  }

  static void addVariableInterfaceAdjacencyQuadratureNormalFlux(
      const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector,
      const Device::StaticVector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          left_quadrature_node_conserved_variable,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          right_quadrature_node_conserved_variable,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber>>
          left_quadrature_node_variable_adjacency_quadrature,
      const Real jacobian_determinant_multiply_weight) {
    Device::StaticVector<Real, SimulationControl::kConservedVariableNumber> convective_normal_flux;
    if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFluxEnum::Central) {
      ConvectiveFluxDevice<SimulationControl>::computeCentralFlux(
          normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_computational_variable,
          right_quadrature_node_computational_variable, convective_normal_flux);
    } else if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFluxEnum::LaxFriedrichs) {
      ConvectiveFluxDevice<SimulationControl>::computeLaxFriedrichsFlux(
          normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_conserved_variable,
          left_quadrature_node_computational_variable, right_quadrature_node_conserved_variable,
          right_quadrature_node_computational_variable, convective_normal_flux);
    } else if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFluxEnum::HLLC) {
      ConvectiveFluxDevice<SimulationControl>::computeHLLCFlux(
          normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_conserved_variable,
          left_quadrature_node_computational_variable, right_quadrature_node_conserved_variable,
          right_quadrature_node_computational_variable, convective_normal_flux);
    } else if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFluxEnum::Roe) {
      // ConvectiveFluxDevice<SimulationControl>::computeRoeFlux(
      //     normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_conserved_variable,
      //     left_quadrature_node_computational_variable, right_quadrature_node_conserved_variable,
      //     right_quadrature_node_computational_variable, convective_normal_flux);
    } else if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFluxEnum::Exact) {
      // ConvectiveFluxDevice<SimulationControl>::computeExactFlux(
      //     normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_conserved_variable,
      //     left_quadrature_node_computational_variable, right_quadrature_node_conserved_variable,
      //     right_quadrature_node_computational_variable, convective_normal_flux);
    }
    for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
      left_quadrature_node_variable_adjacency_quadrature(m) +=
          convective_normal_flux(m) * jacobian_determinant_multiply_weight;
    }
  }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CONVECTIVE_FLUX_CPP_
