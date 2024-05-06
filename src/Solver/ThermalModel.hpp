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
  inline static constexpr Real kSpecificHeatConstantVolume = 25.0 / 14.0;

  [[nodiscard]] inline Real calculateInternalEnergyFromTemperature(const Real temperature) const {
    return this->kSpecificHeatConstantVolume * temperature;
  }

  [[nodiscard]] inline Real calculateTemperatureFormInternalEnergy(const Real internal_energy) const {
    return internal_energy / this->kSpecificHeatConstantVolume;
  }
};

template <>
struct ThermodynamicModel<ThermodynamicModelEnum::ConstantH> {
  inline static constexpr Real kSpecificHeatConstantPressure = 5.0 / 2.0;

  [[nodiscard]] inline Real calculateEnthalpyFromTemperature(const Real temperature) const {
    return this->kSpecificHeatConstantPressure * temperature;
  }

  [[nodiscard]] inline Real calculateTemperatureFromEnthalpy(const Real enthalpy) const {
    return enthalpy / this->kSpecificHeatConstantPressure;
  }
};

template <EquationOfStateEnum EquationOfStateType>
struct EquationOfState;

template <>
struct EquationOfState<EquationOfStateEnum::IdealGas> {
  inline static constexpr Real kSpecificHeatRatio = 1.4;

  [[nodiscard]] inline Real calculatePressureFormDensityInternalEnergy(const Real density,
                                                                       const Real internal_energy) const {
    return (this->kSpecificHeatRatio - 1.0) * density * internal_energy;
  }

  [[nodiscard]] inline Real calculateInternalEnergyFromDensityPressure(const Real density, const Real pressure) const {
    return pressure / ((this->kSpecificHeatRatio - 1.0) * density);
  }

  [[nodiscard]] inline Real calculateInternalEnergyFromEnthalpy(const Real enthalpy) const {
    return enthalpy / this->kSpecificHeatRatio;
  }

  [[nodiscard]] inline Real calculateEnthalpyFromInternalEnergy(const Real internal_energy) const {
    return internal_energy * this->kSpecificHeatRatio;
  }

  [[nodiscard]] inline Real calculateSoundSpeedFromInternalEnergy(const Real internal_energy) const {
    return std::sqrt(this->kSpecificHeatRatio * (this->kSpecificHeatRatio - 1.0) * internal_energy);
  }

  [[nodiscard]] inline Real calculateInternalEnergyFromSoundSpeed(const Real sound_speed) const {
    return sound_speed * sound_speed / (this->kSpecificHeatRatio * (this->kSpecificHeatRatio - 1.0));
  }

  [[nodiscard]] inline Real calculateRiemannInvariantPart(const Real internal_energy) const {
    return 2 * this->calculateSoundSpeedFromInternalEnergy(internal_energy) / (this->kSpecificHeatRatio - 1.0);
  }

  [[nodiscard]] inline Real calculateInternalEnergyFromRiemannInvariantPart(const Real riemann_invariant_part) const {
    return this->calculateInternalEnergyFromSoundSpeed((this->kSpecificHeatRatio - 1.0) * riemann_invariant_part / 2.0);
  }

  [[nodiscard]] inline Real calculateEntropyFromDensityPressure(const Real density, const Real pressure) const {
    return pressure / std::pow(density, this->kSpecificHeatRatio);
  }

  [[nodiscard]] inline Real calculateDensityFromEntropyInternalEnergy(const Real entropy,
                                                                      const Real internal_energy) const {
    return std::pow((this->kSpecificHeatRatio - 1.0) * internal_energy / entropy,
                    1.0 / (this->kSpecificHeatRatio - 1.0));
  }
};

template <TransportModelEnum TransportModelType>
struct TransportModel;

template <>
struct TransportModel<TransportModelEnum::Constant> {
  inline static Real dynamic_viscosity;
  inline static Real thermal_conductivity;

  [[nodiscard]] inline Real calculateDynamicViscosity() const { return this->dynamic_viscosity; }

  [[nodiscard]] inline Real calculateThermalConductivity() const { return this->thermal_conductivity; }
};

template <>
struct TransportModel<TransportModelEnum::Sutherland> {
  inline static Real dynamic_viscosity;
  inline static Real thermal_conductivity;

  inline static constexpr Real kReferenceTemperature = 273.15;
  inline static constexpr Real kSutherlandTemperature = 110.4;

  [[nodiscard]] inline Real calculateSutherlandRatio(const Real temperature) const {
    return std::sqrt(temperature * temperature * temperature) *
           (this->kReferenceTemperature + this->kSutherlandTemperature) /
           (temperature * this->kReferenceTemperature + this->kSutherlandTemperature);
  }

  [[nodiscard]] inline Real calculateDynamicViscosity(const Real temperature) const {
    return this->dynamic_viscosity * this->calculateSutherlandRatio(temperature);
  }

  [[nodiscard]] inline Real calculateThermalConductivity(const Real temperature) const {
    return this->thermal_conductivity * this->calculateSutherlandRatio(temperature);
  }
};

template <typename SimulationControl, EquationModelEnum EquationModelType>
struct ThermalModelData;

template <typename SimulationControl>
struct ThermalModelData<SimulationControl, EquationModelEnum::Euler> {
  ThermodynamicModel<SimulationControl::kThermodynamicModel> thermodynamic_model_;
  EquationOfState<SimulationControl::kEquationOfState> equation_of_state_;
};

template <typename SimulationControl>
struct ThermalModelData<SimulationControl, EquationModelEnum::NavierStokes> {
  ThermodynamicModel<SimulationControl::kThermodynamicModel> thermodynamic_model_;
  EquationOfState<SimulationControl::kEquationOfState> equation_of_state_;
  TransportModel<SimulationControl::kTransportModel> transport_model_;
};

template <typename SimulationControl>
struct ThermalModel : ThermalModelData<SimulationControl, SimulationControl::kEquationModel> {
  [[nodiscard]] inline Real calculateInternalEnergyFromTemperature(const Real temperature) const {
    if constexpr (SimulationControl::kThermodynamicModel == ThermodynamicModelEnum::ConstantE) {
      return this->thermodynamic_model_.calculateInternalEnergyFromTemperature(temperature);
    }
    if constexpr (SimulationControl::kThermodynamicModel == ThermodynamicModelEnum::ConstantH) {
      if constexpr (SimulationControl::kEquationOfState == EquationOfStateEnum::IdealGas) {
        return this->equation_of_state_.calculateInternalEnergyFromEnthalpy(
            this->thermodynamic_model_.calculateEnthalpyFromTemperature(temperature));
      }
    }
  }

  [[nodiscard]] inline Real calculateTemperatureFromInternalEnergy(const Real internal_energy) const {
    if constexpr (SimulationControl::kThermodynamicModel == ThermodynamicModelEnum::ConstantE) {
      return this->thermodynamic_model_.calculateTemperatureFormInternalEnergy(internal_energy);
    }
    if constexpr (SimulationControl::kThermodynamicModel == ThermodynamicModelEnum::ConstantH) {
      if constexpr (SimulationControl::kEquationOfState == EquationOfStateEnum::IdealGas) {
        return this->thermodynamic_model_.calculateTemperatureFromEnthalpy(
            this->equation_of_state_.calculateEnthalpyFromInternalEnergy(internal_energy));
      }
    }
  }

  [[nodiscard]] inline Real calculatePressureFormDensityInternalEnergy(const Real density,
                                                                       const Real internal_energy) const {
    if constexpr (SimulationControl::kEquationOfState == EquationOfStateEnum::IdealGas) {
      return this->equation_of_state_.calculatePressureFormDensityInternalEnergy(density, internal_energy);
    }
  }

  [[nodiscard]] inline Real calculateInternalEnergyFromDensityPressure(const Real density, const Real pressure) const {
    if constexpr (SimulationControl::kEquationOfState == EquationOfStateEnum::IdealGas) {
      return this->equation_of_state_.calculateInternalEnergyFromDensityPressure(density, pressure);
    }
  }

  [[nodiscard]] inline Real calculateInternalEnergyFromEnthalpy(const Real enthalpy) const {
    if constexpr (SimulationControl::kEquationOfState == EquationOfStateEnum::IdealGas) {
      return this->equation_of_state_.calculateInternalEnergyFromEnthalpy(enthalpy);
    }
  }

  [[nodiscard]] inline Real calculateEnthalpyFromInternalEnergy(const Real internal_energy) const {
    if constexpr (SimulationControl::kEquationOfState == EquationOfStateEnum::IdealGas) {
      return this->equation_of_state_.calculateEnthalpyFromInternalEnergy(internal_energy);
    }
  }

  [[nodiscard]] inline Real calculateSoundSpeedFromInternalEnergy(const Real internal_energy) const {
    if constexpr (SimulationControl::kEquationOfState == EquationOfStateEnum::IdealGas) {
      return this->equation_of_state_.calculateSoundSpeedFromInternalEnergy(internal_energy);
    }
  }

  [[nodiscard]] inline Real calculateInternalEnergyFromSoundSpeed(const Real sound_speed) const {
    if constexpr (SimulationControl::kEquationOfState == EquationOfStateEnum::IdealGas) {
      return this->equation_of_state_.calculateInternalEnergyFromSoundSpeed(sound_speed);
    }
  }

  [[nodiscard]] inline Real calculateRiemannInvariantPart(const Real internal_energy) const {
    if constexpr (SimulationControl::kEquationOfState == EquationOfStateEnum::IdealGas) {
      return this->equation_of_state_.calculateRiemannInvariantPart(internal_energy);
    }
  }

  [[nodiscard]] inline Real calculateInternalEnergyFromRiemannInvariantPart(const Real riemann_invariant_part) const {
    if constexpr (SimulationControl::kEquationOfState == EquationOfStateEnum::IdealGas) {
      return this->equation_of_state_.calculateInternalEnergyFromRiemannInvariantPart(riemann_invariant_part);
    }
  }

  [[nodiscard]] inline Real calculateEntropyFromDensityPressure(const Real density, const Real pressure) const {
    if constexpr (SimulationControl::kEquationOfState == EquationOfStateEnum::IdealGas) {
      return this->equation_of_state_.calculateEntropyFromDensityPressure(density, pressure);
    }
  }

  [[nodiscard]] inline Real calculateDensityFromEntropyInternalEnergy(const Real entropy,
                                                                      const Real internal_energy) const {
    if constexpr (SimulationControl::kEquationOfState == EquationOfStateEnum::IdealGas) {
      return this->equation_of_state_.calculateDensityFromEntropyInternalEnergy(entropy, internal_energy);
    }
  }

  inline void calculateThermalConductivityFromDynamicViscosity() {
    if constexpr (SimulationControl::kThermodynamicModel == ThermodynamicModelEnum::ConstantE) {
      if constexpr (SimulationControl::kEquationOfState == EquationOfStateEnum::IdealGas) {
        this->transport_model_.thermal_conductivity =
            (this->thermodynamic_model_.kSpecificHeatConstantVolume + this->equation_of_state_.kSpecificHeatRatio) *
            this->transport_model_.dynamic_viscosity / 0.71;
      }
    }
    if constexpr (SimulationControl::kThermodynamicModel == ThermodynamicModelEnum::ConstantH) {
      this->transport_model_.thermal_conductivity =
          this->thermodynamic_model_.kSpecificHeatConstantPressure * this->transport_model_.dynamic_viscosity / 0.71;
    }
  }

  [[nodiscard]] inline Real calculateDynamicViscosity([[maybe_unused]] const Real temperature) const {
    if constexpr (SimulationControl::kTransportModel == TransportModelEnum::Constant) {
      return this->transport_model_.calculateDynamicViscosity();
    }
    if constexpr (SimulationControl::kTransportModel == TransportModelEnum::Sutherland) {
      return this->transport_model_.calculateDynamicViscosity(temperature);
    }
  }

  [[nodiscard]] inline Real calculateThermalConductivity([[maybe_unused]] const Real temperature) const {
    if constexpr (SimulationControl::kTransportModel == TransportModelEnum::Constant) {
      return this->transport_model_.calculateThermalConductivity();
    }
    if constexpr (SimulationControl::kTransportModel == TransportModelEnum::Sutherland) {
      return this->transport_model_.calculateThermalConductivity(temperature);
    }
  }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_THERMAL_MODEL_HPP_
