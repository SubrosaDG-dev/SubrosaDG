/**
 * @file SourceTerm.cpp
 * @brief
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2024-05-13
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2025 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_SOURCE_TERM_CPP_
#define SUBROSA_DG_SOURCE_TERM_CPP_

#include "Solver/PhysicalModel.cpp"
#include "Solver/VariableConvertor.cpp"
#include "Utils/BasicDataType.cpp"
#include "Utils/Enum.cpp"

namespace SubrosaDG {

template <typename SimulationControl, SourceTermEnum SourceTermType>
struct SourceTermBase;

template <typename SimulationControl>
struct SourceTermBase<SimulationControl, SourceTermEnum::None> {};

template <typename SimulationControl>
struct SourceTermBase<SimulationControl, SourceTermEnum::Boussinesq> {
  inline static constexpr Real kGravity = 1.0_r;
  inline static Real thermal_expansion_coefficient;
  inline static Real reference_temperature;

  template <int N>
  inline void calculateSourceTerm(const PhysicalModel<SimulationControl>& physical_model,
                                  const Variable<SimulationControl, N>& quadrature_node_variable,
                                  FluxNormalVariable<SimulationControl>& source_flux, const Isize column) const {
    source_flux.normal_variable_.setZero();
    if constexpr (SimulationControl::kDimension == 2) {
      source_flux.template setScalar<ConservedVariableEnum::MomentumY>(
          quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(column) *
          this->thermal_expansion_coefficient *
          (physical_model.calculateTemperatureFromInternalEnergy(
               quadrature_node_variable.template getScalar<ComputationalVariableEnum::InternalEnergy>(column)) -
           this->reference_temperature) *
          this->kGravity);
    } else if constexpr (SimulationControl::kDimension == 3) {
      source_flux.template setScalar<ConservedVariableEnum::MomentumZ>(
          quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(column) *
          this->thermal_expansion_coefficient *
          (physical_model.calculateTemperatureFromInternalEnergy(
               quadrature_node_variable.template getScalar<ComputationalVariableEnum::InternalEnergy>(column)) -
           this->reference_temperature) *
          this->kGravity);
    }
  }
};

template <typename SimulationControl>
struct SourceTerm : SourceTermBase<SimulationControl, SimulationControl::kSourceTerm> {};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_SOURCE_TERM_CPP_
