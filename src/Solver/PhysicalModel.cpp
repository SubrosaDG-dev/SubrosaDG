/**
 * @file PhysicalModel.cpp
 * @brief The header file of PhysicalModel.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_PHYSICAL_MODEL_CPP_
#define SUBROSA_DG_PHYSICAL_MODEL_CPP_

#include <cmath>

#include "Utils/BasicDataType.cpp"
#include "Utils/Enum.cpp"

namespace SubrosaDG {

struct PhysicalModelData;

template <typename PhysicalModelData, EquationOfStateEnum EquationOfStateType>
struct EquationOfState;

template <typename PhysicalModelData>
struct EquationOfState<PhysicalModelData, EquationOfStateEnum::IdealGas> {
  static constexpr Real kSpecificHeatRatio = static_cast<Real>(PhysicalModelData::kSpecificHeatRatio);

  [[nodiscard]] static Real computePressureFromDensityInternalEnergy(const Real density, const Real internal_energy) {
    return (kSpecificHeatRatio - 1.0_r) * density * internal_energy;
  }

  [[nodiscard]] static Real computeSoundSpeedFromDensityPressure(const Real density, const Real pressure) {
#ifndef __SYCL_DEVICE_ONLY__
    return std::sqrt(kSpecificHeatRatio * pressure / density);
#else   // __SYCL_DEVICE_ONLY__
    return sycl::sqrt(kSpecificHeatRatio * pressure / density);
#endif  // __SYCL_DEVICE_ONLY__
  }
};

template <typename PhysicalModelData>
struct EquationOfState<PhysicalModelData, EquationOfStateEnum::WeakCompressibleFluid> {
  static constexpr Real kReferenceSoundSpeed = static_cast<Real>(PhysicalModelData::kReferenceSoundSpeed);
  static constexpr Real kReferenceDensity = static_cast<Real>(PhysicalModelData::kReferenceDensity);
  static constexpr Real kReferencePressureAddition =
      static_cast<Real>(0.01 * PhysicalModelData::kReferenceDensity * PhysicalModelData::kReferenceSoundSpeed *
                        PhysicalModelData::kReferenceSoundSpeed);

  [[nodiscard]] static Real computePressureFromDensity(const Real density) {
    return kReferenceSoundSpeed * kReferenceSoundSpeed * (density - kReferenceDensity) + kReferencePressureAddition;
  }

  [[nodiscard]] static Real getSoundSpeed() { return kReferenceSoundSpeed; }
};

template <typename PhysicalModelData, ThermodynamicModelEnum ThermodynamicModelType>
struct ThermodynamicModel;

template <typename PhysicalModelData>
struct ThermodynamicModel<PhysicalModelData, ThermodynamicModelEnum::Constant> {
  static constexpr Real kSpecificHeatConstantPressure =
      static_cast<Real>(PhysicalModelData::kSpecificHeatConstantPressure);
  static constexpr Real kSpecificHeatConstantVolume = static_cast<Real>(PhysicalModelData::kSpecificHeatConstantVolume);

  [[nodiscard]] static Real computeInternalEnergyFromTemperature(const Real temperature) {
    return kSpecificHeatConstantVolume * temperature;
  }

  [[nodiscard]] static Real computeTemperatureFromInternalEnergy(const Real internal_energy) {
    return internal_energy / kSpecificHeatConstantVolume;
  }
};

template <typename PhysicalModelData, TransportModelEnum TransportModelType>
struct TransportModel;

template <typename PhysicalModelData>
struct TransportModel<PhysicalModelData, TransportModelEnum::None> {};

template <typename PhysicalModelData>
struct TransportModel<PhysicalModelData, TransportModelEnum::Constant> {
  static constexpr Real kDynamicViscosity = static_cast<Real>(PhysicalModelData::kDynamicViscosity);
  static constexpr Real kThermalConductivity = static_cast<Real>(PhysicalModelData::kThermalConductivity);

  [[nodiscard]] static Real getDynamicViscosity() { return kDynamicViscosity; }

  [[nodiscard]] static Real getThermalConductivity() { return kThermalConductivity; }
};

template <typename PhysicalModelData>
struct TransportModel<PhysicalModelData, TransportModelEnum::Sutherland> {
  static constexpr Real kDynamicViscosity = static_cast<Real>(PhysicalModelData::kDynamicViscosity);
  static constexpr Real kThermalConductivity = static_cast<Real>(PhysicalModelData::kThermalConductivity);
  static constexpr Real kSutherlandTemperature = static_cast<Real>(110.4 / 273.15);

  [[nodiscard]] static Real computeSutherlandRatio(const Real temperature) {
#ifndef __SYCL_DEVICE_ONLY__
    return std::sqrt(temperature * temperature * temperature) * (1.0_r + kSutherlandTemperature) /
           (temperature + kSutherlandTemperature);
#else   // __SYCL_DEVICE_ONLY__
    return sycl::sqrt(temperature * temperature * temperature) * (1.0_r + kSutherlandTemperature) /
           (temperature + kSutherlandTemperature);
#endif  // __SYCL_DEVICE_ONLY__
  }

  [[nodiscard]] static Real computeDynamicViscosity(const Real temperature) {
    return kDynamicViscosity * computeSutherlandRatio(temperature);
  }

  [[nodiscard]] static Real computeThermalConductivity(const Real temperature) {
    return kThermalConductivity * computeSutherlandRatio(temperature);
  }
};

template <typename SimulationControl, typename PhysicalModelData>
struct PhysicalModel {
  [[nodiscard]] static Real getSpecificHeatRatio()
    requires(SimulationControl::kEquationOfState == EquationOfStateEnum::IdealGas)
  {
    return EquationOfState<PhysicalModelData, SimulationControl::kEquationOfState>::kSpecificHeatRatio;
  }

  [[nodiscard]] static Real computePressureFromDensityInternalEnergy(const Real density,
                                                                     [[maybe_unused]] const Real internal_energy) {
    if constexpr (SimulationControl::kEquationOfState == EquationOfStateEnum::IdealGas) {
      return EquationOfState<PhysicalModelData, SimulationControl::kEquationOfState>::
          computePressureFromDensityInternalEnergy(density, internal_energy);
    } else if constexpr (SimulationControl::kEquationOfState == EquationOfStateEnum::WeakCompressibleFluid) {
      return EquationOfState<PhysicalModelData, SimulationControl::kEquationOfState>::computePressureFromDensity(
          density);
    }
  }

  [[nodiscard]] static Real computeSoundSpeedFromDensityPressure([[maybe_unused]] const Real density,
                                                                 [[maybe_unused]] const Real pressure) {
    if constexpr (SimulationControl::kEquationOfState == EquationOfStateEnum::IdealGas) {
      return EquationOfState<PhysicalModelData,
                             SimulationControl::kEquationOfState>::computeSoundSpeedFromDensityPressure(density,
                                                                                                        pressure);
    } else if constexpr (SimulationControl::kEquationOfState == EquationOfStateEnum::WeakCompressibleFluid) {
      return EquationOfState<PhysicalModelData, SimulationControl::kEquationOfState>::getSoundSpeed();
    }
  }

  [[nodiscard]] static Real computeEntropyFromDensityPressure(const Real density, const Real pressure) {
#ifndef __SYCL_DEVICE_ONLY__
    return pressure /
           std::pow(density,
                    EquationOfState<PhysicalModelData, SimulationControl::kEquationOfState>::kSpecificHeatRatio);
#else   // __SYCL_DEVICE_ONLY__
    return pressure /
           sycl::pow(density,
                     EquationOfState<PhysicalModelData, SimulationControl::kEquationOfState>::kSpecificHeatRatio);
#endif  // __SYCL_DEVICE_ONLY__
  }

  [[nodiscard]] static Real computeInternalEnergyFromTemperature(const Real temperature) {
    return ThermodynamicModel<
        PhysicalModelData, SimulationControl::kThermodynamicModel>::computeInternalEnergyFromTemperature(temperature);
  }

  [[nodiscard]] static Real computeTemperatureFromInternalEnergy(const Real internal_energy) {
    return ThermodynamicModel<PhysicalModelData, SimulationControl::kThermodynamicModel>::
        computeTemperatureFromInternalEnergy(internal_energy);
  }

  [[nodiscard]] static Real computeDynamicViscosity([[maybe_unused]] const Real temperature)
    requires(SimulationControl::kTransportModel != TransportModelEnum::None)
  {
    if constexpr (SimulationControl::kTransportModel == TransportModelEnum::Constant) {
      return TransportModel<PhysicalModelData, SimulationControl::kTransportModel>::getDynamicViscosity();
    } else if constexpr (SimulationControl::kTransportModel == TransportModelEnum::Sutherland) {
      return TransportModel<PhysicalModelData, SimulationControl::kTransportModel>::computeDynamicViscosity(
          temperature);
    }
  }

  [[nodiscard]] static Real computeThermalConductivity([[maybe_unused]] const Real temperature)
    requires(SimulationControl::kTransportModel != TransportModelEnum::None)
  {
    if constexpr (SimulationControl::kTransportModel == TransportModelEnum::Constant) {
      return TransportModel<PhysicalModelData, SimulationControl::kTransportModel>::getThermalConductivity();
    } else if constexpr (SimulationControl::kTransportModel == TransportModelEnum::Sutherland) {
      return TransportModel<PhysicalModelData, SimulationControl::kTransportModel>::computeThermalConductivity(
          temperature);
    }
  }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_PHYSICAL_MODEL_CPP_
