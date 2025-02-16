/**
 * @file ConvectiveFlux.cpp
 * @brief The header file of ConvectiveFlux.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2025 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CONVETIVE_FLUX_CPP_
#define SUBROSA_DG_CONVETIVE_FLUX_CPP_

#include <Eigen/Core>
#include <algorithm>
#include <cmath>

#include "Solver/PhysicalModel.cpp"
#include "Solver/VariableConvertor.cpp"
#include "Utils/BasicDataType.cpp"
#include "Utils/Enum.cpp"

namespace SubrosaDG {

template <typename SimulationControl, int N>
inline void calculateConvectiveRawFlux(const Variable<SimulationControl, N>& variable,
                                       FluxVariable<SimulationControl>& convective_raw_flux, const Isize column) {
  if constexpr (IsCompresible<SimulationControl::kEquationModel>) {
    const Real density = variable.template getScalar<ComputationalVariableEnum::Density>(column);
    const Eigen::Vector<Real, SimulationControl::kDimension> velocity =
        variable.template getVector<ComputationalVariableEnum::Velocity>(column);
    convective_raw_flux.template setVector<ConservedVariableEnum::Density>(density * velocity);
    const Real pressure = variable.template getScalar<ComputationalVariableEnum::Pressure>(column);
    convective_raw_flux.template setMatrix<ConservedVariableEnum::Momentum>(
        density * velocity * velocity.transpose() +
        pressure * Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>::Identity());
    const Real total_energy =
        variable.template getScalar<ComputationalVariableEnum::InternalEnergy>(column) +
        variable.template getScalar<ComputationalVariableEnum::VelocitySquaredNorm>(column) / 2.0_r;
    convective_raw_flux.template setVector<ConservedVariableEnum::DensityTotalEnergy>(
        (density * total_energy + pressure) * velocity);
  }
  if constexpr (IsIncompresible<SimulationControl::kEquationModel>) {
    const Real density = variable.template getScalar<ComputationalVariableEnum::Density>(column);
    const Eigen::Vector<Real, SimulationControl::kDimension> velocity =
        variable.template getVector<ComputationalVariableEnum::Velocity>(column);
    convective_raw_flux.template setVector<ConservedVariableEnum::Density>(density * velocity);
    const Real pressure = variable.template getScalar<ComputationalVariableEnum::Pressure>(column);
    convective_raw_flux.template setMatrix<ConservedVariableEnum::Momentum>(
        density * velocity * velocity.transpose() +
        pressure * Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>::Identity());
    convective_raw_flux.template setVector<ConservedVariableEnum::DensityInternalEnergy>(
        density * variable.template getScalar<ComputationalVariableEnum::InternalEnergy>(column) * velocity);
  }
}

template <typename SimulationControl, int N>
inline void calculateConvectiveNormalFlux(const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                          const Variable<SimulationControl, N>& variable,
                                          FluxNormalVariable<SimulationControl>& convective_normal_flux,
                                          const Isize column) {
  if constexpr (IsCompresible<SimulationControl::kEquationModel>) {
    const Real density = variable.template getScalar<ComputationalVariableEnum::Density>(column);
    const Eigen::Vector<Real, SimulationControl::kDimension> velocity =
        variable.template getVector<ComputationalVariableEnum::Velocity>(column);
    const Real normal_velocity = velocity.transpose() * normal_vector;
    convective_normal_flux.template setScalar<ConservedVariableEnum::Density>(density * normal_velocity);
    const Real pressure = variable.template getScalar<ComputationalVariableEnum::Pressure>(column);
    convective_normal_flux.template setVector<ConservedVariableEnum::Momentum>(density * normal_velocity * velocity +
                                                                               pressure * normal_vector);
    const Real total_energy =
        variable.template getScalar<ComputationalVariableEnum::InternalEnergy>(column) +
        variable.template getScalar<ComputationalVariableEnum::VelocitySquaredNorm>(column) / 2.0_r;
    convective_normal_flux.template setScalar<ConservedVariableEnum::DensityTotalEnergy>(
        (density * total_energy + pressure) * normal_velocity);
  }
  if constexpr (IsIncompresible<SimulationControl::kEquationModel>) {
    const Real density = variable.template getScalar<ComputationalVariableEnum::Density>(column);
    const Eigen::Vector<Real, SimulationControl::kDimension> velocity =
        variable.template getVector<ComputationalVariableEnum::Velocity>(column);
    const Real normal_velocity = velocity.transpose() * normal_vector;
    convective_normal_flux.template setScalar<ConservedVariableEnum::Density>(density * normal_velocity);
    const Real pressure = variable.template getScalar<ComputationalVariableEnum::Pressure>(column);
    convective_normal_flux.template setVector<ConservedVariableEnum::Momentum>(density * normal_velocity * velocity +
                                                                               pressure * normal_vector);
    convective_normal_flux.template setScalar<ConservedVariableEnum::DensityInternalEnergy>(
        density * variable.template getScalar<ComputationalVariableEnum::InternalEnergy>(column) * normal_velocity);
  }
}

template <typename SimulationControl, int N>
inline void calculateConvectiveCentralFlux([[maybe_unused]] const PhysicalModel<SimulationControl>& physical_model,
                                           const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                           const Variable<SimulationControl, N>& left_quadrature_node_variable,
                                           const Variable<SimulationControl, N>& right_quadrature_node_variable,
                                           Flux<SimulationControl>& convective_flux, const Isize left_column,
                                           const Isize right_column) {
  calculateConvectiveNormalFlux(normal_vector, left_quadrature_node_variable, convective_flux.left_, left_column);
  calculateConvectiveNormalFlux(normal_vector, right_quadrature_node_variable, convective_flux.right_, right_column);
  convective_flux.result_.normal_variable_.noalias() =
      (convective_flux.left_.normal_variable_ + convective_flux.right_.normal_variable_) / 2.0_r;
}

template <typename SimulationControl, int N>
inline void calculateConvectiveLaxFriedrichsFlux(
    const PhysicalModel<SimulationControl>& physical_model,
    const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
    const Variable<SimulationControl, N>& left_quadrature_node_variable,
    const Variable<SimulationControl, N>& right_quadrature_node_variable, Flux<SimulationControl>& convective_flux,
    const Isize left_column, const Isize right_column) {
  calculateConvectiveNormalFlux(normal_vector, left_quadrature_node_variable, convective_flux.left_, left_column);
  calculateConvectiveNormalFlux(normal_vector, right_quadrature_node_variable, convective_flux.right_, right_column);
  const Real left_normal_velocity =
      left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(left_column).transpose() *
      normal_vector;
  const Real right_normal_velocity =
      right_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(right_column).transpose() *
      normal_vector;
  const Real left_sound_speed = physical_model.calculateSoundSpeedFromDensityPressure(
      left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(left_column),
      left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Pressure>(left_column));
  const Real right_sound_speed = physical_model.calculateSoundSpeedFromDensityPressure(
      right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(right_column),
      right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Pressure>(right_column));
  const Real spectral_radius = std::ranges::max(std::fabs(left_normal_velocity) + left_sound_speed,
                                                std::fabs(right_normal_velocity) + right_sound_speed);
  convective_flux.result_.normal_variable_.noalias() =
      ((convective_flux.left_.normal_variable_ + convective_flux.right_.normal_variable_) -
       spectral_radius * (right_quadrature_node_variable.conserved_.col(right_column) -
                          left_quadrature_node_variable.conserved_.col(left_column))) /
      2.0_r;
}

template <typename SimulationControl, int N>
inline void calculateConvectiveHLLCFlux(const PhysicalModel<SimulationControl>& physical_model,
                                        const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                        const Variable<SimulationControl, N>& left_quadrature_node_variable,
                                        const Variable<SimulationControl, N>& right_quadrature_node_variable,
                                        Flux<SimulationControl>& convective_flux, const Isize left_column,
                                        const Isize right_column) {
  Variable<SimulationControl, 1> contact_variable;
  const Real left_density =
      left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(left_column);
  const Real right_density =
      right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(right_column);
  const Real left_pressure =
      left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Pressure>(left_column);
  const Real right_pressure =
      right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Pressure>(right_column);
  const Real left_normal_velocity =
      left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(left_column).transpose() *
      normal_vector;
  const Real right_normal_velocity =
      right_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(right_column).transpose() *
      normal_vector;
  const Real left_sound_speed = physical_model.calculateSoundSpeedFromDensityPressure(left_density, left_pressure);
  const Real right_sound_speed = physical_model.calculateSoundSpeedFromDensityPressure(right_density, right_pressure);
  const Real average_density = (left_density + right_density) / 2.0_r;
  const Real average_sound_speed = (left_sound_speed + right_sound_speed) / 2.0_r;
  const Real contact_pressure = std::ranges::max(
      0.0_r, (left_pressure + right_pressure) / 2.0_r -
                 (right_normal_velocity - left_normal_velocity) * average_density * average_sound_speed);
  const Real left_wave_speed =
      left_normal_velocity -
      left_sound_speed * (contact_pressure <= left_pressure
                              ? 1.0_r
                              : std::sqrt(1.0_r + (physical_model.equation_of_state_.kSpecificHeatRatio + 1.0_r) *
                                                      (contact_pressure / left_pressure - 1.0_r) / 2.0_r /
                                                      physical_model.equation_of_state_.kSpecificHeatRatio));
  if (left_wave_speed >= 0.0_r) {
    calculateConvectiveNormalFlux(normal_vector, left_quadrature_node_variable, convective_flux.result_, left_column);
    return;
  }
  const Real right_wave_speed =
      right_normal_velocity +
      right_sound_speed * (contact_pressure <= right_pressure
                               ? 1.0_r
                               : std::sqrt(1.0_r + (physical_model.equation_of_state_.kSpecificHeatRatio + 1.0_r) *
                                                       (contact_pressure / right_pressure - 1.0_r) / 2.0_r /
                                                       physical_model.equation_of_state_.kSpecificHeatRatio));
  if (right_wave_speed <= 0.0_r) {
    calculateConvectiveNormalFlux(normal_vector, right_quadrature_node_variable, convective_flux.result_, right_column);
    return;
  }
  const Real contact_wave_speed =
      (right_pressure - left_pressure + left_density * left_normal_velocity * (left_wave_speed - left_normal_velocity) -
       right_density * right_normal_velocity * (right_wave_speed - right_normal_velocity)) /
      (left_density * (left_wave_speed - left_normal_velocity) -
       right_density * (right_wave_speed - right_normal_velocity));
  if (contact_wave_speed >= 0.0_r) {
    calculateConvectiveNormalFlux(normal_vector, left_quadrature_node_variable, convective_flux.left_, left_column);
    contact_variable.template setScalar<ConservedVariableEnum::Density>(
        left_density * (left_wave_speed - left_normal_velocity) / (left_wave_speed - contact_wave_speed), 0);
    const Eigen::Vector<Real, SimulationControl::kDimension> contact_momentum =
        ((left_wave_speed - left_normal_velocity) * left_density *
             left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(left_column) +
         (contact_pressure - left_pressure) * normal_vector) /
        (left_wave_speed - contact_wave_speed);
    contact_variable.template setVector<ConservedVariableEnum::Momentum>(contact_momentum, 0);
    contact_variable.template setScalar<ConservedVariableEnum::DensityTotalEnergy>(
        ((left_wave_speed - left_normal_velocity) * left_density *
             (left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::InternalEnergy>(left_column) +
              left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::VelocitySquaredNorm>(
                  left_column) /
                  2.0_r) -
         left_pressure * left_normal_velocity + contact_pressure * contact_wave_speed) /
            (left_wave_speed - contact_wave_speed),
        0);
    convective_flux.result_.normal_variable_.noalias() =
        convective_flux.left_.normal_variable_ +
        left_wave_speed * (contact_variable.conserved_ - left_quadrature_node_variable.conserved_.col(left_column));
  } else {
    calculateConvectiveNormalFlux(normal_vector, right_quadrature_node_variable, convective_flux.right_, right_column);
    contact_variable.template setScalar<ConservedVariableEnum::Density>(
        right_density * (right_wave_speed - right_normal_velocity) / (right_wave_speed - contact_wave_speed), 0);
    const Eigen::Vector<Real, SimulationControl::kDimension> contact_momentum =
        ((right_wave_speed - right_normal_velocity) * right_density *
             right_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(right_column) +
         (contact_pressure - right_pressure) * normal_vector) /
        (right_wave_speed - contact_wave_speed);
    contact_variable.template setVector<ConservedVariableEnum::Momentum>(contact_momentum, 0);
    contact_variable.template setScalar<ConservedVariableEnum::DensityTotalEnergy>(
        ((right_wave_speed - right_normal_velocity) * right_density *
             (right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::InternalEnergy>(
                  right_column) +
              right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::VelocitySquaredNorm>(
                  right_column) /
                  2.0_r) -
         right_pressure * right_normal_velocity + contact_pressure * contact_wave_speed) /
            (right_wave_speed - contact_wave_speed),
        0);
    convective_flux.result_.normal_variable_.noalias() =
        convective_flux.right_.normal_variable_ +
        right_wave_speed * (contact_variable.conserved_ - right_quadrature_node_variable.conserved_.col(right_column));
  }
}

template <typename SimulationControl, int N>
inline void calculateConvectiveRoeFlux(const PhysicalModel<SimulationControl>& physical_model,
                                       const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                       const Variable<SimulationControl, N>& left_quadrature_node_variable,
                                       const Variable<SimulationControl, N>& right_quadrature_node_variable,
                                       Flux<SimulationControl>& convective_flux, const Isize left_column,
                                       const Isize right_column) {
  Variable<SimulationControl, 1> roe_variable;
  Variable<SimulationControl, 1> delta_variable;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, SimulationControl::kConservedVariableNumber>
      roe_matrix = Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber,
                                 SimulationControl::kConservedVariableNumber>::Zero();
  calculateConvectiveNormalFlux(normal_vector, left_quadrature_node_variable, convective_flux.left_, left_column);
  calculateConvectiveNormalFlux(normal_vector, right_quadrature_node_variable, convective_flux.right_, right_column);
  const Real left_sqrt_density =
      std::sqrt(left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(left_column));
  const Real right_sqrt_density =
      std::sqrt(right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(right_column));
  const Real sqrt_density_summation = left_sqrt_density + right_sqrt_density;
  roe_variable.template setScalar<ComputationalVariableEnum::Density>(
      std::sqrt(left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(left_column) *
                right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(right_column)),
      0);
  roe_variable.template setVector<ComputationalVariableEnum::Velocity>(
      (left_sqrt_density *
           left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(left_column) +
       right_sqrt_density *
           right_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(right_column)) /
          sqrt_density_summation,
      0);
  const Real left_total_enthapy =
      left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::InternalEnergy>(left_column) *
          physical_model.equation_of_state_.kSpecificHeatRatio +
      left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::VelocitySquaredNorm>(left_column) /
          2.0_r;
  const Real right_total_enthapy =
      right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::InternalEnergy>(right_column) *
          physical_model.equation_of_state_.kSpecificHeatRatio +
      right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::VelocitySquaredNorm>(right_column) /
          2.0_r;
  const Real roe_total_enthapy =
      (left_sqrt_density * left_total_enthapy + right_sqrt_density * right_total_enthapy) / sqrt_density_summation;
  roe_variable.template setScalar<ComputationalVariableEnum::InternalEnergy>(
      (roe_total_enthapy - roe_variable.template getScalar<ComputationalVariableEnum::VelocitySquaredNorm>(0) / 2.0_r) /
          physical_model.equation_of_state_.kSpecificHeatRatio,
      0);
  roe_variable.template setScalar<ComputationalVariableEnum::Pressure>(
      physical_model.calculatePressureFormDensityInternalEnergy(
          roe_variable.template getScalar<ComputationalVariableEnum::Density>(0),
          roe_variable.template getScalar<ComputationalVariableEnum::InternalEnergy>(0)),
      0);
  const Real roe_normal_velocity =
      roe_variable.template getVector<ComputationalVariableEnum::Velocity>(0).transpose() * normal_vector;
  const Real roe_sound_speed = physical_model.calculateSoundSpeedFromDensityPressure(
      roe_variable.template getScalar<ComputationalVariableEnum::Density>(0),
      roe_variable.template getScalar<ComputationalVariableEnum::Pressure>(0));
  delta_variable.computational_ = right_quadrature_node_variable.computational_.col(right_column) -
                                  left_quadrature_node_variable.computational_.col(left_column);
  const Real delta_normal_velocity =
      delta_variable.template getVector<ComputationalVariableEnum::Velocity>(0).transpose() * normal_vector;
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
  roe_matrix.col(0) << 1.0_r,
      roe_variable.template getVector<ComputationalVariableEnum::Velocity>(0) - roe_sound_speed * normal_vector,
      roe_total_enthapy - roe_sound_speed * roe_normal_velocity;
  roe_matrix.col(0) *= lambda_velocity_subtract_sound_speed *
                       (delta_variable.template getScalar<ComputationalVariableEnum::Pressure>(0) -
                        roe_variable.template getScalar<ComputationalVariableEnum::Density>(0) * roe_sound_speed *
                            delta_normal_velocity) /
                       (2.0_r * roe_sound_speed * roe_sound_speed);
  roe_matrix.col(1) << 1.0_r, roe_variable.template getVector<ComputationalVariableEnum::Velocity>(0),
      roe_variable.template getScalar<ComputationalVariableEnum::VelocitySquaredNorm>(0) / 2.0_r;
  roe_matrix.col(1) *=
      std::fabs(roe_normal_velocity) *
      (delta_variable.template getScalar<ComputationalVariableEnum::Density>(0) -
       delta_variable.template getScalar<ComputationalVariableEnum::Pressure>(0) / (roe_sound_speed * roe_sound_speed));
  if constexpr (SimulationControl::kDimension == 2 || SimulationControl::kDimension == 3) {
    roe_matrix.col(2) << 0.0_r,
        delta_variable.template getVector<ComputationalVariableEnum::Velocity>(0) -
            delta_normal_velocity * normal_vector,
        roe_variable.template getVector<ComputationalVariableEnum::Velocity>(0).transpose() *
                delta_variable.template getVector<ComputationalVariableEnum::Velocity>(0) -
            roe_normal_velocity * delta_normal_velocity;
    roe_matrix.col(2) *=
        std::fabs(roe_normal_velocity) * roe_variable.template getScalar<ComputationalVariableEnum::Density>(0);
  }
  roe_matrix.col(SimulationControl::kDimension + 1) << 1.0_r,
      roe_variable.template getVector<ComputationalVariableEnum::Velocity>(0) + roe_sound_speed * normal_vector,
      roe_total_enthapy + roe_sound_speed * roe_normal_velocity;
  roe_matrix.col(SimulationControl::kDimension + 1) *=
      lambda_velocity_add_sound_speed *
      (delta_variable.template getScalar<ComputationalVariableEnum::Pressure>(0) +
       roe_variable.template getScalar<ComputationalVariableEnum::Density>(0) * roe_sound_speed *
           delta_normal_velocity) /
      (2.0_r * roe_sound_speed * roe_sound_speed);
  convective_flux.result_.normal_variable_.noalias() =
      ((convective_flux.left_.normal_variable_ + convective_flux.right_.normal_variable_) -
       roe_matrix.rowwise().sum()) /
      2.0_r;
}

template <typename SimulationControl, int N>
inline void calculateConvectiveExactFlux(const PhysicalModel<SimulationControl>& physical_model,
                                         const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                         const Variable<SimulationControl, N>& left_quadrature_node_variable,
                                         const Variable<SimulationControl, N>& right_quadrature_node_variable,
                                         Flux<SimulationControl>& convective_flux, const Isize left_column,
                                         const Isize right_column) {
  Variable<SimulationControl, 1> exact_variable;
  Real exact_internal_energy;
  Eigen::Vector<Real, SimulationControl::kDimension> exact_velocity;
  const Real sound_speed = physical_model.calculateSoundSpeedFromDensityPressure(0.0_r, 0.0_r);
  const Real left_normal_velocity =
      left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(left_column).transpose() *
      normal_vector;
  const Real right_normal_velocity =
      right_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(right_column).transpose() *
      normal_vector;
  const Real exact_density =
      std::sqrt(left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(left_column) *
                right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(right_column) *
                std::exp((left_normal_velocity - right_normal_velocity) / sound_speed));
  const Real exact_normal_velocity =
      (left_normal_velocity + right_normal_velocity) / 2.0_r +
      std::log(left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(left_column) /
               right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(right_column)) *
          sound_speed / 2.0_r;
  if (exact_normal_velocity < 0.0_r) {
    exact_internal_energy =
        right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::InternalEnergy>(right_column) *
        right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(right_column) /
        exact_density;
    exact_velocity =
        right_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(right_column) +
        (exact_normal_velocity -
         right_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(right_column)
                 .transpose() *
             normal_vector) *
            normal_vector;
  } else {
    exact_internal_energy =
        left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::InternalEnergy>(left_column) *
        left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(left_column) /
        exact_density;
    exact_velocity =
        left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(left_column) +
        (exact_normal_velocity -
         left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(left_column)
                 .transpose() *
             normal_vector) *
            normal_vector;
  }
  const Real exact_pressure =
      physical_model.calculatePressureFormDensityInternalEnergy(exact_density, exact_internal_energy);
  exact_variable.template setScalar<ComputationalVariableEnum::Density>(exact_density, 0);
  exact_variable.template setVector<ComputationalVariableEnum::Velocity>(exact_velocity, 0);
  exact_variable.template setScalar<ComputationalVariableEnum::InternalEnergy>(exact_internal_energy, 0);
  exact_variable.template setScalar<ComputationalVariableEnum::Pressure>(exact_pressure, 0);
  calculateConvectiveNormalFlux(normal_vector, exact_variable, convective_flux.result_, 0);
}

template <typename SimulationControl, int N>
inline void calculateConvectiveFlux(const PhysicalModel<SimulationControl>& physical_model,
                                    const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                    const Variable<SimulationControl, N>& left_quadrature_node_variable,
                                    const Variable<SimulationControl, N>& right_quadrature_node_variable,
                                    Flux<SimulationControl>& convective_flux, const Isize left_column,
                                    const Isize right_column) {
  if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFluxEnum::Central) {
    calculateConvectiveCentralFlux(physical_model, normal_vector, left_quadrature_node_variable,
                                   right_quadrature_node_variable, convective_flux, left_column, right_column);
  } else if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFluxEnum::LaxFriedrichs) {
    calculateConvectiveLaxFriedrichsFlux(physical_model, normal_vector, left_quadrature_node_variable,
                                         right_quadrature_node_variable, convective_flux, left_column, right_column);
  } else if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFluxEnum::HLLC) {
    calculateConvectiveHLLCFlux(physical_model, normal_vector, left_quadrature_node_variable,
                                right_quadrature_node_variable, convective_flux, left_column, right_column);
  } else if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFluxEnum::Roe) {
    calculateConvectiveRoeFlux(physical_model, normal_vector, left_quadrature_node_variable,
                               right_quadrature_node_variable, convective_flux, left_column, right_column);
  } else if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFluxEnum::Exact) {
    calculateConvectiveExactFlux(physical_model, normal_vector, left_quadrature_node_variable,
                                 right_quadrature_node_variable, convective_flux, left_column, right_column);
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CONVETIVE_FLUX_CPP_
