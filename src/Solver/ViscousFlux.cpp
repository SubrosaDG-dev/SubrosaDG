/**
 * @file ViscousFlux.cpp
 * @brief The header file of SubrosaDG viscous flux.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-12-02
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2025 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_VISCOUS_FLUX_HPP
#define SUBROSA_DG_VISCOUS_FLUX_HPP

#include <Eigen/Core>

#include "Solver/PhysicalModel.cpp"
#include "Solver/VariableConvertor.cpp"
#include "Utils/BasicDataType.cpp"
#include "Utils/Enum.cpp"

namespace SubrosaDG {

template <typename SimulationControl, int N>
inline void calculateGardientRawFlux(const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                     const Variable<SimulationControl, N>& variable,
                                     FluxVariable<SimulationControl>& gardient_raw_flux, const Isize column) {
  gardient_raw_flux.variable_.noalias() = normal_vector * variable.conserved_.col(column).transpose();
}

template <typename SimulationControl, int N>
inline void calculateVolumeGardientFlux(const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                        const Variable<SimulationControl, N>& left_quadrature_node_variable,
                                        const Variable<SimulationControl, N>& right_quadrature_node_variable,
                                        FluxVariable<SimulationControl>& gardient_volume_flux, const Isize left_column,
                                        const Isize right_column) {
  gardient_volume_flux.variable_.noalias() = normal_vector *
                                             (left_quadrature_node_variable.conserved_.col(left_column) +
                                              right_quadrature_node_variable.conserved_.col(right_column))
                                                 .transpose() /
                                             2.0_r;
}

template <typename SimulationControl, int N>
inline void calculateInterfaceGardientFlux(const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                           const Variable<SimulationControl, N>& left_quadrature_node_variable,
                                           const Variable<SimulationControl, N>& right_quadrature_node_variable,
                                           FluxVariable<SimulationControl>& gardient_interface_flux,
                                           const Isize left_column, const Isize right_column) {
  gardient_interface_flux.variable_.noalias() = normal_vector *
                                                (right_quadrature_node_variable.conserved_.col(right_column) -
                                                 left_quadrature_node_variable.conserved_.col(left_column))
                                                    .transpose() /
                                                2.0_r;
}

template <typename SimulationControl, int N>
inline void calculateViscousRawFlux(const PhysicalModel<SimulationControl>& physical_model,
                                    const Variable<SimulationControl, N>& variable,
                                    const VariableGradient<SimulationControl, N>& variable_gradient,
                                    FluxVariable<SimulationControl>& viscous_raw_flux, const Isize column) {
  if constexpr (SimulationControl::kEquationModel == EquationModelEnum::CompresibleNS) {
    viscous_raw_flux.template setVector<ConservedVariableEnum::Density>(
        Eigen::Vector<Real, SimulationControl::kDimension>::Zero());
    const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>& velocity_gradient =
        variable_gradient.template getMatrix<PrimitiveVariableEnum::Velocity>(column);
    const Real tempurature = physical_model.calculateTemperatureFromInternalEnergy(
        variable.template getScalar<ComputationalVariableEnum::InternalEnergy>(column));
    const Real dynamic_viscosity = physical_model.calculateDynamicViscosity(tempurature);
    const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension> viscous_stress =
        dynamic_viscosity * (velocity_gradient + velocity_gradient.transpose()) -
        2.0_r / 3.0_r * dynamic_viscosity * velocity_gradient.trace() *
            Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>::Identity();
    viscous_raw_flux.template setMatrix<ConservedVariableEnum::Momentum>(viscous_stress);
    const Eigen::Vector<Real, SimulationControl::kDimension>& velocity =
        variable.template getVector<ComputationalVariableEnum::Velocity>(column);
    const Real thermal_conductivity = physical_model.calculateThermalConductivity(tempurature);
    const Eigen::Vector<Real, SimulationControl::kDimension>& tempurature_gradient =
        variable_gradient.template getVector<PrimitiveVariableEnum::Temperature>(column);
    viscous_raw_flux.template setVector<ConservedVariableEnum::DensityTotalEnergy>(
        viscous_stress * velocity + thermal_conductivity * tempurature_gradient);
  }
  if constexpr (SimulationControl::kEquationModel == EquationModelEnum::IncompresibleNS) {
    viscous_raw_flux.template setVector<ConservedVariableEnum::Density>(
        Eigen::Vector<Real, SimulationControl::kDimension>::Zero());
    const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>& velocity_gradient =
        variable_gradient.template getMatrix<PrimitiveVariableEnum::Velocity>(column);
    const Real tempurature = physical_model.calculateTemperatureFromInternalEnergy(
        variable.template getScalar<ComputationalVariableEnum::InternalEnergy>(column));
    const Real dynamic_viscosity = physical_model.calculateDynamicViscosity(tempurature);
    const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension> viscous_stress =
        dynamic_viscosity * (velocity_gradient + velocity_gradient.transpose()) -
        2.0_r / 3.0_r * dynamic_viscosity * velocity_gradient.trace() *
            Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>::Identity();
    viscous_raw_flux.template setMatrix<ConservedVariableEnum::Momentum>(viscous_stress);
    const Real thermal_conductivity = physical_model.calculateThermalConductivity(tempurature);
    const Eigen::Vector<Real, SimulationControl::kDimension>& tempurature_gradient =
        variable_gradient.template getVector<PrimitiveVariableEnum::Temperature>(column);
    viscous_raw_flux.template setVector<ConservedVariableEnum::DensityInternalEnergy>(thermal_conductivity *
                                                                                      tempurature_gradient);
  }
}

template <typename SimulationControl, int N>
inline void calculateArtificialViscousRawFlux(const Real artificial_viscosity,
                                              const VariableGradient<SimulationControl, N>& variable_volume_gradient,
                                              FluxVariable<SimulationControl>& artificial_viscous_raw_flux,
                                              const Isize column) {
  artificial_viscous_raw_flux.variable_.noalias() =
      artificial_viscosity * variable_volume_gradient.conserved_.col(column).reshaped(
                                 SimulationControl::kDimension, SimulationControl::kConservedVariableNumber);
}

template <typename SimulationControl, int N>
inline void calculateViscousNormalFlux(const PhysicalModel<SimulationControl>& physical_model,
                                       const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                       const Variable<SimulationControl, N>& variable,
                                       const VariableGradient<SimulationControl, N>& variable_gradient,
                                       FluxNormalVariable<SimulationControl>& viscous_normal_flux, const Isize column) {
  FluxVariable<SimulationControl> viscous_raw_flux;
  calculateViscousRawFlux(physical_model, variable, variable_gradient, viscous_raw_flux, column);
  viscous_normal_flux.normal_variable_.noalias() = viscous_raw_flux.variable_.transpose() * normal_vector;
}

template <typename SimulationControl, int N>
inline void calculateArtificialViscousNormalFlux(
    const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector, const Real artificial_viscosity,
    const VariableGradient<SimulationControl, N>& variable_volume_gradient,
    FluxNormalVariable<SimulationControl>& artificial_viscous_normal_flux, const Isize column) {
  FluxVariable<SimulationControl> artificial_viscous_raw_flux;
  calculateArtificialViscousRawFlux(artificial_viscosity, variable_volume_gradient, artificial_viscous_raw_flux,
                                    column);
  artificial_viscous_normal_flux.normal_variable_.noalias() =
      artificial_viscous_raw_flux.variable_.transpose() * normal_vector;
}

template <typename SimulationControl, int N>
inline void calculateViscousFlux(const PhysicalModel<SimulationControl>& physical_model,
                                 const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                 const Variable<SimulationControl, N>& left_quadrature_node_variable,
                                 const VariableGradient<SimulationControl, N>& left_quadrature_node_variable_gradient,
                                 const Variable<SimulationControl, N>& right_quadrature_node_variable,
                                 const VariableGradient<SimulationControl, N>& right_quadrature_node_variable_gradient,
                                 Flux<SimulationControl>& viscous_flux, const Isize left_column,
                                 const Isize right_column) {
  calculateViscousNormalFlux(physical_model, normal_vector, left_quadrature_node_variable,
                             left_quadrature_node_variable_gradient, viscous_flux.left_, left_column);
  calculateViscousNormalFlux(physical_model, normal_vector, right_quadrature_node_variable,
                             right_quadrature_node_variable_gradient, viscous_flux.right_, right_column);
  viscous_flux.result_.normal_variable_.noalias() =
      (viscous_flux.left_.normal_variable_ + viscous_flux.right_.normal_variable_) / 2.0_r;
}

template <typename SimulationControl, int N>
inline void calculateViscousFlux(const PhysicalModel<SimulationControl>& physical_model,
                                 const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                 const Variable<SimulationControl, N>& left_quadrature_node_variable,
                                 const VariableGradient<SimulationControl, N>& left_quadrature_node_variable_gradient,
                                 const Variable<SimulationControl, 1>& right_quadrature_node_variable,
                                 const VariableGradient<SimulationControl, 1>& right_quadrature_node_variable_gradient,
                                 Flux<SimulationControl>& viscous_flux, const Isize left_column,
                                 const Isize right_column) {
  calculateViscousNormalFlux(physical_model, normal_vector, left_quadrature_node_variable,
                             left_quadrature_node_variable_gradient, viscous_flux.left_, left_column);
  calculateViscousNormalFlux(physical_model, normal_vector, right_quadrature_node_variable,
                             right_quadrature_node_variable_gradient, viscous_flux.right_, right_column);
  viscous_flux.result_.normal_variable_.noalias() =
      (viscous_flux.left_.normal_variable_ + viscous_flux.right_.normal_variable_) / 2.0_r;
}

template <typename SimulationControl, int N>
inline void calculateArtificialViscousFlux(
    const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector, const Real left_artificial_viscosity,
    const VariableGradient<SimulationControl, N>& left_quadrature_node_variable_volume_gradient,
    const Real right_artificial_viscosity,
    const VariableGradient<SimulationControl, N>& right_quadrature_node_variable_volume_gradient,
    Flux<SimulationControl>& artificial_viscous_flux, const Isize left_column, const Isize right_column) {
  calculateArtificialViscousNormalFlux(normal_vector, left_artificial_viscosity,
                                       left_quadrature_node_variable_volume_gradient, artificial_viscous_flux.left_,
                                       left_column);
  calculateArtificialViscousNormalFlux(normal_vector, right_artificial_viscosity,
                                       right_quadrature_node_variable_volume_gradient, artificial_viscous_flux.right_,
                                       right_column);
  artificial_viscous_flux.result_.normal_variable_.noalias() =
      (artificial_viscous_flux.left_.normal_variable_ + artificial_viscous_flux.right_.normal_variable_) / 2.0_r;
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_VISCOUS_FLUX_HPP
