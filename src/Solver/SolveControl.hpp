/**
 * @file SolveControl.hpp
 * @brief The header file of SubrosaDG solve control.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-09
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
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
template <typename SimulationControl>
struct InitialCondition;
template <TimeIntegrationEnum TimeIntegrationType>
struct TimeIntegrationData;
template <typename SimulationControl, int Dimension>
struct SolverData;
template <typename SimulationControl>
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

template <typename ElementTrait, typename SimulationControl, EquationModelEnum EquationModelType>
struct PerElementSolver;

template <typename ElementTrait, typename SimulationControl>
struct PerElementSolver<ElementTrait, SimulationControl, EquationModelEnum::Euler>
    : PerElementSolverBase<ElementTrait, SimulationControl> {};

template <typename ElementTrait, typename SimulationControl>
struct PerElementSolver<ElementTrait, SimulationControl, EquationModelEnum::NS>
    : PerElementSolverBase<ElementTrait, SimulationControl> {
  // TODO: add NS specific data
};

template <typename ElementTrait, typename SimulationControl, EquationModelEnum EquationModelType>
struct ElementSolver {
  Isize number_;
  Eigen::Array<PerElementSolver<ElementTrait, SimulationControl, EquationModelType>, Eigen::Dynamic, 1> element_;

  Eigen::Vector<Real, Eigen::Dynamic> delta_time_;

  inline void initializeElementSolver(
      const ElementMesh<ElementTrait>& element_mesh, const ThermalModel<SimulationControl>& thermal_model,
      const std::unordered_map<std::string, InitialCondition<SimulationControl>>& initial_condition);

  inline void copyElementBasisFunctionCoefficient();

  inline void calculateElementDeltaTime(
      const ElementMesh<ElementTrait>& element_mesh, const ThermalModel<SimulationControl>& thermal_model,
      const TimeIntegrationData<SimulationControl::kTimeIntegration>& time_integration);

  inline void calculateElementGaussianQuadrature(const ElementMesh<ElementTrait>& element_mesh,
                                                 const ThermalModel<SimulationControl>& thermal_model);

  inline void calculateElementResidual(const ElementMesh<ElementTrait>& element_mesh);

  inline void updateElementBasisFunctionCoefficient(
      int step, const ElementMesh<ElementTrait>& element_mesh,
      const TimeIntegrationData<SimulationControl::kTimeIntegration>& time_integration);

  inline void calculateElementRelativeError(
      const ElementMesh<ElementTrait>& element_mesh,
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& relative_error);

  inline void writeElementRawBinary(std::fstream& fout) const;
};

template <typename AdjacencyElementTrait, typename SimulationControl>
struct AdjacencyElementSolverBase;

template <typename SimulationControl>
struct AdjacencyElementSolverBase<AdjacencyPointTrait<SimulationControl::kPolynomialOrder>, SimulationControl> {
  template <bool IsLeft>
  [[nodiscard]] inline Isize getAdjacencyParentElementQuadratureNodeSequenceInParent(
      [[maybe_unused]] Isize parent_gmsh_type_number, Isize adjacency_sequence_in_parent,
      Isize qudrature_sequence_in_adjacency) const;

  inline void storeAdjacencyElementNodeGaussianQuadrature(
      Isize parent_gmsh_type_number, Isize parent_index, Isize adjacency_gaussian_quadrature_node_sequence_in_parent,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>&
          adjacency_element_gaussian_quadrature_node_temporary_variable,
      Solver<SimulationControl>& solver);
};

template <typename SimulationControl>
struct AdjacencyElementSolverBase<AdjacencyLineTrait<SimulationControl::kPolynomialOrder>, SimulationControl> {
  template <bool IsLeft>
  [[nodiscard]] inline Isize getAdjacencyParentElementQuadratureNodeSequenceInParent(
      Isize parent_gmsh_type_number, Isize adjacency_sequence_in_parent, Isize qudrature_sequence_in_adjacency) const;

  inline void storeAdjacencyElementNodeGaussianQuadrature(
      Isize parent_gmsh_type_number, Isize parent_index, Isize adjacency_gaussian_quadrature_node_sequence_in_parent,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>&
          adjacency_element_gaussian_quadrature_node_temporary_variable,
      Solver<SimulationControl>& solver);
};

template <typename AdjacencyElementTrait, typename SimulationControl>
struct AdjacencyElementSolver : AdjacencyElementSolverBase<AdjacencyElementTrait, SimulationControl> {
  inline void calculateInteriorAdjacencyElementGaussianQuadrature(const Mesh<SimulationControl>& mesh,
                                                                  const ThermalModel<SimulationControl>& thermal_model,
                                                                  Solver<SimulationControl>& solver);

  inline void calculateBoundaryAdjacencyElementGaussianQuadrature(
      const Mesh<SimulationControl>& mesh, const ThermalModel<SimulationControl>& thermal_model,
      const std::unordered_map<std::string, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>&
          boundary_condition,
      Solver<SimulationControl>& solver);
};

template <typename SimulationControl>
struct SolverData<SimulationControl, 1> {
  AdjacencyElementSolver<AdjacencyPointTrait<SimulationControl::kPolynomialOrder>, SimulationControl> point_;
  ElementSolver<LineTrait<SimulationControl::kPolynomialOrder>, SimulationControl, SimulationControl::kEquationModel>
      line_;
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> relative_error_;
};

template <typename SimulationControl>
struct SolverData<SimulationControl, 2> {
  AdjacencyElementSolver<AdjacencyLineTrait<SimulationControl::kPolynomialOrder>, SimulationControl> line_;
  ElementSolver<TriangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl,
                SimulationControl::kEquationModel>
      triangle_;
  ElementSolver<QuadrangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl,
                SimulationControl::kEquationModel>
      quadrangle_;
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> relative_error_;
};

template <typename SimulationControl>
struct Solver : SolverData<SimulationControl, SimulationControl::kDimension> {
  template <typename ElementTrait>
  inline static ElementSolver<ElementTrait, SimulationControl, SimulationControl::kEquationModel> Solver::*
  getElement() {
    if constexpr (SimulationControl::kDimension == 1) {
      if constexpr (ElementTrait::kElementType == ElementEnum::Line) {
        return &Solver<SimulationControl>::line_;
      }
    } else if constexpr (SimulationControl::kDimension == 2) {
      if constexpr (ElementTrait::kElementType == ElementEnum::Triangle) {
        return &Solver<SimulationControl>::triangle_;
      }
      if constexpr (ElementTrait::kElementType == ElementEnum::Quadrangle) {
        return &Solver<SimulationControl>::quadrangle_;
      }
    }
    return nullptr;
  }

  template <typename AdjacencyElementTrait>
  inline static AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl> Solver::*getAdjacencyElement() {
    if constexpr (SimulationControl::kDimension == 1) {
      if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
        return &Solver<SimulationControl>::point_;
      }
    } else if constexpr (SimulationControl::kDimension == 2) {
      if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
        return &Solver<SimulationControl>::line_;
      }
    }
    return nullptr;
  }

  inline void initializeSolver(
      const Mesh<SimulationControl>& mesh, const ThermalModel<SimulationControl>& thermal_model,
      std::unordered_map<std::string, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition,
      const std::unordered_map<std::string, InitialCondition<SimulationControl>>& initial_condition);

  inline void copyBasisFunctionCoefficient();

  inline Real setDeltaTime(const TimeIntegrationData<SimulationControl::kTimeIntegration>& time_integration);

  inline Real calculateDeltaTime(const Mesh<SimulationControl>& mesh,
                                 const ThermalModel<SimulationControl>& thermal_model,
                                 const TimeIntegrationData<SimulationControl::kTimeIntegration>& time_integration);

  inline void calculateGaussianQuadrature(const Mesh<SimulationControl>& mesh,
                                          const ThermalModel<SimulationControl>& thermal_model);

  inline void calculateAdjacencyGaussianQuadrature(
      const Mesh<SimulationControl>& mesh, const ThermalModel<SimulationControl>& thermal_model,
      const std::unordered_map<std::string, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>&
          boundary_condition);

  inline void calculateResidual(const Mesh<SimulationControl>& mesh);

  inline void updateBasisFunctionCoefficient(
      int step, const Mesh<SimulationControl>& mesh,
      const TimeIntegrationData<SimulationControl::kTimeIntegration>& time_integration);

  inline void stepSolver(
      int step, const Mesh<SimulationControl>& mesh, const ThermalModel<SimulationControl>& thermal_model,
      const std::unordered_map<std::string, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>&
          boundary_condition,
      const TimeIntegrationData<SimulationControl::kTimeIntegration>& time_integration);

  inline void calculateRelativeError(const Mesh<SimulationControl>& mesh);

  inline void writeRawBinary(std::fstream& fout) const;
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_SOLVE_CONTROL_HPP_
