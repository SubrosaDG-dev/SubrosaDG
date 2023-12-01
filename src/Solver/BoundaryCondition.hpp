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

template <typename SimulationControl, BoundaryCondition BoundaryConditionType>
struct BoundaryConditionData : BoundaryConditionBase<SimulationControl> {};

template <typename SimulationControl>
struct BoundaryConditionData<SimulationControl, BoundaryCondition::NormalFarfield>
    : BoundaryConditionBase<SimulationControl> {
  inline void calculateBoundaryConvectiveFlux(
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl>& left_quadrature_node_variable,
      Flux<SimulationControl, SimulationControl::kEquationModel>& flux) const override {
    calculateConvectiveFlux<SimulationControl>(thermal_model, normal_vector, left_quadrature_node_variable,
                                               this->variable_, flux);
  }
};

template <typename SimulationControl>
struct BoundaryConditionData<SimulationControl, BoundaryCondition::CharacteristicFarfield>
    : BoundaryConditionBase<SimulationControl> {
  inline void calculateBoundaryConvectiveFlux(
      [[maybe_unused]] const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl>& left_quadrature_node_variable,
      Flux<SimulationControl, SimulationControl::kEquationModel>& flux) const override {
    if constexpr (SimulationControl::kDimension == 2) {
      Variable<SimulationControl> farfield_variable;
      const Real velocity_normal = left_quadrature_node_variable.getVelocity().transpose() * normal_vector;
      const Real sound_speed = thermal_model.calculateSoundSpeedFromInternalEnergy(
          left_quadrature_node_variable.template get<ComputationalVariable::InternalEnergy>());
      const Real mach_number_normal = velocity_normal / sound_speed;
      const bool is_supersonic = (std::fabs(mach_number_normal) <=> 1.0) == std::partial_ordering::greater;
      const bool is_positive = (mach_number_normal <=> 0.0) == std::partial_ordering::greater;
      if (is_supersonic) {
        if (is_positive) {  // Supersonic outflow
          calculateConvectiveVariable(left_quadrature_node_variable, flux.left_convective_);
        } else {  // Supersonic inflow
          calculateConvectiveVariable(this->variable_, flux.left_convective_);
        }
      } else {
        if (is_positive) {  // Subsonic outflow
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
          farfield_variable.template set<ComputationalVariable::VelocityY>(
              left_quadrature_node_variable.template get<ComputationalVariable::VelocityY>() -
              delta_pressure / (reference_rho * sound_speed) * normal_vector.y());
          farfield_variable.template set<ComputationalVariable::InternalEnergy>(
              thermal_model.calculateInternalEnergyFromDensityPressure(farfield_rho, farfield_pressure));
          farfield_variable.template set<ComputationalVariable::Pressure>(farfield_pressure);
          calculateConvectiveVariable(farfield_variable, flux.left_convective_);
        } else {  // Subsonic inflow
          const Real reference_rho = left_quadrature_node_variable.template get<ComputationalVariable::Density>();
          const Real farfield_pressure =
              0.5 * (this->variable_.template get<ComputationalVariable::Pressure>() +
                     left_quadrature_node_variable.template get<ComputationalVariable::Pressure>() -
                     reference_rho * sound_speed *
                         ((this->variable_.template get<ComputationalVariable::VelocityX>() -
                           left_quadrature_node_variable.template get<ComputationalVariable::VelocityX>()) *
                              normal_vector.x() +
                          (this->variable_.template get<ComputationalVariable::VelocityY>() -
                           left_quadrature_node_variable.template get<ComputationalVariable::VelocityY>()) *
                              normal_vector.y()));
          const Real delta_pressure =
              farfield_pressure - this->variable_.template get<ComputationalVariable::Pressure>();
          const Real farfield_rho = this->variable_.template get<ComputationalVariable::Density>() +
                                    delta_pressure / (sound_speed * sound_speed);
          farfield_variable.template set<ComputationalVariable::Density>(farfield_rho);
          farfield_variable.template set<ComputationalVariable::VelocityX>(
              this->variable_.template get<ComputationalVariable::VelocityX>() +
              delta_pressure / (reference_rho * sound_speed) * normal_vector.x());
          farfield_variable.template set<ComputationalVariable::VelocityY>(
              this->variable_.template get<ComputationalVariable::VelocityY>() +
              delta_pressure / (reference_rho * sound_speed) * normal_vector.y());
          farfield_variable.template set<ComputationalVariable::InternalEnergy>(
              thermal_model.calculateInternalEnergyFromDensityPressure(farfield_rho, farfield_pressure));
          farfield_variable.template set<ComputationalVariable::Pressure>(farfield_pressure);
          calculateConvectiveVariable(farfield_variable, flux.left_convective_);
        }
      }
      flux.convective_n_.noalias() = flux.left_convective_ * normal_vector;
    }
  }
};

template <typename SimulationControl>
struct BoundaryConditionData<SimulationControl, BoundaryCondition::NoSlipWall>
    : BoundaryConditionBase<SimulationControl> {
  inline void calculateBoundaryConvectiveFlux(
      [[maybe_unused]] const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl>& left_quadrature_node_variable,
      Flux<SimulationControl, SimulationControl::kEquationModel>& flux) const override {
    Variable<SimulationControl> wall_variable;
    if constexpr (SimulationControl::kDimension == 2) {
      wall_variable.template set<ComputationalVariable::Density>(left_quadrature_node_variable);
      wall_variable.template set<ComputationalVariable::VelocityX>(0.0);
      wall_variable.template set<ComputationalVariable::VelocityY>(0.0);
      wall_variable.template set<ComputationalVariable::InternalEnergy>(left_quadrature_node_variable);
      wall_variable.template set<ComputationalVariable::Pressure>(left_quadrature_node_variable);
      calculateConvectiveVariable(wall_variable, flux.left_convective_);
      flux.convective_n_.noalias() = flux.left_convective_ * normal_vector;
    }
  }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_BOUNDARY_CONDITION_HPP_
