/**
 * @file BoundaryCondition.hpp
 * @brief The header file of BoundaryCondition.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_BOUNDARY_CONDITION_HPP_
#define SUBROSA_DG_BOUNDARY_CONDITION_HPP_

#include <Eigen/Core>
#include <cmath>
#include <compare>

#include "Solver/ConvectiveFlux.hpp"
#include "Solver/ThermalModel.hpp"
#include "Solver/VariableConvertor.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Enum.hpp"

namespace SubrosaDG {

template <typename SimulationControl>
struct BoundaryConditionBase {
  Variable<SimulationControl> variable_;

  virtual inline void calculateBoundaryConvectiveFlux(
      const ThermalModel<SimulationControl>& thermal_model,
      const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>& transition_matrix,
      const Variable<SimulationControl>& left_quadrature_node_variable, Flux<SimulationControl>& flux) const = 0;

  virtual ~BoundaryConditionBase() = default;
};

template <typename SimulationControl, typename Derived>
struct BoundaryConditionCRTP : BoundaryConditionBase<SimulationControl> {
  inline void calculateBoundaryConvectiveFlux(
      const ThermalModel<SimulationControl>& thermal_model,
      const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>& transition_matrix,
      const Variable<SimulationControl>& left_quadrature_node_variable, Flux<SimulationControl>& flux) const override {
    static_cast<Derived const*>(this)->calculateBoundaryConvectiveFluxImpl(thermal_model, transition_matrix,
                                                                           left_quadrature_node_variable, flux);
  }
};

template <typename SimulationControl, BoundaryConditionEnum BoundaryConditionType>
struct BoundaryCondition
    : BoundaryConditionCRTP<SimulationControl, BoundaryCondition<SimulationControl, BoundaryConditionType>> {};

template <typename SimulationControl>
struct BoundaryCondition<SimulationControl, BoundaryConditionEnum::NormalFarfield>
    : BoundaryConditionCRTP<SimulationControl,
                            BoundaryCondition<SimulationControl, BoundaryConditionEnum::NormalFarfield>> {
  inline void calculateBoundaryConvectiveFluxImpl(
      const ThermalModel<SimulationControl>& thermal_model,
      const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>& transition_matrix,
      const Variable<SimulationControl>& left_quadrature_node_variable, Flux<SimulationControl>& flux) const {
    calculateConvectiveFlux(thermal_model, transition_matrix, left_quadrature_node_variable, this->variable_, flux);
  }
};

template <typename SimulationControl>
struct BoundaryCondition<SimulationControl, BoundaryConditionEnum::RiemannFarfield>
    : BoundaryConditionCRTP<SimulationControl,
                            BoundaryCondition<SimulationControl, BoundaryConditionEnum::RiemannFarfield>> {
  inline void calculateBoundaryConvectiveFluxImpl(
      const ThermalModel<SimulationControl>& thermal_model,
      const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>& transition_matrix,
      const Variable<SimulationControl>& left_quadrature_node_variable, Flux<SimulationControl>& flux) const {
    Variable<SimulationControl> farfield_variable;
    const Real velocity_normal =
        left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>().transpose() *
        transition_matrix.col(0);
    const Real sound_speed = thermal_model.calculateSoundSpeedFromInternalEnergy(
        left_quadrature_node_variable.template get<ComputationalVariableEnum::InternalEnergy>());
    const Real mach_number_normal = velocity_normal / sound_speed;
    const bool is_supersonic = (std::abs(mach_number_normal) <=> 1.0) == std::partial_ordering::greater;
    const bool is_negative = (mach_number_normal <=> 0.0) == std::partial_ordering::less;
    if (is_supersonic) {
      if (is_negative) {  // Supersonic inflow
        calculateConvectiveNormalVariable(this->variable_, transition_matrix, flux.convective_n_);
      } else {  // Supersonic outflow
        calculateConvectiveNormalVariable(left_quadrature_node_variable, transition_matrix, flux.convective_n_);
      }
    } else {
      if (is_negative) {  // Subsonic inflow
        const Real left_riemann_invariant =
            this->variable_.template getVector<ComputationalVariableEnum::Velocity>().transpose() *
                transition_matrix.col(0) -
            thermal_model.calculateRiemannInvariantPart(
                this->variable_.template get<ComputationalVariableEnum::InternalEnergy>());
        const Real right_riemann_invariant =
            left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>().transpose() *
                transition_matrix.col(0) +
            thermal_model.calculateRiemannInvariantPart(
                left_quadrature_node_variable.template get<ComputationalVariableEnum::InternalEnergy>());
        const Real farfield_normal_velocity = (left_riemann_invariant + right_riemann_invariant) / 2.0;
        const Eigen::Vector<Real, SimulationControl::kDimension> farfield_velocity =
            this->variable_.template getVector<ComputationalVariableEnum::Velocity>() +
            (farfield_normal_velocity -
             this->variable_.template getVector<ComputationalVariableEnum::Velocity>().transpose() *
                 transition_matrix.col(0)) *
                transition_matrix.col(0);
        const Real farfield_internal_energy = thermal_model.calculateInternalEnergyFromRiemannInvariantPart(
            (right_riemann_invariant - left_riemann_invariant) / 2.0);
        const Real farfield_density = thermal_model.calculateDensityFromEntropyInternalEnergy(
            thermal_model.calculateEntropyFromDensityPressure(
                this->variable_.template get<ComputationalVariableEnum::Density>(),
                this->variable_.template get<ComputationalVariableEnum::Pressure>()),
            farfield_internal_energy);
        const Real farfield_pressure =
            thermal_model.calculatePressureFormDensityInternalEnergy(farfield_density, farfield_internal_energy);
        farfield_variable.template set<ComputationalVariableEnum::Density>(farfield_density);
        farfield_variable.template setVector<ComputationalVariableEnum::Velocity>(farfield_velocity);
        farfield_variable.template set<ComputationalVariableEnum::InternalEnergy>(farfield_internal_energy);
        farfield_variable.template set<ComputationalVariableEnum::Pressure>(farfield_pressure);
      } else {  // Subsonic outflow
        const Real left_riemann_invariant =
            this->variable_.template getVector<ComputationalVariableEnum::Velocity>().transpose() *
                transition_matrix.col(0) -
            thermal_model.calculateRiemannInvariantPart(
                this->variable_.template get<ComputationalVariableEnum::InternalEnergy>());
        const Real right_riemann_invariant =
            left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>().transpose() *
                transition_matrix.col(0) +
            thermal_model.calculateRiemannInvariantPart(
                left_quadrature_node_variable.template get<ComputationalVariableEnum::InternalEnergy>());
        const Real farfield_normal_velocity = (left_riemann_invariant + right_riemann_invariant) / 2.0;
        const Eigen::Vector<Real, SimulationControl::kDimension> farfield_velocity =
            left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>() +
            (farfield_normal_velocity -
             left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>().transpose() *
                 transition_matrix.col(0)) *
                transition_matrix.col(0);
        const Real farfield_internal_energy = thermal_model.calculateInternalEnergyFromRiemannInvariantPart(
            (right_riemann_invariant - left_riemann_invariant) / 2.0);
        const Real farfield_density = thermal_model.calculateDensityFromEntropyInternalEnergy(
            thermal_model.calculateEntropyFromDensityPressure(
                left_quadrature_node_variable.template get<ComputationalVariableEnum::Density>(),
                left_quadrature_node_variable.template get<ComputationalVariableEnum::Pressure>()),
            farfield_internal_energy);
        const Real farfield_pressure =
            thermal_model.calculatePressureFormDensityInternalEnergy(farfield_density, farfield_internal_energy);
        farfield_variable.template set<ComputationalVariableEnum::Density>(farfield_density);
        farfield_variable.template setVector<ComputationalVariableEnum::Velocity>(farfield_velocity);
        farfield_variable.template set<ComputationalVariableEnum::InternalEnergy>(farfield_internal_energy);
        farfield_variable.template set<ComputationalVariableEnum::Pressure>(farfield_pressure);
      }
      calculateConvectiveNormalVariable(farfield_variable, transition_matrix, flux.convective_n_);
    }
  }
};

template <typename SimulationControl>
  requires(SimulationControl::kDimension == 2 || SimulationControl::kDimension == 3)
struct BoundaryCondition<SimulationControl, BoundaryConditionEnum::AdiabaticWall>
    : BoundaryConditionCRTP<SimulationControl,
                            BoundaryCondition<SimulationControl, BoundaryConditionEnum::AdiabaticWall>> {
  inline void calculateBoundaryConvectiveFluxImpl(
      [[maybe_unused]] const ThermalModel<SimulationControl>& thermal_model,
      const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>& transition_matrix,
      const Variable<SimulationControl>& left_quadrature_node_variable, Flux<SimulationControl>& flux) const {
    Variable<SimulationControl> wall_variable;
    const Eigen::Vector<Real, SimulationControl::kDimension> wall_velocity =
        left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>() -
        (left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>().transpose() *
         transition_matrix.col(0)) *
            transition_matrix.col(0);
    wall_variable.template set<ComputationalVariableEnum::Density>(left_quadrature_node_variable);
    wall_variable.template setVector<ComputationalVariableEnum::Velocity>(wall_velocity);
    wall_variable.template set<ComputationalVariableEnum::InternalEnergy>(left_quadrature_node_variable);
    wall_variable.template set<ComputationalVariableEnum::Pressure>(left_quadrature_node_variable);
    wall_variable.calculateConservedFromComputational();
    calculateConvectiveNormalVariable(wall_variable, transition_matrix, flux.convective_n_);
    // Variable<SimulationControl> wall_variable;
    // const Eigen::Vector<Real, SimulationControl::kDimension> wall_velocity =
    //     left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>() -
    //     (2.0 * left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>().transpose() *
    //     transition_matrix.col(0)) * transition_matrix.col(0);
    // wall_variable.template set<ComputationalVariableEnum::Density>(left_quadrature_node_variable);
    // wall_variable.template setVector<ComputationalVariableEnum::Velocity>(wall_velocity);
    // wall_variable.template set<ComputationalVariableEnum::InternalEnergy>(left_quadrature_node_variable);
    // wall_variable.template set<ComputationalVariableEnum::Pressure>(left_quadrature_node_variable);
    // wall_variable.calculateConservedFromComputational();
    // calculateConvectiveFlux(thermal_model, transition_matrix, left_quadrature_node_variable, this->variable_, flux);
  }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_BOUNDARY_CONDITION_HPP_
