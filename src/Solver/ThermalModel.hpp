/**
 * @file ThermalModel.hpp
 * @brief The header file of ThermalModel.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_THERMAL_MODEL_HPP_
#define SUBROSA_DG_THERMAL_MODEL_HPP_

#include <cmath>

#include "Utils/BasicDataType.hpp"
#include "Utils/Enum.hpp"

namespace SubrosaDG {

template <ThermodynamicModel ThermodynamicModelType>
struct ThermodynamicModelData;

template <>
struct ThermodynamicModelData<ThermodynamicModel::ConstantE> {
  Real specific_heat_constant_volume_ = 25.0 / 14.0;

  [[nodiscard]] inline Real getInternalEnergy(const Real temperature) const {
    return specific_heat_constant_volume_ * temperature;
  }

  [[nodiscard]] inline Real getTemperature(const Real internal_energy) const {
    return internal_energy / specific_heat_constant_volume_;
  }
};

template <>
struct ThermodynamicModelData<ThermodynamicModel::ConstantH> {
  Real specific_heat_constant_pressure_ = 5.0 / 2.0;

  [[nodiscard]] inline Real getEnthalpy(const Real temperature) const {
    return specific_heat_constant_pressure_ * temperature;
  }

  [[nodiscard]] inline Real getTemperature(const Real enthalpy) const {
    return enthalpy / specific_heat_constant_pressure_;
  }
};

template <EquationOfState EquationOfStateType>
struct EquationOfStateData;

template <>
struct EquationOfStateData<EquationOfState::IdealGas> {
  Real specific_heat_ratio_ = 1.4;

  [[nodiscard]] inline Real getPressureFormInternalEnergy(const Real density, const Real internal_energy) const {
    return (specific_heat_ratio_ - 1.0) * density * internal_energy;
  }

  [[nodiscard]] inline Real getPressureFromEnthalpy(const Real density, const Real enthalpy) const {
    return (specific_heat_ratio_ - 1.0) * density * enthalpy / specific_heat_ratio_;
  }

  [[nodiscard]] inline Real getInternalEnergyFromEnthalpy(const Real enthalpy) const {
    return enthalpy / specific_heat_ratio_;
  }

  [[nodiscard]] inline Real getEnthalpyFromInternalEnergy(const Real internal_energy) const {
    return internal_energy * specific_heat_ratio_;
  }

  [[nodiscard]] inline Real getSoundSpeedFromInternalEnergy(const Real internal_energy) const {
    return std::sqrt(specific_heat_ratio_ * (specific_heat_ratio_ - 1.0) * internal_energy);
  }

  [[nodiscard]] inline Real getSoundSpeedFromEnthalpySubtractVelocitySquareSummation(
      const Real enthalpy_subtract_velocity_square_summation) const {
    return std::sqrt((specific_heat_ratio_ - 1.0) * enthalpy_subtract_velocity_square_summation);
  }
};

template <typename SimulationControl, EquationModel EquationModelType>
struct ThermalModel;

template <typename SimulationControl>
struct ThermalModel<SimulationControl, EquationModel::Euler>
    : ThermodynamicModelData<SimulationControl::kThermodynamicModel>,
      EquationOfStateData<SimulationControl::kEquationOfState> {};  // NOTE shell getInternalEnergy function here

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_THERMAL_MODEL_HPP_
