/**
 * @file InitialCondition.cpp
 * @brief The header file of SubrosaDG initial condition.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-09
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2025 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_INITIAL_CONDITION_CPP_
#define SUBROSA_DG_INITIAL_CONDITION_CPP_

#include <Eigen/Core>
#include <filesystem>
#include <sstream>

#include "Mesh/ReadControl.cpp"
#include "Solver/BoundaryCondition.cpp"
#include "Solver/PhysicalModel.cpp"
#include "Solver/SimulationControl.cpp"
#include "Solver/SolveControl.cpp"
#include "Solver/VariableConvertor.cpp"
#include "Utils/BasicDataType.cpp"
#include "Utils/Concept.cpp"
#include "Utils/Constant.cpp"
#include "Utils/Enum.cpp"

namespace SubrosaDG {

template <typename SimulationControl>
struct InitialCondition {
  std::filesystem::path raw_binary_path_;
  std::stringstream raw_binary_ss_;

  inline Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber> calculatePrimitiveFromCoordinate(
      const Eigen::Vector<Real, SimulationControl::kDimension>& coordinate) const;

  template <typename ElementTrait>
  void getVariableBasisFunctionCoefficient(
      const ElementMesh<ElementTrait>& element_mesh,
      Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kBasisFunctionNumber>,
                   Eigen::Dynamic, 1>& variable_basis_function_coefficient) {
    if constexpr (SimulationControl::kInitialCondition == InitialConditionEnum::SpecificFile) {
      constexpr int kBasisFunctionNumber{
          getElementBasisFunctionNumber<ElementTrait::kElementType, SimulationControl::kPolynomialOrder - 1>()};
      Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, kBasisFunctionNumber>
          initial_variable_basis_function_coefficient;
      for (Isize i = 0; i < element_mesh.number_; i++) {
        this->raw_binary_ss_.read(reinterpret_cast<char*>(initial_variable_basis_function_coefficient.data()),
                                  SimulationControl::kConservedVariableNumber * kBasisFunctionNumber * kRealSize);
        if constexpr (SimulationControl::kEquationModel == EquationModelEnum::CompresibleNS ||
                      SimulationControl::kEquationModel == EquationModelEnum::IncompresibleNS) {
          Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                        kBasisFunctionNumber>
              variable_gradient_basis_function_coefficient;
          this->raw_binary_ss_.read(reinterpret_cast<char*>(variable_gradient_basis_function_coefficient.data()),
                                    SimulationControl::kConservedVariableNumber * SimulationControl::kDimension *
                                        kBasisFunctionNumber * kRealSize);
        }
        variable_basis_function_coefficient(i).setZero();
        variable_basis_function_coefficient(i)(Eigen::placeholders::all,
                                               Eigen::seqN(Eigen::fix<0>, Eigen::fix<kBasisFunctionNumber>)) =
            initial_variable_basis_function_coefficient;
      }
    } else if constexpr (SimulationControl::kInitialCondition == InitialConditionEnum::LastStep) {
      for (Isize i = 0; i < element_mesh.number_; i++) {
        this->raw_binary_ss_.read(
            reinterpret_cast<char*>(variable_basis_function_coefficient(i).data()),
            SimulationControl::kConservedVariableNumber * ElementTrait::kBasisFunctionNumber * kRealSize);
        if constexpr (SimulationControl::kEquationModel == EquationModelEnum::CompresibleNS ||
                      SimulationControl::kEquationModel == EquationModelEnum::IncompresibleNS) {
          Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                        ElementTrait::kBasisFunctionNumber>
              variable_gradient_basis_function_coefficient;
          this->raw_binary_ss_.read(reinterpret_cast<char*>(variable_gradient_basis_function_coefficient.data()),
                                    SimulationControl::kConservedVariableNumber * SimulationControl::kDimension *
                                        ElementTrait::kBasisFunctionNumber * kRealSize);
        }
      }
    }
  }
};

template <typename ElementTrait, typename SimulationControl>
inline void ElementSolver<ElementTrait, SimulationControl>::initializeElementSolver(
    const ElementMesh<ElementTrait>& element_mesh, const PhysicalModel<SimulationControl>& physical_model,
    InitialCondition<SimulationControl>& initial_condition) {
  this->number_ = element_mesh.number_;
  this->element_.resize(this->number_);
  if constexpr (SimulationControl::kInitialCondition == InitialConditionEnum::Function) {
    tbb::parallel_for(tbb::blocked_range<Isize>(0, this->number_), [&](const tbb::blocked_range<Isize>& range) {
      for (Isize i = range.begin(); i != range.end(); i++) {
        ElementVariable<ElementTrait, SimulationControl> variable;
        for (Isize j = 0; j < ElementTrait::kQuadratureNumber; j++) {
          variable.primitive_.col(j) = initial_condition.calculatePrimitiveFromCoordinate(
              element_mesh.element_(i).quadrature_node_coordinate_.col(j));
        }
        variable.calculateConservedFromPrimitive(physical_model);
        this->element_(i).variable_basis_function_coefficient_.noalias() =
            variable.conserved_ * element_mesh.basis_function_.modal_value_ *
            element_mesh.basis_function_.modal_least_squares_inverse_;
      }
    });
  } else {
    Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kBasisFunctionNumber>,
                 Eigen::Dynamic, 1>
        variable_basis_function_coefficient(this->number_);
    initial_condition.getVariableBasisFunctionCoefficient(element_mesh, variable_basis_function_coefficient);
    tbb::parallel_for(tbb::blocked_range<Isize>(0, this->number_), [&](const tbb::blocked_range<Isize>& range) {
      for (Isize i = range.begin(); i != range.end(); i++) {
        this->element_(i).variable_basis_function_coefficient_.noalias() = variable_basis_function_coefficient(i);
      }
    });
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl>::initializeAdjacencyElementSolver(
    const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
    const PhysicalModel<SimulationControl>& physical_model,
    const BoundaryCondition<SimulationControl>& boundary_condition) {
  this->interior_number_ = adjacency_element_mesh.interior_number_;
  this->boundary_number_ = adjacency_element_mesh.boundary_number_;
  this->boundary_dummy_variable_.resize(this->boundary_number_);
  tbb::parallel_for(
      tbb::blocked_range<Isize>(0, adjacency_element_mesh.boundary_number_),
      [&](const tbb::blocked_range<Isize>& range) {
        for (Isize i = range.begin(); i != range.end(); i++) {
          const Isize gmsh_physical_index =
              adjacency_element_mesh.element_(i + adjacency_element_mesh.interior_number_).gmsh_physical_index_;
          for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
            if constexpr (SimulationControl::kBoundaryTime == BoundaryTimeEnum::Steady) {
              this->boundary_dummy_variable_(i).primitive_.col(j) = boundary_condition.calculatePrimitiveFromCoordinate(
                  adjacency_element_mesh.element_(i + adjacency_element_mesh.interior_number_)
                      .quadrature_node_coordinate_.col(j),
                  gmsh_physical_index);
            } else if constexpr (SimulationControl::kBoundaryTime == BoundaryTimeEnum::TimeVarying) {
              this->boundary_dummy_variable_(i).primitive_.col(j) = boundary_condition.calculatePrimitiveFromCoordinate(
                  adjacency_element_mesh.element_(i + adjacency_element_mesh.interior_number_)
                      .quadrature_node_coordinate_.col(j),
                  0.0_r, gmsh_physical_index);
            }
          }
          this->boundary_dummy_variable_(i).calculateConservedFromPrimitive(physical_model);
          this->boundary_dummy_variable_(i).calculateComputationalFromPrimitive(physical_model);
        }
      });
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::initializeSolver(const Mesh<SimulationControl>& mesh,
                                                        const PhysicalModel<SimulationControl>& physical_model,
                                                        const BoundaryCondition<SimulationControl>& boundary_condition,
                                                        InitialCondition<SimulationControl>& initial_condition) {
  this->node_artificial_viscosity_.resize(mesh.node_number_);
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

#endif  // SUBROSA_DG_INITIAL_CONDITION_CPP_
