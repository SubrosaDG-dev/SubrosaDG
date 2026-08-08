/**
 * @file BoundaryCondition.cpp
 * @brief The header file of BoundaryCondition.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_BOUNDARY_CONDITION_CPP_
#define SUBROSA_DG_BOUNDARY_CONDITION_CPP_

#include <Eigen/Core>
#include <cmath>

#include "Mesh/ReadControl.cpp"
#include "Solver/PhysicalModel.cpp"
#include "Solver/SolveControl.cpp"
#include "Solver/VariableConvertor.cpp"
#include "Utils/BasicDataType.cpp"
#include "Utils/Concept.cpp"
#include "Utils/Enum.cpp"

namespace SubrosaDG {

template <typename SimulationControl, BoundaryConditionEnum BoundaryConditionType>
struct BoundaryConditionImpl;

template <typename SimulationControl, BoundaryConditionEnum BoundaryConditionType>
struct BoundaryConditionDeviceImpl;

template <typename SimulationControl>
struct BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::RiemannFarfield> {
  static void computeBoundaryVariable(
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          boundary_quadrature_node_computational_variable) {
    const Real left_density = Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
        left_quadrature_node_computational_variable);
    const Real right_density = Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
        right_quadrature_node_computational_variable);
    const Eigen::Ref<const Eigen::Vector<Real, SimulationControl::kDimension>> left_velocity =
        Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
            left_quadrature_node_computational_variable);
    const Eigen::Ref<const Eigen::Vector<Real, SimulationControl::kDimension>> right_velocity =
        Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
            right_quadrature_node_computational_variable);
    const Real left_pressure = Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
        left_quadrature_node_computational_variable);
    [[maybe_unused]] const Real right_pressure =
        Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
            right_quadrature_node_computational_variable);
    const Real left_normal_velocity = left_velocity.transpose() * normal_vector;
    const Real right_normal_velocity = right_velocity.transpose() * normal_vector;
    const Real normal_mach_number =
        left_normal_velocity /
        PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(left_density,
                                                                                                  left_pressure);
    if (std::fabs(normal_mach_number) > 1.0_r) {
      if (normal_mach_number < 0.0_r) {  // Supersonic inflow
        boundary_quadrature_node_computational_variable = right_quadrature_node_computational_variable;
      } else {  // Supersonic outflow
        boundary_quadrature_node_computational_variable = left_quadrature_node_computational_variable;
      }
    } else {
      if (normal_mach_number < 0.0_r) {  // Subsonic inflow
        if constexpr (IsCompressible<SimulationControl::kEquationModel>) {
          const Real specific_heat_ratio = PhysicalModel<SimulationControl, PhysicalModelData>::getSpecificHeatRatio();
          const Real left_toward_riemann_invariant =
              right_normal_velocity -
              2.0_r *
                  PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(
                      right_density, right_pressure) /
                  (specific_heat_ratio - 1.0_r);
          const Real right_toward_riemann_invariant =
              left_normal_velocity +
              2.0_r *
                  PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(
                      left_density, left_pressure) /
                  (specific_heat_ratio - 1.0_r);
          const Real boundary_sound_speed =
              (specific_heat_ratio - 1.0_r) * (right_toward_riemann_invariant - left_toward_riemann_invariant) / 4.0_r;
          const Real boundary_entropy =
              PhysicalModel<SimulationControl, PhysicalModelData>::computeEntropyFromDensityPressure(right_density,
                                                                                                     right_pressure);
          const Real boundary_density =
              std::pow(boundary_sound_speed * boundary_sound_speed / (specific_heat_ratio * boundary_entropy),
                       1.0_r / (specific_heat_ratio - 1.0_r));
          Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
              boundary_quadrature_node_computational_variable) = boundary_density;
          const Real boundary_normal_velocity =
              (left_toward_riemann_invariant + right_toward_riemann_invariant) / 2.0_r;
          Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
              boundary_quadrature_node_computational_variable) =
              right_velocity + (boundary_normal_velocity - right_normal_velocity) * normal_vector;
          const Real boundary_pressure =
              boundary_density * boundary_sound_speed * boundary_sound_speed / specific_heat_ratio;
          Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
              boundary_quadrature_node_computational_variable) =
              boundary_pressure / ((specific_heat_ratio - 1.0_r) * boundary_density);
          Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
              boundary_quadrature_node_computational_variable) = boundary_pressure;
        }
        if constexpr (IsIncompressible<SimulationControl::kEquationModel>) {
          const Real right_internal_energy =
              Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
                  right_quadrature_node_computational_variable);
          const Real sound_speed =
              PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(0.0_r, 0.0_r);
          const Real boundary_density = std::sqrt(
              left_density * right_density * std::exp((left_normal_velocity - right_normal_velocity) / sound_speed));
          Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
              boundary_quadrature_node_computational_variable) = boundary_density;
          const Real boundary_normal_velocity = (left_normal_velocity + right_normal_velocity) / 2.0_r +
                                                std::log(left_density / right_density) * sound_speed / 2.0_r;
          Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
              boundary_quadrature_node_computational_variable) =
              right_velocity + (boundary_normal_velocity - right_normal_velocity) * normal_vector;
          const Real boundary_internal_energy = right_internal_energy;
          Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
              boundary_quadrature_node_computational_variable) = boundary_internal_energy;
          Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
              boundary_quadrature_node_computational_variable) =
              PhysicalModel<SimulationControl, PhysicalModelData>::computePressureFromDensityInternalEnergy(
                  boundary_density, boundary_internal_energy);
        }
      } else {  // Subsonic outflow
        if constexpr (IsCompressible<SimulationControl::kEquationModel>) {
          const Real specific_heat_ratio = PhysicalModel<SimulationControl, PhysicalModelData>::getSpecificHeatRatio();
          const Real left_toward_riemann_invariant =
              right_normal_velocity -
              2.0 *
                  PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(
                      right_density, right_pressure) /
                  (specific_heat_ratio - 1.0_r);
          const Real right_toward_riemann_invariant =
              left_normal_velocity +
              2.0 *
                  PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(
                      left_density, left_pressure) /
                  (specific_heat_ratio - 1.0_r);
          const Real boundary_sound_speed =
              (specific_heat_ratio - 1.0_r) * (right_toward_riemann_invariant - left_toward_riemann_invariant) / 4.0_r;
          const Real boundary_entropy =
              PhysicalModel<SimulationControl, PhysicalModelData>::computeEntropyFromDensityPressure(left_density,
                                                                                                     left_pressure);
          const Real boundary_density =
              std::pow(boundary_sound_speed * boundary_sound_speed / (specific_heat_ratio * boundary_entropy),
                       1.0_r / (specific_heat_ratio - 1.0_r));
          Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
              boundary_quadrature_node_computational_variable) = boundary_density;
          const Real boundary_normal_velocity =
              (left_toward_riemann_invariant + right_toward_riemann_invariant) / 2.0_r;
          Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
              boundary_quadrature_node_computational_variable) =
              left_velocity + (boundary_normal_velocity - left_normal_velocity) * normal_vector;
          const Real boundary_pressure =
              boundary_density * boundary_sound_speed * boundary_sound_speed / specific_heat_ratio;
          Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
              boundary_quadrature_node_computational_variable) =
              boundary_pressure / ((specific_heat_ratio - 1.0_r) * boundary_density);
          Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
              boundary_quadrature_node_computational_variable) = boundary_pressure;
        }
        if constexpr (IsIncompressible<SimulationControl::kEquationModel>) {
          const Real left_internal_energy =
              Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
                  left_quadrature_node_computational_variable);
          const Real sound_speed =
              PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(0.0_r, 0.0_r);
          const Real boundary_density = std::sqrt(
              left_density * right_density * std::exp((left_normal_velocity - right_normal_velocity) / sound_speed));
          Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
              boundary_quadrature_node_computational_variable) = boundary_density;
          const Real boundary_normal_velocity = (left_normal_velocity + right_normal_velocity) / 2.0_r +
                                                std::log(left_density / right_density) * sound_speed / 2.0_r;
          Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
              boundary_quadrature_node_computational_variable) =
              left_velocity + (boundary_normal_velocity - left_normal_velocity) * normal_vector;
          const Real boundary_internal_energy = left_internal_energy;
          Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
              boundary_quadrature_node_computational_variable) = boundary_internal_energy;
          Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
              boundary_quadrature_node_computational_variable) =
              PhysicalModel<SimulationControl, PhysicalModelData>::computePressureFromDensityInternalEnergy(
                  boundary_density, boundary_internal_energy);
        }
      }
    }
  }

  static void modifyBoundaryVariableForViscousFlux(
      [[maybe_unused]] Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      const Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          left_quadrature_node_primitive_variable_gradient,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          boundary_quadrature_node_computational_variable,
      Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          boundary_quadrature_node_primitive_variable_gradient) {
    boundary_quadrature_node_primitive_variable_gradient = left_quadrature_node_primitive_variable_gradient;
  }

  static void computeBoundaryGradientVariable(
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& left_quadrature_node_conserved_variable,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>&
          boundary_quadrature_node_volume_gradient_variable,
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>&
          boundary_quadrature_node_interface_gradient_variable) {
    boundary_quadrature_node_volume_gradient_variable = left_quadrature_node_conserved_variable;
    boundary_quadrature_node_interface_gradient_variable.setZero();
  }
};

template <typename SimulationControl>
struct BoundaryConditionDeviceImpl<SimulationControl, BoundaryConditionEnum::RiemannFarfield> {
  static void computeBoundaryVariable(
      const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector,
      [[maybe_unused]] const Device::StaticVector<Real, SimulationControl::kDimension>&
          quadrature_node_rotation_velocity,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          boundary_quadrature_node_computational_variable) {
    const Real left_density = VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
        left_quadrature_node_computational_variable);
    const Real right_density =
        VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
            right_quadrature_node_computational_variable);
    const Device::View<const Device::StaticVector<Real, SimulationControl::kDimension>> left_velocity =
        VariableDevice<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
            left_quadrature_node_computational_variable);
    const Device::View<const Device::StaticVector<Real, SimulationControl::kDimension>> right_velocity =
        VariableDevice<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
            right_quadrature_node_computational_variable);
    const Real left_pressure =
        VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
            left_quadrature_node_computational_variable);
    [[maybe_unused]] const Real right_pressure =
        VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
            right_quadrature_node_computational_variable);
    Real left_normal_velocity = 0.0_r;
    Real right_normal_velocity = 0.0_r;
    for (Isize m = 0; m < SimulationControl::kDimension; m++) {
      left_normal_velocity += left_velocity(m) * normal_vector(m);
      right_normal_velocity += right_velocity(m) * normal_vector(m);
    }
    const Real normal_mach_number =
        left_normal_velocity /
        PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(left_density,
                                                                                                  left_pressure);
    if (sycl::fabs(normal_mach_number) > 1.0_r) {
      if (normal_mach_number < 0.0_r) {  // Supersonic inflow
        for (Isize m = 0; m < SimulationControl::kComputationalVariableNumber; m++) {
          boundary_quadrature_node_computational_variable(m) = right_quadrature_node_computational_variable(m);
        }
      } else {  // Supersonic outflow
        for (Isize m = 0; m < SimulationControl::kComputationalVariableNumber; m++) {
          boundary_quadrature_node_computational_variable(m) = left_quadrature_node_computational_variable(m);
        }
      }
    } else {
      if (normal_mach_number < 0.0_r) {  // Subsonic inflow
        if constexpr (IsCompressible<SimulationControl::kEquationModel>) {
          const Real specific_heat_ratio = PhysicalModel<SimulationControl, PhysicalModelData>::getSpecificHeatRatio();
          const Real left_toward_riemann_invariant =
              right_normal_velocity -
              2.0_r *
                  PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(
                      right_density, right_pressure) /
                  (specific_heat_ratio - 1.0_r);
          const Real right_toward_riemann_invariant =
              left_normal_velocity +
              2.0_r *
                  PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(
                      left_density, left_pressure) /
                  (specific_heat_ratio - 1.0_r);
          const Real boundary_sound_speed =
              (specific_heat_ratio - 1.0_r) * (right_toward_riemann_invariant - left_toward_riemann_invariant) / 4.0_r;
          const Real boundary_entropy =
              PhysicalModel<SimulationControl, PhysicalModelData>::computeEntropyFromDensityPressure(right_density,
                                                                                                     right_pressure);
          const Real boundary_density =
              sycl::pow(boundary_sound_speed * boundary_sound_speed / (specific_heat_ratio * boundary_entropy),
                        1.0_r / (specific_heat_ratio - 1.0_r));
          VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
              boundary_quadrature_node_computational_variable) = boundary_density;
          const Real boundary_normal_velocity =
              (left_toward_riemann_invariant + right_toward_riemann_invariant) / 2.0_r;
          Device::View<Device::StaticVector<Real, SimulationControl::kDimension>> boundary_velocity =
              VariableDevice<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
                  boundary_quadrature_node_computational_variable);
          for (Isize m = 0; m < SimulationControl::kDimension; m++) {
            boundary_velocity(m) =
                right_velocity(m) + (boundary_normal_velocity - right_normal_velocity) * normal_vector(m);
          }
          const Real boundary_pressure =
              boundary_density * boundary_sound_speed * boundary_sound_speed / specific_heat_ratio;
          VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
              boundary_quadrature_node_computational_variable) =
              boundary_pressure / ((specific_heat_ratio - 1.0_r) * boundary_density);
          VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
              boundary_quadrature_node_computational_variable) = boundary_pressure;
        }
        if constexpr (IsIncompressible<SimulationControl::kEquationModel>) {
          const Real right_internal_energy =
              VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
                  right_quadrature_node_computational_variable);
          const Real sound_speed =
              PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(0.0_r, 0.0_r);
          const Real boundary_density = sycl::sqrt(
              left_density * right_density * sycl::exp((left_normal_velocity - right_normal_velocity) / sound_speed));
          VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
              boundary_quadrature_node_computational_variable) = boundary_density;
          const Real boundary_normal_velocity = (left_normal_velocity + right_normal_velocity) / 2.0_r +
                                                sycl::log(left_density / right_density) * sound_speed / 2.0_r;
          Device::View<Device::StaticVector<Real, SimulationControl::kDimension>> boundary_velocity =
              VariableDevice<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
                  boundary_quadrature_node_computational_variable);
          for (Isize m = 0; m < SimulationControl::kDimension; m++) {
            boundary_velocity(m) =
                right_velocity(m) + (boundary_normal_velocity - right_normal_velocity) * normal_vector(m);
          }
          const Real boundary_internal_energy = right_internal_energy;
          VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
              boundary_quadrature_node_computational_variable) = boundary_internal_energy;
          VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
              boundary_quadrature_node_computational_variable) =
              PhysicalModel<SimulationControl, PhysicalModelData>::computePressureFromDensityInternalEnergy(
                  boundary_density, boundary_internal_energy);
        }
      } else {  // Subsonic outflow
        if constexpr (IsCompressible<SimulationControl::kEquationModel>) {
          const Real specific_heat_ratio = PhysicalModel<SimulationControl, PhysicalModelData>::getSpecificHeatRatio();
          const Real left_toward_riemann_invariant =
              right_normal_velocity -
              2.0_r *
                  PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(
                      right_density, right_pressure) /
                  (specific_heat_ratio - 1.0_r);
          const Real right_toward_riemann_invariant =
              left_normal_velocity +
              2.0_r *
                  PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(
                      left_density, left_pressure) /
                  (specific_heat_ratio - 1.0_r);
          const Real boundary_sound_speed =
              (specific_heat_ratio - 1.0_r) * (right_toward_riemann_invariant - left_toward_riemann_invariant) / 4.0_r;
          const Real boundary_entropy =
              PhysicalModel<SimulationControl, PhysicalModelData>::computeEntropyFromDensityPressure(left_density,
                                                                                                     left_pressure);
          const Real boundary_density =
              sycl::pow(boundary_sound_speed * boundary_sound_speed / (specific_heat_ratio * boundary_entropy),
                        1.0_r / (specific_heat_ratio - 1.0_r));
          VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
              boundary_quadrature_node_computational_variable) = boundary_density;
          const Real boundary_normal_velocity =
              (left_toward_riemann_invariant + right_toward_riemann_invariant) / 2.0_r;
          Device::View<Device::StaticVector<Real, SimulationControl::kDimension>> boundary_velocity =
              VariableDevice<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
                  boundary_quadrature_node_computational_variable);
          for (Isize m = 0; m < SimulationControl::kDimension; m++) {
            boundary_velocity(m) =
                left_velocity(m) + (boundary_normal_velocity - left_normal_velocity) * normal_vector(m);
          }
          const Real boundary_pressure =
              boundary_density * boundary_sound_speed * boundary_sound_speed / specific_heat_ratio;
          VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
              boundary_quadrature_node_computational_variable) =
              boundary_pressure / ((specific_heat_ratio - 1.0_r) * boundary_density);
          VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
              boundary_quadrature_node_computational_variable) = boundary_pressure;
        }
        if constexpr (IsIncompressible<SimulationControl::kEquationModel>) {
          const Real left_internal_energy =
              VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
                  left_quadrature_node_computational_variable);
          const Real sound_speed =
              PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(0.0_r, 0.0_r);
          const Real boundary_density = sycl::sqrt(
              left_density * right_density * sycl::exp((left_normal_velocity - right_normal_velocity) / sound_speed));
          VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
              boundary_quadrature_node_computational_variable) = boundary_density;
          const Real boundary_normal_velocity = (left_normal_velocity + right_normal_velocity) / 2.0_r +
                                                sycl::log(left_density / right_density) * sound_speed / 2.0_r;
          Device::View<Device::StaticVector<Real, SimulationControl::kDimension>> boundary_velocity =
              VariableDevice<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
                  boundary_quadrature_node_computational_variable);
          for (Isize m = 0; m < SimulationControl::kDimension; m++) {
            boundary_velocity(m) =
                left_velocity(m) + (boundary_normal_velocity - left_normal_velocity) * normal_vector(m);
          }
          const Real boundary_internal_energy = left_internal_energy;
          VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
              boundary_quadrature_node_computational_variable) = boundary_internal_energy;
          VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
              boundary_quadrature_node_computational_variable) =
              PhysicalModel<SimulationControl, PhysicalModelData>::computePressureFromDensityInternalEnergy(
                  boundary_density, boundary_internal_energy);
        }
      }
    }
  }

  static void modifyBoundaryVariableForViscousFlux(
      [[maybe_unused]] Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      const Device::StaticVector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          left_quadrature_node_primitive_variable_gradient,
      [[maybe_unused]] const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          boundary_quadrature_node_computational_variable,
      Device::StaticVector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          boundary_quadrature_node_primitive_variable_gradient) {
    for (Isize m = 0; m < SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension; m++) {
      boundary_quadrature_node_primitive_variable_gradient(m) = left_quadrature_node_primitive_variable_gradient(m);
    }
  }

  static void computeBoundaryGradientVariable(
      [[maybe_unused]] const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector,
      [[maybe_unused]] const Device::StaticVector<Real, SimulationControl::kDimension>&
          quadrature_node_rotation_velocity,
      const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          left_quadrature_node_conserved_variable,
      [[maybe_unused]] const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          boundary_quadrature_node_volume_gradient_variable,
      Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          boundary_quadrature_node_interface_gradient_variable) {
    for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
      boundary_quadrature_node_volume_gradient_variable(m) = left_quadrature_node_conserved_variable(m);
      boundary_quadrature_node_interface_gradient_variable(m) = 0.0_r;
    }
  }
};

template <typename SimulationControl>
struct BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::VelocityInflow> {
  static void computeBoundaryVariable(
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          boundary_quadrature_node_computational_variable) {
    const Real normal_velocity = Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
                                     left_quadrature_node_computational_variable)
                                     .transpose() *
                                 normal_vector;
    const Real normal_mach_number =
        normal_velocity / PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(
                              Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
                                  left_quadrature_node_computational_variable),
                              Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
                                  left_quadrature_node_computational_variable));
    boundary_quadrature_node_computational_variable = right_quadrature_node_computational_variable;
    if (normal_mach_number > -1.0_r) {  // Subsonic inflow
      Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
          boundary_quadrature_node_computational_variable) =
          Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
              left_quadrature_node_computational_variable);
    }
  }

  static void modifyBoundaryVariableForViscousFlux(
      [[maybe_unused]] Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      const Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          left_quadrature_node_primitive_variable_gradient,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          boundary_quadrature_node_computational_variable,
      Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          boundary_quadrature_node_primitive_variable_gradient) {
    boundary_quadrature_node_primitive_variable_gradient = left_quadrature_node_primitive_variable_gradient;
  }

  static void computeBoundaryGradientVariable(
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& left_quadrature_node_conserved_variable,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>&
          boundary_quadrature_node_volume_gradient_variable,
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>&
          boundary_quadrature_node_interface_gradient_variable) {
    boundary_quadrature_node_volume_gradient_variable = left_quadrature_node_conserved_variable;
    boundary_quadrature_node_interface_gradient_variable.setZero();
  }
};

template <typename SimulationControl>
struct BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::PressureOutflow> {
  static void computeBoundaryVariable(
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          boundary_quadrature_node_computational_variable) {
    const Real normal_velocity = Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
                                     left_quadrature_node_computational_variable)
                                     .transpose() *
                                 normal_vector;
    const Real normal_mach_number =
        normal_velocity / PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(
                              Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
                                  left_quadrature_node_computational_variable),
                              Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
                                  left_quadrature_node_computational_variable));
    boundary_quadrature_node_computational_variable = left_quadrature_node_computational_variable;
    if (normal_mach_number < 1.0_r) {  // Subsonic outflow
      Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
          boundary_quadrature_node_computational_variable) =
          Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
              right_quadrature_node_computational_variable);
    }
  }

  static void modifyBoundaryVariableForViscousFlux(
      [[maybe_unused]] Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      const Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          left_quadrature_node_primitive_variable_gradient,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          boundary_quadrature_node_computational_variable,
      Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          boundary_quadrature_node_primitive_variable_gradient) {
    boundary_quadrature_node_primitive_variable_gradient = left_quadrature_node_primitive_variable_gradient;
  }

  static void computeBoundaryGradientVariable(
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& left_quadrature_node_conserved_variable,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>&
          boundary_quadrature_node_volume_gradient_variable,
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>&
          boundary_quadrature_node_interface_gradient_variable) {
    boundary_quadrature_node_volume_gradient_variable = left_quadrature_node_conserved_variable;
    boundary_quadrature_node_interface_gradient_variable.setZero();
  }
};

template <typename SimulationControl>
struct BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::AdiabaticSlipWall> {
  static void computeBoundaryVariable(
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Eigen::Vector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          boundary_quadrature_node_computational_variable) {
    boundary_quadrature_node_computational_variable = left_quadrature_node_computational_variable;
    const Eigen::Ref<const Eigen::Vector<Real, SimulationControl::kDimension>> left_velocity =
        Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
            left_quadrature_node_computational_variable);
    const Real left_normal_velocity = left_velocity.transpose() * normal_vector;
    const Real quadrature_node_normal_velocity = quadrature_node_rotation_velocity.transpose() * normal_vector;
    Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
        boundary_quadrature_node_computational_variable) =
        left_velocity - left_normal_velocity * normal_vector + quadrature_node_normal_velocity * normal_vector;
  }

  static void modifyBoundaryVariableForViscousFlux(
      Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>& left_quadrature_node_computational_variable,
      const Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          left_quadrature_node_primitive_variable_gradient,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          boundary_quadrature_node_computational_variable,
      Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          boundary_quadrature_node_primitive_variable_gradient) {
    left_quadrature_node_computational_variable = boundary_quadrature_node_computational_variable;
    boundary_quadrature_node_primitive_variable_gradient = left_quadrature_node_primitive_variable_gradient;
    VariableGradient<SimulationControl>::template getScalarGradient<PrimitiveVariableEnum::Temperature>(
        boundary_quadrature_node_primitive_variable_gradient) =
        Eigen::Vector<Real, SimulationControl::kDimension>::Zero();
  }

  static void computeBoundaryGradientVariable(
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Eigen::Vector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& left_quadrature_node_conserved_variable,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>&
          boundary_quadrature_node_volume_gradient_variable,
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>&
          boundary_quadrature_node_interface_gradient_variable) {
    Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber> left_quadrature_node_computational_variable;
    Variable<SimulationControl>::convertComputationalFromConserved(left_quadrature_node_conserved_variable,
                                                                   left_quadrature_node_computational_variable);
    Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>
        boundary_quadrature_node_computational_variable;
    BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::AdiabaticSlipWall>::computeBoundaryVariable(
        normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_computational_variable,
        right_quadrature_node_computational_variable, boundary_quadrature_node_computational_variable);
    Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> boundary_quadrature_node_conserved_variable;
    Variable<SimulationControl>::convertConservedFromComputational(boundary_quadrature_node_computational_variable,
                                                                   boundary_quadrature_node_conserved_variable);
    boundary_quadrature_node_volume_gradient_variable = boundary_quadrature_node_conserved_variable;
    boundary_quadrature_node_interface_gradient_variable =
        boundary_quadrature_node_conserved_variable - left_quadrature_node_conserved_variable;
  }
};

template <typename SimulationControl>
struct BoundaryConditionDeviceImpl<SimulationControl, BoundaryConditionEnum::AdiabaticSlipWall> {
  static void computeBoundaryVariable(
      const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector,
      const Device::StaticVector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      [[maybe_unused]] const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          boundary_quadrature_node_computational_variable) {
    for (Isize m = 0; m < SimulationControl::kComputationalVariableNumber; m++) {
      boundary_quadrature_node_computational_variable(m) = left_quadrature_node_computational_variable(m);
    }
    const Device::View<const Device::StaticVector<Real, SimulationControl::kDimension>> left_velocity =
        VariableDevice<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
            left_quadrature_node_computational_variable);
    Real left_normal_velocity = 0.0_r;
    Real quadrature_node_normal_velocity = 0.0_r;
    for (Isize m = 0; m < SimulationControl::kDimension; m++) {
      left_normal_velocity += left_velocity(m) * normal_vector(m);
      quadrature_node_normal_velocity += quadrature_node_rotation_velocity(m) * normal_vector(m);
    }
    Device::View<Device::StaticVector<Real, SimulationControl::kDimension>> boundary_velocity =
        VariableDevice<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
            boundary_quadrature_node_computational_variable);
    for (Isize m = 0; m < SimulationControl::kDimension; m++) {
      boundary_velocity(m) = left_velocity(m) - left_normal_velocity * normal_vector(m) +
                             quadrature_node_normal_velocity * normal_vector(m);
    }
  }

  static void modifyBoundaryVariableForViscousFlux(
      Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      const Device::StaticVector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          left_quadrature_node_primitive_variable_gradient,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          boundary_quadrature_node_computational_variable,
      Device::StaticVector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          boundary_quadrature_node_primitive_variable_gradient) {
    for (Isize m = 0; m < SimulationControl::kComputationalVariableNumber; m++) {
      left_quadrature_node_computational_variable(m) = boundary_quadrature_node_computational_variable(m);
    }
    for (Isize m = 0; m < SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension; m++) {
      boundary_quadrature_node_primitive_variable_gradient(m) = left_quadrature_node_primitive_variable_gradient(m);
    }
    Device::View<Device::StaticVector<Real, SimulationControl::kDimension>> boundary_temperature_gradient =
        VariableGradientDevice<SimulationControl>::template getScalarGradient<PrimitiveVariableEnum::Temperature>(
            boundary_quadrature_node_primitive_variable_gradient);
    boundary_temperature_gradient.setZero();
  }

  static void computeBoundaryGradientVariable(
      const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector,
      const Device::StaticVector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          left_quadrature_node_conserved_variable,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          boundary_quadrature_node_volume_gradient_variable,
      Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          boundary_quadrature_node_interface_gradient_variable) {
    Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>
        left_quadrature_node_computational_variable;
    VariableDevice<SimulationControl>::convertComputationalFromConserved(left_quadrature_node_conserved_variable,
                                                                         left_quadrature_node_computational_variable);
    Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>
        boundary_quadrature_node_computational_variable;
    BoundaryConditionDeviceImpl<SimulationControl, BoundaryConditionEnum::AdiabaticSlipWall>::computeBoundaryVariable(
        normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_computational_variable,
        right_quadrature_node_computational_variable, boundary_quadrature_node_computational_variable);
    Device::StaticVector<Real, SimulationControl::kConservedVariableNumber> boundary_quadrature_node_conserved_variable;
    VariableDevice<SimulationControl>::convertConservedFromComputational(
        boundary_quadrature_node_computational_variable, boundary_quadrature_node_conserved_variable);
    for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
      boundary_quadrature_node_volume_gradient_variable(m) = boundary_quadrature_node_conserved_variable(m);
      boundary_quadrature_node_interface_gradient_variable(m) =
          boundary_quadrature_node_conserved_variable(m) - left_quadrature_node_conserved_variable(m);
    }
  }
};

template <typename SimulationControl>
struct BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::IsoThermalNonSlipWall> {
  static void computeBoundaryVariable(
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Eigen::Vector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          boundary_quadrature_node_computational_variable) {
    const Real boundary_density = Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
        left_quadrature_node_computational_variable);
    Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
        boundary_quadrature_node_computational_variable) = boundary_density;
    Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
        boundary_quadrature_node_computational_variable) =
        Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
            right_quadrature_node_computational_variable) +
        quadrature_node_rotation_velocity;
    const Real boundary_internal_energy =
        Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
            right_quadrature_node_computational_variable);
    Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
        boundary_quadrature_node_computational_variable) = boundary_internal_energy;
    Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
        boundary_quadrature_node_computational_variable) =
        PhysicalModel<SimulationControl, PhysicalModelData>::computePressureFromDensityInternalEnergy(
            boundary_density, boundary_internal_energy);
  }

  static void modifyBoundaryVariableForViscousFlux(
      Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>& left_quadrature_node_computational_variable,
      const Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          left_quadrature_node_primitive_variable_gradient,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          boundary_quadrature_node_computational_variable,
      Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          boundary_quadrature_node_primitive_variable_gradient) {
    left_quadrature_node_computational_variable = boundary_quadrature_node_computational_variable;
    boundary_quadrature_node_primitive_variable_gradient = left_quadrature_node_primitive_variable_gradient;
  }

  static void computeBoundaryGradientVariable(
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Eigen::Vector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& left_quadrature_node_conserved_variable,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>&
          boundary_quadrature_node_volume_gradient_variable,
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>&
          boundary_quadrature_node_interface_gradient_variable) {
    Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber> left_quadrature_node_computational_variable;
    Variable<SimulationControl>::convertComputationalFromConserved(left_quadrature_node_conserved_variable,
                                                                   left_quadrature_node_computational_variable);
    Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>
        boundary_quadrature_node_computational_variable;
    BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::IsoThermalNonSlipWall>::computeBoundaryVariable(
        normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_computational_variable,
        right_quadrature_node_computational_variable, boundary_quadrature_node_computational_variable);
    Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> boundary_quadrature_node_conserved_variable;
    Variable<SimulationControl>::convertConservedFromComputational(boundary_quadrature_node_computational_variable,
                                                                   boundary_quadrature_node_conserved_variable);
    boundary_quadrature_node_volume_gradient_variable = boundary_quadrature_node_conserved_variable;
    boundary_quadrature_node_interface_gradient_variable =
        boundary_quadrature_node_conserved_variable - left_quadrature_node_conserved_variable;
  }
};

template <typename SimulationControl>
struct BoundaryConditionDeviceImpl<SimulationControl, BoundaryConditionEnum::IsoThermalNonSlipWall> {
  static void computeBoundaryVariable(
      [[maybe_unused]] const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector,
      const Device::StaticVector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          boundary_quadrature_node_computational_variable) {
    const Real boundary_density =
        VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
            left_quadrature_node_computational_variable);
    VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
        boundary_quadrature_node_computational_variable) = boundary_density;
    const Device::View<const Device::StaticVector<Real, SimulationControl::kDimension>> right_velocity =
        VariableDevice<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
            right_quadrature_node_computational_variable);
    Device::View<Device::StaticVector<Real, SimulationControl::kDimension>> boundary_velocity =
        VariableDevice<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
            boundary_quadrature_node_computational_variable);
    for (Isize m = 0; m < SimulationControl::kDimension; m++) {
      boundary_velocity(m) = right_velocity(m) + quadrature_node_rotation_velocity(m);
    }
    const Real boundary_internal_energy =
        VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
            right_quadrature_node_computational_variable);
    VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
        boundary_quadrature_node_computational_variable) = boundary_internal_energy;
    VariableDevice<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
        boundary_quadrature_node_computational_variable) =
        PhysicalModel<SimulationControl, PhysicalModelData>::computePressureFromDensityInternalEnergy(
            boundary_density, boundary_internal_energy);
  }

  static void modifyBoundaryVariableForViscousFlux(
      Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      const Device::StaticVector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          left_quadrature_node_primitive_variable_gradient,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          boundary_quadrature_node_computational_variable,
      Device::StaticVector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          boundary_quadrature_node_primitive_variable_gradient) {
    for (Isize m = 0; m < SimulationControl::kComputationalVariableNumber; m++) {
      left_quadrature_node_computational_variable(m) = boundary_quadrature_node_computational_variable(m);
    }
    for (Isize m = 0; m < SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension; m++) {
      boundary_quadrature_node_primitive_variable_gradient(m) = left_quadrature_node_primitive_variable_gradient(m);
    }
  }

  static void computeBoundaryGradientVariable(
      const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector,
      const Device::StaticVector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          left_quadrature_node_conserved_variable,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          boundary_quadrature_node_volume_gradient_variable,
      Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          boundary_quadrature_node_interface_gradient_variable) {
    Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>
        left_quadrature_node_computational_variable;
    VariableDevice<SimulationControl>::convertComputationalFromConserved(left_quadrature_node_conserved_variable,
                                                                         left_quadrature_node_computational_variable);
    Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>
        boundary_quadrature_node_computational_variable;
    BoundaryConditionDeviceImpl<SimulationControl, BoundaryConditionEnum::IsoThermalNonSlipWall>::
        computeBoundaryVariable(
            normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_computational_variable,
            right_quadrature_node_computational_variable, boundary_quadrature_node_computational_variable);
    Device::StaticVector<Real, SimulationControl::kConservedVariableNumber> boundary_quadrature_node_conserved_variable;
    VariableDevice<SimulationControl>::convertConservedFromComputational(
        boundary_quadrature_node_computational_variable, boundary_quadrature_node_conserved_variable);
    for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
      boundary_quadrature_node_volume_gradient_variable(m) = boundary_quadrature_node_conserved_variable(m);
      boundary_quadrature_node_interface_gradient_variable(m) =
          boundary_quadrature_node_conserved_variable(m) - left_quadrature_node_conserved_variable(m);
    }
  }
};

template <typename SimulationControl>
struct BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::AdiabaticNonSlipWall> {
  static void computeBoundaryVariable(
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Eigen::Vector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          boundary_quadrature_node_computational_variable) {
    boundary_quadrature_node_computational_variable = left_quadrature_node_computational_variable;
    Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
        boundary_quadrature_node_computational_variable) =
        Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
            right_quadrature_node_computational_variable) +
        quadrature_node_rotation_velocity;
  }

  static void modifyBoundaryVariableForViscousFlux(
      Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>& left_quadrature_node_computational_variable,
      const Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          left_quadrature_node_primitive_variable_gradient,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          boundary_quadrature_node_computational_variable,
      Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          boundary_quadrature_node_primitive_variable_gradient) {
    left_quadrature_node_computational_variable = boundary_quadrature_node_computational_variable;
    boundary_quadrature_node_primitive_variable_gradient = left_quadrature_node_primitive_variable_gradient;
    VariableGradient<SimulationControl>::template getScalarGradient<PrimitiveVariableEnum::Temperature>(
        boundary_quadrature_node_primitive_variable_gradient) =
        Eigen::Vector<Real, SimulationControl::kDimension>::Zero();
  }

  static void computeBoundaryGradientVariable(
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Eigen::Vector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& left_quadrature_node_conserved_variable,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>&
          boundary_quadrature_node_volume_gradient_variable,
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>&
          boundary_quadrature_node_interface_gradient_variable) {
    Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber> left_quadrature_node_computational_variable;
    Variable<SimulationControl>::convertComputationalFromConserved(left_quadrature_node_conserved_variable,
                                                                   left_quadrature_node_computational_variable);
    Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>
        boundary_quadrature_node_computational_variable;
    BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::AdiabaticNonSlipWall>::computeBoundaryVariable(
        normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_computational_variable,
        right_quadrature_node_computational_variable, boundary_quadrature_node_computational_variable);
    Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> boundary_quadrature_node_conserved_variable;
    Variable<SimulationControl>::convertConservedFromComputational(boundary_quadrature_node_computational_variable,
                                                                   boundary_quadrature_node_conserved_variable);
    boundary_quadrature_node_volume_gradient_variable = boundary_quadrature_node_conserved_variable;
    boundary_quadrature_node_interface_gradient_variable =
        boundary_quadrature_node_conserved_variable - left_quadrature_node_conserved_variable;
  }
};

template <typename SimulationControl>
struct BoundaryConditionDeviceImpl<SimulationControl, BoundaryConditionEnum::AdiabaticNonSlipWall> {
  static void computeBoundaryVariable(
      [[maybe_unused]] const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector,
      const Device::StaticVector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          boundary_quadrature_node_computational_variable) {
    for (Isize m = 0; m < SimulationControl::kComputationalVariableNumber; m++) {
      boundary_quadrature_node_computational_variable(m) = left_quadrature_node_computational_variable(m);
    }
    const Device::View<const Device::StaticVector<Real, SimulationControl::kDimension>> right_velocity =
        VariableDevice<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
            right_quadrature_node_computational_variable);
    Device::View<Device::StaticVector<Real, SimulationControl::kDimension>> boundary_velocity =
        VariableDevice<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
            boundary_quadrature_node_computational_variable);
    for (Isize m = 0; m < SimulationControl::kDimension; m++) {
      boundary_velocity(m) = right_velocity(m) + quadrature_node_rotation_velocity(m);
    }
  }

  static void modifyBoundaryVariableForViscousFlux(
      Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      const Device::StaticVector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          left_quadrature_node_primitive_variable_gradient,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          boundary_quadrature_node_computational_variable,
      Device::StaticVector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          boundary_quadrature_node_primitive_variable_gradient) {
    for (Isize m = 0; m < SimulationControl::kComputationalVariableNumber; m++) {
      left_quadrature_node_computational_variable(m) = boundary_quadrature_node_computational_variable(m);
    }
    for (Isize m = 0; m < SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension; m++) {
      boundary_quadrature_node_primitive_variable_gradient(m) = left_quadrature_node_primitive_variable_gradient(m);
    }
    Device::View<Device::StaticVector<Real, SimulationControl::kDimension>> boundary_temperature_gradient =
        VariableGradientDevice<SimulationControl>::template getScalarGradient<PrimitiveVariableEnum::Temperature>(
            boundary_quadrature_node_primitive_variable_gradient);
    boundary_temperature_gradient.setZero();
  }

  static void computeBoundaryGradientVariable(
      const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector,
      const Device::StaticVector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          left_quadrature_node_conserved_variable,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          boundary_quadrature_node_volume_gradient_variable,
      Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          boundary_quadrature_node_interface_gradient_variable) {
    Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>
        left_quadrature_node_computational_variable;
    VariableDevice<SimulationControl>::convertComputationalFromConserved(left_quadrature_node_conserved_variable,
                                                                         left_quadrature_node_computational_variable);
    Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>
        boundary_quadrature_node_computational_variable;
    BoundaryConditionDeviceImpl<SimulationControl, BoundaryConditionEnum::AdiabaticNonSlipWall>::
        computeBoundaryVariable(
            normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_computational_variable,
            right_quadrature_node_computational_variable, boundary_quadrature_node_computational_variable);
    Device::StaticVector<Real, SimulationControl::kConservedVariableNumber> boundary_quadrature_node_conserved_variable;
    VariableDevice<SimulationControl>::convertConservedFromComputational(
        boundary_quadrature_node_computational_variable, boundary_quadrature_node_conserved_variable);
    for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
      boundary_quadrature_node_volume_gradient_variable(m) = boundary_quadrature_node_conserved_variable(m);
      boundary_quadrature_node_interface_gradient_variable(m) =
          boundary_quadrature_node_conserved_variable(m) - left_quadrature_node_conserved_variable(m);
    }
  }
};

template <typename SimulationControl>
struct BoundaryCondition {
  inline static void computePrimitiveFromCoordinate(
      const Eigen::Vector<Real, SimulationControl::kDimension>& coordinate,
      Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber>& boundary_primitive_variable,
      /*const*/ Isize gmsh_physical_index);

  inline static void computePrimitiveFromCoordinate(
      const Eigen::Vector<Real, SimulationControl::kDimension>& coordinate,
      Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber>& boundary_primitive_variable,
      /*const*/ Real time,
      /*const*/ Isize gmsh_physical_index);

  static void computeBoundaryVariable(
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Eigen::Vector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          boundary_quadrature_node_computational_variable,
      const BoundaryConditionEnum boundary_condition_type) {
    switch (boundary_condition_type) {
    case BoundaryConditionEnum::RiemannFarfield:
      BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::RiemannFarfield>::computeBoundaryVariable(
          normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_computational_variable,
          right_quadrature_node_computational_variable, boundary_quadrature_node_computational_variable);
      break;
    case BoundaryConditionEnum::VelocityInflow:
      BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::VelocityInflow>::computeBoundaryVariable(
          normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_computational_variable,
          right_quadrature_node_computational_variable, boundary_quadrature_node_computational_variable);
      break;
    case BoundaryConditionEnum::PressureOutflow:
      BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::PressureOutflow>::computeBoundaryVariable(
          normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_computational_variable,
          right_quadrature_node_computational_variable, boundary_quadrature_node_computational_variable);
      break;
    case BoundaryConditionEnum::AdiabaticSlipWall:
      BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::AdiabaticSlipWall>::computeBoundaryVariable(
          normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_computational_variable,
          right_quadrature_node_computational_variable, boundary_quadrature_node_computational_variable);
      break;
    case BoundaryConditionEnum::IsoThermalNonSlipWall:
      BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::IsoThermalNonSlipWall>::computeBoundaryVariable(
          normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_computational_variable,
          right_quadrature_node_computational_variable, boundary_quadrature_node_computational_variable);
      break;
    case BoundaryConditionEnum::AdiabaticNonSlipWall:
      BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::AdiabaticNonSlipWall>::computeBoundaryVariable(
          normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_computational_variable,
          right_quadrature_node_computational_variable, boundary_quadrature_node_computational_variable);
      break;
    default:
      break;
    }
  }

  static void modifyBoundaryVariableForViscousFlux(
      Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>& left_quadrature_node_computational_variable,
      const Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          left_quadrature_node_primitive_variable_gradient,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          boundary_quadrature_node_computational_variable,
      Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          boundary_quadrature_node_primitive_variable_gradient,
      const BoundaryConditionEnum boundary_condition_type) {
    switch (boundary_condition_type) {
    case BoundaryConditionEnum::RiemannFarfield:
      BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::RiemannFarfield>::
          modifyBoundaryVariableForViscousFlux(
              left_quadrature_node_computational_variable, left_quadrature_node_primitive_variable_gradient,
              boundary_quadrature_node_computational_variable, boundary_quadrature_node_primitive_variable_gradient);
      break;
    case BoundaryConditionEnum::VelocityInflow:
      BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::VelocityInflow>::
          modifyBoundaryVariableForViscousFlux(
              left_quadrature_node_computational_variable, left_quadrature_node_primitive_variable_gradient,
              boundary_quadrature_node_computational_variable, boundary_quadrature_node_primitive_variable_gradient);
      break;
    case BoundaryConditionEnum::PressureOutflow:
      BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::PressureOutflow>::
          modifyBoundaryVariableForViscousFlux(
              left_quadrature_node_computational_variable, left_quadrature_node_primitive_variable_gradient,
              boundary_quadrature_node_computational_variable, boundary_quadrature_node_primitive_variable_gradient);
      break;
    case BoundaryConditionEnum::AdiabaticSlipWall:
      BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::AdiabaticSlipWall>::
          modifyBoundaryVariableForViscousFlux(
              left_quadrature_node_computational_variable, left_quadrature_node_primitive_variable_gradient,
              boundary_quadrature_node_computational_variable, boundary_quadrature_node_primitive_variable_gradient);
      break;
    case BoundaryConditionEnum::IsoThermalNonSlipWall:
      BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::IsoThermalNonSlipWall>::
          modifyBoundaryVariableForViscousFlux(
              left_quadrature_node_computational_variable, left_quadrature_node_primitive_variable_gradient,
              boundary_quadrature_node_computational_variable, boundary_quadrature_node_primitive_variable_gradient);
      break;
    case BoundaryConditionEnum::AdiabaticNonSlipWall:
      BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::AdiabaticNonSlipWall>::
          modifyBoundaryVariableForViscousFlux(
              left_quadrature_node_computational_variable, left_quadrature_node_primitive_variable_gradient,
              boundary_quadrature_node_computational_variable, boundary_quadrature_node_primitive_variable_gradient);
      break;
    default:
      break;
    }
  }

  static void computeBoundaryGradientVariable(
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Eigen::Vector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& left_quadrature_node_conserved_variable,
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>&
          boundary_quadrature_node_volume_gradient_variable,
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>&
          boundary_quadrature_node_interface_gradient_variable,
      const BoundaryConditionEnum boundary_condition_type) {
    switch (boundary_condition_type) {
    case BoundaryConditionEnum::RiemannFarfield:
      BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::RiemannFarfield>::computeBoundaryGradientVariable(
          normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_conserved_variable,
          right_quadrature_node_computational_variable, boundary_quadrature_node_volume_gradient_variable,
          boundary_quadrature_node_interface_gradient_variable);
      break;
    case BoundaryConditionEnum::VelocityInflow:
      BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::VelocityInflow>::computeBoundaryGradientVariable(
          normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_conserved_variable,
          right_quadrature_node_computational_variable, boundary_quadrature_node_volume_gradient_variable,
          boundary_quadrature_node_interface_gradient_variable);
      break;
    case BoundaryConditionEnum::PressureOutflow:
      BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::PressureOutflow>::computeBoundaryGradientVariable(
          normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_conserved_variable,
          right_quadrature_node_computational_variable, boundary_quadrature_node_volume_gradient_variable,
          boundary_quadrature_node_interface_gradient_variable);
      break;
    case BoundaryConditionEnum::AdiabaticSlipWall:
      BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::AdiabaticSlipWall>::
          computeBoundaryGradientVariable(
              normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_conserved_variable,
              right_quadrature_node_computational_variable, boundary_quadrature_node_volume_gradient_variable,
              boundary_quadrature_node_interface_gradient_variable);
      break;
    case BoundaryConditionEnum::IsoThermalNonSlipWall:
      BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::IsoThermalNonSlipWall>::
          computeBoundaryGradientVariable(
              normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_conserved_variable,
              right_quadrature_node_computational_variable, boundary_quadrature_node_volume_gradient_variable,
              boundary_quadrature_node_interface_gradient_variable);
      break;
    case BoundaryConditionEnum::AdiabaticNonSlipWall:
      BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::AdiabaticNonSlipWall>::
          computeBoundaryGradientVariable(
              normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_conserved_variable,
              right_quadrature_node_computational_variable, boundary_quadrature_node_volume_gradient_variable,
              boundary_quadrature_node_interface_gradient_variable);
      break;
    default:
      break;
    }
  }
};

template <typename SimulationControl>
struct BoundaryConditionDevice {
  static void computeBoundaryVariable(
      const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector,
      const Device::StaticVector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          boundary_quadrature_node_computational_variable,
      const BoundaryConditionEnum boundary_condition_type) {
    switch (boundary_condition_type) {
    case BoundaryConditionEnum::RiemannFarfield:
      BoundaryConditionDeviceImpl<SimulationControl, BoundaryConditionEnum::RiemannFarfield>::computeBoundaryVariable(
          normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_computational_variable,
          right_quadrature_node_computational_variable, boundary_quadrature_node_computational_variable);
      break;
    case BoundaryConditionEnum::AdiabaticSlipWall:
      BoundaryConditionDeviceImpl<SimulationControl, BoundaryConditionEnum::AdiabaticSlipWall>::computeBoundaryVariable(
          normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_computational_variable,
          right_quadrature_node_computational_variable, boundary_quadrature_node_computational_variable);
      break;
    case BoundaryConditionEnum::IsoThermalNonSlipWall:
      BoundaryConditionDeviceImpl<SimulationControl, BoundaryConditionEnum::IsoThermalNonSlipWall>::
          computeBoundaryVariable(
              normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_computational_variable,
              right_quadrature_node_computational_variable, boundary_quadrature_node_computational_variable);
      break;
    case BoundaryConditionEnum::AdiabaticNonSlipWall:
      BoundaryConditionDeviceImpl<SimulationControl, BoundaryConditionEnum::AdiabaticNonSlipWall>::
          computeBoundaryVariable(
              normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_computational_variable,
              right_quadrature_node_computational_variable, boundary_quadrature_node_computational_variable);
      break;
    default:
      break;
    }
  }

  static void modifyBoundaryVariableForViscousFlux(
      Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          left_quadrature_node_computational_variable,
      const Device::StaticVector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          left_quadrature_node_primitive_variable_gradient,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          boundary_quadrature_node_computational_variable,
      Device::StaticVector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
          boundary_quadrature_node_primitive_variable_gradient,
      const BoundaryConditionEnum boundary_condition_type) {
    switch (boundary_condition_type) {
    case BoundaryConditionEnum::RiemannFarfield:
      BoundaryConditionDeviceImpl<SimulationControl, BoundaryConditionEnum::RiemannFarfield>::
          modifyBoundaryVariableForViscousFlux(
              left_quadrature_node_computational_variable, left_quadrature_node_primitive_variable_gradient,
              boundary_quadrature_node_computational_variable, boundary_quadrature_node_primitive_variable_gradient);
      break;
    case BoundaryConditionEnum::AdiabaticSlipWall:
      BoundaryConditionDeviceImpl<SimulationControl, BoundaryConditionEnum::AdiabaticSlipWall>::
          modifyBoundaryVariableForViscousFlux(
              left_quadrature_node_computational_variable, left_quadrature_node_primitive_variable_gradient,
              boundary_quadrature_node_computational_variable, boundary_quadrature_node_primitive_variable_gradient);
      break;
    case BoundaryConditionEnum::IsoThermalNonSlipWall:
      BoundaryConditionDeviceImpl<SimulationControl, BoundaryConditionEnum::IsoThermalNonSlipWall>::
          modifyBoundaryVariableForViscousFlux(
              left_quadrature_node_computational_variable, left_quadrature_node_primitive_variable_gradient,
              boundary_quadrature_node_computational_variable, boundary_quadrature_node_primitive_variable_gradient);
      break;
    case BoundaryConditionEnum::AdiabaticNonSlipWall:
      BoundaryConditionDeviceImpl<SimulationControl, BoundaryConditionEnum::AdiabaticNonSlipWall>::
          modifyBoundaryVariableForViscousFlux(
              left_quadrature_node_computational_variable, left_quadrature_node_primitive_variable_gradient,
              boundary_quadrature_node_computational_variable, boundary_quadrature_node_primitive_variable_gradient);
      break;
    default:
      break;
    }
  }

  static void computeBoundaryGradientVariable(
      const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector,
      const Device::StaticVector<Real, SimulationControl::kDimension>& quadrature_node_rotation_velocity,
      const Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          left_quadrature_node_conserved_variable,
      const Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>&
          right_quadrature_node_computational_variable,
      Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          boundary_quadrature_node_volume_gradient_variable,
      Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>&
          boundary_quadrature_node_interface_gradient_variable,
      const BoundaryConditionEnum boundary_condition_type) {
    switch (boundary_condition_type) {
    case BoundaryConditionEnum::RiemannFarfield:
      BoundaryConditionDeviceImpl<SimulationControl, BoundaryConditionEnum::RiemannFarfield>::
          computeBoundaryGradientVariable(
              normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_conserved_variable,
              right_quadrature_node_computational_variable, boundary_quadrature_node_volume_gradient_variable,
              boundary_quadrature_node_interface_gradient_variable);
      break;
    case BoundaryConditionEnum::AdiabaticSlipWall:
      BoundaryConditionDeviceImpl<SimulationControl, BoundaryConditionEnum::AdiabaticSlipWall>::
          computeBoundaryGradientVariable(
              normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_conserved_variable,
              right_quadrature_node_computational_variable, boundary_quadrature_node_volume_gradient_variable,
              boundary_quadrature_node_interface_gradient_variable);
      break;
    case BoundaryConditionEnum::IsoThermalNonSlipWall:
      BoundaryConditionDeviceImpl<SimulationControl, BoundaryConditionEnum::IsoThermalNonSlipWall>::
          computeBoundaryGradientVariable(
              normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_conserved_variable,
              right_quadrature_node_computational_variable, boundary_quadrature_node_volume_gradient_variable,
              boundary_quadrature_node_interface_gradient_variable);
      break;
    case BoundaryConditionEnum::AdiabaticNonSlipWall:
      BoundaryConditionDeviceImpl<SimulationControl, BoundaryConditionEnum::AdiabaticNonSlipWall>::
          computeBoundaryGradientVariable(
              normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_conserved_variable,
              right_quadrature_node_computational_variable, boundary_quadrature_node_volume_gradient_variable,
              boundary_quadrature_node_interface_gradient_variable);
      break;
    default:
      break;
    }
  }
};

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl>::updateAdjacencyElementBoundaryVariable(
    const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
    const TimeIntegration<SimulationControl>& time_integration, const int rk_step) {
  tbb::parallel_for(
      tbb::blocked_range<Isize>(0, this->boundary_number_), [&](const tbb::blocked_range<Isize>& range) -> void {
        for (Isize i = range.begin(); i != range.end(); i++) {
          const Isize element_index = i + this->interior_number_;
          const Isize gmsh_physical_index = adjacency_element_mesh.gmsh_physical_index_(element_index);
          Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber> boundary_primitive_variable;
          Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber> boundary_computational_variable;
          for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
            BoundaryCondition<SimulationControl>::computePrimitiveFromCoordinate(
                adjacency_element_mesh.quadrature_node_coordinate_.col(j), boundary_primitive_variable,
                (static_cast<Real>(time_integration.iteration_) +
                 TimeIntegration<SimulationControl>::kButcherCoefficients[static_cast<Usize>(rk_step)]) *
                    time_integration.delta_time_,
                gmsh_physical_index);
            Variable<SimulationControl>::convertComputationalFromPrimitive(boundary_primitive_variable,
                                                                           boundary_computational_variable);
            this->boundary_dummy_right_computational_variable_(i).col(j) = boundary_computational_variable;
          }
        }
      });
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::updateBoundaryVariable(
    const Mesh<SimulationControl>& mesh, const TimeIntegration<SimulationControl>& time_integration,
    const int rk_step) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->point_.updateAdjacencyElementBoundaryVariable(mesh.point_, time_integration, rk_step);
  } else if constexpr (SimulationControl::kDimension == 2) {
    this->line_.updateAdjacencyElementBoundaryVariable(mesh.line_, time_integration, rk_step);
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasAdjacencyTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.updateAdjacencyElementBoundaryVariable(mesh.triangle_, time_integration, rk_step);
    }
    if constexpr (HasAdjacencyQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.updateAdjacencyElementBoundaryVariable(mesh.quadrangle_, time_integration, rk_step);
    }
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_BOUNDARY_CONDITION_CPP_
