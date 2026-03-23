/**
 * @file InitialCondition.cpp
 * @brief The header file of SubrosaDG initial condition.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-09
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_INITIAL_CONDITION_CPP_
#define SUBROSA_DG_INITIAL_CONDITION_CPP_

#include <Eigen/Core>
#include <sstream>

#include "Mesh/ReadControl.cpp"
#include "Solver/BoundaryCondition.cpp"
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
  inline static void computePrimitiveFromCoordinate(
      const Eigen::Vector<Real, SimulationControl::kDimension>& coordinate,
      Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber>& initial_primitive_variable);

  template <typename VolumeElementTrait>
  static void getVariableBasisFunctionCoefficient(
      const VolumeElementMesh<VolumeElementTrait>& volume_element_mesh,
      Eigen::Array<
          Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, VolumeElementTrait::kBasisFunctionNumber>,
          Eigen::Dynamic, 1>& variable_basis_function_coefficient,
      std::stringstream& raw_binary_ss) {
    if constexpr (SimulationControl::kInitialCondition == InitialConditionEnum::Function) {
      tbb::parallel_for(
          tbb::blocked_range<Isize>(0, volume_element_mesh.number_),
          [&](const tbb::blocked_range<Isize>& range) -> void {
            for (Isize i = range.begin(); i != range.end(); i++) {
              Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, VolumeElementTrait::kQuadratureNumber>
                  initial_all_conserved_variable;
              Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber> initial_primitive_variable;
              Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber> initial_computational_variable;
              Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> initial_conserved_variable;
              for (Isize j = 0; j < VolumeElementTrait::kQuadratureNumber; j++) {
                InitialCondition<SimulationControl>::computePrimitiveFromCoordinate(
                    volume_element_mesh.quadrature_node_coordinate_(i).col(j), initial_primitive_variable);
                Variable<SimulationControl>::convertComputationalFromPrimitive(initial_primitive_variable,
                                                                               initial_computational_variable);
                Variable<SimulationControl>::convertConservedFromComputational(initial_computational_variable,
                                                                               initial_conserved_variable);
                initial_all_conserved_variable.col(j) = initial_conserved_variable;
              }
              variable_basis_function_coefficient(i).noalias() = initial_all_conserved_variable *
                                                                 volume_element_mesh.nodal_basis_function_ *
                                                                 volume_element_mesh.nodal_least_squares_inverse_;
            }
          });
    } else if constexpr (SimulationControl::kInitialCondition == InitialConditionEnum::LastStep) {
      for (Isize i = 0; i < volume_element_mesh.number_; i++) {
        raw_binary_ss.read(
            reinterpret_cast<char*>(variable_basis_function_coefficient(i).data()),
            SimulationControl::kConservedVariableNumber * VolumeElementTrait::kBasisFunctionNumber * kRealSize);
        raw_binary_ss.seekg(SimulationControl::kConservedVariableNumber * SimulationControl::kDimension *
                                VolumeElementTrait::kBasisFunctionNumber * kRealSize,
                            std::ios::cur);
      }
    } else if constexpr (SimulationControl::kInitialCondition == InitialConditionEnum::LowOrder) {
      constexpr int kLowOrderBasisFunctionNumber{
          getElementBasisFunctionNumber<VolumeElementTrait::kElementType, SimulationControl::kPolynomialOrder - 1>()};
      Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, kLowOrderBasisFunctionNumber>
          initial_variable_basis_function_coefficient;
      for (Isize i = 0; i < volume_element_mesh.number_; i++) {
        raw_binary_ss.read(reinterpret_cast<char*>(initial_variable_basis_function_coefficient.data()),
                           SimulationControl::kConservedVariableNumber * kLowOrderBasisFunctionNumber * kRealSize);
        raw_binary_ss.seekg(SimulationControl::kConservedVariableNumber * SimulationControl::kDimension *
                                kLowOrderBasisFunctionNumber * kRealSize,
                            std::ios::cur);
        variable_basis_function_coefficient(i).setZero();
        variable_basis_function_coefficient(i)(Eigen::placeholders::all,
                                               Eigen::seqN(Eigen::fix<0>, Eigen::fix<kLowOrderBasisFunctionNumber>)) =
            initial_variable_basis_function_coefficient;
      }
    }
  }
};

template <typename VolumeElementTrait, typename SimulationControl>
inline void VolumeElementSolver<VolumeElementTrait, SimulationControl>::initializeVolumeElementSolver(
    const VolumeElementMesh<VolumeElementTrait>& volume_element_mesh,
    [[maybe_unused]] std::stringstream& raw_binary_ss) {
  this->number_ = volume_element_mesh.number_;
  // VolumeElementSolverBase
  this->variable_basis_function_coefficient_last_.resize(this->number_);
  this->variable_basis_function_coefficient_.resize(this->number_);
  this->variable_quadrature_.resize(this->number_);
  this->variable_adjacency_quadrature_.resize(this->number_);
  this->variable_residual_.resize(this->number_);
  if constexpr (IsNS<SimulationControl::kEquationModel>) {
    this->variable_gradient_basis_function_coefficient_.resize(this->number_);
  }
  // VolumeElementGradientSolver
  this->variable_volume_gradient_basis_function_coefficient_.resize(this->number_);
  this->variable_volume_gradient_quadrature_.resize(this->number_);
  this->variable_volume_gradient_adjacency_quadrature_.resize(this->number_);
  this->variable_volume_gradient_residual_.resize(this->number_);
  if constexpr (IsNS<SimulationControl::kEquationModel>) {
    this->variable_interface_gradient_basis_function_coefficient_.resize(this->number_);
    this->variable_interface_gradient_adjacency_quadrature_.resize(this->number_);
    this->variable_interface_gradient_residual_.resize(this->number_);
  }
  // VolumeElementSourceSolver
  if constexpr (SimulationControl::kSourceTerm == SourceTermEnum::Boussinesq) {
    this->variable_source_quadrature_.resize(this->number_);
  }
  InitialCondition<SimulationControl>::getVariableBasisFunctionCoefficient(
      volume_element_mesh, this->variable_basis_function_coefficient_, raw_binary_ss);
}

template <typename VolumeElementTrait, typename SimulationControl>
inline void VolumeElementSolverDevice<VolumeElementTrait, SimulationControl>::transferVolumeElementSolverToDevice(
    const VolumeElementSolver<VolumeElementTrait, SimulationControl>& volume_element_solver) {
  this->number_ = volume_element_solver.number_;
  // VolumeElementSolverBase
  this->variable_basis_function_coefficient_last_.resize(this->number_);
  this->variable_basis_function_coefficient_.resize(this->number_);
  this->variable_quadrature_.resize(this->number_);
  this->variable_adjacency_quadrature_.resize(this->number_);
  this->variable_residual_.resize(this->number_);
  if constexpr (IsNS<SimulationControl::kEquationModel>) {
    this->variable_gradient_basis_function_coefficient_.resize(this->number_);
  }
  // VolumeElementGradientSolver
  this->variable_volume_gradient_basis_function_coefficient_.resize(this->number_);
  this->variable_volume_gradient_quadrature_.resize(this->number_);
  this->variable_volume_gradient_adjacency_quadrature_.resize(this->number_);
  this->variable_volume_gradient_residual_.resize(this->number_);
  if constexpr (IsNS<SimulationControl::kEquationModel>) {
    this->variable_interface_gradient_basis_function_coefficient_.resize(this->number_);
    this->variable_interface_gradient_adjacency_quadrature_.resize(this->number_);
    this->variable_interface_gradient_residual_.resize(this->number_);
  }
  // VolumeElementSourceSolver
  if constexpr (SimulationControl::kSourceTerm == SourceTermEnum::Boussinesq) {
    this->variable_source_quadrature_.resize(this->number_);
  }
  Utils::transferToDevice<Real, SimulationControl::kConservedVariableNumber, VolumeElementTrait::kBasisFunctionNumber>(
      volume_element_solver.variable_basis_function_coefficient_, this->variable_basis_function_coefficient_);
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl>::initializeAdjacencyElementSolver(
    const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh) {
  this->number_ = adjacency_element_mesh.number_;
  this->interior_number_ = adjacency_element_mesh.interior_number_;
  this->boundary_number_ = adjacency_element_mesh.boundary_number_;
  this->boundary_dummy_right_computational_variable_.resize(this->boundary_number_);
  tbb::parallel_for(
      tbb::blocked_range<Isize>(0, adjacency_element_mesh.boundary_number_),
      [&](const tbb::blocked_range<Isize>& range) -> void {
        for (Isize i = range.begin(); i != range.end(); i++) {
          const Isize gmsh_physical_index =
              adjacency_element_mesh.gmsh_physical_index_(i + adjacency_element_mesh.interior_number_);
          Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber> boundary_dummy_right_primitive_variable;
          Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>
              boundary_dummy_right_computational_variable;
          for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
            if constexpr (SimulationControl::kBoundaryTime == BoundaryTimeEnum::Steady) {
              BoundaryCondition<SimulationControl>::computePrimitiveFromCoordinate(
                  adjacency_element_mesh.quadrature_node_coordinate_(i + adjacency_element_mesh.interior_number_)
                      .col(j),
                  boundary_dummy_right_primitive_variable, gmsh_physical_index);
            } else if constexpr (SimulationControl::kBoundaryTime == BoundaryTimeEnum::TimeVarying) {
              BoundaryCondition<SimulationControl>::computePrimitiveFromCoordinate(
                  adjacency_element_mesh.quadrature_node_coordinate_(i + adjacency_element_mesh.interior_number_)
                      .col(j),
                  boundary_dummy_right_primitive_variable, 0.0_r, gmsh_physical_index);
            }
            Variable<SimulationControl>::convertComputationalFromPrimitive(boundary_dummy_right_primitive_variable,
                                                                           boundary_dummy_right_computational_variable);
            this->boundary_dummy_right_computational_variable_(i).col(j) = boundary_dummy_right_computational_variable;
          }
        }
      });
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void
AdjacencyElementSolverDevice<AdjacencyElementTrait, SimulationControl>::transferAdjacencyElementSolverToDevice(
    const AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl>& adjacency_element_solver) {
  this->number_ = adjacency_element_solver.number_;
  this->interior_number_ = adjacency_element_solver.interior_number_;
  this->boundary_number_ = adjacency_element_solver.boundary_number_;
  this->boundary_dummy_right_computational_variable_.resize(this->boundary_number_);
  Utils::transferToDevice<Real, SimulationControl::kComputationalVariableNumber,
                          AdjacencyElementTrait::kQuadratureNumber>(
      adjacency_element_solver.boundary_dummy_right_computational_variable_,
      this->boundary_dummy_right_computational_variable_);
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::initializeSolver(const Mesh<SimulationControl>& mesh) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.initializeVolumeElementSolver(mesh.line_, this->raw_binary_ss_);
    this->point_.initializeAdjacencyElementSolver(mesh.point_);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.initializeVolumeElementSolver(mesh.triangle_, this->raw_binary_ss_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.initializeVolumeElementSolver(mesh.quadrangle_, this->raw_binary_ss_);
    }
    this->line_.initializeAdjacencyElementSolver(mesh.line_);
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.initializeVolumeElementSolver(mesh.tetrahedron_, this->raw_binary_ss_);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.initializeVolumeElementSolver(mesh.pyramid_, this->raw_binary_ss_);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.initializeVolumeElementSolver(mesh.hexahedron_, this->raw_binary_ss_);
    }
    if constexpr (HasAdjacencyTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.initializeAdjacencyElementSolver(mesh.triangle_);
    }
    if constexpr (HasAdjacencyQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.initializeAdjacencyElementSolver(mesh.quadrangle_);
    }
  }
  this->raw_binary_ss_ = std::stringstream();
}

template <typename SimulationControl>
inline void SolverDevice<SimulationControl>::transferSolverToDevice(const Solver<SimulationControl>& solver) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.transferVolumeElementSolverToDevice(solver.line_);
    this->point_.transferAdjacencyElementSolverToDevice(solver.point_);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.transferVolumeElementSolverToDevice(solver.triangle_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.transferVolumeElementSolverToDevice(solver.quadrangle_);
    }
    this->line_.transferAdjacencyElementSolverToDevice(solver.line_);
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.transferVolumeElementSolverToDevice(solver.tetrahedron_);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.transferVolumeElementSolverToDevice(solver.pyramid_);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.transferVolumeElementSolverToDevice(solver.hexahedron_);
    }
    if constexpr (HasAdjacencyTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.transferAdjacencyElementSolverToDevice(solver.triangle_);
    }
    if constexpr (HasAdjacencyQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.transferAdjacencyElementSolverToDevice(solver.quadrangle_);
    }
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_INITIAL_CONDITION_CPP_
