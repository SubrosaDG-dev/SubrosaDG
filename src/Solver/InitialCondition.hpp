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
#include <filesystem>
#include <functional>
#include <memory>
#include <sstream>
#include <unordered_map>

#include "Mesh/ReadControl.hpp"
#include "Solver/BoundaryCondition.hpp"
#include "Solver/PhysicalModel.hpp"
#include "Solver/SimulationControl.hpp"
#include "Solver/SolveControl.hpp"
#include "Solver/VariableConvertor.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Concept.hpp"
#include "Utils/Constant.hpp"
#include "Utils/Enum.hpp"

namespace SubrosaDG {

template <typename SimulationControl, InitialConditionEnum InitialConditionType>
struct InitialConditionBase;

template <typename SimulationControl>
struct InitialConditionBase<SimulationControl, InitialConditionEnum::Function> {
  std::function<Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber>(
      const Eigen::Vector<Real, SimulationControl::kDimension>& coordinate)>
      function_;

  template <typename ElementTrait>
  void getVariableBasisFunctionCoefficient(
      const ElementMesh<ElementTrait>& element_mesh, const PhysicalModel<SimulationControl>& physical_model,
      Eigen::Array<ElementVariable<ElementTrait, SimulationControl>, Eigen::Dynamic, 1>& variable) {
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto) \
    shared(Eigen::Dynamic, element_mesh, physical_model, variable)
#endif  // SUBROSA_DG_DEVELOP
    for (Isize i = 0; i < element_mesh.number_; i++) {
      for (Isize j = 0; j < ElementTrait::kQuadratureNumber; j++) {
        variable(i).primitive_.col(j) = this->function_(element_mesh.element_(i).quadrature_node_coordinate_.col(j));
      }
      variable(i).calculateConservedFromPrimitive(physical_model);
    }
  }
};

template <typename SimulationControl>
struct InitialConditionBase<SimulationControl, InitialConditionEnum::SpecificFile> {
  std::filesystem::path raw_binary_path_;
  std::stringstream raw_binary_ss_;

  template <typename ElementTrait>
  void getVariableBasisFunctionCoefficient(
      const ElementMesh<ElementTrait>& element_mesh,
      Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kBasisFunctionNumber>,
                   Eigen::Dynamic, 1>& variable) {
    constexpr int kBasisFunctionNumber{
        getElementBasisFunctionNumber<ElementTrait::kElementType, SimulationControl::kPolynomialOrder - 1>()};
    Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, kBasisFunctionNumber> initial_variable;
    for (Isize i = 0; i < element_mesh.number_; i++) {
      this->raw_binary_ss_.read(reinterpret_cast<char*>(initial_variable.data()),
                                SimulationControl::kConservedVariableNumber * kBasisFunctionNumber * kRealSize);
      if constexpr (SimulationControl::kEquationModel == EquationModelEnum::NavierStokes) {
        Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                      kBasisFunctionNumber>
            variable_gradient_basis_function_coefficient;
        this->raw_binary_ss_.read(reinterpret_cast<char*>(variable_gradient_basis_function_coefficient.data()),
                                  SimulationControl::kConservedVariableNumber * SimulationControl::kDimension *
                                      kBasisFunctionNumber * kRealSize);
      }
      variable(i).setZero();
      variable(i)(Eigen::all, Eigen::seqN(Eigen::fix<0>, Eigen::fix<kBasisFunctionNumber>)) = initial_variable;
    }
  }
};

template <typename SimulationControl>
struct InitialConditionBase<SimulationControl, InitialConditionEnum::LastStep> {
  std::filesystem::path raw_binary_path_;
  std::stringstream raw_binary_ss_;

  template <typename ElementTrait>
  void getVariableBasisFunctionCoefficient(
      const ElementMesh<ElementTrait>& element_mesh,
      Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kBasisFunctionNumber>,
                   Eigen::Dynamic, 1>& variable) {
    for (Isize i = 0; i < element_mesh.number_; i++) {
      this->raw_binary_ss_.read(
          reinterpret_cast<char*>(variable(i).data()),
          SimulationControl::kConservedVariableNumber * ElementTrait::kBasisFunctionNumber * kRealSize);
      if constexpr (SimulationControl::kEquationModel == EquationModelEnum::NavierStokes) {
        Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                      ElementTrait::kBasisFunctionNumber>
            variable_gradient_basis_function_coefficient;
        this->raw_binary_ss_.read(reinterpret_cast<char*>(variable_gradient_basis_function_coefficient.data()),
                                  SimulationControl::kConservedVariableNumber * SimulationControl::kDimension *
                                      ElementTrait::kBasisFunctionNumber * kRealSize);
      }
    }
  }
};

template <typename SimulationControl>
struct InitialCondition : InitialConditionBase<SimulationControl, SimulationControl::kInitialCondition> {};

template <typename ElementTrait, typename SimulationControl>
inline void ElementSolverBase<ElementTrait, SimulationControl>::initializeElementSolver(
    const ElementMesh<ElementTrait>& element_mesh, const PhysicalModel<SimulationControl>& physical_model,
    InitialCondition<SimulationControl>& initial_condition) {
  this->number_ = element_mesh.number_;
  this->element_.resize(this->number_);
  if constexpr (SimulationControl::kInitialCondition == InitialConditionEnum::Function) {
    Eigen::Array<ElementVariable<ElementTrait, SimulationControl>, Eigen::Dynamic, 1> variable(this->number_);
    initial_condition.getVariableBasisFunctionCoefficient(element_mesh, physical_model, variable);
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto) shared(Eigen::Dynamic, element_mesh, variable)
#endif  // SUBROSA_DG_DEVELOP
    for (Isize i = 0; i < this->number_; i++) {
      this->element_(i).variable_basis_function_coefficient_(1).noalias() =
          variable(i).conserved_ * element_mesh.basis_function_.modal_value_ *
          element_mesh.basis_function_.modal_least_squares_inverse_;
    }
  } else {
    Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kBasisFunctionNumber>,
                 Eigen::Dynamic, 1>
        variable(this->number_);
    initial_condition.getVariableBasisFunctionCoefficient(element_mesh, variable);
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto) shared(Eigen::Dynamic, element_mesh, variable)
#endif  // SUBROSA_DG_DEVELOP
    for (Isize i = 0; i < this->number_; i++) {
      this->element_(i).variable_basis_function_coefficient_ = variable(i);
    }
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementSolverBase<AdjacencyElementTrait, SimulationControl>::initializeAdjacencyElementSolver(
    const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
    const PhysicalModel<SimulationControl>& physical_model,
    const std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition) {
  this->interior_number_ = adjacency_element_mesh.interior_number_;
  this->boundary_number_ = adjacency_element_mesh.boundary_number_;
  this->boundary_dummy_variable_.resize(this->boundary_number_);
  for (Isize i = 0; i < adjacency_element_mesh.boundary_number_; i++) {
    for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
      this->boundary_dummy_variable_(i).primitive_.col(j) =
          boundary_condition
              .at(adjacency_element_mesh.element_(i + adjacency_element_mesh.interior_number_).gmsh_physical_index_)
              ->function_(adjacency_element_mesh.element_(i + adjacency_element_mesh.interior_number_)
                              .quadrature_node_coordinate_.col(j));
    }
    this->boundary_dummy_variable_(i).calculateConservedFromPrimitive(physical_model);
    this->boundary_dummy_variable_(i).calculateComputationalFromPrimitive(physical_model);
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::initializeSolver(
    const Mesh<SimulationControl>& mesh, const PhysicalModel<SimulationControl>& physical_model,
    const std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition,
    InitialCondition<SimulationControl>& initial_condition) {
  this->node_artificial_viscosity_.resize(mesh.node_number_);
  if constexpr (SimulationControl::kInitialCondition != InitialConditionEnum::Function) {
    this->raw_binary_ss.read(reinterpret_cast<char*>(this->node_artificial_viscosity.data()),
                             mesh.node_number_ * kRealSize);
  }
  this->node_artificial_viscosity_.setZero();
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.initializeElementSolver(mesh.line_, physical_model, initial_condition);
    this->point_.initializeAdjacencyElementSolver(mesh.point_, physical_model, boundary_condition);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.initializeElementSolver(mesh.triangle_, physical_model, initial_condition);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.initializeElementSolver(mesh.quadrangle_, physical_model, initial_condition);
    }
    this->line_.initializeAdjacencyElementSolver(mesh.line_, physical_model, boundary_condition);
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.initializeElementSolver(mesh.tetrahedron_, physical_model, initial_condition);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.initializeElementSolver(mesh.pyramid_, physical_model, initial_condition);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.initializeElementSolver(mesh.hexahedron_, physical_model, initial_condition);
    }
    if constexpr (HasAdjacencyTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.initializeAdjacencyElementSolver(mesh.triangle_, physical_model, boundary_condition);
    }
    if constexpr (HasAdjacencyQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.initializeAdjacencyElementSolver(mesh.quadrangle_, physical_model, boundary_condition);
    }
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_INITIAL_CONDITION_HPP_
