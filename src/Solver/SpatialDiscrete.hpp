/**
 * @file SpatialDiscrete.hpp
 * @brief The header file of SubrosaDG spatial discrete.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-16
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_SPATIAL_DISCRETE_HPP_
#define SUBROSA_DG_SPATIAL_DISCRETE_HPP_

#include <Eigen/Core>
#include <memory>
#include <string>
#include <unordered_map>

#include "Mesh/ReadControl.hpp"
#include "Solver/BoundaryCondition.hpp"
#include "Solver/ConvectiveFlux.hpp"
#include "Solver/SimulationControl.hpp"
#include "Solver/SolveControl.hpp"
#include "Solver/ThermalModel.hpp"
#include "Solver/VariableConvertor.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Concept.hpp"
#include "Utils/Enum.hpp"

namespace SubrosaDG {

template <typename ElementTrait, typename SimulationControl, EquationModelEnum EquationModelType>
inline void ElementSolver<ElementTrait, SimulationControl, EquationModelType>::calculateElementGaussianQuadrature(
    const ElementMesh<ElementTrait>& element_mesh, const ThermalModel<SimulationControl>& thermal_model) {
#if defined(SUBROSA_DG_WITH_OPENMP) && !defined(SUBROSA_DG_DEVELOP)
#pragma omp parallel for collapse(2) default(none) schedule(auto) \
    shared(Eigen::all, Eigen::fix<SimulationControl::kDimension>, Eigen::Dynamic, element_mesh, thermal_model)
#endif  // SUBROSA_DG_WITH_OPENMP && !SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < element_mesh.number_; i++) {
    for (Isize j = 0; j < ElementTrait::kQuadratureNumber; j++) {
      Variable<SimulationControl> gaussian_quadrature_node_variable;
      Eigen::Matrix<Real, ElementTrait::kDimension, ElementTrait::kDimension>
          gaussian_quadrature_node_jacobian_transpose_inverse;
      Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, SimulationControl::kDimension>
          convective_variable;
      Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, SimulationControl::kDimension>
          element_gaussian_quadrature_node_temporary_variable;
      gaussian_quadrature_node_variable.template getFromSelf<ElementTrait>(i, j, element_mesh, thermal_model, *this);
      calculateConvectiveVariable(gaussian_quadrature_node_variable, convective_variable);
      gaussian_quadrature_node_jacobian_transpose_inverse =
          element_mesh.element_(i).jacobian_transpose_inverse_.col(j).reshaped(ElementTrait::kDimension,
                                                                               ElementTrait::kDimension);
      element_gaussian_quadrature_node_temporary_variable.noalias() =
          convective_variable * gaussian_quadrature_node_jacobian_transpose_inverse *
          element_mesh.element_(i).jacobian_determinant_(j) * element_mesh.gaussian_quadrature_.weight_(j);
      this->element_(i).quadrature_without_gradient_basis_function_value_(
          Eigen::all, Eigen::seqN(j * SimulationControl::kDimension, Eigen::fix<SimulationControl::kDimension>)) =
          element_gaussian_quadrature_node_temporary_variable;
    }
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::calculateGaussianQuadrature(
    const Mesh<SimulationControl>& mesh, const ThermalModel<SimulationControl>& thermal_model) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.calculateElementGaussianQuadrature(mesh.line_, thermal_model);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.calculateElementGaussianQuadrature(mesh.triangle_, thermal_model);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.calculateElementGaussianQuadrature(mesh.quadrangle_, thermal_model);
    }
  }
}

template <typename SimulationControl>
[[nodiscard]] inline Isize
AdjacencyElementSolverBase<AdjacencyPointTrait<SimulationControl::kPolynomialOrder>, SimulationControl>::
    getAdjacencyParentElementQuadratureNodeSequenceInParent(
        [[maybe_unused]] const Isize parent_gmsh_type_number, [[maybe_unused]] const bool is_left,
        const Isize adjacency_sequence_in_parent, [[maybe_unused]] const Isize qudrature_sequence_in_adjacency) const {
  return adjacency_sequence_in_parent;
}

template <typename SimulationControl>
[[nodiscard]] inline Isize
AdjacencyElementSolverBase<AdjacencyLineTrait<SimulationControl::kPolynomialOrder>, SimulationControl>::
    getAdjacencyParentElementQuadratureNodeSequenceInParent([[maybe_unused]] const Isize parent_gmsh_type_number,
                                                            const bool is_left,
                                                            const Isize adjacency_sequence_in_parent,
                                                            const Isize qudrature_sequence_in_adjacency) const {
  if (is_left) {
    return adjacency_sequence_in_parent * AdjacencyLineTrait<SimulationControl::kPolynomialOrder>::kQuadratureNumber +
           qudrature_sequence_in_adjacency;
  }
  return (adjacency_sequence_in_parent + 1) *
             AdjacencyLineTrait<SimulationControl::kPolynomialOrder>::kQuadratureNumber -
         qudrature_sequence_in_adjacency - 1;
}

template <typename SimulationControl>
inline void AdjacencyElementSolverBase<AdjacencyPointTrait<SimulationControl::kPolynomialOrder>, SimulationControl>::
    storeAdjacencyElementNodeGaussianQuadrature([[maybe_unused]] const Isize parent_gmsh_type_number,
                                                const Isize parent_index,
                                                const Isize adjacency_gaussian_quadrature_node_sequence_in_parent,
                                                const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>&
                                                    adjacency_element_gaussian_quadrature_node_temporary_variable,
                                                Solver<SimulationControl>& solver) {
  solver.line_.element_(parent_index)
      .adjacency_quadrature_without_basis_function_value_.col(adjacency_gaussian_quadrature_node_sequence_in_parent) =
      adjacency_element_gaussian_quadrature_node_temporary_variable;
}

template <typename SimulationControl>
inline void AdjacencyElementSolverBase<AdjacencyLineTrait<SimulationControl::kPolynomialOrder>, SimulationControl>::
    storeAdjacencyElementNodeGaussianQuadrature(const Isize parent_gmsh_type_number, const Isize parent_index,
                                                const Isize adjacency_gaussian_quadrature_node_sequence_in_parent,
                                                const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>&
                                                    adjacency_element_gaussian_quadrature_node_temporary_variable,
                                                Solver<SimulationControl>& solver) {
  if (parent_gmsh_type_number == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
    solver.triangle_.element_(parent_index)
        .adjacency_quadrature_without_basis_function_value_.col(adjacency_gaussian_quadrature_node_sequence_in_parent) =
        adjacency_element_gaussian_quadrature_node_temporary_variable;
  } else if (parent_gmsh_type_number == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
    solver.quadrangle_.element_(parent_index)
        .adjacency_quadrature_without_basis_function_value_.col(adjacency_gaussian_quadrature_node_sequence_in_parent) =
        adjacency_element_gaussian_quadrature_node_temporary_variable;
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void
AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl>::calculateInteriorAdjacencyElementGaussianQuadrature(
    const Mesh<SimulationControl>& mesh, const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
    const ThermalModel<SimulationControl>& thermal_model, Solver<SimulationControl>& solver) {
#if defined(SUBROSA_DG_WITH_OPENMP) && !defined(SUBROSA_DG_DEVELOP)
#pragma omp parallel for collapse(2) default(none) schedule(auto) \
    shared(Eigen::Dynamic, mesh, adjacency_element_mesh, thermal_model, solver)
#endif  // SUBROSA_DG_WITH_OPENMP && !SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < adjacency_element_mesh.interior_number_; i++) {
    for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
      Variable<SimulationControl> left_gaussian_quadrature_node_variable;
      Variable<SimulationControl> right_gaussian_quadrature_node_variable;
      Flux<SimulationControl> flux;
      const Eigen::Vector<Isize, 2> parent_index_each_type = adjacency_element_mesh.element_(i).parent_index_each_type_;
      const Eigen::Vector<Isize, 2> adjacency_sequence_in_parent =
          adjacency_element_mesh.element_(i).adjacency_sequence_in_parent_;
      const Eigen::Vector<Isize, 2> parent_gmsh_type_number =
          adjacency_element_mesh.element_(i).parent_gmsh_type_number_;
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>
          adjacency_element_gaussian_quadrature_node_temporary_variable;
      Eigen::Vector<Isize, 2> adjacency_gaussian_quadrature_node_sequence_in_parent;
      adjacency_gaussian_quadrature_node_sequence_in_parent(0) =
          this->getAdjacencyParentElementQuadratureNodeSequenceInParent(parent_gmsh_type_number(0), true,
                                                                        adjacency_sequence_in_parent(0), j);
      adjacency_gaussian_quadrature_node_sequence_in_parent(1) =
          this->getAdjacencyParentElementQuadratureNodeSequenceInParent(parent_gmsh_type_number(1), false,
                                                                        adjacency_sequence_in_parent(1), j);
      left_gaussian_quadrature_node_variable.getFromParent(parent_gmsh_type_number(0), parent_index_each_type(0),
                                                           adjacency_gaussian_quadrature_node_sequence_in_parent(0),
                                                           mesh, thermal_model, solver);
      right_gaussian_quadrature_node_variable.getFromParent(parent_gmsh_type_number(1), parent_index_each_type(1),
                                                            adjacency_gaussian_quadrature_node_sequence_in_parent(1),
                                                            mesh, thermal_model, solver);
      calculateConvectiveFlux<SimulationControl>(thermal_model, adjacency_element_mesh.element_(i).transition_matrix_,
                                                 left_gaussian_quadrature_node_variable,
                                                 right_gaussian_quadrature_node_variable, flux);
      adjacency_element_gaussian_quadrature_node_temporary_variable =
          flux.convective_n_ * adjacency_element_mesh.element_(i).jacobian_determinant_(j) *
          adjacency_element_mesh.gaussian_quadrature_.weight_(j);
      this->storeAdjacencyElementNodeGaussianQuadrature(parent_gmsh_type_number(0), parent_index_each_type(0),
                                                        adjacency_gaussian_quadrature_node_sequence_in_parent(0),
                                                        adjacency_element_gaussian_quadrature_node_temporary_variable,
                                                        solver);
      this->storeAdjacencyElementNodeGaussianQuadrature(parent_gmsh_type_number(1), parent_index_each_type(1),
                                                        adjacency_gaussian_quadrature_node_sequence_in_parent(1),
                                                        -adjacency_element_gaussian_quadrature_node_temporary_variable,
                                                        solver);
    }
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void
AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl>::calculateBoundaryAdjacencyElementGaussianQuadrature(
    const Mesh<SimulationControl>& mesh, const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
    const ThermalModel<SimulationControl>& thermal_model,
    const std::unordered_map<std::string, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>&
        boundary_condition,
    Solver<SimulationControl>& solver) {
#if defined(SUBROSA_DG_WITH_OPENMP) && !defined(SUBROSA_DG_DEVELOP)
#pragma omp parallel for collapse(2) default(none) schedule(auto) \
    shared(Eigen::Dynamic, mesh, adjacency_element_mesh, thermal_model, boundary_condition, solver)
#endif  // SUBROSA_DG_WITH_OPENMP && !SUBROSA_DG_DEVELOP
  for (Isize i = adjacency_element_mesh.interior_number_;
       i < adjacency_element_mesh.boundary_number_ + adjacency_element_mesh.interior_number_; i++) {
    for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
      Variable<SimulationControl> left_gaussian_quadrature_node_variable;
      Flux<SimulationControl> flux;
      const Isize parent_index_each_type = adjacency_element_mesh.element_(i).parent_index_each_type_(0);
      const Isize adjacency_sequence_in_parent = adjacency_element_mesh.element_(i).adjacency_sequence_in_parent_(0);
      const Isize parent_gmsh_type_number = adjacency_element_mesh.element_(i).parent_gmsh_type_number_(0);
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>
          adjacency_element_gaussian_quadrature_node_temporary_variable;
      const Isize adjacency_gaussian_quadrature_node_sequence_in_parent =
          this->getAdjacencyParentElementQuadratureNodeSequenceInParent(parent_gmsh_type_number, true,
                                                                        adjacency_sequence_in_parent, j);
      left_gaussian_quadrature_node_variable.getFromParent(parent_gmsh_type_number, parent_index_each_type,
                                                           adjacency_gaussian_quadrature_node_sequence_in_parent, mesh,
                                                           thermal_model, solver);
      boundary_condition.at(adjacency_element_mesh.element_(i).gmsh_physical_name_)
          ->calculateBoundaryConvectiveFlux(thermal_model, adjacency_element_mesh.element_(i).transition_matrix_,
                                            left_gaussian_quadrature_node_variable, flux);
      adjacency_element_gaussian_quadrature_node_temporary_variable =
          flux.convective_n_ * adjacency_element_mesh.element_(i).jacobian_determinant_(j) *
          adjacency_element_mesh.gaussian_quadrature_.weight_(j);
      this->storeAdjacencyElementNodeGaussianQuadrature(
          parent_gmsh_type_number, parent_index_each_type, adjacency_gaussian_quadrature_node_sequence_in_parent,
          adjacency_element_gaussian_quadrature_node_temporary_variable, solver);
    }
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::calculateAdjacencyGaussianQuadrature(
    const Mesh<SimulationControl>& mesh, const ThermalModel<SimulationControl>& thermal_model,
    const std::unordered_map<std::string, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>&
        boundary_condition) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->point_.calculateInteriorAdjacencyElementGaussianQuadrature(mesh, mesh.point_, thermal_model, *this);
    this->point_.calculateBoundaryAdjacencyElementGaussianQuadrature(mesh, mesh.point_, thermal_model,
                                                                     boundary_condition, *this);
  } else if constexpr (SimulationControl::kDimension == 2) {
    this->line_.calculateInteriorAdjacencyElementGaussianQuadrature(mesh, mesh.line_, thermal_model, *this);
    this->line_.calculateBoundaryAdjacencyElementGaussianQuadrature(mesh, mesh.line_, thermal_model, boundary_condition,
                                                                    *this);
  }
}

template <typename ElementTrait, typename SimulationControl, EquationModelEnum EquationModelType>
inline void ElementSolver<ElementTrait, SimulationControl, EquationModelType>::calculateElementResidual(
    const ElementMesh<ElementTrait>& element_mesh) {
#if defined(SUBROSA_DG_WITH_OPENMP) && !defined(SUBROSA_DG_DEVELOP)
#pragma omp parallel for default(none) schedule(auto) shared(Eigen::Dynamic, element_mesh)
#endif  // SUBROSA_DG_WITH_OPENMP && !SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < this->number_; i++) {
    this->element_(i).residual_.noalias() = this->element_(i).quadrature_without_gradient_basis_function_value_ *
                                                element_mesh.basis_function_.gradient_value_ -
                                            this->element_(i).adjacency_quadrature_without_basis_function_value_ *
                                                element_mesh.basis_function_.adjacency_value_;
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::calculateResidual(const Mesh<SimulationControl>& mesh) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.calculateElementResidual(mesh.line_);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.calculateElementResidual(mesh.triangle_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.calculateElementResidual(mesh.quadrangle_);
    }
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_SPATIAL_DISCRETE_HPP_
