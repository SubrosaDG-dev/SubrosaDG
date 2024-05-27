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
                                   const Variable<SimulationControl>& boundary_variable,
                                   Variable<SimulationControl>& riemann_variable, const Isize column) {
  const Real normal_velocity =
      left_variable.template getVector<ComputationalVariableEnum::Velocity>(column).transpose() * normal_vector;
  const Real normal_mach_number =
      normal_velocity / thermal_model.calculateSoundSpeedFromInternalEnergy(
                            left_variable.template getScalar<ComputationalVariableEnum::InternalEnergy>(column));
  if (std::fabs(normal_mach_number) > 1.0) {
    if (normal_mach_number < 0.0) {  // Supersonic inflow
      riemann_variable.computational_ = boundary_variable.computational_;
    } else {  // Supersonic outflow
      riemann_variable.computational_ = left_variable.computational_.col(column);
    }
  } else {
    if (normal_mach_number < 0.0) {  // Subsonic inflow
      const Real left_riemann_invariant =
          boundary_variable.template getVector<ComputationalVariableEnum::Velocity>(0).transpose() * normal_vector -
          thermal_model.calculateRiemannInvariantPart(
              boundary_variable.template getScalar<ComputationalVariableEnum::InternalEnergy>(0));
      const Real right_riemann_invariant =
          left_variable.template getVector<ComputationalVariableEnum::Velocity>(column).transpose() * normal_vector +
          thermal_model.calculateRiemannInvariantPart(
              left_variable.template getScalar<ComputationalVariableEnum::InternalEnergy>(column));
      const Real boundary_normal_velocity = (left_riemann_invariant + right_riemann_invariant) / 2.0;
      const Eigen::Vector<Real, SimulationControl::kDimension> boundary_velocity =
          boundary_variable.template getVector<ComputationalVariableEnum::Velocity>(0) +
          (boundary_normal_velocity -
           boundary_variable.template getVector<ComputationalVariableEnum::Velocity>(0).transpose() * normal_vector) *
              normal_vector;
      const Real boundary_internal_energy = thermal_model.calculateInternalEnergyFromRiemannInvariantPart(
          (right_riemann_invariant - left_riemann_invariant) / 2.0);
      const Real boundary_density = thermal_model.calculateDensityFromEntropyInternalEnergy(
          thermal_model.calculateEntropyFromDensityPressure(
              boundary_variable.template getScalar<ComputationalVariableEnum::Density>(0),
              boundary_variable.template getScalar<ComputationalVariableEnum::Pressure>(0)),
          boundary_internal_energy);
      const Real boundary_pressure =
          thermal_model.calculatePressureFormDensityInternalEnergy(boundary_density, boundary_internal_energy);
      riemann_variable.template setScalar<ComputationalVariableEnum::Density>(boundary_density, 0);
      riemann_variable.template setVector<ComputationalVariableEnum::Velocity>(boundary_velocity, 0);
      riemann_variable.template setScalar<ComputationalVariableEnum::InternalEnergy>(boundary_internal_energy, 0);
      riemann_variable.template setScalar<ComputationalVariableEnum::Pressure>(boundary_pressure, 0);
    } else {  // Subsonic outflow
      const Real left_riemann_invariant =
          boundary_variable.template getVector<ComputationalVariableEnum::Velocity>(0).transpose() * normal_vector -
          thermal_model.calculateRiemannInvariantPart(
              boundary_variable.template getScalar<ComputationalVariableEnum::InternalEnergy>(0));
      const Real right_riemann_invariant =
          left_variable.template getVector<ComputationalVariableEnum::Velocity>(column).transpose() * normal_vector +
          thermal_model.calculateRiemannInvariantPart(
              left_variable.template getScalar<ComputationalVariableEnum::InternalEnergy>(column));
      const Real boundary_normal_velocity = (left_riemann_invariant + right_riemann_invariant) / 2.0;
      const Eigen::Vector<Real, SimulationControl::kDimension> boundary_velocity =
          left_variable.template getVector<ComputationalVariableEnum::Velocity>(column) +
          (boundary_normal_velocity -
           left_variable.template getVector<ComputationalVariableEnum::Velocity>(column).transpose() * normal_vector) *
              normal_vector;
      const Real boundary_internal_energy = thermal_model.calculateInternalEnergyFromRiemannInvariantPart(
          (right_riemann_invariant - left_riemann_invariant) / 2.0);
      const Real boundary_density = thermal_model.calculateDensityFromEntropyInternalEnergy(
          thermal_model.calculateEntropyFromDensityPressure(
              left_variable.template getScalar<ComputationalVariableEnum::Density>(column),
              left_variable.template getScalar<ComputationalVariableEnum::Pressure>(column)),
          boundary_internal_energy);
      const Real boundary_pressure =
          thermal_model.calculatePressureFormDensityInternalEnergy(boundary_density, boundary_internal_energy);
      riemann_variable.template setScalar<ComputationalVariableEnum::Density>(boundary_density, 0);
      riemann_variable.template setVector<ComputationalVariableEnum::Velocity>(boundary_velocity, 0);
      riemann_variable.template setScalar<ComputationalVariableEnum::InternalEnergy>(boundary_internal_energy, 0);
      riemann_variable.template setScalar<ComputationalVariableEnum::Pressure>(boundary_pressure, 0);
    }
  }
}

template <typename SimulationControl>
struct BoundaryConditionBase {
  Variable<SimulationControl> boundary_dummy_variable_;

  virtual inline void calculateBoundaryVariable(const ThermalModel<SimulationControl>& thermal_model,
                                                const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                                const Variable<SimulationControl>& left_quadrature_node_variable,
                                                Variable<SimulationControl>& boundary_quadrature_node_variable,
                                                Isize column) const = 0;

  virtual inline void calculateBoundaryGradientVariable(
      const ThermalModel<SimulationControl>& thermal_model,
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl>& left_quadrature_node_variable,
      Variable<SimulationControl>& boundary_quadrature_node_volume_gradient_variable,
      Variable<SimulationControl>& boundary_quadrature_node_interface_gradient_variable, Isize column) const = 0;

  virtual inline void calculateBoundaryVariableGradient(
      Variable<SimulationControl>& left_quadrature_node_variable,
      const VariableGradient<SimulationControl>& left_quadrature_node_variable_gradient,
      const Variable<SimulationControl>& boundary_quadrature_node_variable,
      VariableGradient<SimulationControl>& boundary_quadrature_node_variable_gradient, Isize column) const = 0;

  virtual ~BoundaryConditionBase() = default;
};

template <typename SimulationControl, BoundaryConditionEnum BoundaryConditionType>
struct BoundaryCondition;

template <typename SimulationControl>
struct BoundaryCondition<SimulationControl, BoundaryConditionEnum::RiemannFarfield>
    : BoundaryConditionBase<SimulationControl> {
  inline void calculateBoundaryVariable(const ThermalModel<SimulationControl>& thermal_model,
                                        const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                        const Variable<SimulationControl>& left_quadrature_node_variable,
                                        Variable<SimulationControl>& boundary_quadrature_node_variable,
                                        const Isize column) const override {
    calculateRiemannSolver(thermal_model, normal_vector, left_quadrature_node_variable, this->boundary_dummy_variable_,
                           boundary_quadrature_node_variable, column);
  }

  inline void calculateBoundaryGradientVariable(
      [[maybe_unused]] const ThermalModel<SimulationControl>& thermal_model,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl>& left_quadrature_node_variable,
      Variable<SimulationControl>& boundary_quadrature_node_volume_gradient_variable,
      Variable<SimulationControl>& boundary_quadrature_node_interface_gradient_variable,
      const Isize column) const override {
    boundary_quadrature_node_volume_gradient_variable.conserved_ = left_quadrature_node_variable.conserved_.col(column);
    boundary_quadrature_node_interface_gradient_variable.conserved_.setZero();
  }

  inline void calculateBoundaryVariableGradient(
      [[maybe_unused]] Variable<SimulationControl>& left_quadrature_node_variable,
      const VariableGradient<SimulationControl>& left_quadrature_node_variable_gradient,
      [[maybe_unused]] const Variable<SimulationControl>& boundary_quadrature_node_variable,
      VariableGradient<SimulationControl>& boundary_quadrature_node_variable_gradient,
      const Isize column) const override {
    boundary_quadrature_node_variable_gradient.primitive_ =
        left_quadrature_node_variable_gradient.primitive_.col(column);
  }
};

template <typename SimulationControl>
struct BoundaryCondition<SimulationControl, BoundaryConditionEnum::CharacteristicInflow>
    : BoundaryConditionBase<SimulationControl> {
  inline void calculateBoundaryVariable(const ThermalModel<SimulationControl>& thermal_model,
                                        const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                        const Variable<SimulationControl>& left_quadrature_node_variable,
                                        Variable<SimulationControl>& boundary_quadrature_node_variable,
                                        const Isize column) const override {
    calculateRiemannSolver(thermal_model, normal_vector, left_quadrature_node_variable, this->boundary_dummy_variable_,
                           boundary_quadrature_node_variable, column);
  }

  inline void calculateBoundaryGradientVariable(
      [[maybe_unused]] const ThermalModel<SimulationControl>& thermal_model,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl>& left_quadrature_node_variable,
      Variable<SimulationControl>& boundary_quadrature_node_volume_gradient_variable,
      Variable<SimulationControl>& boundary_quadrature_node_interface_gradient_variable,
      const Isize column) const override {
    boundary_quadrature_node_volume_gradient_variable.conserved_ = left_quadrature_node_variable.conserved_.col(column);
    boundary_quadrature_node_interface_gradient_variable.conserved_.setZero();
  }

  inline void calculateBoundaryVariableGradient(
      [[maybe_unused]] Variable<SimulationControl>& left_quadrature_node_variable,
      const VariableGradient<SimulationControl>& left_quadrature_node_variable_gradient,
      [[maybe_unused]] const Variable<SimulationControl>& boundary_quadrature_node_variable,
      VariableGradient<SimulationControl>& boundary_quadrature_node_variable_gradient,
      const Isize column) const override {
    boundary_quadrature_node_variable_gradient.primitive_ =
        left_quadrature_node_variable_gradient.primitive_.col(column);
  }
};

template <typename SimulationControl>
struct BoundaryCondition<SimulationControl, BoundaryConditionEnum::PressureOutflow>
    : BoundaryConditionBase<SimulationControl> {
  inline void calculateBoundaryVariableLocally(const ThermalModel<SimulationControl>& thermal_model,
                                               const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                               const Variable<SimulationControl>& left_quadrature_node_variable,
                                               Variable<SimulationControl>& boundary_quadrature_node_variable,
                                               const Isize column) const {
    const Real normal_velocity =
        left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(column).transpose() *
        normal_vector;
    const Real normal_mach_number =
        normal_velocity /
        thermal_model.calculateSoundSpeedFromInternalEnergy(
            left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::InternalEnergy>(column));
    boundary_quadrature_node_variable.computational_ = left_quadrature_node_variable.computational_.col(column);
    if (normal_mach_number < 1.0) {  // Subsonic outflow
      const Real farfield_internal_energy = thermal_model.calculateInternalEnergyFromDensityPressure(
          left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(column),
          this->boundary_dummy_variable_.template getScalar<ComputationalVariableEnum::Pressure>(0));
      boundary_quadrature_node_variable.template setScalar<ComputationalVariableEnum::InternalEnergy>(
          farfield_internal_energy, 0);
      boundary_quadrature_node_variable.template setScalar<ComputationalVariableEnum::Pressure>(
          this->boundary_dummy_variable_.template getScalar<ComputationalVariableEnum::Pressure>(0), 0);
    }
  }

  inline void calculateBoundaryVariable(const ThermalModel<SimulationControl>& thermal_model,
                                        const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                        const Variable<SimulationControl>& left_quadrature_node_variable,
                                        Variable<SimulationControl>& boundary_quadrature_node_variable,
                                        const Isize column) const override {
    this->calculateBoundaryVariableLocally(thermal_model, normal_vector, left_quadrature_node_variable,
                                           boundary_quadrature_node_variable, column);
  }

  inline void calculateBoundaryGradientVariable(
      [[maybe_unused]] const ThermalModel<SimulationControl>& thermal_model,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl>& left_quadrature_node_variable,
      Variable<SimulationControl>& boundary_quadrature_node_volume_gradient_variable,
      Variable<SimulationControl>& boundary_quadrature_node_interface_gradient_variable,
      const Isize column) const override {
    boundary_quadrature_node_volume_gradient_variable.conserved_ = left_quadrature_node_variable.conserved_.col(column);
    boundary_quadrature_node_interface_gradient_variable.conserved_.setZero();
  }

  inline void calculateBoundaryVariableGradient(
      [[maybe_unused]] Variable<SimulationControl>& left_quadrature_node_variable,
      const VariableGradient<SimulationControl>& left_quadrature_node_variable_gradient,
      [[maybe_unused]] const Variable<SimulationControl>& boundary_quadrature_node_variable,
      VariableGradient<SimulationControl>& boundary_quadrature_node_variable_gradient,
      const Isize column) const override {
    boundary_quadrature_node_variable_gradient.primitive_ =
        left_quadrature_node_variable_gradient.primitive_.col(column);
  }
};

template <typename SimulationControl>
struct BoundaryCondition<SimulationControl, BoundaryConditionEnum::IsothermalNoslipWall>
    : BoundaryConditionBase<SimulationControl> {
  inline void calculateBoundaryVariableLocally(const ThermalModel<SimulationControl>& thermal_model,
                                               const Variable<SimulationControl>& left_quadrature_node_variable,
                                               Variable<SimulationControl>& boundary_quadrature_node_variable,
                                               const Isize column) const {
    const Real boundary_density =
        left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(column);
    boundary_quadrature_node_variable.template setScalar<ComputationalVariableEnum::Density>(boundary_density, 0);
    boundary_quadrature_node_variable.template setVector<ComputationalVariableEnum::Velocity>(
        Eigen::Vector<Real, SimulationControl::kDimension>::Zero(), 0);
    boundary_quadrature_node_variable.template setScalar<ComputationalVariableEnum::InternalEnergy>(
        this->boundary_dummy_variable_.template getScalar<ComputationalVariableEnum::InternalEnergy>(0), 0);
    const Real boundary_pressure = thermal_model.calculatePressureFormDensityInternalEnergy(
        boundary_density,
        this->boundary_dummy_variable_.template getScalar<ComputationalVariableEnum::InternalEnergy>(0));
    boundary_quadrature_node_variable.template setScalar<ComputationalVariableEnum::Pressure>(boundary_pressure, 0);
  }

  inline void calculateBoundaryVariable(
      const ThermalModel<SimulationControl>& thermal_model,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl>& left_quadrature_node_variable,
      Variable<SimulationControl>& boundary_quadrature_node_variable, const Isize column) const override {
    this->calculateBoundaryVariableLocally(thermal_model, left_quadrature_node_variable,
                                           boundary_quadrature_node_variable, column);
  }

  inline void calculateBoundaryGradientVariable(
      const ThermalModel<SimulationControl>& thermal_model,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl>& left_quadrature_node_variable,
      Variable<SimulationControl>& boundary_quadrature_node_volume_gradient_variable,
      Variable<SimulationControl>& boundary_quadrature_node_interface_gradient_variable,
      const Isize column) const override {
    Variable<SimulationControl> boundary_quadrature_node_variable;
    this->calculateBoundaryVariableLocally(thermal_model, left_quadrature_node_variable,
                                           boundary_quadrature_node_variable, column);
    boundary_quadrature_node_variable.calculateConservedFromComputational();
    boundary_quadrature_node_volume_gradient_variable.conserved_ = boundary_quadrature_node_variable.conserved_;
    boundary_quadrature_node_interface_gradient_variable.conserved_ =
        boundary_quadrature_node_variable.conserved_ - left_quadrature_node_variable.conserved_.col(column);
  }

  inline void calculateBoundaryVariableGradient(
      Variable<SimulationControl>& left_quadrature_node_variable,
      const VariableGradient<SimulationControl>& left_quadrature_node_variable_gradient,
      const Variable<SimulationControl>& boundary_quadrature_node_variable,
      VariableGradient<SimulationControl>& boundary_quadrature_node_variable_gradient,
      const Isize column) const override {
    left_quadrature_node_variable.computational_.col(column) = boundary_quadrature_node_variable.computational_;
    boundary_quadrature_node_variable_gradient.primitive_ =
        left_quadrature_node_variable_gradient.primitive_.col(column);
  }
};

template <typename SimulationControl>
struct BoundaryCondition<SimulationControl, BoundaryConditionEnum::AdiabaticSlipWall>
    : BoundaryConditionBase<SimulationControl> {
  inline void calculateBoundaryVariableLocally(const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                               const Variable<SimulationControl>& left_quadrature_node_variable,
                                               Variable<SimulationControl>& boundary_quadrature_node_variable,
                                               const Isize column) const {
    boundary_quadrature_node_variable.computational_ = left_quadrature_node_variable.computational_.col(column);
    const Eigen::Vector<Real, SimulationControl::kDimension> boundary_velocity =
        left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(column) -
        (left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(column).transpose() *
         normal_vector) *
            normal_vector;
    boundary_quadrature_node_variable.template setVector<ComputationalVariableEnum::Velocity>(boundary_velocity, 0);
  }

  inline void calculateBoundaryVariable([[maybe_unused]] const ThermalModel<SimulationControl>& thermal_model,
                                        const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                        const Variable<SimulationControl>& left_quadrature_node_variable,
                                        Variable<SimulationControl>& boundary_quadrature_node_variable,
                                        const Isize column) const override {
    this->calculateBoundaryVariableLocally(normal_vector, left_quadrature_node_variable,
                                           boundary_quadrature_node_variable, column);
  }

  inline void calculateBoundaryGradientVariable(
      [[maybe_unused]] const ThermalModel<SimulationControl>& thermal_model,
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl>& left_quadrature_node_variable,
      Variable<SimulationControl>& boundary_quadrature_node_volume_gradient_variable,
      Variable<SimulationControl>& boundary_quadrature_node_interface_gradient_variable,
      const Isize column) const override {
    Variable<SimulationControl> boundary_quadrature_node_variable;
    this->calculateBoundaryVariableLocally(normal_vector, left_quadrature_node_variable,
                                           boundary_quadrature_node_variable, column);
    boundary_quadrature_node_variable.calculateConservedFromComputational();
    boundary_quadrature_node_volume_gradient_variable.conserved_ = boundary_quadrature_node_variable.conserved_;
    boundary_quadrature_node_interface_gradient_variable.conserved_ =
        boundary_quadrature_node_variable.conserved_ - left_quadrature_node_variable.conserved_.col(column);
  }

  inline void calculateBoundaryVariableGradient(
      Variable<SimulationControl>& left_quadrature_node_variable,
      const VariableGradient<SimulationControl>& left_quadrature_node_variable_gradient,
      const Variable<SimulationControl>& boundary_quadrature_node_variable,
      VariableGradient<SimulationControl>& boundary_quadrature_node_variable_gradient,
      const Isize column) const override {
    left_quadrature_node_variable.computational_.col(column) = boundary_quadrature_node_variable.computational_;
    boundary_quadrature_node_variable_gradient.primitive_ =
        left_quadrature_node_variable_gradient.primitive_.col(column);
    boundary_quadrature_node_variable_gradient.template setVector<PrimitiveVariableEnum::Temperature>(
        Eigen::Vector<Real, SimulationControl::kDimension>::Zero(), 0);
  }
};

template <typename SimulationControl>
struct BoundaryCondition<SimulationControl, BoundaryConditionEnum::AdiabaticNoSlipWall>
    : BoundaryConditionBase<SimulationControl> {
  inline void calculateBoundaryVariableLocally(const Variable<SimulationControl>& left_quadrature_node_variable,
                                               Variable<SimulationControl>& boundary_quadrature_node_variable,
                                               const Isize column) const {
    boundary_quadrature_node_variable.computational_ = left_quadrature_node_variable.computational_.col(column);
    boundary_quadrature_node_variable.template setVector<ComputationalVariableEnum::Velocity>(
        Eigen::Vector<Real, SimulationControl::kDimension>::Zero(), 0);
  }

  inline void calculateBoundaryVariable(
      [[maybe_unused]] const ThermalModel<SimulationControl>& thermal_model,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl>& left_quadrature_node_variable,
      Variable<SimulationControl>& boundary_quadrature_node_variable, const Isize column) const override {
    this->calculateBoundaryVariableLocally(left_quadrature_node_variable, boundary_quadrature_node_variable, column);
  }

  inline void calculateBoundaryGradientVariable(
      [[maybe_unused]] const ThermalModel<SimulationControl>& thermal_model,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl>& left_quadrature_node_variable,
      Variable<SimulationControl>& boundary_quadrature_node_volume_gradient_variable,
      Variable<SimulationControl>& boundary_quadrature_node_interface_gradient_variable,
      const Isize column) const override {
    Variable<SimulationControl> boundary_quadrature_node_variable;
    this->calculateBoundaryVariableLocally(left_quadrature_node_variable, boundary_quadrature_node_variable, column);
    boundary_quadrature_node_variable.calculateConservedFromComputational();
    boundary_quadrature_node_volume_gradient_variable.conserved_ = boundary_quadrature_node_variable.conserved_;
    boundary_quadrature_node_interface_gradient_variable.conserved_ =
        boundary_quadrature_node_variable.conserved_ - left_quadrature_node_variable.conserved_.col(column);
  }

  inline void calculateBoundaryVariableGradient(
      Variable<SimulationControl>& left_quadrature_node_variable,
      const VariableGradient<SimulationControl>& left_quadrature_node_variable_gradient,
      const Variable<SimulationControl>& boundary_quadrature_node_variable,
      VariableGradient<SimulationControl>& boundary_quadrature_node_variable_gradient,
      const Isize column) const override {
    left_quadrature_node_variable.computational_.col(column) = boundary_quadrature_node_variable.computational_;
    boundary_quadrature_node_variable_gradient.primitive_ =
        left_quadrature_node_variable_gradient.primitive_.col(column);
    boundary_quadrature_node_variable_gradient.template setVector<PrimitiveVariableEnum::Temperature>(
        Eigen::Vector<Real, SimulationControl::kDimension>::Zero(), 0);
  }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_BOUNDARY_CONDITION_HPP_
