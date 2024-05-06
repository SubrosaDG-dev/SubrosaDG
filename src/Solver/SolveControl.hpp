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
      variable_basis_function_coefficient_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber,
                ElementTrait::kQuadratureNumber * SimulationControl::kDimension>
      variable_quadrature_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kAllAdjacencyQuadratureNumber>
      variable_adjacency_quadrature_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kBasisFunctionNumber>
      variable_residual_;
};

template <typename ElementTrait, typename SimulationControl, EquationModelEnum EquationModelType>
struct PerElementSolver;

template <typename ElementTrait, typename SimulationControl>
struct PerElementSolver<ElementTrait, SimulationControl, EquationModelEnum::Euler>
    : PerElementSolverBase<ElementTrait, SimulationControl> {};

template <typename ElementTrait, typename SimulationControl>
struct PerElementSolver<ElementTrait, SimulationControl, EquationModelEnum::NavierStokes>
    : PerElementSolverBase<ElementTrait, SimulationControl> {
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                ElementTrait::kBasisFunctionNumber>
      variable_gradient_basis_function_coefficient_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                ElementTrait::kBasisFunctionNumber>
      variable_gradient_volume_basis_function_coefficient_;
  Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                             ElementTrait::kBasisFunctionNumber>,
               ElementTrait::kAdjacencyNumber, 1>
      variable_gradient_interface_basis_function_coefficient_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                ElementTrait::kQuadratureNumber * SimulationControl::kDimension>
      variable_gradient_volume_quadrature_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                ElementTrait::kAllAdjacencyQuadratureNumber>
      variable_gradient_volume_adjacency_quadrature_;
  Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                             ElementTrait::kAllAdjacencyQuadratureNumber>,
               ElementTrait::kAdjacencyNumber, 1>
      variable_gradient_interface_adjacency_quadrature_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                ElementTrait::kBasisFunctionNumber>
      variable_gradient_volume_residual_;
  Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                             ElementTrait::kBasisFunctionNumber>,
               ElementTrait::kAdjacencyNumber, 1>
      variable_gradient_interface_residual_;
};

template <typename ElementTrait, typename SimulationControl>
struct ElementSolverBase {
  Isize number_{0};
  Eigen::Array<PerElementSolver<ElementTrait, SimulationControl, SimulationControl::kEquationModel>, Eigen::Dynamic, 1>
      element_;

  inline void initializeElementSolver(
      const ElementMesh<ElementTrait>& element_mesh, const ThermalModel<SimulationControl>& thermal_model,
      const std::unordered_map<Isize, InitialCondition<SimulationControl>>& initial_condition);

  inline void copyElementBasisFunctionCoefficient();

  inline Real calculateElementDeltaTime(const ElementMesh<ElementTrait>& element_mesh,
                                        const ThermalModel<SimulationControl>& thermal_model,
                                        Real courant_friedrichs_lewy_number);

  inline void calculateElementResidual(const ElementMesh<ElementTrait>& element_mesh);

  inline void updateElementBasisFunctionCoefficient(
      int step, const ElementMesh<ElementTrait>& element_mesh,
      const TimeIntegrationData<SimulationControl::kTimeIntegration>& time_integration);

  inline void calculateElementRelativeError(
      const ElementMesh<ElementTrait>& element_mesh,
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& relative_error);
};

template <typename ElementTrait, typename SimulationControl, EquationModelEnum EquationModelType>
struct ElementSolver;

template <typename ElementTrait, typename SimulationControl>
struct ElementSolver<ElementTrait, SimulationControl, EquationModelEnum::Euler>
    : ElementSolverBase<ElementTrait, SimulationControl> {
  inline void calculateElementQuadrature(const ElementMesh<ElementTrait>& element_mesh,
                                         const ThermalModel<SimulationControl>& thermal_model);

  inline void writeElementRawBinary(std::fstream& fout) const;
};

template <typename ElementTrait, typename SimulationControl>
struct ElementSolver<ElementTrait, SimulationControl, EquationModelEnum::NavierStokes>
    : ElementSolverBase<ElementTrait, SimulationControl> {
  inline void initializeElementGardientSolver();

  inline void calculateElementQuadrature(const ElementMesh<ElementTrait>& element_mesh,
                                         const ThermalModel<SimulationControl>& thermal_model);

  inline void calculateElementGardientQuadrature(const ElementMesh<ElementTrait>& element_mesh);

  inline void calculateElementGardientResidual(const ElementMesh<ElementTrait>& element_mesh);

  inline void updateElementGardientBasisFunctionCoefficient(const ElementMesh<ElementTrait>& element_mesh);

  inline void writeElementRawBinary(std::fstream& fout) const;
};

template <typename AdjacencyElementTrait, typename SimulationControl>
struct AdjacencyElementSolverBase {
  template <AdjacencyEnum AdjacencyType>
  [[nodiscard]] inline Isize getAdjacencyParentElementQuadratureNodeSequenceInParent(
      Isize parent_gmsh_type_number, Isize adjacency_sequence_in_parent, Isize qudrature_sequence_in_adjacency) const;

  inline void storeAdjacencyElementNodeQuadrature(
      Isize parent_gmsh_type_number, Isize parent_index, Isize adjacency_quadrature_node_sequence_in_parent,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>&
          adjacency_element_quadrature_node_temporary_variable,
      Solver<SimulationControl>& solver);
};

template <typename AdjacencyElementTrait, typename SimulationControl, EquationModelEnum EquationModelType>
struct AdjacencyElementSolver;

template <typename AdjacencyElementTrait, typename SimulationControl>
struct AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl, EquationModelEnum::Euler>
    : AdjacencyElementSolverBase<AdjacencyElementTrait, SimulationControl> {
  inline void calculateInteriorAdjacencyElementQuadrature(const Mesh<SimulationControl>& mesh,
                                                          const ThermalModel<SimulationControl>& thermal_model,
                                                          Solver<SimulationControl>& solver);

  inline void calculateBoundaryAdjacencyElementQuadrature(
      const Mesh<SimulationControl>& mesh, const ThermalModel<SimulationControl>& thermal_model,
      const std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition,
      Solver<SimulationControl>& solver);
};

template <typename AdjacencyElementTrait, typename SimulationControl>
struct AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl, EquationModelEnum::NavierStokes>
    : AdjacencyElementSolverBase<AdjacencyElementTrait, SimulationControl> {
  inline void storeAdjacencyElementNodeVolumeGardientQuadrature(
      Isize parent_gmsh_type_number, Isize parent_index, Isize adjacency_quadrature_node_sequence_in_parent,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>&
          adjacency_element_quadrature_node_temporary_variable,
      Solver<SimulationControl>& solver);

  inline void storeAdjacencyElementNodeInterfaceGardientQuadrature(
      Isize parent_gmsh_type_number, Isize parent_index, Isize adjacency_sequence_in_parent,
      Isize adjacency_quadrature_node_sequence_in_parent,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>&
          adjacency_element_quadrature_node_temporary_variable,
      Solver<SimulationControl>& solver);

  inline void calculateInteriorAdjacencyElementQuadrature(const Mesh<SimulationControl>& mesh,
                                                          const ThermalModel<SimulationControl>& thermal_model,
                                                          Solver<SimulationControl>& solver);

  inline void calculateInteriorAdjacencyElementGardientQuadrature(const Mesh<SimulationControl>& mesh,
                                                                  Solver<SimulationControl>& solver);

  inline void calculateBoundaryAdjacencyElementQuadrature(
      const Mesh<SimulationControl>& mesh, const ThermalModel<SimulationControl>& thermal_model,
      const std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition,
      Solver<SimulationControl>& solver);

  inline void calculateBoundaryAdjacencyElementGardientQuadrature(
      const Mesh<SimulationControl>& mesh, const ThermalModel<SimulationControl>& thermal_model,
      const std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition,
      Solver<SimulationControl>& solver);
};

template <typename SimulationControl>
struct SolverData<SimulationControl, 1> {
  AdjacencyElementSolver<AdjacencyPointTrait<SimulationControl::kPolynomialOrder>, SimulationControl,
                         SimulationControl::kEquationModel>
      point_;
  ElementSolver<LineTrait<SimulationControl::kPolynomialOrder>, SimulationControl, SimulationControl::kEquationModel>
      line_;

  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> relative_error_;
};

template <typename SimulationControl>
struct SolverData<SimulationControl, 2> {
  AdjacencyElementSolver<AdjacencyLineTrait<SimulationControl::kPolynomialOrder>, SimulationControl,
                         SimulationControl::kEquationModel>
      line_;
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
  inline static AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl, SimulationControl::kEquationModel>
      Solver::*getAdjacencyElement() {
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
      std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition,
      const std::unordered_map<Isize, InitialCondition<SimulationControl>>& initial_condition);

  inline void copyBasisFunctionCoefficient();

  inline void calculateDeltaTime(const Mesh<SimulationControl>& mesh,
                                 const ThermalModel<SimulationControl>& thermal_model,
                                 TimeIntegrationData<SimulationControl::kTimeIntegration>& time_integration);

  inline void calculateQuadrature(const Mesh<SimulationControl>& mesh,
                                  const ThermalModel<SimulationControl>& thermal_model);

  inline void calculateGardientQuadrature(const Mesh<SimulationControl>& mesh);

  inline void calculateAdjacencyQuadrature(
      const Mesh<SimulationControl>& mesh, const ThermalModel<SimulationControl>& thermal_model,
      const std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition);

  inline void calculateAdjacencyGardientQuadrature(
      const Mesh<SimulationControl>& mesh, const ThermalModel<SimulationControl>& thermal_model,
      const std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition);

  inline void calculateResidual(const Mesh<SimulationControl>& mesh);

  inline void calculateGardientResidual(const Mesh<SimulationControl>& mesh);

  inline void updateBasisFunctionCoefficient(
      int step, const Mesh<SimulationControl>& mesh,
      const TimeIntegrationData<SimulationControl::kTimeIntegration>& time_integration);

  inline void updateGardientBasisFunctionCoefficient(const Mesh<SimulationControl>& mesh);

  inline void stepSolver(
      int step, const Mesh<SimulationControl>& mesh, const ThermalModel<SimulationControl>& thermal_model,
      const std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition,
      const TimeIntegrationData<SimulationControl::kTimeIntegration>& time_integration);

  inline void calculateRelativeError(const Mesh<SimulationControl>& mesh);

  inline void writeRawBinary(std::fstream& fout) const;
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_SOLVE_CONTROL_HPP_
