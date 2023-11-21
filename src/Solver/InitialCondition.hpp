/**
 * @file InitialCondition.hpp
 * @brief The header file of SubroseDG initial condition.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-09
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_INITIAL_CONDITION_HPP_
#define SUBROSA_DG_INITIAL_CONDITION_HPP_

#include <memory>
#include <string>
#include <unordered_map>

#include "Mesh/ReadControl.hpp"
#include "Solver/BoundaryCondition.hpp"
#include "Solver/SolveControl.hpp"
#include "Solver/ThermalModel.hpp"
#include "Solver/VariableConvertor.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Concept.hpp"
#include "Utils/Enum.hpp"

namespace SubrosaDG {

template <typename ElementTrait, typename SimulationControl, EquationModel EquationModelType>
inline void ElementSolver<ElementTrait, SimulationControl, EquationModelType>::initializeElementSolver(
    const ElementMesh<ElementTrait>& element_mesh,
    const std::unordered_map<std::string, Variable<SimulationControl, SimulationControl::kDimension>>&
        initial_condition) {
  this->number_ = element_mesh.number_;
  this->element_.resize(this->number_);
  this->delta_time_.resize(this->number_);
  for (Isize i = 0; i < this->number_; i++) {
    this->element_(i).conserved_variable_basis_function_coefficient_(1).colwise() =
        initial_condition.at(element_mesh.element_(i).gmsh_physical_name_).conserved_;
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl, 2>::initializeSolver(
    const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
    const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
    std::unordered_map<std::string, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition,
    std::unordered_map<std::string, Variable<SimulationControl, SimulationControl::kDimension>>& initial_condition) {
  for (auto& [boundary_condition_name, boundary_condition_variable] : boundary_condition) {
    boundary_condition_variable->variable_.calculatePrimitiveFromHumanReadablePrimitive(thermal_model);
  }
  for (auto& [initial_condition_name, initial_condition_variable] : initial_condition) {
    initial_condition_variable.calculateConservedFromHumanReadablePrimitive(thermal_model);
  }
  if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
    this->triangle_.initializeElementSolver(mesh.triangle_, initial_condition);
  }
  if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
    this->quadrangle_.initializeElementSolver(mesh.quadrangle_, initial_condition);
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_INITIAL_CONDITION_HPP_
