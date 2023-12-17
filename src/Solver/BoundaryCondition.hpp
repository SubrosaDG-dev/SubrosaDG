/**
 * @file BoundaryCondition.hpp
 * @brief The header file of BoundaryCondition.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
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
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl>& left_quadrature_node_variable,
      Flux<SimulationControl, SimulationControl::kEquationModel>& flux) const = 0;

  virtual ~BoundaryConditionBase() = default;
};

template <typename SimulationControl, typename Derived>
struct BoundaryConditionCRTP : BoundaryConditionBase<SimulationControl> {
  inline void calculateBoundaryConvectiveFlux(
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl>& left_quadrature_node_variable,
      Flux<SimulationControl, SimulationControl::kEquationModel>& flux) const override {
    static_cast<Derived const*>(this)->calculateBoundaryConvectiveFluxImpl(thermal_model, normal_vector,
                                                                           left_quadrature_node_variable, flux);
  }
};

template <typename SimulationControl, BoundaryCondition BoundaryConditionType>
struct BoundaryConditionData
    : BoundaryConditionCRTP<SimulationControl, BoundaryConditionData<SimulationControl, BoundaryConditionType>> {};

template <typename SimulationControl>
struct BoundaryConditionData<SimulationControl, BoundaryCondition::NormalFarfield>
    : BoundaryConditionCRTP<SimulationControl,
                            BoundaryConditionData<SimulationControl, BoundaryCondition::NormalFarfield>> {
  inline void calculateBoundaryConvectiveFluxImpl(
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl>& left_quadrature_node_variable,
      Flux<SimulationControl, SimulationControl::kEquationModel>& flux) const {
    calculateConvectiveFlux<SimulationControl>(thermal_model, normal_vector, left_quadrature_node_variable,
                                               this->variable_, flux);
  }
};

template <typename SimulationControl>
struct BoundaryConditionData<SimulationControl, BoundaryCondition::RiemannFarfield>
    : BoundaryConditionCRTP<SimulationControl,
                            BoundaryConditionData<SimulationControl, BoundaryCondition::RiemannFarfield>> {
  inline void calculateBoundaryConvectiveFluxImpl(
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl>& left_quadrature_node_variable,
      Flux<SimulationControl, SimulationControl::kEquationModel>& flux) const {
    Variable<SimulationControl> farfield_variable;
    const Real velocity_normal = left_quadrature_node_variable.getVelocity().transpose() * normal_vector;
    const Real sound_speed = thermal_model.calculateSoundSpeedFromInternalEnergy(
        left_quadrature_node_variable.template get<ComputationalVariable::InternalEnergy>());
    const Real mach_number_normal = velocity_normal / sound_speed;
    const bool is_supersonic = (std::fabs(mach_number_normal) <=> 1.0) == std::partial_ordering::greater;
    const bool is_negative = (mach_number_normal <=> 0.0) == std::partial_ordering::less;
    if (is_supersonic) {
      if (is_negative) {  // Supersonic inflow
        calculateConvectiveVariable(this->variable_, flux.left_convective_);
      } else {  // Supersonic outflow
        calculateConvectiveVariable(left_quadrature_node_variable, flux.left_convective_);
      }
    } else {
      if (is_negative) {  // Subsonic inflow
        const Real left_riemann_invariant = this->variable_.getVelocity().transpose() * normal_vector -
                                            thermal_model.calculateRiemannInvariantPart(
                                                this->variable_.template get<ComputationalVariable::InternalEnergy>());
        const Real right_riemann_invariant =
            left_quadrature_node_variable.getVelocity().transpose() * normal_vector +
            thermal_model.calculateRiemannInvariantPart(
                left_quadrature_node_variable.template get<ComputationalVariable::InternalEnergy>());
        const Real farfield_normal_velocity = (left_riemann_invariant + right_riemann_invariant) / 2.0;
        const Eigen::Vector<Real, SimulationControl::kDimension> farfield_velocity =
            this->variable_.getVelocity() +
            (farfield_normal_velocity - this->variable_.getVelocity().transpose() * normal_vector) * normal_vector;
        const Real farfield_internal_energy = thermal_model.calculateInternalEnergyFromRiemannInvariantPart(
            (right_riemann_invariant - left_riemann_invariant) / 2.0);
        const Real farfield_density = thermal_model.calculateDensityFromEntropyInternalEnergy(
            thermal_model.calculateEntropyFromDensityPressure(
                this->variable_.template get<ComputationalVariable::Density>(),
                this->variable_.template get<ComputationalVariable::Pressure>()),
            farfield_internal_energy);
        const Real farfield_pressure =
            thermal_model.calculatePressureFormDensityInternalEnergy(farfield_density, farfield_internal_energy);
        farfield_variable.template set<ComputationalVariable::Density>(farfield_density);
        farfield_variable.template set<ComputationalVariable::VelocityX>(farfield_velocity.x());
        if constexpr (SimulationControl::kDimension >= 2) {
          farfield_variable.template set<ComputationalVariable::VelocityY>(farfield_velocity.y());
        }
        if constexpr (SimulationControl::kDimension >= 3) {
          farfield_variable.template set<ComputationalVariable::VelocityZ>(farfield_velocity.z());
        }
        farfield_variable.template set<ComputationalVariable::InternalEnergy>(farfield_internal_energy);
        farfield_variable.template set<ComputationalVariable::Pressure>(farfield_pressure);
      } else {  // Subsonic outflow
        const Real left_riemann_invariant = this->variable_.getVelocity().transpose() * normal_vector -
                                            thermal_model.calculateRiemannInvariantPart(
                                                this->variable_.template get<ComputationalVariable::InternalEnergy>());
        const Real right_riemann_invariant =
            left_quadrature_node_variable.getVelocity().transpose() * normal_vector +
            thermal_model.calculateRiemannInvariantPart(
                left_quadrature_node_variable.template get<ComputationalVariable::InternalEnergy>());
        const Real farfield_normal_velocity = (left_riemann_invariant + right_riemann_invariant) / 2.0;
        const Eigen::Vector<Real, SimulationControl::kDimension> farfield_velocity =
            left_quadrature_node_variable.getVelocity() +
            (farfield_normal_velocity - left_quadrature_node_variable.getVelocity().transpose() * normal_vector) *
                normal_vector;
        const Real farfield_internal_energy = thermal_model.calculateInternalEnergyFromRiemannInvariantPart(
            (right_riemann_invariant - left_riemann_invariant) / 2.0);
        const Real farfield_density = thermal_model.calculateDensityFromEntropyInternalEnergy(
            thermal_model.calculateEntropyFromDensityPressure(
                left_quadrature_node_variable.template get<ComputationalVariable::Density>(),
                left_quadrature_node_variable.template get<ComputationalVariable::Pressure>()),
            farfield_internal_energy);
        const Real farfield_pressure =
            thermal_model.calculatePressureFormDensityInternalEnergy(farfield_density, farfield_internal_energy);
        farfield_variable.template set<ComputationalVariable::Density>(farfield_density);
        farfield_variable.template set<ComputationalVariable::VelocityX>(farfield_velocity.x());
        if constexpr (SimulationControl::kDimension >= 2) {
          farfield_variable.template set<ComputationalVariable::VelocityY>(farfield_velocity.y());
        }
        if constexpr (SimulationControl::kDimension >= 3) {
          farfield_variable.template set<ComputationalVariable::VelocityZ>(farfield_velocity.z());
        }
        farfield_variable.template set<ComputationalVariable::InternalEnergy>(farfield_internal_energy);
        farfield_variable.template set<ComputationalVariable::Pressure>(farfield_pressure);
      }
      calculateConvectiveVariable(farfield_variable, flux.left_convective_);
    }
    flux.convective_n_.noalias() = flux.left_convective_ * normal_vector;
  }
};

template <typename SimulationControl>
struct BoundaryConditionData<SimulationControl, BoundaryCondition::CharacteristicFarfield>
    : BoundaryConditionCRTP<SimulationControl,
                            BoundaryConditionData<SimulationControl, BoundaryCondition::CharacteristicFarfield>> {
  inline void calculateBoundaryConvectiveFluxImpl(
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl>& left_quadrature_node_variable,
      Flux<SimulationControl, SimulationControl::kEquationModel>& flux) const {
    Variable<SimulationControl> farfield_variable;
    const Real velocity_normal = left_quadrature_node_variable.getVelocity().transpose() * normal_vector;
    const Real sound_speed = thermal_model.calculateSoundSpeedFromInternalEnergy(
        left_quadrature_node_variable.template get<ComputationalVariable::InternalEnergy>());
    const Real mach_number_normal = velocity_normal / sound_speed;
    const bool is_supersonic = (std::fabs(mach_number_normal) <=> 1.0) == std::partial_ordering::greater;
    const bool is_negative = (mach_number_normal <=> 0.0) == std::partial_ordering::less;
    if (is_supersonic) {
      if (is_negative) {  // Supersonic inflow
        calculateConvectiveVariable(this->variable_, flux.left_convective_);
      } else {  // Supersonic outflow
        calculateConvectiveVariable(left_quadrature_node_variable, flux.left_convective_);
      }
    } else {
      if (is_negative) {  // Subsonic inflow
        const Real reference_rho = left_quadrature_node_variable.template get<ComputationalVariable::Density>();
        Real farfield_pressure;
        if constexpr (SimulationControl::kDimension == 1) {
          farfield_pressure = (this->variable_.template get<ComputationalVariable::Pressure>() +
                               left_quadrature_node_variable.template get<ComputationalVariable::Pressure>() -
                               reference_rho * sound_speed *
                                   (this->variable_.template get<ComputationalVariable::VelocityX>() -
                                    left_quadrature_node_variable.template get<ComputationalVariable::VelocityX>())) /
                              2.0;
        } else if constexpr (SimulationControl::kDimension == 2) {
          farfield_pressure = (this->variable_.template get<ComputationalVariable::Pressure>() +
                               left_quadrature_node_variable.template get<ComputationalVariable::Pressure>() -
                               reference_rho * sound_speed *
                                   ((this->variable_.template get<ComputationalVariable::VelocityX>() -
                                     left_quadrature_node_variable.template get<ComputationalVariable::VelocityX>()) *
                                        normal_vector.x() +
                                    (this->variable_.template get<ComputationalVariable::VelocityY>() -
                                     left_quadrature_node_variable.template get<ComputationalVariable::VelocityY>()) *
                                        normal_vector.y())) /
                              2.0;
        } else if constexpr (SimulationControl::kDimension == 3) {
          farfield_pressure = (this->variable_.template get<ComputationalVariable::Pressure>() +
                               left_quadrature_node_variable.template get<ComputationalVariable::Pressure>() -
                               reference_rho * sound_speed *
                                   ((this->variable_.template get<ComputationalVariable::VelocityX>() -
                                     left_quadrature_node_variable.template get<ComputationalVariable::VelocityX>()) *
                                        normal_vector.x() +
                                    (this->variable_.template get<ComputationalVariable::VelocityY>() -
                                     left_quadrature_node_variable.template get<ComputationalVariable::VelocityY>()) *
                                        normal_vector.y() +
                                    (this->variable_.template get<ComputationalVariable::VelocityZ>() -
                                     left_quadrature_node_variable.template get<ComputationalVariable::VelocityZ>()) *
                                        normal_vector.z())) /
                              2.0;
        }
        const Real delta_pressure = farfield_pressure - this->variable_.template get<ComputationalVariable::Pressure>();
        const Real farfield_rho = this->variable_.template get<ComputationalVariable::Density>() +
                                  delta_pressure / (sound_speed * sound_speed);
        farfield_variable.template set<ComputationalVariable::Density>(farfield_rho);
        farfield_variable.template set<ComputationalVariable::VelocityX>(
            this->variable_.template get<ComputationalVariable::VelocityX>() +
            delta_pressure / (reference_rho * sound_speed) * normal_vector.x());
        if constexpr (SimulationControl::kDimension >= 2) {
          farfield_variable.template set<ComputationalVariable::VelocityY>(
              this->variable_.template get<ComputationalVariable::VelocityY>() +
              delta_pressure / (reference_rho * sound_speed) * normal_vector.y());
        }
        if constexpr (SimulationControl::kDimension >= 3) {
          farfield_variable.template set<ComputationalVariable::VelocityZ>(
              this->variable_.template get<ComputationalVariable::VelocityZ>() +
              delta_pressure / (reference_rho * sound_speed) * normal_vector.z());
        }
        farfield_variable.template set<ComputationalVariable::InternalEnergy>(
            thermal_model.calculateInternalEnergyFromDensityPressure(farfield_rho, farfield_pressure));
        farfield_variable.template set<ComputationalVariable::Pressure>(farfield_pressure);
      } else {  // Subsonic outflow
        const Real reference_rho = left_quadrature_node_variable.template get<ComputationalVariable::Density>();
        const Real farfield_pressure = this->variable_.template get<ComputationalVariable::Pressure>();
        const Real delta_pressure =
            farfield_pressure - left_quadrature_node_variable.template get<ComputationalVariable::Pressure>();
        const Real farfield_rho = left_quadrature_node_variable.template get<ComputationalVariable::Density>() +
                                  delta_pressure / (sound_speed * sound_speed);
        farfield_variable.template set<ComputationalVariable::Density>(farfield_rho);
        farfield_variable.template set<ComputationalVariable::VelocityX>(
            left_quadrature_node_variable.template get<ComputationalVariable::VelocityX>() -
            delta_pressure / (reference_rho * sound_speed) * normal_vector.x());
        if constexpr (SimulationControl::kDimension >= 2) {
          farfield_variable.template set<ComputationalVariable::VelocityY>(
              left_quadrature_node_variable.template get<ComputationalVariable::VelocityY>() -
              delta_pressure / (reference_rho * sound_speed) * normal_vector.y());
        }
        if constexpr (SimulationControl::kDimension >= 3) {
          farfield_variable.template set<ComputationalVariable::VelocityZ>(
              left_quadrature_node_variable.template get<ComputationalVariable::VelocityZ>() -
              delta_pressure / (reference_rho * sound_speed) * normal_vector.z());
        }
        farfield_variable.template set<ComputationalVariable::InternalEnergy>(
            thermal_model.calculateInternalEnergyFromDensityPressure(farfield_rho, farfield_pressure));
        farfield_variable.template set<ComputationalVariable::Pressure>(farfield_pressure);
      }
      calculateConvectiveVariable(farfield_variable, flux.left_convective_);
    }
    flux.convective_n_.noalias() = flux.left_convective_ * normal_vector;
  }
};

template <typename SimulationControl>
  requires(SimulationControl::kDimension == 2 || SimulationControl::kDimension == 3)
struct BoundaryConditionData<SimulationControl, BoundaryCondition::AdiabaticWall>
    : BoundaryConditionCRTP<SimulationControl,
                            BoundaryConditionData<SimulationControl, BoundaryCondition::AdiabaticWall>> {
  inline void calculateBoundaryConvectiveFluxImpl(
      [[maybe_unused]] const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl>& left_quadrature_node_variable,
      Flux<SimulationControl, SimulationControl::kEquationModel>& flux) const {
    Variable<SimulationControl> wall_variable;
    const Eigen::Vector<Real, SimulationControl::kDimension> wall_velocity =
        left_quadrature_node_variable.getVelocity() -
        (left_quadrature_node_variable.getVelocity().transpose() * normal_vector) * normal_vector;
    wall_variable.template set<ComputationalVariable::Density>(left_quadrature_node_variable);
    wall_variable.template set<ComputationalVariable::VelocityX>(wall_velocity.x());
    wall_variable.template set<ComputationalVariable::VelocityY>(wall_velocity.y());
    if constexpr (SimulationControl::kDimension >= 3) {
      wall_variable.template set<ComputationalVariable::VelocityZ>(wall_velocity.z());
    }
    wall_variable.template set<ComputationalVariable::InternalEnergy>(left_quadrature_node_variable);
    wall_variable.template set<ComputationalVariable::Pressure>(left_quadrature_node_variable);
    wall_variable.calculateConservedFromComputational();
    calculateConvectiveVariable(wall_variable, flux.left_convective_);
    flux.convective_n_.noalias() = flux.left_convective_ * normal_vector;
    // Variable<SimulationControl> wall_variable;
    // const Eigen::Vector<Real, SimulationControl::kDimension> wall_velocity =
    //     left_quadrature_node_variable.getVelocity() -
    //     (2.0 * left_quadrature_node_variable.getVelocity().transpose() * normal_vector) * normal_vector;
    // wall_variable.template set<ComputationalVariable::Density>(left_quadrature_node_variable);
    // wall_variable.template set<ComputationalVariable::VelocityX>(wall_velocity.x());
    // wall_variable.template set<ComputationalVariable::VelocityY>(wall_velocity.y());
    // if constexpr (SimulationControl::kDimension >= 3) {
    //   wall_variable.template set<ComputationalVariable::VelocityZ>(wall_velocity.z());
    // }
    // wall_variable.template set<ComputationalVariable::InternalEnergy>(left_quadrature_node_variable);
    // wall_variable.template set<ComputationalVariable::Pressure>(left_quadrature_node_variable);
    // wall_variable.calculateConservedFromComputational();
    // calculateConvectiveFlux(thermal_model, normal_vector, left_quadrature_node_variable, wall_variable, flux);
  }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_BOUNDARY_CONDITION_HPP_
