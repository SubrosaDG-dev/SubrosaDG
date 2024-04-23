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

#include "Solver/ThermalModel.hpp"
#include "Solver/VariableConvertor.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Enum.hpp"

namespace SubrosaDG {

template <typename SimulationControl>
inline void calculateRiemannSolver(const ThermalModel<SimulationControl>& thermal_model,
                                   const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                   const Variable<SimulationControl>& left_variable,
                                   const Variable<SimulationControl>& right_variable,
                                   Variable<SimulationControl>& boundary_variable) {
  const Real normal_velocity =
      left_variable.template getVector<ComputationalVariableEnum::Velocity>().transpose() * normal_vector;
  const Real normal_mach_number =
      normal_velocity / thermal_model.calculateSoundSpeedFromInternalEnergy(
                            left_variable.template getScalar<ComputationalVariableEnum::InternalEnergy>());
  if (std::abs(normal_mach_number) > 1.0) {
    if (normal_mach_number < 0.0) {  // Supersonic inflow
      boundary_variable.computational_ = right_variable.computational_;
    } else {  // Supersonic outflow
      boundary_variable.computational_ = left_variable.computational_;
    }
  } else {
    if (normal_mach_number < 0.0) {  // Subsonic inflow
      const Real left_riemann_invariant =
          right_variable.template getVector<ComputationalVariableEnum::Velocity>().transpose() * normal_vector -
          thermal_model.calculateRiemannInvariantPart(
              right_variable.template getScalar<ComputationalVariableEnum::InternalEnergy>());
      const Real right_riemann_invariant =
          left_variable.template getVector<ComputationalVariableEnum::Velocity>().transpose() * normal_vector +
          thermal_model.calculateRiemannInvariantPart(
              left_variable.template getScalar<ComputationalVariableEnum::InternalEnergy>());
      const Real boundary_normal_velocity = (left_riemann_invariant + right_riemann_invariant) / 2.0;
      const Eigen::Vector<Real, SimulationControl::kDimension> boundary_velocity =
          right_variable.template getVector<ComputationalVariableEnum::Velocity>() +
          (boundary_normal_velocity -
           right_variable.template getVector<ComputationalVariableEnum::Velocity>().transpose() * normal_vector) *
              normal_vector;
      const Real boundary_internal_energy = thermal_model.calculateInternalEnergyFromRiemannInvariantPart(
          (right_riemann_invariant - left_riemann_invariant) / 2.0);
      const Real boundary_density = thermal_model.calculateDensityFromEntropyInternalEnergy(
          thermal_model.calculateEntropyFromDensityPressure(
              right_variable.template getScalar<ComputationalVariableEnum::Density>(),
              right_variable.template getScalar<ComputationalVariableEnum::Pressure>()),
          boundary_internal_energy);
      const Real boundary_pressure =
          thermal_model.calculatePressureFormDensityInternalEnergy(boundary_density, boundary_internal_energy);
      boundary_variable.template setScalar<ComputationalVariableEnum::Density>(boundary_density);
      boundary_variable.template setVector<ComputationalVariableEnum::Velocity>(boundary_velocity);
      boundary_variable.template setScalar<ComputationalVariableEnum::InternalEnergy>(boundary_internal_energy);
      boundary_variable.template setScalar<ComputationalVariableEnum::Pressure>(boundary_pressure);
    } else {  // Subsonic outflow
      const Real left_riemann_invariant =
          right_variable.template getVector<ComputationalVariableEnum::Velocity>().transpose() * normal_vector -
          thermal_model.calculateRiemannInvariantPart(
              right_variable.template getScalar<ComputationalVariableEnum::InternalEnergy>());
      const Real right_riemann_invariant =
          left_variable.template getVector<ComputationalVariableEnum::Velocity>().transpose() * normal_vector +
          thermal_model.calculateRiemannInvariantPart(
              left_variable.template getScalar<ComputationalVariableEnum::InternalEnergy>());
      const Real boundary_normal_velocity = (left_riemann_invariant + right_riemann_invariant) / 2.0;
      const Eigen::Vector<Real, SimulationControl::kDimension> boundary_velocity =
          left_variable.template getVector<ComputationalVariableEnum::Velocity>() +
          (boundary_normal_velocity -
           left_variable.template getVector<ComputationalVariableEnum::Velocity>().transpose() * normal_vector) *
              normal_vector;
      const Real boundary_internal_energy = thermal_model.calculateInternalEnergyFromRiemannInvariantPart(
          (right_riemann_invariant - left_riemann_invariant) / 2.0);
      const Real boundary_density = thermal_model.calculateDensityFromEntropyInternalEnergy(
          thermal_model.calculateEntropyFromDensityPressure(
              left_variable.template getScalar<ComputationalVariableEnum::Density>(),
              left_variable.template getScalar<ComputationalVariableEnum::Pressure>()),
          boundary_internal_energy);
      const Real boundary_pressure =
          thermal_model.calculatePressureFormDensityInternalEnergy(boundary_density, boundary_internal_energy);
      boundary_variable.template setScalar<ComputationalVariableEnum::Density>(boundary_density);
      boundary_variable.template setVector<ComputationalVariableEnum::Velocity>(boundary_velocity);
      boundary_variable.template setScalar<ComputationalVariableEnum::InternalEnergy>(boundary_internal_energy);
      boundary_variable.template setScalar<ComputationalVariableEnum::Pressure>(boundary_pressure);
    }
  }
}

template <typename SimulationControl>
struct BoundaryConditionBase {
  Variable<SimulationControl> boundary_dummy_variable_;

  virtual inline void calculateBoundaryVariable(
      const ThermalModel<SimulationControl>& thermal_model,
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl>& left_quadrature_node_variable,
      Variable<SimulationControl>& boundary_quadrature_node_variable) const = 0;

  virtual inline void calculateBoundaryGradientVariable(
      const VariableGradient<SimulationControl>& left_quadrature_node_variable_gradient,
      VariableGradient<SimulationControl>& boundary_quadrature_node_variable_gradient) const = 0;

  virtual inline void calculateBoundaryLeftVariable(
      Variable<SimulationControl>& left_quadrature_node_variable,
      const Variable<SimulationControl>& boundary_quadrature_node_variable) const = 0;

  virtual ~BoundaryConditionBase() = default;
};

template <typename SimulationControl, typename Derived>
struct BoundaryConditionCRTP : BoundaryConditionBase<SimulationControl> {
  inline void calculateBoundaryVariable(const ThermalModel<SimulationControl>& thermal_model,
                                        const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                        const Variable<SimulationControl>& left_quadrature_node_variable,
                                        Variable<SimulationControl>& boundary_quadrature_node_variable) const override {
    static_cast<Derived const*>(this)->calculateBoundaryVariableImpl(
        thermal_model, normal_vector, left_quadrature_node_variable, boundary_quadrature_node_variable);
  }

  inline void calculateBoundaryGradientVariable(
      const VariableGradient<SimulationControl>& left_quadrature_node_variable_gradient,
      VariableGradient<SimulationControl>& boundary_quadrature_node_variable_gradient) const override {
    static_cast<Derived const*>(this)->calculateBoundaryGradientVariableImpl(
        left_quadrature_node_variable_gradient, boundary_quadrature_node_variable_gradient);
  }

  inline void calculateBoundaryLeftVariable(
      Variable<SimulationControl>& left_quadrature_node_variable,
      const Variable<SimulationControl>& boundary_quadrature_node_variable) const override {
    static_cast<Derived const*>(this)->calculateBoundaryLeftVariableImpl(left_quadrature_node_variable,
                                                                         boundary_quadrature_node_variable);
  }
};

template <typename SimulationControl, BoundaryConditionEnum BoundaryConditionType>
struct BoundaryCondition;

template <typename SimulationControl>
struct BoundaryCondition<SimulationControl, BoundaryConditionEnum::RiemannFarfield>
    : BoundaryConditionCRTP<SimulationControl,
                            BoundaryCondition<SimulationControl, BoundaryConditionEnum::RiemannFarfield>> {
  inline void calculateBoundaryVariableImpl(const ThermalModel<SimulationControl>& thermal_model,
                                            const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                            const Variable<SimulationControl>& left_quadrature_node_variable,
                                            Variable<SimulationControl>& boundary_quadrature_node_variable) const {
    calculateRiemannSolver(thermal_model, normal_vector, left_quadrature_node_variable, this->boundary_dummy_variable_,
                           boundary_quadrature_node_variable);
  }

  inline void calculateBoundaryGradientVariableImpl(
      const VariableGradient<SimulationControl>& left_quadrature_node_variable_gradient,
      VariableGradient<SimulationControl>& boundary_quadrature_node_variable_gradient) const {
    boundary_quadrature_node_variable_gradient.primitive_ = left_quadrature_node_variable_gradient.primitive_;
  }

  inline void calculateBoundaryLeftVariableImpl(
      [[maybe_unused]] Variable<SimulationControl>& left_quadrature_node_variable,
      [[maybe_unused]] const Variable<SimulationControl>& boundary_quadrature_node_variable) const {}
};

template <typename SimulationControl>
struct BoundaryCondition<SimulationControl, BoundaryConditionEnum::CharacteristicInflow>
    : BoundaryConditionCRTP<SimulationControl,
                            BoundaryCondition<SimulationControl, BoundaryConditionEnum::CharacteristicInflow>> {
  inline void calculateBoundaryVariableImpl(const ThermalModel<SimulationControl>& thermal_model,
                                            const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                            const Variable<SimulationControl>& left_quadrature_node_variable,
                                            Variable<SimulationControl>& boundary_quadrature_node_variable) const {
    calculateRiemannSolver(thermal_model, normal_vector, left_quadrature_node_variable, this->boundary_dummy_variable_,
                           boundary_quadrature_node_variable);
  }

  inline void calculateBoundaryGradientVariableImpl(
      const VariableGradient<SimulationControl>& left_quadrature_node_variable_gradient,
      VariableGradient<SimulationControl>& boundary_quadrature_node_variable_gradient) const {
    boundary_quadrature_node_variable_gradient.primitive_ = left_quadrature_node_variable_gradient.primitive_;
  }

  inline void calculateBoundaryLeftVariableImpl(
      [[maybe_unused]] Variable<SimulationControl>& left_quadrature_node_variable,
      [[maybe_unused]] const Variable<SimulationControl>& boundary_quadrature_node_variable) const {}
};

template <typename SimulationControl>
struct BoundaryCondition<SimulationControl, BoundaryConditionEnum::PressureOutflow>
    : BoundaryConditionCRTP<SimulationControl,
                            BoundaryCondition<SimulationControl, BoundaryConditionEnum::PressureOutflow>> {
  inline void calculateBoundaryVariableImpl(const ThermalModel<SimulationControl>& thermal_model,
                                            const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                            const Variable<SimulationControl>& left_quadrature_node_variable,
                                            Variable<SimulationControl>& boundary_quadrature_node_variable) const {
    const Real normal_velocity =
        left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>().transpose() *
        normal_vector;
    const Real normal_mach_number =
        normal_velocity /
        thermal_model.calculateSoundSpeedFromInternalEnergy(
            left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::InternalEnergy>());
    boundary_quadrature_node_variable.computational_ = left_quadrature_node_variable.computational_;
    if (normal_mach_number < 1.0) {  // Subsonic outflow
      const Real farfield_internal_energy = thermal_model.calculateInternalEnergyFromDensityPressure(
          left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(),
          this->boundary_dummy_variable_.template getScalar<ComputationalVariableEnum::Pressure>());
      boundary_quadrature_node_variable.template setScalar<ComputationalVariableEnum::InternalEnergy>(
          farfield_internal_energy);
      boundary_quadrature_node_variable.template setScalar<ComputationalVariableEnum::Pressure>(
          this->boundary_dummy_variable_.template getScalar<ComputationalVariableEnum::Pressure>());
    }
  }

  inline void calculateBoundaryGradientVariableImpl(
      const VariableGradient<SimulationControl>& left_quadrature_node_variable_gradient,
      VariableGradient<SimulationControl>& boundary_quadrature_node_variable_gradient) const {
    boundary_quadrature_node_variable_gradient.primitive_ = left_quadrature_node_variable_gradient.primitive_;
  }

  inline void calculateBoundaryLeftVariableImpl(
      [[maybe_unused]] Variable<SimulationControl>& left_quadrature_node_variable,
      [[maybe_unused]] const Variable<SimulationControl>& boundary_quadrature_node_variable) const {}
};

template <typename SimulationControl>
struct BoundaryCondition<SimulationControl, BoundaryConditionEnum::IsothermalNoslipWall>
    : BoundaryConditionCRTP<SimulationControl,
                            BoundaryCondition<SimulationControl, BoundaryConditionEnum::IsothermalNoslipWall>> {
  inline void calculateBoundaryVariableImpl(
      const ThermalModel<SimulationControl>& thermal_model,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl>& left_quadrature_node_variable,
      Variable<SimulationControl>& boundary_quadrature_node_variable) const {
    const Real boundary_density =
        left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>();
    boundary_quadrature_node_variable.template setScalar<ComputationalVariableEnum::Density>(boundary_density);
    boundary_quadrature_node_variable.template setVector<ComputationalVariableEnum::Velocity>(
        Eigen::Vector<Real, SimulationControl::kDimension>::Zero());
    boundary_quadrature_node_variable.template setScalar<ComputationalVariableEnum::InternalEnergy>(
        this->boundary_dummy_variable_.template getScalar<ComputationalVariableEnum::InternalEnergy>());
    const Real boundary_pressure = thermal_model.calculatePressureFormDensityInternalEnergy(
        boundary_density,
        this->boundary_dummy_variable_.template getScalar<ComputationalVariableEnum::InternalEnergy>());
    boundary_quadrature_node_variable.template setScalar<ComputationalVariableEnum::Pressure>(boundary_pressure);
  }

  inline void calculateBoundaryGradientVariableImpl(
      const VariableGradient<SimulationControl>& left_quadrature_node_variable_gradient,
      VariableGradient<SimulationControl>& boundary_quadrature_node_variable_gradient) const {
    boundary_quadrature_node_variable_gradient.primitive_ = left_quadrature_node_variable_gradient.primitive_;
  }

  inline void calculateBoundaryLeftVariableImpl(
      Variable<SimulationControl>& left_quadrature_node_variable,
      const Variable<SimulationControl>& boundary_quadrature_node_variable) const {
    left_quadrature_node_variable.computational_ = boundary_quadrature_node_variable.computational_;
  }
};

template <typename SimulationControl>
struct BoundaryCondition<SimulationControl, BoundaryConditionEnum::AdiabaticSlipWall>
    : BoundaryConditionCRTP<SimulationControl,
                            BoundaryCondition<SimulationControl, BoundaryConditionEnum::AdiabaticSlipWall>> {
  inline void calculateBoundaryVariableImpl([[maybe_unused]] const ThermalModel<SimulationControl>& thermal_model,
                                            const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                            const Variable<SimulationControl>& left_quadrature_node_variable,
                                            Variable<SimulationControl>& boundary_quadrature_node_variable) const {
    boundary_quadrature_node_variable.computational_ = left_quadrature_node_variable.computational_;
    const Eigen::Vector<Real, SimulationControl::kDimension> boundary_velocity =
        left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>() -
        (left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>().transpose() *
         normal_vector) *
            normal_vector;
    boundary_quadrature_node_variable.template setVector<ComputationalVariableEnum::Velocity>(boundary_velocity);
  }

  inline void calculateBoundaryGradientVariableImpl(
      const VariableGradient<SimulationControl>& left_quadrature_node_variable_gradient,
      VariableGradient<SimulationControl>& boundary_quadrature_node_variable_gradient) const {
    boundary_quadrature_node_variable_gradient.primitive_ = left_quadrature_node_variable_gradient.primitive_;
  }

  inline void calculateBoundaryLeftVariableImpl(
      Variable<SimulationControl>& left_quadrature_node_variable,
      const Variable<SimulationControl>& boundary_quadrature_node_variable) const {
    left_quadrature_node_variable.computational_ = boundary_quadrature_node_variable.computational_;
  }
};

template <typename SimulationControl>
struct BoundaryCondition<SimulationControl, BoundaryConditionEnum::AdiabaticNoSlipWall>
    : BoundaryConditionCRTP<SimulationControl,
                            BoundaryCondition<SimulationControl, BoundaryConditionEnum::AdiabaticNoSlipWall>> {
  inline void calculateBoundaryVariableImpl(
      [[maybe_unused]] const ThermalModel<SimulationControl>& thermal_model,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl>& left_quadrature_node_variable,
      Variable<SimulationControl>& boundary_quadrature_node_variable) const {
    boundary_quadrature_node_variable.computational_ = left_quadrature_node_variable.computational_;
    boundary_quadrature_node_variable.template setVector<ComputationalVariableEnum::Velocity>(
        Eigen::Vector<Real, SimulationControl::kDimension>::Zero());
  }

  inline void calculateBoundaryGradientVariableImpl(
      const VariableGradient<SimulationControl>& left_quadrature_node_variable_gradient,
      VariableGradient<SimulationControl>& boundary_quadrature_node_variable_gradient) const {
    boundary_quadrature_node_variable_gradient.primitive_ = left_quadrature_node_variable_gradient.primitive_;
    boundary_quadrature_node_variable_gradient.template setVector<PrimitiveVariableEnum::Temperature>(
        Eigen::Vector<Real, SimulationControl::kDimension>::Zero());
  }

  inline void calculateBoundaryLeftVariableImpl(
      Variable<SimulationControl>& left_quadrature_node_variable,
      const Variable<SimulationControl>& boundary_quadrature_node_variable) const {
    left_quadrature_node_variable.computational_ = boundary_quadrature_node_variable.computational_;
  }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_BOUNDARY_CONDITION_HPP_
