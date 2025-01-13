/**
 * @file BoundaryCondition.cpp
 * @brief The header file of BoundaryCondition.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2025 by SubrosaDG developers. All rights reserved.
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

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl>::updateAdjacencyElementBoundaryVariable(
    const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
    const PhysicalModel<SimulationControl>& physical_model,
    const BoundaryCondition<SimulationControl>& boundary_condition,
    const TimeIntegration<SimulationControl>& time_integration) {
  tbb::parallel_for(
      tbb::blocked_range<Isize>(0, adjacency_element_mesh.boundary_number_),
      [&](const tbb::blocked_range<Isize>& range) {
        for (Isize i = range.begin(); i != range.end(); i++) {
          const Isize gmsh_physical_index =
              adjacency_element_mesh.element_(i + adjacency_element_mesh.interior_number_).gmsh_physical_index_;
          for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
            this->boundary_dummy_variable_(i).primitive_.col(j) = boundary_condition.calculatePrimitiveFromCoordinate(
                adjacency_element_mesh.element_(i + adjacency_element_mesh.interior_number_)
                    .quadrature_node_coordinate_.col(j),
                time_integration.iteration_ * time_integration.delta_time_, gmsh_physical_index);
          }
          this->boundary_dummy_variable_(i).calculateConservedFromPrimitive(physical_model);
          this->boundary_dummy_variable_(i).calculateComputationalFromPrimitive(physical_model);
        }
      });
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::updateBoundaryVariable(
    const Mesh<SimulationControl>& mesh, const PhysicalModel<SimulationControl>& physical_model,
    const BoundaryCondition<SimulationControl>& boundary_condition,
    const TimeIntegration<SimulationControl>& time_integration) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->point_.updateAdjacencyElementBoundaryVariable(mesh.point_, physical_model, boundary_condition,
                                                        time_integration);
  } else if constexpr (SimulationControl::kDimension == 2) {
    this->line_.updateAdjacencyElementBoundaryVariable(mesh.line_, physical_model, boundary_condition,
                                                       time_integration);
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasAdjacencyTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.updateAdjacencyElementBoundaryVariable(mesh.triangle_, physical_model, boundary_condition,
                                                             time_integration);
    }
    if constexpr (HasAdjacencyQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.updateAdjacencyElementBoundaryVariable(mesh.quadrangle_, physical_model, boundary_condition,
                                                               time_integration);
    }
  }
}

template <typename SimulationControl, BoundaryConditionEnum BoundaryConditionType>
struct BoundaryConditionImpl;

template <typename SimulationControl>
struct BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::RiemannFarfield> {
  template <int N>
  inline static void calculateBoundaryVariable(const PhysicalModel<SimulationControl>& physical_model,
                                               const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                               const Variable<SimulationControl, N>& left_quadrature_node_variable,
                                               const Variable<SimulationControl, N>& right_quadrature_node_variable,
                                               Variable<SimulationControl, 1>& boundary_quadrature_node_variable,
                                               const Isize column) {
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
        if constexpr (SimulationControl::kEquationModel == EquationModelEnum::CompresibleEuler ||
                      SimulationControl::kEquationModel == EquationModelEnum::CompresibleNS) {
          const Real left_toward_riemann_invariant =
              right_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(column)
                      .transpose() *
                  normal_vector -
              2.0_r *
                  physical_model.calculateSoundSpeedFromDensityPressure(
                      right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(column),
                      right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Pressure>(column)) /
                  (physical_model.equation_of_state_.kSpecificHeatRatio - 1.0_r);
          const Real right_toward_riemann_invariant =
              left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(column)
                      .transpose() *
                  normal_vector +
              2.0_r *
                  physical_model.calculateSoundSpeedFromDensityPressure(
                      left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(column),
                      left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Pressure>(column)) /
                  (physical_model.equation_of_state_.kSpecificHeatRatio - 1.0_r);
          const Real boundary_normal_velocity =
              (left_toward_riemann_invariant + right_toward_riemann_invariant) / 2.0_r;
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
          boundary_quadrature_node_variable.template setVector<ComputationalVariableEnum::Velocity>(boundary_velocity,
                                                                                                    0);
          boundary_quadrature_node_variable.template setScalar<ComputationalVariableEnum::InternalEnergy>(
              boundary_internal_energy, 0);
          boundary_quadrature_node_variable.template setScalar<ComputationalVariableEnum::Pressure>(boundary_pressure,
                                                                                                    0);
        }
        if constexpr (SimulationControl::kEquationModel == EquationModelEnum::IncompresibleEuler ||
                      SimulationControl::kEquationModel == EquationModelEnum::IncompresibleNS) {
          const Real sound_speed = physical_model.calculateSoundSpeedFromDensityPressure(0.0_r, 0.0_r);
          const Real left_normal_velocity =
              left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(column)
                  .transpose() *
              normal_vector;
          const Real right_normal_velocity =
              right_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(column)
                  .transpose() *
              normal_vector;
          const Real boundary_density =
              std::sqrt(left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(column) *
                        right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(column) *
                        std::exp((left_normal_velocity - right_normal_velocity) / sound_speed));
          const Real boundary_normal_velocity =
              (left_normal_velocity + right_normal_velocity) / 2.0_r +
              std::log(left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(column) /
                       right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(column)) *
                  sound_speed / 2.0_r;
          const Real boundary_internal_energy =
              right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::InternalEnergy>(column) *
              right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(column) /
              boundary_density;
          const Eigen::Vector<Real, SimulationControl::kDimension> boundary_velocity =
              right_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(column) +
              (boundary_normal_velocity -
               right_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(column)
                       .transpose() *
                   normal_vector) *
                  normal_vector;
          const Real boundary_pressure =
              physical_model.calculatePressureFormDensityInternalEnergy(boundary_density, boundary_internal_energy);
          boundary_quadrature_node_variable.template setScalar<ComputationalVariableEnum::Density>(boundary_density, 0);
          boundary_quadrature_node_variable.template setVector<ComputationalVariableEnum::Velocity>(boundary_velocity,
                                                                                                    0);
          boundary_quadrature_node_variable.template setScalar<ComputationalVariableEnum::InternalEnergy>(
              boundary_internal_energy, 0);
          boundary_quadrature_node_variable.template setScalar<ComputationalVariableEnum::Pressure>(boundary_pressure,
                                                                                                    0);
        }
      } else {  // Subsonic outflow
        if constexpr (SimulationControl::kEquationModel == EquationModelEnum::CompresibleEuler ||
                      SimulationControl::kEquationModel == EquationModelEnum::CompresibleNS) {
          const Real left_toward_riemann_invariant =
              right_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(column)
                      .transpose() *
                  normal_vector -
              2.0 *
                  physical_model.calculateSoundSpeedFromDensityPressure(
                      right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(column),
                      right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Pressure>(column)) /
                  (physical_model.equation_of_state_.kSpecificHeatRatio - 1.0_r);
          const Real right_toward_riemann_invariant =
              left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(column)
                      .transpose() *
                  normal_vector +
              2.0 *
                  physical_model.calculateSoundSpeedFromDensityPressure(
                      left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(column),
                      left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Pressure>(column)) /
                  (physical_model.equation_of_state_.kSpecificHeatRatio - 1.0_r);
          const Real boundary_normal_velocity =
              (left_toward_riemann_invariant + right_toward_riemann_invariant) / 2.0_r;
          const Eigen::Vector<Real, SimulationControl::kDimension> boundary_velocity =
              left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(column) +
              (boundary_normal_velocity -
               left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(column)
                       .transpose() *
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
          boundary_quadrature_node_variable.template setVector<ComputationalVariableEnum::Velocity>(boundary_velocity,
                                                                                                    0);
          boundary_quadrature_node_variable.template setScalar<ComputationalVariableEnum::InternalEnergy>(
              boundary_internal_energy, 0);
          boundary_quadrature_node_variable.template setScalar<ComputationalVariableEnum::Pressure>(boundary_pressure,
                                                                                                    0);
        }
        if constexpr (SimulationControl::kEquationModel == EquationModelEnum::IncompresibleEuler ||
                      SimulationControl::kEquationModel == EquationModelEnum::IncompresibleNS) {
          const Real sound_speed = physical_model.calculateSoundSpeedFromDensityPressure(0.0_r, 0.0_r);
          const Real left_normal_velocity =
              left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(column)
                  .transpose() *
              normal_vector;
          const Real right_normal_velocity =
              right_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(column)
                  .transpose() *
              normal_vector;
          const Real boundary_density =
              std::sqrt(left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(column) *
                        right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(column) *
                        std::exp((left_normal_velocity - right_normal_velocity) / sound_speed));
          const Real boundary_normal_velocity =
              (left_normal_velocity + right_normal_velocity) / 2.0_r +
              std::log(left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(column) /
                       right_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(column)) *
                  sound_speed / 2.0_r;
          const Real boundary_internal_energy =
              left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::InternalEnergy>(column) *
              left_quadrature_node_variable.template getScalar<ComputationalVariableEnum::Density>(column) /
              boundary_density;
          const Eigen::Vector<Real, SimulationControl::kDimension> boundary_velocity =
              left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(column) +
              (boundary_normal_velocity -
               left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(column)
                       .transpose() *
                   normal_vector) *
                  normal_vector;
          const Real boundary_pressure =
              physical_model.calculatePressureFormDensityInternalEnergy(boundary_density, boundary_internal_energy);
          boundary_quadrature_node_variable.template setScalar<ComputationalVariableEnum::Density>(boundary_density, 0);
          boundary_quadrature_node_variable.template setVector<ComputationalVariableEnum::Velocity>(boundary_velocity,
                                                                                                    0);
          boundary_quadrature_node_variable.template setScalar<ComputationalVariableEnum::InternalEnergy>(
              boundary_internal_energy, 0);
          boundary_quadrature_node_variable.template setScalar<ComputationalVariableEnum::Pressure>(boundary_pressure,
                                                                                                    0);
        }
      }
    }
  }

  template <int N>
  inline static void calculateBoundaryGradientVariable(
      [[maybe_unused]] const PhysicalModel<SimulationControl>& physical_model,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl, N>& left_quadrature_node_variable,
      [[maybe_unused]] const Variable<SimulationControl, N>& right_quadrature_node_variable,
      Variable<SimulationControl, 1>& boundary_quadrature_node_volume_gradient_variable,
      Variable<SimulationControl, 1>& boundary_quadrature_node_interface_gradient_variable, const Isize column) {
    boundary_quadrature_node_volume_gradient_variable.conserved_ = left_quadrature_node_variable.conserved_.col(column);
    boundary_quadrature_node_interface_gradient_variable.conserved_.setZero();
  }

  template <int N>
  inline static void modifyBoundaryVariable(
      [[maybe_unused]] Variable<SimulationControl, N>& left_quadrature_node_variable,
      VariableGradient<SimulationControl, N>& left_quadrature_node_variable_gradient,
      [[maybe_unused]] Variable<SimulationControl, 1>& boundary_quadrature_node_variable,
      VariableGradient<SimulationControl, 1>& boundary_quadrature_node_variable_gradient, const Isize column) {
    boundary_quadrature_node_variable_gradient.primitive_ =
        left_quadrature_node_variable_gradient.primitive_.col(column);
  }
};

template <typename SimulationControl>
struct BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::VelocityInflow> {
  template <int N>
  inline static void calculateBoundaryVariable(const PhysicalModel<SimulationControl>& physical_model,
                                               const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                               const Variable<SimulationControl, N>& left_quadrature_node_variable,
                                               const Variable<SimulationControl, N>& right_quadrature_node_variable,
                                               Variable<SimulationControl, 1>& boundary_quadrature_node_variable,
                                               const Isize column) {
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

  template <int N>
  inline static void calculateBoundaryGradientVariable(
      [[maybe_unused]] const PhysicalModel<SimulationControl>& physical_model,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl, N>& left_quadrature_node_variable,
      [[maybe_unused]] const Variable<SimulationControl, N>& right_quadrature_node_variable,
      Variable<SimulationControl, 1>& boundary_quadrature_node_volume_gradient_variable,
      Variable<SimulationControl, 1>& boundary_quadrature_node_interface_gradient_variable, const Isize column) {
    boundary_quadrature_node_volume_gradient_variable.conserved_ = left_quadrature_node_variable.conserved_.col(column);
    boundary_quadrature_node_interface_gradient_variable.conserved_.setZero();
  }

  template <int N>
  inline static void modifyBoundaryVariable(
      [[maybe_unused]] Variable<SimulationControl, N>& left_quadrature_node_variable,
      VariableGradient<SimulationControl, N>& left_quadrature_node_variable_gradient,
      [[maybe_unused]] Variable<SimulationControl, 1>& boundary_quadrature_node_variable,
      VariableGradient<SimulationControl, 1>& boundary_quadrature_node_variable_gradient, const Isize column) {
    boundary_quadrature_node_variable_gradient.primitive_ =
        left_quadrature_node_variable_gradient.primitive_.col(column);
  }
};

template <typename SimulationControl>
struct BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::PressureOutflow> {
  template <int N>
  inline static void calculateBoundaryVariable(const PhysicalModel<SimulationControl>& physical_model,
                                               const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
                                               const Variable<SimulationControl, N>& left_quadrature_node_variable,
                                               const Variable<SimulationControl, N>& right_quadrature_node_variable,
                                               Variable<SimulationControl, 1>& boundary_quadrature_node_variable,
                                               const Isize column) {
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

  template <int N>
  inline static void calculateBoundaryGradientVariable(
      [[maybe_unused]] const PhysicalModel<SimulationControl>& physical_model,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl, N>& left_quadrature_node_variable,
      [[maybe_unused]] const Variable<SimulationControl, N>& right_quadrature_node_variable,
      Variable<SimulationControl, 1>& boundary_quadrature_node_volume_gradient_variable,
      Variable<SimulationControl, 1>& boundary_quadrature_node_interface_gradient_variable, const Isize column) {
    boundary_quadrature_node_volume_gradient_variable.conserved_ = left_quadrature_node_variable.conserved_.col(column);
    boundary_quadrature_node_interface_gradient_variable.conserved_.setZero();
  }

  template <int N>
  inline static void modifyBoundaryVariable(
      [[maybe_unused]] Variable<SimulationControl, N>& left_quadrature_node_variable,
      VariableGradient<SimulationControl, N>& left_quadrature_node_variable_gradient,
      [[maybe_unused]] Variable<SimulationControl, 1>& boundary_quadrature_node_variable,
      VariableGradient<SimulationControl, 1>& boundary_quadrature_node_variable_gradient, const Isize column) {
    boundary_quadrature_node_variable_gradient.primitive_ =
        left_quadrature_node_variable_gradient.primitive_.col(column);
  }
};

template <typename SimulationControl>
struct BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::IsoThermalNonSlipWall> {
  template <int N>
  inline static void calculateBoundaryVariable(
      const PhysicalModel<SimulationControl>& physical_model,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl, N>& left_quadrature_node_variable,
      const Variable<SimulationControl, N>& right_quadrature_node_variable,
      Variable<SimulationControl, 1>& boundary_quadrature_node_variable, const Isize column) {
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

  template <int N>
  inline static void calculateBoundaryGradientVariable(
      const PhysicalModel<SimulationControl>& physical_model,
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl, N>& left_quadrature_node_variable,
      const Variable<SimulationControl, N>& right_quadrature_node_variable,
      Variable<SimulationControl, 1>& boundary_quadrature_node_volume_gradient_variable,
      Variable<SimulationControl, 1>& boundary_quadrature_node_interface_gradient_variable, const Isize column) {
    Variable<SimulationControl, 1> boundary_quadrature_node_variable;
    calculateBoundaryVariable(physical_model, normal_vector, left_quadrature_node_variable,
                              right_quadrature_node_variable, boundary_quadrature_node_variable, column);
    boundary_quadrature_node_variable.calculateConservedFromComputational();
    boundary_quadrature_node_volume_gradient_variable.conserved_ = boundary_quadrature_node_variable.conserved_;
    boundary_quadrature_node_interface_gradient_variable.conserved_ =
        boundary_quadrature_node_variable.conserved_ - left_quadrature_node_variable.conserved_.col(column);
  }

  template <int N>
  inline static void modifyBoundaryVariable(
      Variable<SimulationControl, N>& left_quadrature_node_variable,
      VariableGradient<SimulationControl, N>& left_quadrature_node_variable_gradient,
      Variable<SimulationControl, 1>& boundary_quadrature_node_variable,
      VariableGradient<SimulationControl, 1>& boundary_quadrature_node_variable_gradient, const Isize column) {
    left_quadrature_node_variable.computational_.col(column) = boundary_quadrature_node_variable.computational_;
    boundary_quadrature_node_variable_gradient.primitive_ =
        left_quadrature_node_variable_gradient.primitive_.col(column);
  }
};

template <typename SimulationControl>
struct BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::AdiabaticSlipWall> {
  template <int N>
  inline static void calculateBoundaryVariable(
      [[maybe_unused]] const PhysicalModel<SimulationControl>& physical_model,
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl, N>& left_quadrature_node_variable,
      [[maybe_unused]] const Variable<SimulationControl, N>& right_quadrature_node_variable,
      Variable<SimulationControl, 1>& boundary_quadrature_node_variable, const Isize column) {
    boundary_quadrature_node_variable.computational_ = left_quadrature_node_variable.computational_.col(column);
    const Eigen::Vector<Real, SimulationControl::kDimension> boundary_velocity =
        left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(column) -
        (left_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(column).transpose() *
         normal_vector) *
            normal_vector;
    boundary_quadrature_node_variable.template setVector<ComputationalVariableEnum::Velocity>(boundary_velocity, 0);
  }

  template <int N>
  inline static void calculateBoundaryGradientVariable(
      const PhysicalModel<SimulationControl>& physical_model,
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl, N>& left_quadrature_node_variable,
      const Variable<SimulationControl, N>& right_quadrature_node_variable,
      Variable<SimulationControl, 1>& boundary_quadrature_node_volume_gradient_variable,
      Variable<SimulationControl, 1>& boundary_quadrature_node_interface_gradient_variable, const Isize column) {
    Variable<SimulationControl, 1> boundary_quadrature_node_variable;
    calculateBoundaryVariable(physical_model, normal_vector, left_quadrature_node_variable,
                              right_quadrature_node_variable, boundary_quadrature_node_variable, column);
    boundary_quadrature_node_variable.calculateConservedFromComputational();
    boundary_quadrature_node_volume_gradient_variable.conserved_ = boundary_quadrature_node_variable.conserved_;
    boundary_quadrature_node_interface_gradient_variable.conserved_ =
        boundary_quadrature_node_variable.conserved_ - left_quadrature_node_variable.conserved_.col(column);
  }

  template <int N>
  inline static void modifyBoundaryVariable(
      Variable<SimulationControl, N>& left_quadrature_node_variable,
      VariableGradient<SimulationControl, N>& left_quadrature_node_variable_gradient,
      Variable<SimulationControl, 1>& boundary_quadrature_node_variable,
      VariableGradient<SimulationControl, 1>& boundary_quadrature_node_variable_gradient, const Isize column) {
    left_quadrature_node_variable.computational_.col(column) = boundary_quadrature_node_variable.computational_;
    boundary_quadrature_node_variable_gradient.primitive_ =
        left_quadrature_node_variable_gradient.primitive_.col(column);
    boundary_quadrature_node_variable_gradient.template setVector<PrimitiveVariableEnum::Temperature>(
        Eigen::Vector<Real, SimulationControl::kDimension>::Zero(), 0);
  }
};

template <typename SimulationControl>
struct BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::AdiabaticNonSlipWall> {
  template <int N>
  inline static void calculateBoundaryVariable(
      [[maybe_unused]] const PhysicalModel<SimulationControl>& physical_model,
      [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl, N>& left_quadrature_node_variable,
      const Variable<SimulationControl, N>& right_quadrature_node_variable,
      Variable<SimulationControl, 1>& boundary_quadrature_node_variable, const Isize column) {
    boundary_quadrature_node_variable.computational_ = left_quadrature_node_variable.computational_.col(column);
    boundary_quadrature_node_variable.template setVector<ComputationalVariableEnum::Velocity>(
        right_quadrature_node_variable.template getVector<ComputationalVariableEnum::Velocity>(column), 0);
  }

  template <int N>
  inline static void calculateBoundaryGradientVariable(
      const PhysicalModel<SimulationControl>& physical_model,
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl, N>& left_quadrature_node_variable,
      const Variable<SimulationControl, N>& right_quadrature_node_variable,
      Variable<SimulationControl, 1>& boundary_quadrature_node_volume_gradient_variable,
      Variable<SimulationControl, 1>& boundary_quadrature_node_interface_gradient_variable, const Isize column) {
    Variable<SimulationControl, 1> boundary_quadrature_node_variable;
    calculateBoundaryVariable(physical_model, normal_vector, left_quadrature_node_variable,
                              right_quadrature_node_variable, boundary_quadrature_node_variable, column);
    boundary_quadrature_node_variable.calculateConservedFromComputational();
    boundary_quadrature_node_volume_gradient_variable.conserved_ = boundary_quadrature_node_variable.conserved_;
    boundary_quadrature_node_interface_gradient_variable.conserved_ =
        boundary_quadrature_node_variable.conserved_ - left_quadrature_node_variable.conserved_.col(column);
  }

  template <int N>
  inline static void modifyBoundaryVariable(
      Variable<SimulationControl, N>& left_quadrature_node_variable,
      VariableGradient<SimulationControl, N>& left_quadrature_node_variable_gradient,
      Variable<SimulationControl, 1>& boundary_quadrature_node_variable,
      VariableGradient<SimulationControl, 1>& boundary_quadrature_node_variable_gradient, const Isize column) {
    left_quadrature_node_variable.computational_.col(column) = boundary_quadrature_node_variable.computational_;
    boundary_quadrature_node_variable_gradient.primitive_ =
        left_quadrature_node_variable_gradient.primitive_.col(column);
    boundary_quadrature_node_variable_gradient.template setVector<PrimitiveVariableEnum::Temperature>(
        Eigen::Vector<Real, SimulationControl::kDimension>::Zero(), 0);
  }
};

template <typename SimulationControl, int N>
using CalculateBoundaryVariableFunction =
    void (*)(const PhysicalModel<SimulationControl>& physical_model,
             const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
             const Variable<SimulationControl, N>& left_quadrature_node_variable,
             const Variable<SimulationControl, N>& right_quadrature_node_variable,
             Variable<SimulationControl, 1>& boundary_quadrature_node_variable, const Isize column);

template <typename SimulationControl, int N>
using CalculateBoundaryGradientVariableFunction =
    void (*)(const PhysicalModel<SimulationControl>& physical_model,
             const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
             const Variable<SimulationControl, N>& left_quadrature_node_variable,
             const Variable<SimulationControl, N>& right_quadrature_node_variable,
             Variable<SimulationControl, 1>& boundary_quadrature_node_volume_gradient_variable,
             Variable<SimulationControl, 1>& boundary_quadrature_node_interface_gradient_variable, const Isize column);

template <typename SimulationControl, int N>
using ModifyBoundaryVariableFunction =
    void (*)(Variable<SimulationControl, N>& left_quadrature_node_variable,
             VariableGradient<SimulationControl, N>& left_quadrature_node_variable_gradient,
             Variable<SimulationControl, 1>& boundary_quadrature_node_variable,
             VariableGradient<SimulationControl, 1>& boundary_quadrature_node_variable_gradient, const Isize column);

template <typename SimulationControl>
struct BoundaryCondition {
  inline Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber> calculatePrimitiveFromCoordinate(
      const Eigen::Vector<Real, SimulationControl::kDimension>& coordinate, Isize gmsh_physical_index) const;

  inline Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber> calculatePrimitiveFromCoordinate(
      const Eigen::Vector<Real, SimulationControl::kDimension>& coordinate, Real time, Isize gmsh_physical_index) const;

  template <typename AdjacencyElementTrait>
  inline CalculateBoundaryVariableFunction<SimulationControl, AdjacencyElementTrait::kQuadratureNumber>
  getCalculateBoundaryVariableFunction(const BoundaryConditionEnum boundary_condition_type) const {
    constexpr std::array<CalculateBoundaryVariableFunction<SimulationControl, AdjacencyElementTrait::kQuadratureNumber>,
                         magic_enum::enum_count<BoundaryConditionEnum>()>
        kCalculateBoundaryVariableFunction = {
            &BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::RiemannFarfield>::
                template calculateBoundaryVariable<AdjacencyElementTrait::kQuadratureNumber>,
            &BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::VelocityInflow>::
                template calculateBoundaryVariable<AdjacencyElementTrait::kQuadratureNumber>,
            &BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::PressureOutflow>::
                template calculateBoundaryVariable<AdjacencyElementTrait::kQuadratureNumber>,
            &BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::IsoThermalNonSlipWall>::
                template calculateBoundaryVariable<AdjacencyElementTrait::kQuadratureNumber>,
            &BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::AdiabaticSlipWall>::
                template calculateBoundaryVariable<AdjacencyElementTrait::kQuadratureNumber>,
            &BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::AdiabaticNonSlipWall>::
                template calculateBoundaryVariable<AdjacencyElementTrait::kQuadratureNumber>};
    return kCalculateBoundaryVariableFunction[static_cast<Usize>(magic_enum::enum_integer(boundary_condition_type))];
  }

  template <typename AdjacencyElementTrait>
  inline CalculateBoundaryGradientVariableFunction<SimulationControl, AdjacencyElementTrait::kQuadratureNumber>
  getCalculateBoundaryGradientVariableFunction(const BoundaryConditionEnum boundary_condition_type) const {
    constexpr std::array<
        CalculateBoundaryGradientVariableFunction<SimulationControl, AdjacencyElementTrait::kQuadratureNumber>,
        magic_enum::enum_count<BoundaryConditionEnum>()>
        kCalculateBoundaryGradientVariableFunction = {
            &BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::RiemannFarfield>::
                template calculateBoundaryGradientVariable<AdjacencyElementTrait::kQuadratureNumber>,
            &BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::VelocityInflow>::
                template calculateBoundaryGradientVariable<AdjacencyElementTrait::kQuadratureNumber>,
            &BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::PressureOutflow>::
                template calculateBoundaryGradientVariable<AdjacencyElementTrait::kQuadratureNumber>,
            &BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::IsoThermalNonSlipWall>::
                template calculateBoundaryGradientVariable<AdjacencyElementTrait::kQuadratureNumber>,
            &BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::AdiabaticSlipWall>::
                template calculateBoundaryGradientVariable<AdjacencyElementTrait::kQuadratureNumber>,
            &BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::AdiabaticNonSlipWall>::
                template calculateBoundaryGradientVariable<AdjacencyElementTrait::kQuadratureNumber>};
    return kCalculateBoundaryGradientVariableFunction[static_cast<Usize>(
        magic_enum::enum_integer(boundary_condition_type))];
  }

  template <typename AdjacencyElementTrait>
  inline ModifyBoundaryVariableFunction<SimulationControl, AdjacencyElementTrait::kQuadratureNumber>
  getModifyBoundaryVariableFunction(const BoundaryConditionEnum boundary_condition_type) const {
    constexpr std::array<ModifyBoundaryVariableFunction<SimulationControl, AdjacencyElementTrait::kQuadratureNumber>,
                         magic_enum::enum_count<BoundaryConditionEnum>()>
        kModifyBoundaryVariableFunction = {
            &BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::RiemannFarfield>::
                template modifyBoundaryVariable<AdjacencyElementTrait::kQuadratureNumber>,
            &BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::VelocityInflow>::
                template modifyBoundaryVariable<AdjacencyElementTrait::kQuadratureNumber>,
            &BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::PressureOutflow>::
                template modifyBoundaryVariable<AdjacencyElementTrait::kQuadratureNumber>,
            &BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::IsoThermalNonSlipWall>::
                template modifyBoundaryVariable<AdjacencyElementTrait::kQuadratureNumber>,
            &BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::AdiabaticSlipWall>::
                template modifyBoundaryVariable<AdjacencyElementTrait::kQuadratureNumber>,
            &BoundaryConditionImpl<SimulationControl, BoundaryConditionEnum::AdiabaticNonSlipWall>::
                template modifyBoundaryVariable<AdjacencyElementTrait::kQuadratureNumber>};
    return kModifyBoundaryVariableFunction[static_cast<Usize>(magic_enum::enum_integer(boundary_condition_type))];
  }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_BOUNDARY_CONDITION_CPP_
