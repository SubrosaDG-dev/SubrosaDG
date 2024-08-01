/**
 * @file PhysicalModel.hpp
 * @brief The header file of PhysicalModel.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_physical_model_HPP_
#define SUBROSA_DG_physical_model_HPP_

#include <cmath>

#include "Utils/BasicDataType.hpp"
#include "Utils/Enum.hpp"

namespace SubrosaDG {

template <ThermodynamicModelEnum ThermodynamicModelType>
struct ThermodynamicModel;

template <>
struct ThermodynamicModel<ThermodynamicModelEnum::ConstantE> {
  Real specific_heat_constant_volume_;

  [[nodiscard]] inline Real calculateInternalEnergyFromTemperature(const Real temperature) const {
    return this->specific_heat_constant_volume_ * temperature;
  }

  [[nodiscard]] inline Real calculateTemperatureFormInternalEnergy(const Real internal_energy) const {
    return internal_energy / this->specific_heat_constant_volume_;
  }
};

template <EquationOfStateEnum EquationOfStateType>
struct EquationOfState;

template <>
struct EquationOfState<EquationOfStateEnum::IdealGas> {
  Real specific_heat_ratio_;

  [[nodiscard]] inline Real calculatePressureFormDensityInternalEnergy(const Real density,
                                                                       const Real internal_energy) const {
    return (this->specific_heat_ratio_ - 1.0_r) * density * internal_energy;
  }

  [[nodiscard]] inline Real calculateInternalEnergyFromDensityPressure(const Real density, const Real pressure) const {
    return pressure / ((this->specific_heat_ratio_ - 1.0_r) * density);
  }

  [[nodiscard]] inline Real calculateInternalEnergyFromEnthalpy(const Real enthalpy) const {
    return enthalpy / this->specific_heat_ratio_;
  }

  [[nodiscard]] inline Real calculateEnthalpyFromInternalEnergy(const Real internal_energy) const {
    return internal_energy * this->specific_heat_ratio_;
  }

  [[nodiscard]] inline Real calculateSoundSpeedFromInternalEnergy(const Real internal_energy) const {
    return std::sqrt(this->specific_heat_ratio_ * (this->specific_heat_ratio_ - 1.0_r) * internal_energy);
  }

  [[nodiscard]] inline Real calculateInternalEnergyFromSoundSpeed(const Real sound_speed) const {
    return sound_speed * sound_speed / (this->specific_heat_ratio_ * (this->specific_heat_ratio_ - 1.0_r));
  }

  [[nodiscard]] inline Real calculateRiemannInvariantPart(const Real internal_energy) const {
    return 2 * this->calculateSoundSpeedFromInternalEnergy(internal_energy) / (this->specific_heat_ratio_ - 1.0_r);
  }

  [[nodiscard]] inline Real calculateInternalEnergyFromRiemannInvariantPart(const Real riemann_invariant_part) const {
    return this->calculateInternalEnergyFromSoundSpeed((this->specific_heat_ratio_ - 1.0_r) * riemann_invariant_part /
                                                       2.0_r);
  }

  [[nodiscard]] inline Real calculateEntropyFromDensityPressure(const Real density, const Real pressure) const {
    return pressure / std::pow(density, this->specific_heat_ratio_);
  }

  [[nodiscard]] inline Real calculateDensityFromEntropyInternalEnergy(const Real entropy,
                                                                      const Real internal_energy) const {
    return std::pow((this->specific_heat_ratio_ - 1.0_r) * internal_energy / entropy,
                    1.0_r / (this->specific_heat_ratio_ - 1.0_r));
  }
};

template <TransportModelEnum TransportModelType>
struct TransportModel;

template <>
struct TransportModel<TransportModelEnum::None> {};

template <>
struct TransportModel<TransportModelEnum::Constant> {
  Real prandtl_number_;
  Real dynamic_viscosity_;
  Real thermal_conductivity_;

  [[nodiscard]] inline Real calculateDynamicViscosity([[maybe_unused]] const Real temperature) const {
    return this->dynamic_viscosity_;
  }

  [[nodiscard]] inline Real calculateThermalConductivity([[maybe_unused]] const Real temperature) const {
    return this->thermal_conductivity_;
  }
};

template <>
struct TransportModel<TransportModelEnum::Sutherland> {
  Real prandtl_number_;
  Real dynamic_viscosity_;
  Real thermal_conductivity_;

  inline static constexpr Real kSutherlandTemperature = 110.4_r / 273.15_r;

  [[nodiscard]] inline Real calculateSutherlandRatio(const Real temperature) const {
    return std::sqrt(temperature * temperature * temperature) * (1.0_r + this->kSutherlandTemperature) /
           (temperature + this->kSutherlandTemperature);
  }

  [[nodiscard]] inline Real calculateDynamicViscosity(const Real temperature) const {
    return this->dynamic_viscosity_ * this->calculateSutherlandRatio(temperature);
  }

  [[nodiscard]] inline Real calculateThermalConductivity(const Real temperature) const {
    return this->thermal_conductivity_ * this->calculateSutherlandRatio(temperature);
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

  [[nodiscard]] inline Real calculateInternalEnergyFromDensityPressure(const Real density, const Real pressure) const {
    return this->equation_of_state_.calculateInternalEnergyFromDensityPressure(density, pressure);
  }

  [[nodiscard]] inline Real calculateInternalEnergyFromEnthalpy(const Real enthalpy) const {
    return this->equation_of_state_.calculateInternalEnergyFromEnthalpy(enthalpy);
  }

  [[nodiscard]] inline Real calculateEnthalpyFromInternalEnergy(const Real internal_energy) const {
    return this->equation_of_state_.calculateEnthalpyFromInternalEnergy(internal_energy);
  }

  [[nodiscard]] inline Real calculateSoundSpeedFromInternalEnergy(const Real internal_energy) const {
    return this->equation_of_state_.calculateSoundSpeedFromInternalEnergy(internal_energy);
  }

  [[nodiscard]] inline Real calculateInternalEnergyFromSoundSpeed(const Real sound_speed) const {
    return this->equation_of_state_.calculateInternalEnergyFromSoundSpeed(sound_speed);
  }

  [[nodiscard]] inline Real calculateRiemannInvariantPart(const Real internal_energy) const {
    return this->equation_of_state_.calculateRiemannInvariantPart(internal_energy);
  }

  [[nodiscard]] inline Real calculateInternalEnergyFromRiemannInvariantPart(const Real riemann_invariant_part) const {
    return this->equation_of_state_.calculateInternalEnergyFromRiemannInvariantPart(riemann_invariant_part);
  }

  [[nodiscard]] inline Real calculateEntropyFromDensityPressure(const Real density, const Real pressure) const {
    return this->equation_of_state_.calculateEntropyFromDensityPressure(density, pressure);
  }

  [[nodiscard]] inline Real calculateDensityFromEntropyInternalEnergy(const Real entropy,
                                                                      const Real internal_energy) const {
    return this->equation_of_state_.calculateDensityFromEntropyInternalEnergy(entropy, internal_energy);
  }

  inline void calculateThermalConductivityFromDynamicViscosity() {
    this->transport_model_.thermal_conductivity_ =
        (this->thermodynamic_model_.specific_heat_constant_volume_ + this->equation_of_state_.specific_heat_ratio_) *
        this->transport_model_.dynamic_viscosity_ / this->transport_model_.prandtl_number_;
  }

  [[nodiscard]] inline Real calculateDynamicViscosity([[maybe_unused]] const Real temperature) const {
    return this->transport_model_.calculateDynamicViscosity(temperature);
  }

  [[nodiscard]] inline Real calculateThermalConductivity([[maybe_unused]] const Real temperature) const {
    return this->transport_model_.calculateThermalConductivity(temperature);
  }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_physical_model_HPP_
