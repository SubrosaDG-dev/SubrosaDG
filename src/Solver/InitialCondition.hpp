/**
 * @file InitialCondition.hpp
 * @brief The header file of SubrosaDG initial condition.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-09
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_INITIAL_CONDITION_HPP_
#define SUBROSA_DG_INITIAL_CONDITION_HPP_

#include <Eigen/Core>
#include <functional>
#include <memory>
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

template <typename SimulationControl>
struct InitialCondition {
  std::function<Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber>(
      const Eigen::Vector<Real, SimulationControl::kDimension>& coordinate)>
      function_;
};

template <typename ElementTrait, typename SimulationControl, EquationModelEnum EquationModelType>
inline void ElementSolver<ElementTrait, SimulationControl, EquationModelType>::initializeElementSolver(
    const ElementMesh<ElementTrait>& element_mesh, const ThermalModel<SimulationControl>& thermal_model,
    const std::unordered_map<Isize, InitialCondition<SimulationControl>>& initial_condition) {
  this->number_ = element_mesh.number_;
  this->element_.resize(this->number_);
  this->delta_time_.resize(this->number_);
#if defined(SUBROSA_DG_WITH_OPENMP) && !defined(SUBROSA_DG_DEVELOP)
#pragma omp parallel for default(none) shared(Eigen::Dynamic, element_mesh, thermal_model, initial_condition)
#endif  // SUBROSA_DG_WITH_OPENMP && !SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < this->number_; i++) {
    Variable<SimulationControl> variable;
    Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kQuadratureNumber>
        quadrature_node_conserved_variable;
    for (Isize j = 0; j < ElementTrait::kQuadratureNumber; j++) {
      variable.primitive_ = initial_condition.at(element_mesh.element_(i).gmsh_physical_index_)
                                .function_(element_mesh.element_(i).gaussian_quadrature_node_coordinate_.col(j));
      variable.calculateConservedFromPrimitive(thermal_model);
      quadrature_node_conserved_variable.col(j) = variable.conserved_;
    }
    this->element_(i).conserved_variable_basis_function_coefficient_(1).noalias() =
        (element_mesh.basis_function_.value_.transpose() * element_mesh.basis_function_.value_)
            .llt()
            .solve((quadrature_node_conserved_variable * element_mesh.basis_function_.value_).transpose())
            .transpose();
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::initializeSolver(
    const Mesh<SimulationControl>& mesh, const ThermalModel<SimulationControl>& thermal_model,
    std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition,
    const std::unordered_map<Isize, InitialCondition<SimulationControl>>& initial_condition) {
  for (auto& [boundary_condition_name, boundary_condition_variable] : boundary_condition) {
    boundary_condition_variable->variable_.calculateConservedFromPrimitive(thermal_model);
    boundary_condition_variable->variable_.calculateComputationalFromPrimitive(thermal_model);
  }
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.initializeElementSolver(mesh.line_, thermal_model, initial_condition);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.initializeElementSolver(mesh.triangle_, thermal_model, initial_condition);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.initializeElementSolver(mesh.quadrangle_, thermal_model, initial_condition);
    }
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_INITIAL_CONDITION_HPP_
