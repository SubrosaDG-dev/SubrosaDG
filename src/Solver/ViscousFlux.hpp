/**
 * @file ViscousFlux.hpp
 * @brief The header file of SubrosaDG viscous flux.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-12-02
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_VISCOUS_FLUX_HPP
#define SUBROSA_DG_VISCOUS_FLUX_HPP

#include <Eigen/Core>

#include "Solver/ThermalModel.hpp"
#include "Solver/VariableConvertor.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Enum.hpp"

namespace SubrosaDG {

template <typename SimulationControl>
inline void calculateGardientRawFlux(const Variable<SimulationControl>& variable,
                                     const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                     FluxVariable<SimulationControl>& gardient_raw_flux) {
  gardient_raw_flux.variable_.noalias() = normal_vector * variable.conserved_.transpose();
}

template <typename SimulationControl>
inline void calculateVolumeGardientFlux(const Variable<SimulationControl>& left_quadrature_node_variable,
                                        const Variable<SimulationControl>& right_quadrature_node_variable,
                                        const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                        FluxVariable<SimulationControl>& gardient_volume_flux) {
  gardient_volume_flux.variable_.noalias() =
      normal_vector *
      (left_quadrature_node_variable.conserved_ + right_quadrature_node_variable.conserved_).transpose() / 2.0;
}

template <typename SimulationControl>
inline void calculateInterfaceGardientFlux(const Variable<SimulationControl>& left_quadrature_node_variable,
                                           const Variable<SimulationControl>& right_quadrature_node_variable,
                                           const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                           FluxVariable<SimulationControl>& gardient_interface_flux) {
  gardient_interface_flux.variable_.noalias() =
      normal_vector *
      (right_quadrature_node_variable.conserved_ - left_quadrature_node_variable.conserved_).transpose() / 2.0;
}

template <typename SimulationControl>
inline void calculateViscousRawFlux(const ThermalModel<SimulationControl>& thermal_model,
                                    const Variable<SimulationControl>& variable,
                                    const VariableGradient<SimulationControl>& variable_gradient,
                                    FluxVariable<SimulationControl>& viscous_raw_flux) {
  viscous_raw_flux.template setVector<ConservedVariableEnum::Density>(
      Eigen::Vector<Real, SimulationControl::kDimension>::Zero());
  const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>& velocity_gradient =
      variable_gradient.template getMatrix<PrimitiveVariableEnum::Velocity>();
  const Real tempurature = thermal_model.calculateTemperatureFromInternalEnergy(
      variable.template getScalar<ComputationalVariableEnum::InternalEnergy>());
  const Real dynamic_viscosity = thermal_model.calculateDynamicViscosity(tempurature);
  const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension> viscous_stress =
      dynamic_viscosity * (velocity_gradient + velocity_gradient.transpose()) -
      2.0 / 3.0 * dynamic_viscosity * velocity_gradient.trace() *
          Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>::Identity();
  viscous_raw_flux.template setMatrix<ConservedVariableEnum::Momentum>(viscous_stress);
  const Eigen::Vector<Real, SimulationControl::kDimension>& velocity =
      variable.template getVector<ComputationalVariableEnum::Velocity>();
  const Real thermal_conductivity = thermal_model.calculateThermalConductivity(tempurature);
  const Eigen::Vector<Real, SimulationControl::kDimension>& tempurature_gradient =
      variable_gradient.template getVector<PrimitiveVariableEnum::Temperature>();
  viscous_raw_flux.template setVector<ConservedVariableEnum::DensityTotalEnergy>(
      viscous_stress * velocity + thermal_conductivity * tempurature_gradient);
}

template <typename SimulationControl>
inline void calculateViscousFlux(const ThermalModel<SimulationControl>& thermal_model,
                                 const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                 const Variable<SimulationControl>& left_quadrature_node_variable,
                                 const VariableGradient<SimulationControl>& left_quadrature_node_variable_gradient,
                                 const Variable<SimulationControl>& right_quadrature_node_variable,
                                 const VariableGradient<SimulationControl>& right_quadrature_node_variable_gradient,
                                 FluxNormalVariable<SimulationControl>& viscous_flux) {
  FluxVariable<SimulationControl> left_quadrature_node_viscous_raw_flux;
  FluxVariable<SimulationControl> right_quadrature_node_viscous_raw_flux;
  calculateViscousRawFlux(thermal_model, left_quadrature_node_variable, left_quadrature_node_variable_gradient,
                          left_quadrature_node_viscous_raw_flux);
  calculateViscousRawFlux(thermal_model, right_quadrature_node_variable, right_quadrature_node_variable_gradient,
                          right_quadrature_node_viscous_raw_flux);
  viscous_flux.normal_variable_.noalias() =
      (left_quadrature_node_viscous_raw_flux.variable_ + right_quadrature_node_viscous_raw_flux.variable_).transpose() *
      normal_vector / 2.0;
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_VISCOUS_FLUX_HPP
