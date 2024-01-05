/**
 * @file ThermalModel.hpp
 * @brief The header file of ThermalModel.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_THERMAL_MODEL_HPP_
#define SUBROSA_DG_THERMAL_MODEL_HPP_

#include <cmath>

#include "Utils/BasicDataType.hpp"
#include "Utils/Enum.hpp"

namespace SubrosaDG {

template <ThermodynamicModelEnum ThermodynamicModelType>
struct ThermodynamicModel;

template <>
struct ThermodynamicModel<ThermodynamicModelEnum::ConstantE> {
  Real specific_heat_constant_volume_ = 25.0 / 14.0;

  [[nodiscard]] inline Real calculateInternalEnergyFromTemperature(const Real temperature) const {
    return this->specific_heat_constant_volume_ * temperature;
  }

  [[nodiscard]] inline Real calculateTemperatureFormInternalEnergy(const Real internal_energy) const {
    return internal_energy / this->specific_heat_constant_volume_;
  }
};

template <>
struct ThermodynamicModel<ThermodynamicModelEnum::ConstantH> {
  Real specific_heat_constant_pressure_ = 5.0 / 2.0;

  [[nodiscard]] inline Real calculateEnthalpyFromTemperature(const Real temperature) const {
    return this->specific_heat_constant_pressure_ * temperature;
  }

  [[nodiscard]] inline Real calculateTemperatureFromEnthalpy(const Real enthalpy) const {
    return enthalpy / this->specific_heat_constant_pressure_;
  }
};

template <EquationOfStateEnum EquationOfStateType>
struct EquationOfState;

template <>
struct EquationOfState<EquationOfStateEnum::IdealGas> {
  Real specific_heat_ratio_ = 1.4;

  [[nodiscard]] inline Real calculatePressureFormDensityInternalEnergy(const Real density,
                                                                       const Real internal_energy) const {
    return (this->specific_heat_ratio_ - 1.0) * density * internal_energy;
  }

  [[nodiscard]] inline Real calculateInternalEnergyFromDensityPressure(const Real density, const Real pressure) const {
    return pressure / ((this->specific_heat_ratio_ - 1.0) * density);
  }

  [[nodiscard]] inline Real calculateInternalEnergyFromEnthalpy(const Real enthalpy) const {
    return enthalpy / this->specific_heat_ratio_;
  }

  [[nodiscard]] inline Real calculateEnthalpyFromInternalEnergy(const Real internal_energy) const {
    return internal_energy * this->specific_heat_ratio_;
  }

  [[nodiscard]] inline Real calculateSoundSpeedFromInternalEnergy(const Real internal_energy) const {
    return std::sqrt(this->specific_heat_ratio_ * (this->specific_heat_ratio_ - 1.0) * internal_energy);
  }

  [[nodiscard]] inline Real calculateInternalEnergyFromSoundSpeed(const Real sound_speed) const {
    return sound_speed * sound_speed / (this->specific_heat_ratio_ * (this->specific_heat_ratio_ - 1.0));
  }

  [[nodiscard]] inline Real calculateRiemannInvariantPart(const Real internal_energy) const {
    return 2 * this->calculateSoundSpeedFromInternalEnergy(internal_energy) / (this->specific_heat_ratio_ - 1.0);
  }

  [[nodiscard]] inline Real calculateInternalEnergyFromRiemannInvariantPart(const Real riemann_invariant_part) const {
    return this->calculateInternalEnergyFromSoundSpeed((this->specific_heat_ratio_ - 1.0) * riemann_invariant_part /
                                                       2.0);
  }

  [[nodiscard]] inline Real calculateEntropyFromDensityPressure(const Real density, const Real pressure) const {
    return pressure / std::pow(density, this->specific_heat_ratio_);
  }

  [[nodiscard]] inline Real calculateDensityFromEntropyInternalEnergy(const Real entropy,
                                                                      const Real internal_energy) const {
    return std::pow((this->specific_heat_ratio_ - 1.0) * internal_energy / entropy,
                    1.0 / (this->specific_heat_ratio_ - 1.0));
  }
};

template <typename SimulationControl, EquationModelEnum EquationModelType>
struct ThermalModelData;

template <typename SimulationControl>
struct ThermalModelData<SimulationControl, EquationModelEnum::Euler> {
  ThermodynamicModel<SimulationControl::kThermodynamicModel> thermodynamic_model_;
  EquationOfState<SimulationControl::kEquationOfState> equation_of_state_;

  [[nodiscard]] inline Real calculateInternalEnergyFromTemperature(const Real temperature) const {
    if constexpr (SimulationControl::kThermodynamicModel == ThermodynamicModelEnum::ConstantE) {
      return thermodynamic_model_.calculateInternalEnergyFromTemperature(temperature);
    }
    if constexpr (SimulationControl::kThermodynamicModel == ThermodynamicModelEnum::ConstantH) {
      if constexpr (SimulationControl::kEquationOfState == EquationOfStateEnum::IdealGas) {
        return equation_of_state_.calculateInternalEnergyFromEnthalpy(
            thermodynamic_model_.calculateEnthalpyFromTemperature(temperature));
      }
    }
  }

  [[nodiscard]] inline Real calculateTemperatureFromInternalEnergy(const Real internal_energy) const {
    if constexpr (SimulationControl::kThermodynamicModel == ThermodynamicModelEnum::ConstantE) {
      return thermodynamic_model_.calculateTemperatureFormInternalEnergy(internal_energy);
    }
    if constexpr (SimulationControl::kThermodynamicModel == ThermodynamicModelEnum::ConstantH) {
      if constexpr (SimulationControl::kEquationOfState == EquationOfStateEnum::IdealGas) {
        return thermodynamic_model_.calculateTemperatureFromEnthalpy(
            equation_of_state_.calculateEnthalpyFromInternalEnergy(internal_energy));
      }
    }
  }

  [[nodiscard]] inline Real calculatePressureFormDensityInternalEnergy(const Real density,
                                                                       const Real internal_energy) const {
    if constexpr (SimulationControl::kEquationOfState == EquationOfStateEnum::IdealGas) {
      return equation_of_state_.calculatePressureFormDensityInternalEnergy(density, internal_energy);
    }
  }

  [[nodiscard]] inline Real calculateInternalEnergyFromDensityPressure(const Real density, const Real pressure) const {
    if constexpr (SimulationControl::kEquationOfState == EquationOfStateEnum::IdealGas) {
      return equation_of_state_.calculateInternalEnergyFromDensityPressure(density, pressure);
    }
  }

  [[nodiscard]] inline Real calculateInternalEnergyFromEnthalpy(const Real enthalpy) const {
    if constexpr (SimulationControl::kEquationOfState == EquationOfStateEnum::IdealGas) {
      return equation_of_state_.calculateInternalEnergyFromEnthalpy(enthalpy);
    }
  }

  [[nodiscard]] inline Real calculateEnthalpyFromInternalEnergy(const Real internal_energy) const {
    if constexpr (SimulationControl::kEquationOfState == EquationOfStateEnum::IdealGas) {
      return equation_of_state_.calculateEnthalpyFromInternalEnergy(internal_energy);
    }
  }

  [[nodiscard]] inline Real calculateSoundSpeedFromInternalEnergy(const Real internal_energy) const {
    if constexpr (SimulationControl::kEquationOfState == EquationOfStateEnum::IdealGas) {
      return equation_of_state_.calculateSoundSpeedFromInternalEnergy(internal_energy);
    }
  }

  [[nodiscard]] inline Real calculateInternalEnergyFromSoundSpeed(const Real sound_speed) const {
    if constexpr (SimulationControl::kEquationOfState == EquationOfStateEnum::IdealGas) {
      return equation_of_state_.calculateInternalEnergyFromSoundSpeed(sound_speed);
    }
  }

  [[nodiscard]] inline Real calculateRiemannInvariantPart(const Real internal_energy) const {
    if constexpr (SimulationControl::kEquationOfState == EquationOfStateEnum::IdealGas) {
      return equation_of_state_.calculateRiemannInvariantPart(internal_energy);
    }
  }

  [[nodiscard]] inline Real calculateInternalEnergyFromRiemannInvariantPart(const Real riemann_invariant_part) const {
    if constexpr (SimulationControl::kEquationOfState == EquationOfStateEnum::IdealGas) {
      return equation_of_state_.calculateInternalEnergyFromRiemannInvariantPart(riemann_invariant_part);
    }
  }

  [[nodiscard]] inline Real calculateEntropyFromDensityPressure(const Real density, const Real pressure) const {
    if constexpr (SimulationControl::kEquationOfState == EquationOfStateEnum::IdealGas) {
      return equation_of_state_.calculateEntropyFromDensityPressure(density, pressure);
    }
  }

  [[nodiscard]] inline Real calculateDensityFromEntropyInternalEnergy(const Real entropy,
                                                                      const Real internal_energy) const {
    if constexpr (SimulationControl::kEquationOfState == EquationOfStateEnum::IdealGas) {
      return equation_of_state_.calculateDensityFromEntropyInternalEnergy(entropy, internal_energy);
    }
  }
};

template <typename SimulationControl>
struct ThermalModel : ThermalModelData<SimulationControl, SimulationControl::kEquationModel> {};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_THERMAL_MODEL_HPP_
