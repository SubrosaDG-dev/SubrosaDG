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

  [[nodiscard]] inline Real calculateInternalEnergyFromTemperature(const Real temperature) const {
    return specific_heat_constant_volume_ * temperature;
  }

  [[nodiscard]] inline Real calculateTemperatureFormInternalEnergy(const Real internal_energy) const {
    return internal_energy / specific_heat_constant_volume_;
  }
};

template <>
struct ThermodynamicModelData<ThermodynamicModel::ConstantH> {
  Real specific_heat_constant_pressure_ = 5.0 / 2.0;

  [[nodiscard]] inline Real calculateEnthalpyFromTemperature(const Real temperature) const {
    return specific_heat_constant_pressure_ * temperature;
  }

  [[nodiscard]] inline Real calculateTemperatureFromEnthalpy(const Real enthalpy) const {
    return enthalpy / specific_heat_constant_pressure_;
  }
};

template <EquationOfState EquationOfStateType>
struct EquationOfStateData;

template <>
struct EquationOfStateData<EquationOfState::IdealGas> {
  Real specific_heat_ratio_ = 1.4;

  [[nodiscard]] inline Real calculatePressureFormDensityInternalEnergy(const Real density,
                                                                       const Real internal_energy) const {
    return (specific_heat_ratio_ - 1.0) * density * internal_energy;
  }

  [[nodiscard]] inline Real calculateInternalEnergyFromEnthalpy(const Real enthalpy) const {
    return enthalpy / specific_heat_ratio_;
  }

  [[nodiscard]] inline Real calculateEnthalpyFromInternalEnergy(const Real internal_energy) const {
    return internal_energy * specific_heat_ratio_;
  }

  [[nodiscard]] inline Real calculateSoundSpeedFromInternalEnergy(const Real internal_energy) const {
    return std::sqrt(specific_heat_ratio_ * (specific_heat_ratio_ - 1.0) * internal_energy);
  }

  [[nodiscard]] inline Real calculateSoundSpeedFromEnthalpySubtractVelocitySquareSummation(
      const Real enthalpy_subtract_velocity_square_summation) const {
    return std::sqrt((specific_heat_ratio_ - 1.0) * enthalpy_subtract_velocity_square_summation);
  }
};

template <typename SimulationControl, EquationModel EquationModelType>
struct ThermalModel;

template <typename SimulationControl>
struct ThermalModel<SimulationControl, EquationModel::Euler> {
  ThermodynamicModelData<SimulationControl::kThermodynamicModel> thermodynamic_model_;
  EquationOfStateData<SimulationControl::kEquationOfState> equation_of_state_;

  [[nodiscard]] inline Real calculateInternalEnergyFromTemperature(const Real temperature) const {
    if constexpr (SimulationControl::kThermodynamicModel == ThermodynamicModel::ConstantE) {
      return thermodynamic_model_.calculateInternalEnergyFromTemperature(temperature);
    } else if constexpr (SimulationControl::kThermodynamicModel == ThermodynamicModel::ConstantH) {
      if constexpr (SimulationControl::kEquationOfState == EquationOfState::IdealGas) {
        return equation_of_state_.calculateInternalEnergyFromEnthalpy(
            thermodynamic_model_.calculateEnthalpyFromTemperature(temperature));
      }
    }
  }

  [[nodiscard]] inline Real calculateTemperatureFromInternalEnergy(const Real internal_energy) const {
    if constexpr (SimulationControl::kThermodynamicModel == ThermodynamicModel::ConstantE) {
      return thermodynamic_model_.calculateTemperatureFormInternalEnergy(internal_energy);
    } else if constexpr (SimulationControl::kThermodynamicModel == ThermodynamicModel::ConstantH) {
      if constexpr (SimulationControl::kEquationOfState == EquationOfState::IdealGas) {
        return thermodynamic_model_.calculateTemperatureFromEnthalpy(
            equation_of_state_.calculateEnthalpyFromInternalEnergy(internal_energy));
      }
    }
  }

  [[nodiscard]] inline Real calculatePressureFormDensityInternalEnergy(const Real density,
                                                                       const Real internal_energy) const {
    if constexpr (SimulationControl::kEquationOfState == EquationOfState::IdealGas) {
      return equation_of_state_.calculatePressureFormDensityInternalEnergy(density, internal_energy);
    }
  }

  [[nodiscard]] inline Real calculateInternalEnergyFromEnthalpy(const Real enthalpy) const {
    if constexpr (SimulationControl::kEquationOfState == EquationOfState::IdealGas) {
      return equation_of_state_.calculateInternalEnergyFromEnthalpy(enthalpy);
    }
  }

  [[nodiscard]] inline Real calculateEnthalpyFromInternalEnergy(const Real internal_energy) const {
    if constexpr (SimulationControl::kEquationOfState == EquationOfState::IdealGas) {
      return equation_of_state_.calculateEnthalpyFromInternalEnergy(internal_energy);
    }
  }

  [[nodiscard]] inline Real calculateSoundSpeedFromInternalEnergy(const Real internal_energy) const {
    if constexpr (SimulationControl::kEquationOfState == EquationOfState::IdealGas) {
      return equation_of_state_.calculateSoundSpeedFromInternalEnergy(internal_energy);
    }
  }

  [[nodiscard]] inline Real calculateSoundSpeedFromEnthalpySubtractVelocitySquareSummation(
      const Real enthalpy_subtract_velocity_square_summation) const {
    if constexpr (SimulationControl::kEquationOfState == EquationOfState::IdealGas) {
      return equation_of_state_.calculateSoundSpeedFromEnthalpySubtractVelocitySquareSummation(
          enthalpy_subtract_velocity_square_summation);
    }
  }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_THERMAL_MODEL_HPP_
