/**
 * @file ConvectiveFlux.hpp
 * @brief The header file of ConvectiveFlux.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CONVETIVE_FLUX_HPP_
#define SUBROSA_DG_CONVETIVE_FLUX_HPP_

#include <Eigen/Core>
#include <algorithm>
#include <cmath>
#include <functional>

#include "Solver/ThermalModel.hpp"
#include "Solver/VariableConvertor.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Enum.hpp"

namespace SubrosaDG {

template <typename SimulationControl>
inline void calculateConvectiveRawFlux(const Variable<SimulationControl>& variable,
                                       FluxVariable<SimulationControl>& convective_raw_flux) {
  const Real density = variable.template getScalar<ComputationalVariableEnum::Density>();
  const Eigen::Vector<Real, SimulationControl::kDimension>& velocity =
      variable.template getVector<ComputationalVariableEnum::Velocity>();
  convective_raw_flux.template setVector<ConservedVariableEnum::Density>(density * velocity);
  const Real pressure = variable.template getScalar<ComputationalVariableEnum::Pressure>();
  convective_raw_flux.template setMatrix<ConservedVariableEnum::Momentum>(
      density * velocity * velocity.transpose() +
      pressure * Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>::Identity());
  const Real total_energy = variable.template getScalar<ComputationalVariableEnum::InternalEnergy>() +
                            variable.template getScalar<ComputationalVariableEnum::VelocitySquareSummation>() / 2.0;
  convective_raw_flux.template setVector<ConservedVariableEnum::DensityTotalEnergy>(
      (density * total_energy + pressure) * velocity);
}

template <typename SimulationControl>
inline void calculateConvectiveNormalFlux(const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                          const Variable<SimulationControl>& variable,
                                          FluxNormalVariable<SimulationControl>& convective_normal_flux) {
  const Real density = variable.template getScalar<ComputationalVariableEnum::Density>();
  const Eigen::Vector<Real, SimulationControl::kDimension>& velocity =
      variable.template getVector<ComputationalVariableEnum::Velocity>();
  const Real normal_velocity = velocity.transpose() * normal_vector;
  convective_normal_flux.template setScalar<ConservedVariableEnum::Density>(density * normal_velocity);
  const Real pressure = variable.template getScalar<ComputationalVariableEnum::Pressure>();
  convective_normal_flux.template setVector<ConservedVariableEnum::Momentum>(density * velocity * normal_velocity +
                                                                             pressure * normal_vector);
  const Real total_energy = variable.template getScalar<ComputationalVariableEnum::InternalEnergy>() +
                            variable.template getScalar<ComputationalVariableEnum::VelocitySquareSummation>() / 2.0;
  convective_normal_flux.template setScalar<ConservedVariableEnum::DensityTotalEnergy>(
      (density * total_energy + pressure) * normal_velocity);
}

template <typename SimulationControl>
inline void calculateConvectiveCentralFlux([[maybe_unused]] const ThermalModel<SimulationControl>& thermal_model,
                                           const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                           const Variable<SimulationControl>& left_quadrature_node_variable,
                                           const Variable<SimulationControl>& right_quadrature_node_variable,
                                           Flux<SimulationControl>& convective_flux) {
  calculateConvectiveNormalFlux(normal_vector, left_quadrature_node_variable, convective_flux.left_);
  calculateConvectiveNormalFlux(normal_vector, right_quadrature_node_variable, convective_flux.right_);
  convective_flux.result_.normal_variable_.noalias() =
      (convective_flux.left_.normal_variable_ + convective_flux.right_.normal_variable_) / 2.0;
}

template <typename SimulationControl>
inline void calculateConvectiveLaxFriedrichsFlux(
    const ThermalModel<SimulationControl>& thermal_model,
    const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
    const Variable<SimulationControl>& left_quadrature_node_variable,
    const Variable<SimulationControl>& right_quadrature_node_variable, Flux<SimulationControl>& convective_flux) {
  calculateConvectiveNormalFlux(normal_vector, left_quadrature_node_variable, convective_flux.left_flux_);
  calculateConvectiveNormalFlux(normal_vector, right_quadrature_node_variable, convective_flux.right_flux_);
  const Real left_normal_velocity =
      left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>().transpose() *
      normal_vector;
  const Real right_normal_velocity =
      right_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>().transpose() *
      normal_vector;
  const Real left_sound_speed = thermal_model.calculateSoundSpeedFromInternalEnergy(
      left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::InternalEnergy>());
  const Real right_sound_speed = thermal_model.calculateSoundSpeedFromInternalEnergy(
      right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::InternalEnergy>());
  const Real spectral_radius = std::ranges::max(std::abs(left_normal_velocity) + left_sound_speed,
                                                std::abs(right_normal_velocity) + right_sound_speed);
  convective_flux.result_.normal_variable_.noalias() =
      ((convective_flux.left_.normal_variable_ + convective_flux.right_.normal_variable_) -
       spectral_radius * (right_quadrature_node_variable.conserved_ - left_quadrature_node_variable.conserved_)) /
      2.0;
}

template <typename SimulationControl>
inline void calculateConvectiveHLLCFlux(const ThermalModel<SimulationControl>& thermal_model,
                                        const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                        const Variable<SimulationControl>& left_quadrature_node_variable,
                                        const Variable<SimulationControl>& right_quadrature_node_variable,
                                        Flux<SimulationControl>& convective_flux) {
  Variable<SimulationControl> contact_variable;
  const Real left_density = left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>();
  const Real right_density = right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>();
  const Real left_pressure = left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Pressure>();
  const Real right_pressure = right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Pressure>();
  const Real left_normal_velocity =
      left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>().transpose() *
      normal_vector;
  const Real right_normal_velocity =
      right_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>().transpose() *
      normal_vector;
  const Real left_sound_speed = thermal_model.calculateSoundSpeedFromInternalEnergy(
      left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::InternalEnergy>());
  const Real right_sound_speed = thermal_model.calculateSoundSpeedFromInternalEnergy(
      right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::InternalEnergy>());
  const Real average_density = (left_density + right_density) / 2.0;
  const Real average_sound_speed = (left_sound_speed + right_sound_speed) / 2.0;
  const Real contact_pressure =
      std::ranges::max(0.0, (left_pressure + right_pressure) / 2.0 -
                                (right_normal_velocity - left_normal_velocity) * average_density * average_sound_speed);
  const Real left_wave_speed =
      left_normal_velocity -
      left_sound_speed * (contact_pressure <= left_pressure
                              ? 1.0
                              : std::sqrt(1.0 + (thermal_model.equation_of_state_.kSpecificHeatRatio + 1.0) *
                                                    (contact_pressure / left_pressure - 1.0) / 2.0 /
                                                    thermal_model.equation_of_state_.kSpecificHeatRatio));
  if (left_wave_speed >= 0.0) {
    calculateConvectiveNormalFlux(normal_vector, left_quadrature_node_variable, convective_flux.result_);
    return;
  }
  const Real right_wave_speed =
      right_normal_velocity +
      right_sound_speed * (contact_pressure <= right_pressure
                               ? 1.0
                               : std::sqrt(1.0 + (thermal_model.equation_of_state_.kSpecificHeatRatio + 1.0) *
                                                     (contact_pressure / right_pressure - 1.0) / 2.0 /
                                                     thermal_model.equation_of_state_.kSpecificHeatRatio));
  if (right_wave_speed <= 0.0) {
    calculateConvectiveNormalFlux(normal_vector, right_quadrature_node_variable, convective_flux.result_);
    return;
  }
  const Real contact_wave_speed =
      (right_pressure - left_pressure + left_density * left_normal_velocity * (left_wave_speed - left_normal_velocity) -
       right_density * right_normal_velocity * (right_wave_speed - right_normal_velocity)) /
      (left_density * (left_wave_speed - left_normal_velocity) -
       right_density * (right_wave_speed - right_normal_velocity));
  if (contact_wave_speed >= 0.0) {
    calculateConvectiveNormalFlux(normal_vector, left_quadrature_node_variable, convective_flux.left_);
    contact_variable.template setScalar<ConservedVariableEnum::Density>(
        left_density * (left_wave_speed - left_normal_velocity) / (left_wave_speed - contact_wave_speed));
    const Eigen::Vector<Real, SimulationControl::kDimension> contact_momentum =
        ((left_wave_speed - left_normal_velocity) * left_density *
             left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>() +
         (contact_pressure - left_pressure) * normal_vector) /
        (left_wave_speed - contact_wave_speed);
    contact_variable.template setVector<ConservedVariableEnum::Momentum>(contact_momentum);
    contact_variable.template setScalar<ConservedVariableEnum::DensityTotalEnergy>(
        ((left_wave_speed - left_normal_velocity) * left_density *
             (left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::InternalEnergy>() +
              left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::VelocitySquareSummation>() /
                  2.0) -
         left_pressure * left_normal_velocity + contact_pressure * contact_wave_speed) /
        (left_wave_speed - contact_wave_speed));
    convective_flux.result_.normal_variable_.noalias() =
        convective_flux.left_.normal_variable_ +
        left_wave_speed * (contact_variable.conserved_ - left_quadrature_node_variable.conserved_);
  } else {
    calculateConvectiveNormalFlux(normal_vector, right_quadrature_node_variable, convective_flux.right_);
    contact_variable.template setScalar<ConservedVariableEnum::Density>(
        right_density * (right_wave_speed - right_normal_velocity) / (right_wave_speed - contact_wave_speed));
    const Eigen::Vector<Real, SimulationControl::kDimension> contact_momentum =
        ((right_wave_speed - right_normal_velocity) * right_density *
             right_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>() +
         (contact_pressure - right_pressure) * normal_vector) /
        (right_wave_speed - contact_wave_speed);
    contact_variable.template setVector<ConservedVariableEnum::Momentum>(contact_momentum);
    contact_variable.template setScalar<ConservedVariableEnum::DensityTotalEnergy>(
        ((right_wave_speed - right_normal_velocity) * right_density *
             (right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::InternalEnergy>() +
              right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::VelocitySquareSummation>() /
                  2.0) -
         right_pressure * right_normal_velocity + contact_pressure * contact_wave_speed) /
        (right_wave_speed - contact_wave_speed));
    convective_flux.result_.normal_variable_.noalias() =
        convective_flux.right_.normal_variable_ +
        right_wave_speed * (contact_variable.conserved_ - right_quadrature_node_variable.conserved_);
  }
}

template <typename SimulationControl>
inline void calculateConvectiveRoeFlux(const ThermalModel<SimulationControl>& thermal_model,
                                       const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                       const Variable<SimulationControl>& left_quadrature_node_variable,
                                       const Variable<SimulationControl>& right_quadrature_node_variable,
                                       Flux<SimulationControl>& convective_flux) {
  Variable<SimulationControl> roe_variable;
  Variable<SimulationControl> delta_variable;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, SimulationControl::kConservedVariableNumber>
      roe_matrix = Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber,
                                 SimulationControl::kConservedVariableNumber>::Zero();
  calculateConvectiveNormalFlux(normal_vector, left_quadrature_node_variable, convective_flux.left_);
  calculateConvectiveNormalFlux(normal_vector, right_quadrature_node_variable, convective_flux.right_);
  const Real left_sqrt_density =
      std::sqrt(left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>());
  const Real right_sqrt_density =
      std::sqrt(right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>());
  const Real sqrt_density_summation = left_sqrt_density + right_sqrt_density;
  roe_variable.template setScalar<ComputationalVariableEnum::Density>(
      std::sqrt(left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>() *
                right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>()));
  roe_variable.template setVector<ComputationalVariableEnum::Velocity>(
      (left_sqrt_density * left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>() +
       right_sqrt_density * right_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>()) /
      sqrt_density_summation);
  const Real left_total_enthapy =
      thermal_model.calculateEnthalpyFromInternalEnergy(
          left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::InternalEnergy>()) +
      left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::VelocitySquareSummation>() / 2.0;
  const Real right_total_enthapy =
      thermal_model.calculateEnthalpyFromInternalEnergy(
          right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::InternalEnergy>()) +
      right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::VelocitySquareSummation>() / 2.0;
  const Real roe_total_enthapy =
      (left_sqrt_density * left_total_enthapy + right_sqrt_density * right_total_enthapy) / sqrt_density_summation;
  roe_variable.template setScalar<ComputationalVariableEnum::InternalEnergy>(
      thermal_model.calculateInternalEnergyFromEnthalpy(
          roe_total_enthapy -
          roe_variable.template getScalar<ComputationalVariableEnum::VelocitySquareSummation>() / 2.0));
  roe_variable.template setScalar<ComputationalVariableEnum::Pressure>(
      thermal_model.calculatePressureFormDensityInternalEnergy(
          roe_variable.template getScalar<ComputationalVariableEnum::Density>(),
          roe_variable.template getScalar<ComputationalVariableEnum::InternalEnergy>()));
  const Real roe_normal_velocity =
      roe_variable.template getVector<ComputationalVariableEnum::Velocity>().transpose() * normal_vector;
  const Real roe_sound_speed = thermal_model.calculateSoundSpeedFromInternalEnergy(
      roe_variable.template getScalar<ComputationalVariableEnum::InternalEnergy>());
  delta_variable.computational_ =
      right_quadrature_node_variable.computational_ - left_quadrature_node_variable.computational_;
  const Real delta_normal_velocity =
      delta_variable.template getVector<ComputationalVariableEnum::Velocity>().transpose() * normal_vector;
  const Real harten_delta = roe_sound_speed / 20.0;
  const Real lambda_velocity_subtract_sound_speed =
      std::abs(roe_normal_velocity - roe_sound_speed) > harten_delta
          ? std::abs(roe_normal_velocity - roe_sound_speed)
          : (std::pow(roe_normal_velocity - roe_sound_speed, 2) + harten_delta * harten_delta) / (2.0 * harten_delta);
  const Real lambda_velocity_add_sound_speed =
      std::abs(roe_normal_velocity + roe_sound_speed) > harten_delta
          ? std::abs(roe_normal_velocity + roe_sound_speed)
          : (std::pow(roe_normal_velocity + roe_sound_speed, 2) + harten_delta * harten_delta) / (2.0 * harten_delta);
  roe_matrix.col(0) << 1.0,
      roe_variable.template getVector<ComputationalVariableEnum::Velocity>() - roe_sound_speed * normal_vector,
      roe_total_enthapy - roe_sound_speed * roe_normal_velocity;
  roe_matrix.col(0) *= lambda_velocity_subtract_sound_speed *
                       (delta_variable.template getScalar<ComputationalVariableEnum::Pressure>() -
                        roe_variable.template getScalar<ComputationalVariableEnum::Density>() * roe_sound_speed *
                            delta_normal_velocity) /
                       (2.0 * roe_sound_speed * roe_sound_speed);
  roe_matrix.col(1) << 1.0, roe_variable.template getVector<ComputationalVariableEnum::Velocity>(),
      roe_variable.template getScalar<ComputationalVariableEnum::VelocitySquareSummation>() / 2.0;
  roe_matrix.col(1) *=
      std::abs(roe_normal_velocity) *
      (delta_variable.template getScalar<ComputationalVariableEnum::Density>() -
       delta_variable.template getScalar<ComputationalVariableEnum::Pressure>() / (roe_sound_speed * roe_sound_speed));
  if constexpr (SimulationControl::kDimension == 2 || SimulationControl::kDimension == 3) {
    roe_matrix.col(2) << 0.0,
        delta_variable.template getVector<ComputationalVariableEnum::Velocity>() -
            delta_normal_velocity * normal_vector,
        roe_variable.template getVector<ComputationalVariableEnum::Velocity>().transpose() *
                delta_variable.template getVector<ComputationalVariableEnum::Velocity>() -
            roe_normal_velocity * delta_normal_velocity;
    roe_matrix.col(2) *=
        std::fabs(roe_normal_velocity) * roe_variable.template getScalar<ComputationalVariableEnum::Density>();
  }
  roe_matrix.col(SimulationControl::kDimension + 1) << 1.0,
      roe_variable.template getVector<ComputationalVariableEnum::Velocity>() + roe_sound_speed * normal_vector,
      roe_total_enthapy + roe_sound_speed * roe_normal_velocity;
  roe_matrix.col(SimulationControl::kDimension + 1) *=
      lambda_velocity_add_sound_speed *
      (delta_variable.template getScalar<ComputationalVariableEnum::Pressure>() +
       roe_variable.template getScalar<ComputationalVariableEnum::Density>() * roe_sound_speed *
           delta_normal_velocity) /
      (2.0 * roe_sound_speed * roe_sound_speed);
  convective_flux.result_.normal_variable_.noalias() =
      ((convective_flux.left_.normal_variable_ + convective_flux.right_.normal_variable_) -
       roe_matrix.rowwise().sum()) /
      2.0;
}

template <typename SimulationControl>
inline void calculateConvectiveFlux(const ThermalModel<SimulationControl>& thermal_model,
                                    const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                    const Variable<SimulationControl>& left_quadrature_node_variable,
                                    const Variable<SimulationControl>& right_quadrature_node_variable,
                                    Flux<SimulationControl>& convective_flux) {
  if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFluxEnum::Central) {
    calculateConvectiveCentralFlux(thermal_model, normal_vector, left_quadrature_node_variable,
                                   right_quadrature_node_variable, convective_flux);
  } else if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFluxEnum::LaxFriedrichs) {
    calculateConvectiveLaxFriedrichsFlux(thermal_model, normal_vector, left_quadrature_node_variable,
                                         right_quadrature_node_variable, convective_flux);
  } else if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFluxEnum::HLLC) {
    calculateConvectiveHLLCFlux(thermal_model, normal_vector, left_quadrature_node_variable,
                                right_quadrature_node_variable, convective_flux);
  } else if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFluxEnum::Roe) {
    calculateConvectiveRoeFlux(thermal_model, normal_vector, left_quadrature_node_variable,
                               right_quadrature_node_variable, convective_flux);
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CONVETIVE_FLUX_HPP_
