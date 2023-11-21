/**
 * @file VariableConvertor.hpp
 * @brief The head file of SubroseDG variable convertor.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-06
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_VARIABLE_CONVERTOR_HPP_
#define SUBROSA_DG_VARIABLE_CONVERTOR_HPP_

#include <Eigen/Core>

#include "Mesh/ReadControl.hpp"
#include "Solver/SimulationControl.hpp"
#include "Solver/SolveControl.hpp"
#include "Solver/ThermalModel.hpp"
#include "Utils/BasicDataType.hpp"

namespace SubrosaDG {

template <typename SimulationControl>
struct Variable<SimulationControl, 2> {
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> conserved_;
  Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber> primitive_;
  Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber> human_readable_primitive_;

  inline void calculateConservedFromPrimitive() {
    const Real density = this->primitive_(0);
    const Real velocity_x = this->primitive_(1);
    const Real velocity_y = this->primitive_(2);
    const Real total_energy = this->primitive_(4);
    this->conserved_ << density, density * velocity_x, density * velocity_y, density * total_energy;
  }

  inline void calculatePrimitiveFromConserved(
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model) {
    const Real density = this->conserved_(0);
    const Real velocity_x = this->conserved_(1) / density;
    const Real velocity_y = this->conserved_(2) / density;
    const Real total_energy = this->conserved_(3) / density;
    const Real internal_energy = total_energy - 0.5 * (velocity_x * velocity_x + velocity_y * velocity_y);
    const Real pressure = thermal_model.getPressureFormInternalEnergy(density, internal_energy);
    this->primitive_ << density, velocity_x, velocity_y, pressure, total_energy;  // NOTE change to internal energy
  }

  inline void calculateConservedFromHumanReadablePrimitive(
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model) {
    const Real density = this->human_readable_primitive_(0);
    const Real velocity_x = this->human_readable_primitive_(1);
    const Real velocity_y = this->human_readable_primitive_(2);
    const Real temperature = this->human_readable_primitive_(4);
    const Real total_energy =  // NOTE remember ConstantH
        thermal_model.getInternalEnergy(temperature) + 0.5 * (velocity_x * velocity_x + velocity_y * velocity_y);
    this->conserved_ << density, density * velocity_x, density * velocity_y, density * total_energy;
  }

  inline void calculatePrimitiveFromHumanReadablePrimitive(
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model) {
    const Real density = this->human_readable_primitive_(0);
    const Real velocity_x = this->human_readable_primitive_(1);
    const Real velocity_y = this->human_readable_primitive_(2);
    const Real pressure = this->human_readable_primitive_(3);
    const Real temperature = this->human_readable_primitive_(4);
    const Real total_energy =  // NOTE remember ConstantH
        thermal_model.getInternalEnergy(temperature) + 0.5 * (velocity_x * velocity_x + velocity_y * velocity_y);
    this->primitive_ << density, velocity_x, velocity_y, pressure, total_energy;
  }

  inline void calculateHumanReadablePrimitiveFromConserved(
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model) {
    const Real density = this->conserved_(0);
    const Real velocity_x = this->conserved_(1) / density;
    const Real velocity_y = this->conserved_(2) / density;
    const Real total_energy = this->conserved_(3) / density;
    const Real internal_energy = total_energy - 0.5 * (velocity_x * velocity_x + velocity_y * velocity_y);
    const Real pressure = thermal_model.getPressureFormInternalEnergy(density, internal_energy);
    const Real temperature = thermal_model.getTemperature(internal_energy);
    this->human_readable_primitive_ << density, velocity_x, velocity_y, pressure, temperature;
  }

  template <typename ElementTrait>
  inline void getFromSelf(
      const Isize element_index, const Isize element_gaussian_quadrature_node_sequence,
      const ElementMesh<ElementTrait>& element_mesh,
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
      const ElementSolver<ElementTrait, SimulationControl, SimulationControl::kEquationModel>& element_solver) {
    this->conserved_.noalias() =
        element_solver.element_(element_index).conserved_variable_basis_function_coefficient_(1) *
        element_mesh.basis_function_.value_.row(element_gaussian_quadrature_node_sequence).transpose();
    this->calculatePrimitiveFromConserved(thermal_model);
  }

  inline void getFromParent(const Isize parent_gmsh_type_number, const Isize parent_index,
                            const Isize adjacency_gaussian_quadrature_node_sequence_in_parent,
                            const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
                            const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
                            const Solver<SimulationControl, SimulationControl::kDimension>& solver) {
    if (parent_gmsh_type_number == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      this->conserved_.noalias() =
          solver.triangle_.element_(parent_index).conserved_variable_basis_function_coefficient_(1) *
          mesh.triangle_.basis_function_.adjacency_value_.row(adjacency_gaussian_quadrature_node_sequence_in_parent)
              .transpose();
    } else if (parent_gmsh_type_number == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      this->conserved_.noalias() =
          solver.quadrangle_.element_(parent_index).conserved_variable_basis_function_coefficient_(1) *
          mesh.quadrangle_.basis_function_.adjacency_value_.row(adjacency_gaussian_quadrature_node_sequence_in_parent)
              .transpose();
    }
    this->calculatePrimitiveFromConserved(thermal_model);
  }
};

template <typename SimulationControl>
struct Variable<SimulationControl, 3> {
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> conserved_;
  Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber> primitive_;
  Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber> human_readable_primitive_;

  inline void calculateConservedFromPrimitive() {
    const Real density = this->primitive_(0);
    const Real velocity_x = this->primitive_(1);
    const Real velocity_y = this->primitive_(2);
    const Real velocity_z = this->primitive_(3);
    const Real total_energy = this->primitive_(5);
    this->conserved_ << density, density * velocity_x, density * velocity_y, density * velocity_z,
        density * total_energy;
  }

  inline void calculatePrimitiveFromConserved(
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model) {
    const Real density = this->conserved_(0);
    const Real velocity_x = this->conserved_(1) / density;
    const Real velocity_y = this->conserved_(2) / density;
    const Real velocity_z = this->conserved_(3) / density;
    const Real total_energy = this->conserved_(4) / density;
    const Real internal_energy =
        total_energy - 0.5 * (velocity_x * velocity_x + velocity_y * velocity_y + velocity_z * velocity_z);
    const Real pressure = thermal_model.getPressureFormInternalEnergy(density, internal_energy);
    this->primitive_ << density, velocity_x, velocity_y, velocity_z, pressure, total_energy;
  }

  inline void calculateConservedFromHumanReadablePrimitive(
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model) {
    const Real density = this->human_readable_primitive_(0);
    const Real velocity_x = this->human_readable_primitive_(1);
    const Real velocity_y = this->human_readable_primitive_(2);
    const Real velocity_z = this->human_readable_primitive_(3);
    const Real temperature = this->human_readable_primitive_(5);
    const Real total_energy = thermal_model.getInternalEnergy(temperature) +
                              0.5 * (velocity_x * velocity_x + velocity_y * velocity_y + velocity_z * velocity_z);
    this->conserved_ << density, density * velocity_x, density * velocity_y, density * velocity_z,
        density * total_energy;
  }

  inline void calculatePrimitiveFromHumanReadablePrimitive(
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model) {
    const Real density = this->human_readable_primitive_(0);
    const Real velocity_x = this->human_readable_primitive_(1);
    const Real velocity_y = this->human_readable_primitive_(2);
    const Real velocity_z = this->human_readable_primitive_(3);
    const Real pressure = this->human_readable_primitive_(4);
    const Real temperature = this->human_readable_primitive_(5);
    const Real total_energy = thermal_model.getInternalEnergy(temperature) +
                              0.5 * (velocity_x * velocity_x + velocity_y * velocity_y + velocity_z * velocity_z);
    this->primitive_ << density, velocity_x, velocity_y, velocity_z, pressure, total_energy;
  }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_VARIABLE_CONVERTOR_HPP_
