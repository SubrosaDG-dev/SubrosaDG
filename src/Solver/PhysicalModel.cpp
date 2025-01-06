/**
 * @file PhysicalModel.cpp
 * @brief The header file of PhysicalModel.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2025 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_PHYSICAL_MODEL_CPP_
#define SUBROSA_DG_PHYSICAL_MODEL_CPP_

#include <cmath>

#include "Utils/BasicDataType.cpp"
#include "Utils/Enum.cpp"

namespace SubrosaDG {

template <ThermodynamicModelEnum ThermodynamicModelType>
struct ThermodynamicModel;

template <>
struct ThermodynamicModel<ThermodynamicModelEnum::Constant> {
  inline static Real specific_heat_constant_pressure;
  inline static Real specific_heat_constant_volume;

  [[nodiscard]] inline Real calculateInternalEnergyFromTemperature(const Real temperature) const {
    return this->specific_heat_constant_volume * temperature;
  }

  [[nodiscard]] inline Real calculateTemperatureFormInternalEnergy(const Real internal_energy) const {
    return internal_energy / this->specific_heat_constant_volume;
  }
};

template <EquationOfStateEnum EquationOfStateType>
struct EquationOfState;

template <>
struct EquationOfState<EquationOfStateEnum::IdealGas> {
  inline static constexpr Real kSpecificHeatRatio = 1.4_r;

  [[nodiscard]] inline Real calculatePressureFormDensityInternalEnergy(const Real density,
                                                                       const Real internal_energy) const {
    return (this->kSpecificHeatRatio - 1.0_r) * density * internal_energy;
  }

  [[nodiscard]] inline Real calculateSoundSpeedFromDensityPressure(const Real density, const Real pressure) const {
    return std::sqrt(this->kSpecificHeatRatio * pressure / density);
  }
};

template <>
struct EquationOfState<EquationOfStateEnum::WeakCompressibleFluid> {
  inline static Real reference_sound_speed;
  inline static Real reference_density;
  inline static Real reference_pressure_addition;

  inline void calculatePressureAdditionFromSoundSpeedDensity() {
    this->reference_pressure_addition =
        0.01 * this->reference_density * this->reference_sound_speed * this->reference_sound_speed;
  }

  [[nodiscard]] inline Real calculatePressureFormDensityInternalEnergy(
      const Real density, [[maybe_unused]] const Real internal_energy) const {
    return this->reference_sound_speed * this->reference_sound_speed * (density - this->reference_density) +
           this->reference_pressure_addition;
  }

  [[nodiscard]] inline Real calculateSoundSpeedFromDensityPressure([[maybe_unused]] const Real density,
                                                                   [[maybe_unused]] const Real pressure) const {
    return this->reference_sound_speed;
  }
};

template <TransportModelEnum TransportModelType>
struct TransportModel;

template <>
struct TransportModel<TransportModelEnum::None> {};

template <>
struct TransportModel<TransportModelEnum::Constant> {
  inline static Real dynamic_viscosity;
  inline static Real thermal_conductivity;

  inline static constexpr Real kPrandtlNumber = 0.71_r;

  [[nodiscard]] inline Real calculateDynamicViscosity([[maybe_unused]] const Real temperature) const {
    return this->dynamic_viscosity;
  }

  [[nodiscard]] inline Real calculateThermalConductivity([[maybe_unused]] const Real temperature) const {
    return this->thermal_conductivity;
  }
};

template <>
struct TransportModel<TransportModelEnum::Sutherland> {
  inline static Real dynamic_viscosity;
  inline static Real thermal_conductivity;

  inline static constexpr Real kPrandtlNumber = 0.71_r;

  inline static constexpr Real kSutherlandTemperature = 110.4_r / 273.15_r;

  [[nodiscard]] inline Real calculateSutherlandRatio(const Real temperature) const {
    return std::sqrt(temperature * temperature * temperature) * (1.0_r + this->kSutherlandTemperature) /
           (temperature + this->kSutherlandTemperature);
  }

  [[nodiscard]] inline Real calculateDynamicViscosity(const Real temperature) const {
    return this->dynamic_viscosity * this->calculateSutherlandRatio(temperature);
  }

  [[nodiscard]] inline Real calculateThermalConductivity(const Real temperature) const {
    return this->thermal_conductivity * this->calculateSutherlandRatio(temperature);
  }
};

template <typename SimulationControl>
struct PhysicalModel {
  ThermodynamicModel<SimulationControl::kThermodynamicModel> thermodynamic_model_;
  EquationOfState<SimulationControl::kEquationOfState> equation_of_state_;
  TransportModel<SimulationControl::kTransportModel> transport_model_;

  [[nodiscard]] inline Real calculateInternalEnergyFromTemperature(const Real temperature) const {
    return this->thermodynamic_model_.calculateInternalEnergyFromTemperature(temperature);
  }

  [[nodiscard]] inline Real calculateTemperatureFromInternalEnergy(const Real internal_energy) const {
    return this->thermodynamic_model_.calculateTemperatureFormInternalEnergy(internal_energy);
  }

  [[nodiscard]] inline Real calculatePressureFormDensityInternalEnergy(const Real density,
                                                                       const Real internal_energy) const {
    return this->equation_of_state_.calculatePressureFormDensityInternalEnergy(density, internal_energy);
  }

  [[nodiscard]] inline Real calculateSoundSpeedFromDensityPressure(const Real density, const Real pressure) const {
    return this->equation_of_state_.calculateSoundSpeedFromDensityPressure(density, pressure);
  }

  [[nodiscard]] inline Real calculateEntropyFromDensityPressure(const Real density, const Real pressure) const {
    return pressure / std::pow(density, this->equation_of_state_.kSpecificHeatRatio);
  }

  inline void calculateThermalConductivityFromDynamicViscosity() {
    this->transport_model_.thermal_conductivity = this->thermodynamic_model_.specific_heat_constant_pressure *
                                                  this->transport_model_.dynamic_viscosity /
                                                  this->transport_model_.kPrandtlNumber;
  }

  [[nodiscard]] inline Real calculateDynamicViscosity([[maybe_unused]] const Real temperature) const {
    return this->transport_model_.calculateDynamicViscosity(temperature);
  }

  [[nodiscard]] inline Real calculateThermalConductivity([[maybe_unused]] const Real temperature) const {
    return this->transport_model_.calculateThermalConductivity(temperature);
  }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_PHYSICAL_MODEL_CPP_
