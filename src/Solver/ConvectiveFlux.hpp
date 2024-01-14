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
inline void calculateConvectiveVariable(const Variable<SimulationControl>& variable,
                                        Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber,
                                                      SimulationControl::kDimension>& convective_variable) {
  const Real density = variable.template get<ComputationalVariableEnum::Density>();
  const Eigen::Vector<Real, SimulationControl::kDimension>& velocity =
      variable.template getVector<ComputationalVariableEnum::Velocity>();
  const Real pressure = variable.template get<ComputationalVariableEnum::Pressure>();
  const Real total_energy = variable.template get<ComputationalVariableEnum::InternalEnergy>() +
                            variable.template get<ComputationalVariableEnum::VelocitySquareSummation>() / 2.0;
  convective_variable.row(0) = density * velocity.transpose();
  convective_variable(Eigen::seqN(Eigen::fix<1>, Eigen::fix<SimulationControl::kDimension>), Eigen::all).noalias() =
      density * velocity * velocity.transpose();
  convective_variable(Eigen::seqN(Eigen::fix<1>, Eigen::fix<SimulationControl::kDimension>), Eigen::all).noalias() +=
      pressure * Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>::Identity();
  convective_variable.row(SimulationControl::kDimension + 1) =
      (density * total_energy + pressure) * velocity.transpose();
}

template <typename SimulationControl>
inline void calculateConvectiveNormalVariable(
    const Variable<SimulationControl>& variable,
    const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>& transition_matrix,
    Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& convective_normal_variable) {
  const Real density = variable.template get<ComputationalVariableEnum::Density>();
  const Real normal_velocity =
      variable.template getVector<ComputationalVariableEnum::Velocity>().transpose() * transition_matrix.col(0);
  const Real pressure = variable.template get<ComputationalVariableEnum::Pressure>();
  const Real total_energy = variable.template get<ComputationalVariableEnum::InternalEnergy>() +
                            variable.template get<ComputationalVariableEnum::VelocitySquareSummation>() / 2.0;
  convective_normal_variable << density * normal_velocity,
      density * variable.template getVector<ComputationalVariableEnum::Velocity>() * normal_velocity +
          pressure * transition_matrix.col(0),
      (density * total_energy + pressure) * normal_velocity;
}

template <typename SimulationControl>
inline void calculateConvectiveCentralFlux(
    [[maybe_unused]] const ThermalModel<SimulationControl>& thermal_model,
    const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>& transition_matrix,
    const Variable<SimulationControl>& left_quadrature_node_variable,
    const Variable<SimulationControl>& right_quadrature_node_variable, Flux<SimulationControl>& flux) {
  calculateConvectiveNormalVariable(left_quadrature_node_variable, transition_matrix, flux.left_convective_n_);
  calculateConvectiveNormalVariable(right_quadrature_node_variable, transition_matrix, flux.right_convective_n_);
  flux.convective_n_.noalias() = (flux.left_convective_n_ + flux.right_convective_n_) / 2.0;
}

template <typename SimulationControl>
inline void calculateConvectiveLaxFriedrichsFlux(
    const ThermalModel<SimulationControl>& thermal_model,
    const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>& transition_matrix,
    const Variable<SimulationControl>& left_quadrature_node_variable,
    const Variable<SimulationControl>& right_quadrature_node_variable, Flux<SimulationControl>& flux) {
  calculateConvectiveNormalVariable(left_quadrature_node_variable, transition_matrix, flux.left_convective_n_);
  calculateConvectiveNormalVariable(right_quadrature_node_variable, transition_matrix, flux.right_convective_n_);
  const Real left_normal_velocity =
      left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>().transpose() *
      transition_matrix.col(0);
  const Real right_normal_velocity =
      right_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>().transpose() *
      transition_matrix.col(0);
  const Real left_sound_speed = thermal_model.calculateSoundSpeedFromInternalEnergy(
      left_quadrature_node_variable.template get<ComputationalVariableEnum::InternalEnergy>());
  const Real right_sound_speed = thermal_model.calculateSoundSpeedFromInternalEnergy(
      right_quadrature_node_variable.template get<ComputationalVariableEnum::InternalEnergy>());
  const Real spectral_radius = std::ranges::max(std::abs(left_normal_velocity) + left_sound_speed,
                                                std::abs(right_normal_velocity) + right_sound_speed);
  flux.convective_n_.noalias() =
      ((flux.left_convective_n_ + flux.right_convective_n_) -
       spectral_radius * (right_quadrature_node_variable.conserved_ - left_quadrature_node_variable.conserved_)) /
      2.0;
}

template <typename SimulationControl>
inline void calculateConvectiveHLLCFlux(
    const ThermalModel<SimulationControl>& thermal_model,
    const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>& transition_matrix,
    const Variable<SimulationControl>& left_quadrature_node_variable,
    const Variable<SimulationControl>& right_quadrature_node_variable, Flux<SimulationControl>& flux) {}

template <typename SimulationControl>
inline void calculateConvectiveRoeFlux(
    const ThermalModel<SimulationControl>& thermal_model,
    const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>& transition_matrix,
    const Variable<SimulationControl>& left_quadrature_node_variable,
    const Variable<SimulationControl>& right_quadrature_node_variable, Flux<SimulationControl>& flux) {
  Variable<SimulationControl> roe_variable;
  Variable<SimulationControl> delta_variable;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, SimulationControl::kConservedVariableNumber>
      roe_matrix;
  roe_matrix.setZero();
  calculateConvectiveNormalVariable(left_quadrature_node_variable, transition_matrix, flux.left_convective_n_);
  calculateConvectiveNormalVariable(right_quadrature_node_variable, transition_matrix, flux.right_convective_n_);
  const Real left_sqrt_density =
      std::sqrt(left_quadrature_node_variable.template get<ComputationalVariableEnum::Density>());
  const Real right_sqrt_density =
      std::sqrt(right_quadrature_node_variable.template get<ComputationalVariableEnum::Density>());
  const Real sqrt_density_summation = left_sqrt_density + right_sqrt_density;
  roe_variable.template set<ComputationalVariableEnum::Density>(
      std::sqrt(left_quadrature_node_variable.template get<ComputationalVariableEnum::Density>() *
                right_quadrature_node_variable.template get<ComputationalVariableEnum::Density>()));
  roe_variable.template setVector<ComputationalVariableEnum::Velocity>(
      (left_sqrt_density * left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>() +
       right_sqrt_density * right_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>()) /
      sqrt_density_summation);
  const Real left_total_enthapy =
      thermal_model.calculateEnthalpyFromInternalEnergy(
          left_quadrature_node_variable.template get<ComputationalVariableEnum::InternalEnergy>()) +
      left_quadrature_node_variable.template get<ComputationalVariableEnum::VelocitySquareSummation>() / 2.0;
  const Real right_total_enthapy =
      thermal_model.calculateEnthalpyFromInternalEnergy(
          right_quadrature_node_variable.template get<ComputationalVariableEnum::InternalEnergy>()) +
      right_quadrature_node_variable.template get<ComputationalVariableEnum::VelocitySquareSummation>() / 2.0;
  const Real roe_total_enthapy =
      (left_sqrt_density * left_total_enthapy + right_sqrt_density * right_total_enthapy) / sqrt_density_summation;
  roe_variable.template set<ComputationalVariableEnum::InternalEnergy>(
      thermal_model.calculateInternalEnergyFromEnthalpy(
          roe_total_enthapy - roe_variable.template get<ComputationalVariableEnum::VelocitySquareSummation>() / 2.0));
  roe_variable.template set<ComputationalVariableEnum::Pressure>(
      thermal_model.calculatePressureFormDensityInternalEnergy(
          roe_variable.template get<ComputationalVariableEnum::Density>(),
          roe_variable.template get<ComputationalVariableEnum::InternalEnergy>()));
  const Real roe_normal_velocity =
      roe_variable.template getVector<ComputationalVariableEnum::Velocity>().transpose() * transition_matrix.col(0);
  const Real roe_sound_speed = thermal_model.calculateSoundSpeedFromInternalEnergy(
      roe_variable.template get<ComputationalVariableEnum::InternalEnergy>());
  delta_variable.template set<ComputationalVariableEnum::Density>(
      right_quadrature_node_variable.template get<ComputationalVariableEnum::Density>() -
      left_quadrature_node_variable.template get<ComputationalVariableEnum::Density>());
  delta_variable.template setVector<ComputationalVariableEnum::Velocity>(
      right_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>() -
      left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>());
  delta_variable.template set<ComputationalVariableEnum::InternalEnergy>(
      right_quadrature_node_variable.template get<ComputationalVariableEnum::InternalEnergy>() -
      left_quadrature_node_variable.template get<ComputationalVariableEnum::InternalEnergy>());
  delta_variable.template set<ComputationalVariableEnum::Pressure>(
      right_quadrature_node_variable.template get<ComputationalVariableEnum::Pressure>() -
      left_quadrature_node_variable.template get<ComputationalVariableEnum::Pressure>());
  const Real delta_normal_velocity =
      delta_variable.template getVector<ComputationalVariableEnum::Velocity>().transpose() * transition_matrix.col(0);
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
      roe_variable.template getVector<ComputationalVariableEnum::Velocity>() -
          roe_sound_speed * transition_matrix.col(0),
      roe_total_enthapy - roe_sound_speed * roe_normal_velocity;
  roe_matrix.col(0) *=
      lambda_velocity_subtract_sound_speed *
      (delta_variable.template get<ComputationalVariableEnum::Pressure>() -
       roe_variable.template get<ComputationalVariableEnum::Density>() * roe_sound_speed * delta_normal_velocity) /
      (2.0 * roe_sound_speed * roe_sound_speed);
  roe_matrix.col(1) << 1.0, roe_variable.template getVector<ComputationalVariableEnum::Velocity>(),
      roe_variable.template get<ComputationalVariableEnum::VelocitySquareSummation>() / 2.0;
  roe_matrix.col(1) *=
      std::abs(roe_normal_velocity) *
      (delta_variable.template get<ComputationalVariableEnum::Density>() -
       delta_variable.template get<ComputationalVariableEnum::Pressure>() / (roe_sound_speed * roe_sound_speed));
  if constexpr (SimulationControl::kDimension == 2 || SimulationControl::kDimension == 3) {
    roe_matrix.col(2) << 0.0,
        delta_variable.template getVector<ComputationalVariableEnum::Velocity>() -
            delta_normal_velocity * transition_matrix.col(0),
        roe_variable.template getVector<ComputationalVariableEnum::Velocity>().transpose() *
                delta_variable.template getVector<ComputationalVariableEnum::Velocity>() -
            roe_normal_velocity * delta_normal_velocity;
    roe_matrix.col(2) *=
        std::fabs(roe_normal_velocity) * roe_variable.template get<ComputationalVariableEnum::Density>();
  }
  roe_matrix.col(SimulationControl::kDimension + 1) << 1.0,
      roe_variable.template getVector<ComputationalVariableEnum::Velocity>() +
          roe_sound_speed * transition_matrix.col(0),
      roe_total_enthapy + roe_sound_speed * roe_normal_velocity;
  roe_matrix.col(SimulationControl::kDimension + 1) *=
      lambda_velocity_add_sound_speed *
      (delta_variable.template get<ComputationalVariableEnum::Pressure>() +
       roe_variable.template get<ComputationalVariableEnum::Density>() * roe_sound_speed * delta_normal_velocity) /
      (2.0 * roe_sound_speed * roe_sound_speed);
  flux.convective_n_.noalias() =
      ((flux.left_convective_n_ + flux.right_convective_n_) - roe_matrix.rowwise().sum()) / 2.0;
}

template <typename SimulationControl>
inline void calculateConvectiveFlux(
    const ThermalModel<SimulationControl>& thermal_model,
    const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>& transition_matrix,
    const Variable<SimulationControl>& left_quadrature_node_variable,
    const Variable<SimulationControl>& right_quadrature_node_variable, Flux<SimulationControl>& flux) {
  if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFluxEnum::Central) {
    calculateConvectiveCentralFlux(thermal_model, transition_matrix, left_quadrature_node_variable,
                                   right_quadrature_node_variable, flux);
  } else if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFluxEnum::LaxFriedrichs) {
    calculateConvectiveLaxFriedrichsFlux(thermal_model, transition_matrix, left_quadrature_node_variable,
                                         right_quadrature_node_variable, flux);
  } else if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFluxEnum::Roe) {
    calculateConvectiveRoeFlux(thermal_model, transition_matrix, left_quadrature_node_variable,
                               right_quadrature_node_variable, flux);
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CONVETIVE_FLUX_HPP_
