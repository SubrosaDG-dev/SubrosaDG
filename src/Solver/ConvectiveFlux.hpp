/**
 * @file ConvectiveFlux.hpp
 * @brief The header file of ConvectiveFlux.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
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
  const Real density = variable.template get<ComputationalVariable::Density>();
  const Eigen::Vector<Real, SimulationControl::kDimension> velocity = variable.getVelocity();
  const Real pressure = variable.template get<ComputationalVariable::Pressure>();
  const Real total_energy =
      variable.template get<ComputationalVariable::InternalEnergy>() + 0.5 * variable.getVelocitySquareSummation();
  convective_variable.row(0) = density * velocity.transpose();
  convective_variable(Eigen::seqN(Eigen::fix<1>, Eigen::fix<SimulationControl::kDimension>), Eigen::all) =
      density * velocity * velocity.transpose() +
      pressure * Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>::Identity();
  convective_variable.row(SimulationControl::kDimension + 1) =
      (density * total_energy + pressure) * velocity.transpose();
}

template <typename SimulationControl>
inline void calculateConvectiveCentralFlux(
    [[maybe_unused]] const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
    const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
    const Variable<SimulationControl>& left_quadrature_node_variable,
    const Variable<SimulationControl>& right_quadrature_node_variable,
    Flux<SimulationControl, SimulationControl::kEquationModel>& flux) {
  calculateConvectiveVariable(left_quadrature_node_variable, flux.left_convective_);
  calculateConvectiveVariable(right_quadrature_node_variable, flux.right_convective_);
  flux.convective_n_.noalias() = 0.5 * (flux.left_convective_ + flux.right_convective_) * normal_vector;
}

template <typename SimulationControl>
inline void calculateConvectiveLaxFriedrichsFlux(
    const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
    const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
    const Variable<SimulationControl>& left_quadrature_node_variable,
    const Variable<SimulationControl>& right_quadrature_node_variable,
    Flux<SimulationControl, SimulationControl::kEquationModel>& flux) {
  calculateConvectiveVariable(left_quadrature_node_variable, flux.left_convective_);
  calculateConvectiveVariable(right_quadrature_node_variable, flux.right_convective_);
  const Real left_velocity_normal = left_quadrature_node_variable.getVelocity().transpose() * normal_vector;
  const Real right_velocity_normal = right_quadrature_node_variable.getVelocity().transpose() * normal_vector;
  const Real left_sound_speed = thermal_model.calculateSoundSpeedFromInternalEnergy(
      left_quadrature_node_variable.template get<ComputationalVariable::InternalEnergy>());
  const Real right_sound_speed = thermal_model.calculateSoundSpeedFromInternalEnergy(
      right_quadrature_node_variable.template get<ComputationalVariable::InternalEnergy>());
  const Real spectral_radius = std::ranges::max(std::fabs(left_velocity_normal) + left_sound_speed,
                                                std::fabs(right_velocity_normal) + right_sound_speed);
  flux.convective_n_.noalias() =
      0.5 * (flux.left_convective_ + flux.right_convective_) * normal_vector -
      0.5 * spectral_radius * (right_quadrature_node_variable.conserved_ - left_quadrature_node_variable.conserved_);
}

template <typename SimulationControl>
inline void calculateConvectiveRoeFlux(
    const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
    const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
    const Variable<SimulationControl>& left_quadrature_node_variable,
    const Variable<SimulationControl>& right_quadrature_node_variable,
    Flux<SimulationControl, SimulationControl::kEquationModel>& flux) {
  if constexpr (SimulationControl::kDimension == 2) {
    const Real left_sqrt_density =
        std::sqrt(left_quadrature_node_variable.template get<ComputationalVariable::Density>());
    const Real right_sqrt_density =
        std::sqrt(right_quadrature_node_variable.template get<ComputationalVariable::Density>());
    const Real sqrt_density_summation = left_sqrt_density + right_sqrt_density;
    const Real roe_density = std::sqrt(left_quadrature_node_variable.template get<ComputationalVariable::Density>() *
                                       right_quadrature_node_variable.template get<ComputationalVariable::Density>());
    const Real roe_velocity_x =
        (left_sqrt_density * left_quadrature_node_variable.template get<ComputationalVariable::VelocityX>() +
         right_sqrt_density * right_quadrature_node_variable.template get<ComputationalVariable::VelocityX>()) /
        sqrt_density_summation;
    const Real roe_velocity_y =
        (left_sqrt_density * left_quadrature_node_variable.template get<ComputationalVariable::VelocityY>() +
         right_sqrt_density * right_quadrature_node_variable.template get<ComputationalVariable::VelocityY>()) /
        sqrt_density_summation;
    const Real left_enthalpy = thermal_model.calculateEnthalpyFromInternalEnergy(
        left_quadrature_node_variable.template get<ComputationalVariable::InternalEnergy>());
    const Real right_enthalpy = thermal_model.calculateEnthalpyFromInternalEnergy(
        right_quadrature_node_variable.template get<ComputationalVariable::InternalEnergy>());
    const Real roe_enthalpy =
        (left_sqrt_density * left_enthalpy + right_sqrt_density * right_enthalpy) / sqrt_density_summation;
    const Real roe_velocity_square_summation = roe_velocity_x * roe_velocity_x + roe_velocity_y * roe_velocity_y;
    const Real roe_velocity_normal = roe_velocity_x * normal_vector.x() + roe_velocity_y * normal_vector.y();
    const Real roe_sound_speed = thermal_model.calculateSoundSpeedFromEnthalpySubtractVelocitySquareSummation(
        roe_enthalpy - 0.5 * roe_velocity_square_summation);
    const Real delta_density = right_quadrature_node_variable.template get<ComputationalVariable::Density>() -
                               left_quadrature_node_variable.template get<ComputationalVariable::Density>();
    const Real delta_velocity_x = right_quadrature_node_variable.template get<ComputationalVariable::VelocityX>() -
                                  left_quadrature_node_variable.template get<ComputationalVariable::VelocityX>();
    const Real delta_velocity_y = right_quadrature_node_variable.template get<ComputationalVariable::VelocityY>() -
                                  left_quadrature_node_variable.template get<ComputationalVariable::VelocityY>();
    const Real delta_pressure = right_quadrature_node_variable.template get<ComputationalVariable::Pressure>() -
                                left_quadrature_node_variable.template get<ComputationalVariable::Pressure>();
    const Real delta_velocity_normal = delta_velocity_x * normal_vector(0) + delta_velocity_y * normal_vector(1);
    Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> roe_temporary_flux_1;
    Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> roe_temporary_flux_2;
    Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> roe_temporary_flux_34;
    Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> roe_temporary_flux_5;
    roe_temporary_flux_1 << 1.0, roe_velocity_x - roe_sound_speed * normal_vector.x(),
        roe_velocity_y - roe_sound_speed * normal_vector.y(), roe_enthalpy - roe_sound_speed * roe_velocity_normal;
    roe_temporary_flux_1 *= (delta_pressure - roe_density * roe_sound_speed * delta_velocity_normal) /
                            (2.0 * roe_sound_speed * roe_sound_speed);
    roe_temporary_flux_2 << 1.0, roe_velocity_x, roe_velocity_y, 0.5 * roe_velocity_square_summation;
    roe_temporary_flux_2 *= (delta_density - delta_pressure / (roe_sound_speed * roe_sound_speed));
    roe_temporary_flux_34 << 0.0, delta_velocity_x - delta_velocity_normal * normal_vector.x(),
        delta_velocity_y - delta_velocity_normal * normal_vector.y(),
        roe_velocity_x * delta_velocity_x + roe_velocity_y * delta_velocity_y -
            roe_velocity_normal * delta_velocity_normal;
    roe_temporary_flux_34 *= roe_density;
    roe_temporary_flux_5 << 1.0, roe_velocity_x + roe_sound_speed * normal_vector.x(),
        roe_velocity_y + roe_sound_speed * normal_vector.y(), roe_enthalpy + roe_sound_speed * roe_velocity_normal;
    roe_temporary_flux_5 *= (delta_pressure + roe_density * roe_sound_speed * delta_velocity_normal) /
                            (2.0 * roe_sound_speed * roe_sound_speed);
    calculateConvectiveVariable(left_quadrature_node_variable, flux.left_convective_);
    calculateConvectiveVariable(right_quadrature_node_variable, flux.right_convective_);
    flux.convective_n_.noalias() =
        0.5 * (flux.left_convective_ + flux.right_convective_) * normal_vector -
        0.5 * (std::fabs(roe_velocity_normal - roe_sound_speed) * roe_temporary_flux_1 +
               std::fabs(roe_velocity_normal) * (roe_temporary_flux_2 + roe_temporary_flux_34) +
               std::fabs(roe_velocity_normal + roe_sound_speed) * roe_temporary_flux_5);
  }
}

template <typename SimulationControl>
inline void calculateConvectiveFlux(
    const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
    const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
    const Variable<SimulationControl>& left_quadrature_node_variable,
    const Variable<SimulationControl>& right_quadrature_node_variable,
    Flux<SimulationControl, SimulationControl::kEquationModel>& flux) {
  if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFlux::Central) {
    calculateConvectiveCentralFlux(thermal_model, normal_vector, left_quadrature_node_variable,
                                   right_quadrature_node_variable, flux);
  } else if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFlux::LaxFriedrichs) {
    calculateConvectiveLaxFriedrichsFlux(thermal_model, normal_vector, left_quadrature_node_variable,
                                         right_quadrature_node_variable, flux);
  } else if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFlux::Roe) {
    calculateConvectiveRoeFlux(thermal_model, normal_vector, left_quadrature_node_variable,
                               right_quadrature_node_variable, flux);
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CONVETIVE_FLUX_HPP_
