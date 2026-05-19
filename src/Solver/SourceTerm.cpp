/**
 * @file SourceTerm.cpp
 * @brief
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2024-05-13
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
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
struct SourceTerm;

template <typename SimulationControl>
struct SourceTerm<SimulationControl, SourceTermEnum::None> {};

template <typename SimulationControl>
struct SourceTerm<SimulationControl, SourceTermEnum::Boussinesq> {
  static constexpr Real kGravity = static_cast<Real>(1.0);
  Real thermal_expansion_coefficient_;
  Real reference_temperature_;

  void computeSourceQuadrature(
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          quadrature_node_computational_variable,
      Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>> quadrature_node_source_quadrature,
      const Real jacobian_determinant_multiply_weight) const {
    quadrature_node_source_quadrature.setZero();
    const Real temperature = PhysicalModel<SimulationControl, PhysicalModelData>::computeTemperatureFromInternalEnergy(
        Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
            quadrature_node_computational_variable));
    if constexpr (SimulationControl::kDimension == 2) {
      NormalFlux<SimulationControl>::template getScalar<ConservedVariableEnum::MomentumY>(
          quadrature_node_source_quadrature) =
          Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
              quadrature_node_computational_variable) *
          this->thermal_expansion_coefficient_ * (temperature - this->reference_temperature_) * kGravity *
          jacobian_determinant_multiply_weight;
    } else if constexpr (SimulationControl::kDimension == 3) {
      NormalFlux<SimulationControl>::template getScalar<ConservedVariableEnum::MomentumZ>(
          quadrature_node_source_quadrature) =
          Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
              quadrature_node_computational_variable) *
          this->thermal_expansion_coefficient_ * (temperature - this->reference_temperature_) * kGravity *
          jacobian_determinant_multiply_weight;
    }
  }
};

template <typename SimulationControl, SourceTermEnum SourceTermType>
struct SourceTermDevice;

template <typename SimulationControl>
struct SourceTermDevice<SimulationControl, SourceTermEnum::None> {};

template <typename SimulationControl>
struct SourceTermDevice<SimulationControl, SourceTermEnum::Boussinesq> {
  static constexpr Real kGravity = static_cast<Real>(1.0);
  Real thermal_expansion_coefficient_;
  Real reference_temperature_;

  void computeSourceQuadrature(
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          quadrature_node_computational_variable,
      Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber>> quadrature_node_source_quadrature,
      const Real jacobian_determinant_multiply_weight) const {
    quadrature_node_source_quadrature.setZero();
    const Real temperature = PhysicalModel<SimulationControl, PhysicalModelData>::computeTemperatureFromInternalEnergy(
        Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
            quadrature_node_computational_variable));
    if constexpr (SimulationControl::kDimension == 2) {
      NormalFlux<SimulationControl>::template getScalar<ConservedVariableEnum::MomentumY>(
          quadrature_node_source_quadrature) =
          Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
              quadrature_node_computational_variable) *
          this->thermal_expansion_coefficient_ * (temperature - this->reference_temperature_) * kGravity *
          jacobian_determinant_multiply_weight;
    } else if constexpr (SimulationControl::kDimension == 3) {
      NormalFlux<SimulationControl>::template getScalar<ConservedVariableEnum::MomentumZ>(
          quadrature_node_source_quadrature) =
          Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
              quadrature_node_computational_variable) *
          this->thermal_expansion_coefficient_ * (temperature - this->reference_temperature_) * kGravity *
          jacobian_determinant_multiply_weight;
    }
  }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_SOURCE_TERM_CPP_
