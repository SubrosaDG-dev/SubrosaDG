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

#include "Solver/SolveControl.hpp"
#include "Solver/ThermalModel.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Enum.hpp"

namespace SubrosaDG {

template <typename SimulationControl>
inline void calculateConvectiveVariable(const Variable<SimulationControl, SimulationControl::kDimension>& variable,
                                        Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber,
                                                      SimulationControl::kDimension>& convective_variable) {
  if constexpr (SimulationControl::kDimension == 2) {
    const Real density = variable.primitive_(0);
    const Real velocity_x = variable.primitive_(1);
    const Real velocity_y = variable.primitive_(2);
    const Real pressure = variable.primitive_(3);
    const Real total_energy = variable.primitive_(4) + 0.5 * (velocity_x * velocity_x + velocity_y * velocity_y);
    convective_variable.col(0) << density * velocity_x, density * velocity_x * velocity_x + pressure,
        density * velocity_x * velocity_y, density * velocity_x * total_energy + pressure * velocity_x;
    convective_variable.col(1) << density * velocity_y, density * velocity_y * velocity_x,
        density * velocity_y * velocity_y + pressure, density * velocity_y * total_energy + pressure * velocity_y;
  } else if constexpr (SimulationControl::kDimension == 3) {
    const Real density = variable.primitive_(0);
    const Real velocity_x = variable.primitive_(1);
    const Real velocity_y = variable.primitive_(2);
    const Real velocity_z = variable.primitive_(3);
    const Real pressure = variable.primitive_(4);
    const Real total_energy =
        variable.primitive_(5) + 0.5 * (velocity_x * velocity_x + velocity_y * velocity_y + velocity_z * velocity_z);
    convective_variable.col(0) << density * velocity_x, density * velocity_x * velocity_x + pressure,
        density * velocity_x * velocity_y, density * velocity_x * velocity_z,
        density * velocity_x * total_energy + pressure * velocity_x;
    convective_variable.col(1) << density * velocity_y, density * velocity_y * velocity_x,
        density * velocity_y * velocity_y + pressure, density * velocity_y * velocity_z,
        density * velocity_y * total_energy + pressure * velocity_y;
    convective_variable.col(2) << density * velocity_z, density * velocity_z * velocity_x,
        density * velocity_z * velocity_y, density * velocity_z * velocity_z + pressure,
        density * velocity_z * total_energy + pressure * velocity_z;
  }
}

template <typename SimulationControl>
inline void calculateConvectiveCentralFlux(
    [[maybe_unused]] const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
    const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
    const Variable<SimulationControl, SimulationControl::kDimension>& left_quadrature_node_variable,
    const Variable<SimulationControl, SimulationControl::kDimension>& right_quadrature_node_variable,
    Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& convective_flux) {
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, SimulationControl::kDimension>
      left_quadrature_node_convective_variable;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, SimulationControl::kDimension>
      right_quadrature_node_convective_variable;
  calculateConvectiveVariable(left_quadrature_node_variable, left_quadrature_node_convective_variable);
  calculateConvectiveVariable(right_quadrature_node_variable, right_quadrature_node_convective_variable);
  convective_flux =
      0.5 * (left_quadrature_node_convective_variable + right_quadrature_node_convective_variable) * normal_vector;
}

template <typename SimulationControl>
inline void calculateConvectiveLaxFriedrichsFlux(
    const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
    const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
    const Variable<SimulationControl, SimulationControl::kDimension>& left_quadrature_node_variable,
    const Variable<SimulationControl, SimulationControl::kDimension>& right_quadrature_node_variable,
    Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& convective_flux) {
  if constexpr (SimulationControl::kDimension == 2) {
    Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, SimulationControl::kDimension>
        left_quadrature_node_convective_variable;
    Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, SimulationControl::kDimension>
        right_quadrature_node_convective_variable;
    calculateConvectiveVariable(left_quadrature_node_variable, left_quadrature_node_convective_variable);
    calculateConvectiveVariable(right_quadrature_node_variable, right_quadrature_node_convective_variable);
    const Real left_velocity_normal = left_quadrature_node_variable.primitive_(1) * normal_vector.x() +
                                      left_quadrature_node_variable.primitive_(2) * normal_vector.y();
    const Real right_velocity_normal = right_quadrature_node_variable.primitive_(1) * normal_vector.x() +
                                       right_quadrature_node_variable.primitive_(2) * normal_vector.y();
    const Real left_sound_speed =
        thermal_model.calculateSoundSpeedFromInternalEnergy(left_quadrature_node_variable.primitive_(4));
    const Real right_sound_speed =
        thermal_model.calculateSoundSpeedFromInternalEnergy(right_quadrature_node_variable.primitive_(4));
    const Real spectral_radius = std::ranges::max(std::fabs(left_velocity_normal) + left_sound_speed,
                                                  std::fabs(right_velocity_normal) + right_sound_speed);
    convective_flux =
        0.5 * (left_quadrature_node_convective_variable + right_quadrature_node_convective_variable) * normal_vector -
        0.5 * spectral_radius * (right_quadrature_node_variable.conserved_ - left_quadrature_node_variable.conserved_);
  }
}

template <typename SimulationControl>
inline void calculateConvectiveRoeFlux(
    const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
    const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
    const Variable<SimulationControl, SimulationControl::kDimension>& left_quadrature_node_variable,
    const Variable<SimulationControl, SimulationControl::kDimension>& right_quadrature_node_variable,
    Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& convective_flux) {
  if constexpr (SimulationControl::kDimension == 2) {
    const Real left_sqrt_density = std::sqrt(left_quadrature_node_variable.primitive_(0));
    const Real right_sqrt_density = std::sqrt(right_quadrature_node_variable.primitive_(0));
    const Real sqrt_density_summation = left_sqrt_density + right_sqrt_density;
    const Real roe_density =
        std::sqrt(left_quadrature_node_variable.primitive_(0) * right_quadrature_node_variable.primitive_(0));
    const Real roe_velocity_x = (left_sqrt_density * left_quadrature_node_variable.primitive_(1) +
                                 right_sqrt_density * right_quadrature_node_variable.primitive_(1)) /
                                sqrt_density_summation;
    const Real roe_velocity_y = (left_sqrt_density * left_quadrature_node_variable.primitive_(2) +
                                 right_sqrt_density * right_quadrature_node_variable.primitive_(2)) /
                                sqrt_density_summation;
    const Real left_enthalpy =
        thermal_model.calculateEnthalpyFromInternalEnergy(left_quadrature_node_variable.primitive_(4));
    const Real right_enthalpy =
        thermal_model.calculateEnthalpyFromInternalEnergy(right_quadrature_node_variable.primitive_(4));
    const Real roe_enthalpy =
        (left_sqrt_density * left_enthalpy + right_sqrt_density * right_enthalpy) / sqrt_density_summation;
    const Real roe_velocity_square_summation = roe_velocity_x * roe_velocity_x + roe_velocity_y * roe_velocity_y;
    const Real roe_velocity_normal = roe_velocity_x * normal_vector.x() + roe_velocity_y * normal_vector.y();
    const Real roe_sound_speed = thermal_model.calculateSoundSpeedFromEnthalpySubtractVelocitySquareSummation(
        roe_enthalpy - 0.5 * roe_velocity_square_summation);
    const Real delta_density =
        right_quadrature_node_variable.primitive_(0) - left_quadrature_node_variable.primitive_(0);
    const Real delta_velocity_x =
        right_quadrature_node_variable.primitive_(1) - left_quadrature_node_variable.primitive_(1);
    const Real delta_velocity_y =
        right_quadrature_node_variable.primitive_(2) - left_quadrature_node_variable.primitive_(2);
    const Real delta_pressure =
        right_quadrature_node_variable.primitive_(3) - left_quadrature_node_variable.primitive_(3);
    const Real delta_velocity_normal = delta_velocity_x * normal_vector(0) + delta_velocity_y * normal_vector(1);
    Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> roe_temporary_flux_1;
    Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> roe_temporary_flux_2;
    Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> roe_temporary_flux_34;
    Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> roe_temporary_flux_5;
    roe_temporary_flux_1 << 1.0, roe_velocity_x - roe_sound_speed * normal_vector.x(),
        roe_velocity_y - roe_sound_speed * normal_vector.y(), roe_enthalpy - roe_sound_speed * roe_velocity_normal;
    roe_temporary_flux_1 *= std::fabs(roe_velocity_normal - roe_sound_speed) *
                            (delta_pressure - roe_density * roe_sound_speed * delta_velocity_normal) /
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
    roe_temporary_flux_5 *= std::fabs(roe_velocity_normal + roe_sound_speed) *
                            (delta_pressure + roe_density * roe_sound_speed * delta_velocity_normal) /
                            (2.0 * roe_sound_speed * roe_sound_speed);
    Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, SimulationControl::kDimension>
        left_quadrature_node_convective_variable;
    Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, SimulationControl::kDimension>
        right_quadrature_node_convective_variable;
    calculateConvectiveVariable(left_quadrature_node_variable, left_quadrature_node_convective_variable);
    calculateConvectiveVariable(right_quadrature_node_variable, right_quadrature_node_convective_variable);
    convective_flux.noalias() =
        0.5 * (left_quadrature_node_convective_variable + right_quadrature_node_convective_variable) * normal_vector -
        0.5 * (roe_temporary_flux_1 + std::fabs(roe_velocity_normal) * (roe_temporary_flux_2 + roe_temporary_flux_34) +
               roe_temporary_flux_5);
  }
}

template <typename SimulationControl>
inline void calculateConvectiveFlux(
    const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
    const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
    const Variable<SimulationControl, SimulationControl::kDimension>& left_quadrature_node_variable,
    const Variable<SimulationControl, SimulationControl::kDimension>& right_quadrature_node_variable,
    Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& convective_flux) {
  if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFlux::Central) {
    calculateConvectiveCentralFlux(thermal_model, normal_vector, left_quadrature_node_variable,
                                   right_quadrature_node_variable, convective_flux);
  } else if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFlux::LaxFriedrichs) {
    calculateConvectiveLaxFriedrichsFlux(thermal_model, normal_vector, left_quadrature_node_variable,
                                         right_quadrature_node_variable, convective_flux);
  } else if constexpr (SimulationControl::kConvectiveFlux == ConvectiveFlux::Roe) {
    calculateConvectiveRoeFlux(thermal_model, normal_vector, left_quadrature_node_variable,
                               right_quadrature_node_variable, convective_flux);
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CONVETIVE_FLUX_HPP_
