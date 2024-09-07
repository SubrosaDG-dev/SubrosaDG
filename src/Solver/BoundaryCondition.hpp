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
#include <functional>

#include "Solver/PhysicalModel.hpp"
#include "Solver/VariableConvertor.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Enum.hpp"

namespace SubrosaDG {

template <typename SimulationControl>
struct BoundaryConditionBase {
  std::function<Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber>(
      const Eigen::Vector<Real, SimulationControl::kDimension>& coordinate)>
      function_{[]([[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& coordinate) {
        return Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber>::Zero();
      }};

  virtual inline void calculateBoundaryVariable(const PhysicalModel<SimulationControl>& physical_model,
                                                const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                                const Variable<SimulationControl>& left_quadrature_node_variable,
                                                const Variable<SimulationControl>& right_quadrature_node_variable,
                                                Variable<SimulationControl>& boundary_quadrature_node_variable,
                                                Isize column) const = 0;

  virtual inline void calculateBoundaryGradientVariable(
      const PhysicalModel<SimulationControl>& physical_model,
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl>& left_quadrature_node_variable,
      const Variable<SimulationControl>& right_quadrature_node_variable,
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
  inline void calculateBoundaryVariableLocally(const PhysicalModel<SimulationControl>& physical_model,
                                               const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                               const Variable<SimulationControl>& left_quadrature_node_variable,
                                               const Variable<SimulationControl>& right_quadrature_node_variable,
                                               Variable<SimulationControl>& boundary_quadrature_node_variable,
                                               const Isize column) const {
    const Real normal_velocity =
        left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(column).transpose() *
        normal_vector;
    const Real normal_mach_number =
        normal_velocity /
        physical_model.calculateSoundSpeedFromDensityPressure(
            left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(column),
            left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Pressure>(column));
    if (std::fabs(normal_mach_number) > 1.0_r) {
      if (normal_mach_number < 0.0_r) {  // Supersonic inflow
        boundary_quadrature_node_variable.computational_ = right_quadrature_node_variable.computational_.col(column);
      } else {  // Supersonic outflow
        boundary_quadrature_node_variable.computational_ = left_quadrature_node_variable.computational_.col(column);
      }
    } else {
      if (normal_mach_number < 0.0_r) {  // Subsonic inflow
        const Real left_toward_riemann_invariant =
            right_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(column).transpose() *
                normal_vector -
            2.0_r *
                physical_model.calculateSoundSpeedFromDensityPressure(
                    right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(column),
                    right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Pressure>(column)) /
                (physical_model.equation_of_state_.kSpecificHeatRatio - 1.0_r);
        const Real right_toward_riemann_invariant =
            left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(column).transpose() *
                normal_vector +
            2.0_r *
                physical_model.calculateSoundSpeedFromDensityPressure(
                    left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(column),
                    left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Pressure>(column)) /
                (physical_model.equation_of_state_.kSpecificHeatRatio - 1.0_r);
        const Real boundary_normal_velocity = (left_toward_riemann_invariant + right_toward_riemann_invariant) / 2.0_r;
        const Eigen::Vector<Real, SimulationControl::kDimension> boundary_velocity =
            right_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(column) +
            (boundary_normal_velocity -
             right_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(column)
                     .transpose() *
                 normal_vector) *
                normal_vector;
        const Real boundary_sound_speed = (physical_model.equation_of_state_.kSpecificHeatRatio - 1.0_r) *
                                          (right_toward_riemann_invariant - left_toward_riemann_invariant) / 4.0_r;
        const Real boundary_entropy = physical_model.calculateEntropyFromDensityPressure(
            right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(column),
            right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Pressure>(column));
        const Real boundary_density =
            std::pow(boundary_sound_speed * boundary_sound_speed /
                         (physical_model.equation_of_state_.kSpecificHeatRatio * boundary_entropy),
                     1.0_r / (physical_model.equation_of_state_.kSpecificHeatRatio - 1.0_r));
        const Real boundary_pressure = boundary_density * boundary_sound_speed * boundary_sound_speed /
                                       physical_model.equation_of_state_.kSpecificHeatRatio;
        const Real boundary_internal_energy =
            boundary_pressure / ((physical_model.equation_of_state_.kSpecificHeatRatio - 1.0_r) * boundary_density);
        boundary_quadrature_node_variable.template setScalar<ComputationalVariableEnum::Density>(boundary_density, 0);
        boundary_quadrature_node_variable.template setVector<ComputationalVariableEnum::Velocity>(boundary_velocity, 0);
        boundary_quadrature_node_variable.template setScalar<ComputationalVariableEnum::InternalEnergy>(
            boundary_internal_energy, 0);
        boundary_quadrature_node_variable.template setScalar<ComputationalVariableEnum::Pressure>(boundary_pressure, 0);
      } else {  // Subsonic outflow
        const Real left_toward_riemann_invariant =
            right_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(column).transpose() *
                normal_vector -
            2.0 *
                physical_model.calculateSoundSpeedFromDensityPressure(
                    right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(column),
                    right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Pressure>(column)) /
                (physical_model.equation_of_state_.kSpecificHeatRatio - 1.0_r);
        const Real right_toward_riemann_invariant =
            left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(column).transpose() *
                normal_vector +
            2.0 *
                physical_model.calculateSoundSpeedFromDensityPressure(
                    left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(column),
                    left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Pressure>(column)) /
                (physical_model.equation_of_state_.kSpecificHeatRatio - 1.0_r);
        const Real boundary_normal_velocity = (left_toward_riemann_invariant + right_toward_riemann_invariant) / 2.0_r;
        const Eigen::Vector<Real, SimulationControl::kDimension> boundary_velocity =
            left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(column) +
            (boundary_normal_velocity -
             left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(column).transpose() *
                 normal_vector) *
                normal_vector;
        const Real boundary_sound_speed = (physical_model.equation_of_state_.kSpecificHeatRatio - 1.0_r) *
                                          (right_toward_riemann_invariant - left_toward_riemann_invariant) / 4.0_r;
        const Real boundary_entropy = physical_model.calculateEntropyFromDensityPressure(
            left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(column),
            left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Pressure>(column));
        const Real boundary_density =
            std::pow(boundary_sound_speed * boundary_sound_speed /
                         (physical_model.equation_of_state_.kSpecificHeatRatio * boundary_entropy),
                     1.0_r / (physical_model.equation_of_state_.kSpecificHeatRatio - 1.0_r));
        const Real boundary_pressure = boundary_density * boundary_sound_speed * boundary_sound_speed /
                                       physical_model.equation_of_state_.kSpecificHeatRatio;
        const Real boundary_internal_energy =
            boundary_pressure / ((physical_model.equation_of_state_.kSpecificHeatRatio - 1.0_r) * boundary_density);
        boundary_quadrature_node_variable.template setScalar<ComputationalVariableEnum::Density>(boundary_density, 0);
        boundary_quadrature_node_variable.template setVector<ComputationalVariableEnum::Velocity>(boundary_velocity, 0);
        boundary_quadrature_node_variable.template setScalar<ComputationalVariableEnum::InternalEnergy>(
            boundary_internal_energy, 0);
        boundary_quadrature_node_variable.template setScalar<ComputationalVariableEnum::Pressure>(boundary_pressure, 0);
      }
    }
  }

  inline void calculateBoundaryVariable(const PhysicalModel<SimulationControl>& physical_model,
                                        const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                        const Variable<SimulationControl>& left_quadrature_node_variable,
                                        const Variable<SimulationControl>& right_quadrature_node_variable,
                                        Variable<SimulationControl>& boundary_quadrature_node_variable,
                                        const Isize column) const override {
    this->calculateBoundaryVariableLocally(physical_model, normal_vector, left_quadrature_node_variable,
                                           right_quadrature_node_variable, boundary_quadrature_node_variable, column);
  }

  inline void calculateBoundaryGradientVariable(
      [[maybe_unused]] const PhysicalModel<SimulationControl>& physical_model,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl>& left_quadrature_node_variable,
      [[maybe_unused]] const Variable<SimulationControl>& right_quadrature_node_variable,
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
struct BoundaryCondition<SimulationControl, BoundaryConditionEnum::VelocityInflow>
    : BoundaryConditionBase<SimulationControl> {
  inline void calculateBoundaryVariableLocally(const PhysicalModel<SimulationControl>& physical_model,
                                               const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                               const Variable<SimulationControl>& left_quadrature_node_variable,
                                               const Variable<SimulationControl>& right_quadrature_node_variable,
                                               Variable<SimulationControl>& boundary_quadrature_node_variable,
                                               const Isize column) const {
    const Real normal_velocity =
        left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(column).transpose() *
        normal_vector;
    const Real normal_mach_number =
        normal_velocity /
        physical_model.calculateSoundSpeedFromDensityPressure(
            left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(column),
            left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Pressure>(column));
    boundary_quadrature_node_variable.computational_ = right_quadrature_node_variable.computational_.col(column);
    if (normal_mach_number > -1.0_r) {  // Subsonic inflow
      boundary_quadrature_node_variable.template setScalar<ComputationalVariableEnum::Pressure>(
          left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Pressure>(column), 0);
    }
  }

  inline void calculateBoundaryVariable(const PhysicalModel<SimulationControl>& physical_model,
                                        const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                        const Variable<SimulationControl>& left_quadrature_node_variable,
                                        const Variable<SimulationControl>& right_quadrature_node_variable,
                                        Variable<SimulationControl>& boundary_quadrature_node_variable,
                                        const Isize column) const override {
    this->calculateBoundaryVariableLocally(physical_model, normal_vector, left_quadrature_node_variable,
                                           right_quadrature_node_variable, boundary_quadrature_node_variable, column);
  }

  inline void calculateBoundaryGradientVariable(
      [[maybe_unused]] const PhysicalModel<SimulationControl>& physical_model,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl>& left_quadrature_node_variable,
      [[maybe_unused]] const Variable<SimulationControl>& right_quadrature_node_variable,
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
  inline void calculateBoundaryVariableLocally(const PhysicalModel<SimulationControl>& physical_model,
                                               const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                               const Variable<SimulationControl>& left_quadrature_node_variable,
                                               const Variable<SimulationControl>& right_quadrature_node_variable,
                                               Variable<SimulationControl>& boundary_quadrature_node_variable,
                                               const Isize column) const {
    const Real normal_velocity =
        left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(column).transpose() *
        normal_vector;
    const Real normal_mach_number =
        normal_velocity /
        physical_model.calculateSoundSpeedFromDensityPressure(
            left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(column),
            left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Pressure>(column));
    boundary_quadrature_node_variable.computational_ = left_quadrature_node_variable.computational_.col(column);
    if (normal_mach_number < 1.0_r) {  // Subsonic outflow
      boundary_quadrature_node_variable.template setScalar<ComputationalVariableEnum::Pressure>(
          right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Pressure>(column), 0);
    }
  }

  inline void calculateBoundaryVariable(const PhysicalModel<SimulationControl>& physical_model,
                                        const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                        const Variable<SimulationControl>& left_quadrature_node_variable,
                                        const Variable<SimulationControl>& right_quadrature_node_variable,
                                        Variable<SimulationControl>& boundary_quadrature_node_variable,
                                        const Isize column) const override {
    this->calculateBoundaryVariableLocally(physical_model, normal_vector, left_quadrature_node_variable,
                                           right_quadrature_node_variable, boundary_quadrature_node_variable, column);
  }

  inline void calculateBoundaryGradientVariable(
      [[maybe_unused]] const PhysicalModel<SimulationControl>& physical_model,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl>& left_quadrature_node_variable,
      [[maybe_unused]] const Variable<SimulationControl>& right_quadrature_node_variable,
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
struct BoundaryCondition<SimulationControl, BoundaryConditionEnum::IsoThermalNonSlipWall>
    : BoundaryConditionBase<SimulationControl> {
  inline void calculateBoundaryVariableLocally(const PhysicalModel<SimulationControl>& physical_model,
                                               const Variable<SimulationControl>& left_quadrature_node_variable,
                                               const Variable<SimulationControl>& right_quadrature_node_variable,
                                               Variable<SimulationControl>& boundary_quadrature_node_variable,
                                               const Isize column) const {
    const Real boundary_density =
        left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(column);
    boundary_quadrature_node_variable.template setScalar<ComputationalVariableEnum::Density>(boundary_density, 0);
    boundary_quadrature_node_variable.template setVector<ComputationalVariableEnum::Velocity>(
        right_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(column), 0);
    boundary_quadrature_node_variable.template setScalar<ComputationalVariableEnum::InternalEnergy>(
        right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::InternalEnergy>(column), 0);
    const Real boundary_pressure = physical_model.calculatePressureFormDensityInternalEnergy(
        boundary_density,
        right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::InternalEnergy>(column));
    boundary_quadrature_node_variable.template setScalar<ComputationalVariableEnum::Pressure>(boundary_pressure, 0);
  }

  inline void calculateBoundaryVariable(
      const PhysicalModel<SimulationControl>& physical_model,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl>& left_quadrature_node_variable,
      const Variable<SimulationControl>& right_quadrature_node_variable,
      Variable<SimulationControl>& boundary_quadrature_node_variable, const Isize column) const override {
    this->calculateBoundaryVariableLocally(physical_model, left_quadrature_node_variable,
                                           right_quadrature_node_variable, boundary_quadrature_node_variable, column);
  }

  inline void calculateBoundaryGradientVariable(
      const PhysicalModel<SimulationControl>& physical_model,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl>& left_quadrature_node_variable,
      const Variable<SimulationControl>& right_quadrature_node_variable,
      Variable<SimulationControl>& boundary_quadrature_node_volume_gradient_variable,
      Variable<SimulationControl>& boundary_quadrature_node_interface_gradient_variable,
      const Isize column) const override {
    Variable<SimulationControl> boundary_quadrature_node_variable;
    this->calculateBoundaryVariableLocally(physical_model, left_quadrature_node_variable,
                                           right_quadrature_node_variable, boundary_quadrature_node_variable, column);
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

  inline void calculateBoundaryVariable(
      [[maybe_unused]] const PhysicalModel<SimulationControl>& physical_model,
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl>& left_quadrature_node_variable,
      [[maybe_unused]] const Variable<SimulationControl>& right_quadrature_node_variable,
      Variable<SimulationControl>& boundary_quadrature_node_variable, const Isize column) const override {
    this->calculateBoundaryVariableLocally(normal_vector, left_quadrature_node_variable,
                                           boundary_quadrature_node_variable, column);
  }

  inline void calculateBoundaryGradientVariable(
      [[maybe_unused]] const PhysicalModel<SimulationControl>& physical_model,
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl>& left_quadrature_node_variable,
      [[maybe_unused]] const Variable<SimulationControl>& right_quadrature_node_variable,
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
struct BoundaryCondition<SimulationControl, BoundaryConditionEnum::AdiabaticNonSlipWall>
    : BoundaryConditionBase<SimulationControl> {
  inline void calculateBoundaryVariableLocally(const Variable<SimulationControl>& left_quadrature_node_variable,
                                               const Variable<SimulationControl>& right_quadrature_node_variable,
                                               Variable<SimulationControl>& boundary_quadrature_node_variable,
                                               const Isize column) const {
    boundary_quadrature_node_variable.computational_ = left_quadrature_node_variable.computational_.col(column);
    boundary_quadrature_node_variable.template setVector<ComputationalVariableEnum::Velocity>(
        right_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(column), 0);
  }

  inline void calculateBoundaryVariable(
      [[maybe_unused]] const PhysicalModel<SimulationControl>& physical_model,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl>& left_quadrature_node_variable,
      const Variable<SimulationControl>& right_quadrature_node_variable,
      Variable<SimulationControl>& boundary_quadrature_node_variable, const Isize column) const override {
    this->calculateBoundaryVariableLocally(left_quadrature_node_variable, right_quadrature_node_variable,
                                           boundary_quadrature_node_variable, column);
  }

  inline void calculateBoundaryGradientVariable(
      [[maybe_unused]] const PhysicalModel<SimulationControl>& physical_model,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl>& left_quadrature_node_variable,
      const Variable<SimulationControl>& right_quadrature_node_variable,
      Variable<SimulationControl>& boundary_quadrature_node_volume_gradient_variable,
      Variable<SimulationControl>& boundary_quadrature_node_interface_gradient_variable,
      const Isize column) const override {
    Variable<SimulationControl> boundary_quadrature_node_variable;
    this->calculateBoundaryVariableLocally(left_quadrature_node_variable, right_quadrature_node_variable,
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

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_BOUNDARY_CONDITION_HPP_
