/**
 * @file SolveControl.hpp
 * @brief The header file of SubrosaDG solve control.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-09
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_SOLVE_CONTROL_HPP_
#define SUBROSA_DG_SOLVE_CONTROL_HPP_

#include <Eigen/Core>
#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>

#include "Mesh/ReadControl.hpp"
#include "Solver/SimulationControl.hpp"
#include "Solver/ThermalModel.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Enum.hpp"

namespace SubrosaDG {

template <typename SimulationControl>
struct BoundaryConditionBase;
template <TimeIntegration TimeIntegrationType>
struct TimeIntegrationData;
template <typename SimulationControl, int Dimension>
struct Variable;
template <typename SimulationControl, int Dimension>
struct Solver;

template <typename ElementTrait, typename SimulationControl>
struct PerElementSolverBase {
  Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kBasisFunctionNumber>, 2,
               1>
      conserved_variable_basis_function_coefficient_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber,
                ElementTrait::kQuadratureNumber * SimulationControl::kDimension>
      quadrature_without_gradient_basis_function_value_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kAdjacencyQuadratureNumber>
      adjacency_quadrature_without_basis_function_value_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kBasisFunctionNumber> residual_;
};

template <typename ElementTrait, typename SimulationControl, EquationModel EquationModelType>
struct PerElementSolver;

template <typename ElementTrait, typename SimulationControl>
struct PerElementSolver<ElementTrait, SimulationControl, EquationModel::Euler>
    : PerElementSolverBase<ElementTrait, SimulationControl> {};

template <typename ElementTrait, typename SimulationControl>
struct PerElementSolver<ElementTrait, SimulationControl, EquationModel::NS>
    : PerElementSolverBase<ElementTrait, SimulationControl> {
  // TODO: add NS specific data
};

template <typename ElementTrait, typename SimulationControl, EquationModel EquationModelType>
struct ElementSolver {
  Isize number_;
  Eigen::Array<PerElementSolver<ElementTrait, SimulationControl, EquationModelType>, Eigen::Dynamic, 1> element_;

  Eigen::Vector<Real, Eigen::Dynamic> delta_time_;

  inline void initializeElementSolver(
      const ElementMesh<ElementTrait>& element_mesh,
      const std::unordered_map<std::string, Variable<SimulationControl, SimulationControl::kDimension>>&
          initial_condition);

  inline void copyElementBasisFunctionCoefficient();

  inline void calculateElementDeltaTime(
      const ElementMesh<ElementTrait>& element_mesh,
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
      const TimeIntegrationData<SimulationControl::kTimeIntegration>& time_integration);

  inline void calculateElementGaussianQuadrature(
      const ElementMesh<ElementTrait>& element_mesh,
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model);

  inline void calculateElementResidual(const ElementMesh<ElementTrait>& element_mesh);

  inline void updateElementBasisFunctionCoefficient(
      int step, const ElementMesh<ElementTrait>& element_mesh,
      const TimeIntegrationData<SimulationControl::kTimeIntegration>& time_integration);

  inline void calculateElementAbsoluteError(
      const ElementMesh<ElementTrait>& element_mesh,
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& absolute_error);

  inline void writeElementRawBinary(std::fstream& fout) const;
};

template <typename AdjacencyElementTrait, typename SimulationControl>
struct AdjacencyElementSolver;

template <typename SimulationControl>
struct AdjacencyElementSolver<AdjacencyLineTrait<SimulationControl::kPolynomialOrder>, SimulationControl> {
  [[nodiscard]] inline Isize getAdjacencyParentElementQuadratureNodeSequenceInParent(
      [[maybe_unused]] Isize parent_gmsh_type_number, bool is_left, Isize adjacency_sequence_in_parent,
      Isize qudrature_sequence_in_adjacency) const;

  inline void storeAdjacencyElementNodeGaussianQuadrature(
      Isize parent_gmsh_type_number, Isize parent_index, Isize adjacency_gaussian_quadrature_node_sequence_in_parent,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>&
          adjacency_element_gaussian_quadrature_node_temporary_variable,
      Solver<SimulationControl, SimulationControl::kDimension>& solver);

  inline void calculateInteriorAdjacencyElementGaussianQuadrature(
      const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
      const AdjacencyElementMesh<AdjacencyLineTrait<SimulationControl::kPolynomialOrder>>& adjacency_element_mesh,
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
      Solver<SimulationControl, SimulationControl::kDimension>& solver);

  inline void calculateBoundaryAdjacencyElementGaussianQuadrature(
      const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
      const AdjacencyElementMesh<AdjacencyLineTrait<SimulationControl::kPolynomialOrder>>& adjacency_element_mesh,
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
      const std::unordered_map<std::string, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>&
          boundary_condition,
      Solver<SimulationControl, SimulationControl::kDimension>& solver);
};

template <typename SimulationControl, int Dimension>
struct Solver;

template <typename SimulationControl>
struct Solver<SimulationControl, 2> {
  AdjacencyElementSolver<AdjacencyLineTrait<SimulationControl::kPolynomialOrder>, SimulationControl> line_;
  ElementSolver<TriangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl,
                SimulationControl::kEquationModel>
      triangle_;
  ElementSolver<QuadrangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl,
                SimulationControl::kEquationModel>
      quadrangle_;
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> absolute_error_;

  inline void initializeSolver(
      const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
      std::unordered_map<std::string, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition,
      std::unordered_map<std::string, Variable<SimulationControl, SimulationControl::kDimension>>& initial_condition);

  inline void copyBasisFunctionCoefficient();

  inline void setDeltaTime();

  inline void calculateDeltaTime(
      const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
      const TimeIntegrationData<SimulationControl::kTimeIntegration>& time_integration);

  inline void calculateGaussianQuadrature(
      const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model);

  inline void calculateAdjacencyGaussianQuadrature(
      const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
      const std::unordered_map<std::string, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>&
          boundary_condition);

  inline void calculateResidual(const Mesh<SimulationControl, SimulationControl::kDimension>& mesh);

  inline void updateBasisFunctionCoefficient(
      int step, const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
      const TimeIntegrationData<SimulationControl::kTimeIntegration>& time_integration);

  inline void stepTime(int step, const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
                       const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
                       const std::unordered_map<std::string, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>&
                           boundary_condition,
                       const TimeIntegrationData<SimulationControl::kTimeIntegration>& time_integration);

  inline void calculateAbsoluteError(const Mesh<SimulationControl, SimulationControl::kDimension>& mesh);

  inline void writeRawBinary(std::fstream& fout) const;
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_SOLVE_CONTROL_HPP_
