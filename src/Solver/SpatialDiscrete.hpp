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
#include <array>
#include <memory>
#include <type_traits>
#include <unordered_map>

#include "Mesh/ReadControl.hpp"
#include "Solver/BoundaryCondition.hpp"
#include "Solver/ConvectiveFlux.hpp"
#include "Solver/SimulationControl.hpp"
#include "Solver/SolveControl.hpp"
#include "Solver/ThermalModel.hpp"
#include "Solver/VariableConvertor.hpp"
#include "Solver/ViscousFlux.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Concept.hpp"
#include "Utils/Enum.hpp"

namespace SubrosaDG {

template <typename ElementTrait, typename SimulationControl>
inline void ElementSolver<ElementTrait, SimulationControl, EquationModelEnum::Euler>::calculateElementQuadrature(
    const ElementMesh<ElementTrait>& element_mesh, const ThermalModel<SimulationControl>& thermal_model) {
  Variable<SimulationControl> quadrature_node_variable;
  FluxVariable<SimulationControl> convective_raw_flux;
  Eigen::Matrix<Real, ElementTrait::kDimension, ElementTrait::kDimension> quadrature_node_jacobian_transpose_inverse;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, SimulationControl::kDimension>
      element_quadrature_node_temporary_variable;
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto)                            \
    shared(Eigen::all, Eigen::fix<SimulationControl::kDimension>, Eigen::Dynamic, element_mesh, \
               thermal_model) private(quadrature_node_variable, convective_raw_flux,            \
                                          quadrature_node_jacobian_transpose_inverse,           \
                                          element_quadrature_node_temporary_variable)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < element_mesh.number_; i++) {
    for (Isize j = 0; j < ElementTrait::kQuadratureNumber; j++) {
      quadrature_node_variable.template getFromSelf<ElementTrait>(i, j, element_mesh, *this);
      quadrature_node_variable.calculateComputationalFromConserved(thermal_model);
      calculateConvectiveRawFlux(quadrature_node_variable, convective_raw_flux);
      quadrature_node_jacobian_transpose_inverse.noalias() =
          element_mesh.element_(i).jacobian_transpose_inverse_.col(j).reshaped(ElementTrait::kDimension,
                                                                               ElementTrait::kDimension);
      element_quadrature_node_temporary_variable.noalias() =
          convective_raw_flux.variable_.transpose() * quadrature_node_jacobian_transpose_inverse *
          element_mesh.element_(i).jacobian_determinant_(j) * element_mesh.quadrature_.weight_(j);
      this->element_(i).variable_quadrature_(
          Eigen::all, Eigen::seqN(j * SimulationControl::kDimension, Eigen::fix<SimulationControl::kDimension>)) =
          element_quadrature_node_temporary_variable;
    }
  }
}

template <typename ElementTrait, typename SimulationControl>
inline void ElementSolver<ElementTrait, SimulationControl, EquationModelEnum::NavierStokes>::calculateElementQuadrature(
    const ElementMesh<ElementTrait>& element_mesh, const ThermalModel<SimulationControl>& thermal_model) {
  Variable<SimulationControl> quadrature_node_variable;
  VariableGradient<SimulationControl> quadrature_node_variable_gradient;
  FluxVariable<SimulationControl> convective_raw_flux;
  FluxVariable<SimulationControl> viscous_raw_flux;
  Eigen::Matrix<Real, ElementTrait::kDimension, ElementTrait::kDimension> quadrature_node_jacobian_transpose_inverse;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, SimulationControl::kDimension>
      element_quadrature_node_temporary_variable;
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto)                                   \
    shared(Eigen::all, Eigen::fix<SimulationControl::kDimension>, Eigen::Dynamic, element_mesh,        \
               thermal_model) private(quadrature_node_variable, convective_raw_flux, viscous_raw_flux, \
                                          quadrature_node_jacobian_transpose_inverse,                  \
                                          element_quadrature_node_temporary_variable)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < element_mesh.number_; i++) {
    for (Isize j = 0; j < ElementTrait::kQuadratureNumber; j++) {
      quadrature_node_variable.template getFromSelf<ElementTrait>(i, j, element_mesh, *this);
      quadrature_node_variable.calculateComputationalFromConserved(thermal_model);
      quadrature_node_variable_gradient.template getFromSelf<ElementTrait>(i, j, element_mesh, *this);
      quadrature_node_variable_gradient.calculatePrimitiveFromConserved(thermal_model, quadrature_node_variable);
      calculateConvectiveRawFlux(quadrature_node_variable, convective_raw_flux);
      calculateViscousRawFlux(thermal_model, quadrature_node_variable, quadrature_node_variable_gradient,
                              viscous_raw_flux);
      quadrature_node_jacobian_transpose_inverse.noalias() =
          element_mesh.element_(i).jacobian_transpose_inverse_.col(j).reshaped(ElementTrait::kDimension,
                                                                               ElementTrait::kDimension);
      element_quadrature_node_temporary_variable.noalias() =
          (convective_raw_flux.variable_.transpose() - viscous_raw_flux.variable_.transpose()) *
          quadrature_node_jacobian_transpose_inverse * element_mesh.element_(i).jacobian_determinant_(j) *
          element_mesh.quadrature_.weight_(j);
      this->element_(i).variable_quadrature_(
          Eigen::all, Eigen::seqN(j * SimulationControl::kDimension, Eigen::fix<SimulationControl::kDimension>)) =
          element_quadrature_node_temporary_variable;
    }
  }
}

template <typename ElementTrait, typename SimulationControl>
inline void
ElementSolver<ElementTrait, SimulationControl, EquationModelEnum::NavierStokes>::calculateElementGardientQuadrature(
    const ElementMesh<ElementTrait>& element_mesh) {
  Variable<SimulationControl> quadrature_node_variable;
  Eigen::Matrix<Real, ElementTrait::kDimension, ElementTrait::kDimension> quadrature_node_jacobian_transpose_inverse;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                SimulationControl::kDimension>
      element_quadrature_node_temporary_variable;
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto)                                     \
    shared(Eigen::all, Eigen::fix<SimulationControl::kDimension>, Eigen::Dynamic, element_mesh) private( \
            quadrature_node_variable, element_quadrature_node_temporary_variable)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < element_mesh.number_; i++) {
    for (Isize j = 0; j < ElementTrait::kQuadratureNumber; j++) {
      quadrature_node_variable.template getFromSelf<ElementTrait>(i, j, element_mesh, *this);
      quadrature_node_jacobian_transpose_inverse.noalias() =
          element_mesh.element_(i).jacobian_transpose_inverse_.col(j).reshaped(ElementTrait::kDimension,
                                                                               ElementTrait::kDimension);
      for (Isize k = 0; k < SimulationControl::kConservedVariableNumber; k++) {
        element_quadrature_node_temporary_variable(
            Eigen::seqN(k * SimulationControl::kDimension, Eigen::fix<SimulationControl::kDimension>), Eigen::all) =
            quadrature_node_variable.conserved_(k) * quadrature_node_jacobian_transpose_inverse *
            element_mesh.element_(i).jacobian_determinant_(j) * element_mesh.quadrature_.weight_(j);
      }
      this->element_(i).variable_gradient_volume_quadrature_(
          Eigen::all, Eigen::seqN(j * SimulationControl::kDimension, Eigen::fix<SimulationControl::kDimension>)) =
          element_quadrature_node_temporary_variable;
    }
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::calculateQuadrature(const Mesh<SimulationControl>& mesh,
                                                           const ThermalModel<SimulationControl>& thermal_model) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.calculateElementQuadrature(mesh.line_, thermal_model);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.calculateElementQuadrature(mesh.triangle_, thermal_model);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.calculateElementQuadrature(mesh.quadrangle_, thermal_model);
    }
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::calculateGardientQuadrature(const Mesh<SimulationControl>& mesh) {
  if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.calculateElementGardientQuadrature(mesh.triangle_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.calculateElementGardientQuadrature(mesh.quadrangle_);
    }
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
template <bool IsLeft>
[[nodiscard]] inline Isize AdjacencyElementSolverBase<AdjacencyElementTrait, SimulationControl>::
    getAdjacencyParentElementQuadratureNodeSequenceInParent([[maybe_unused]] const Isize parent_gmsh_type_number,
                                                            const Isize adjacency_sequence_in_parent,
                                                            const Isize qudrature_sequence_in_adjacency) const {
  if (parent_gmsh_type_number == LineTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
    constexpr std::array<int, LineTrait<SimulationControl::kPolynomialOrder>::kAdjacencyNumber + 1>
        kElementAccumulateAdjacencyQuadratureNumber{
            getElementAccumulateAdjacencyQuadratureNumber<ElementEnum::Line, SimulationControl::kPolynomialOrder>()};
    if constexpr (IsLeft) {
      return kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent)] +
             qudrature_sequence_in_adjacency;
    } else {
      return kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent) + 1] -
             (qudrature_sequence_in_adjacency + 1);
    }
  } else if (parent_gmsh_type_number == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
    constexpr std::array<int, TriangleTrait<SimulationControl::kPolynomialOrder>::kAdjacencyNumber + 1>
        kElementAccumulateAdjacencyQuadratureNumber{
            getElementAccumulateAdjacencyQuadratureNumber<ElementEnum::Triangle,
                                                          SimulationControl::kPolynomialOrder>()};
    if constexpr (IsLeft) {
      return kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent)] +
             qudrature_sequence_in_adjacency;
    } else {
      return kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent) + 1] -
             (qudrature_sequence_in_adjacency + 1);
    }
  } else if (parent_gmsh_type_number == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
    constexpr std::array<int, QuadrangleTrait<SimulationControl::kPolynomialOrder>::kAdjacencyNumber + 1>
        kElementAccumulateAdjacencyQuadratureNumber{
            getElementAccumulateAdjacencyQuadratureNumber<ElementEnum::Quadrangle,
                                                          SimulationControl::kPolynomialOrder>()};
    if constexpr (IsLeft) {
      return kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent)] +
             qudrature_sequence_in_adjacency;
    } else {
      return kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent) + 1] -
             (qudrature_sequence_in_adjacency + 1);
    }
  }
  return -1;
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementSolverBase<AdjacencyElementTrait, SimulationControl>::storeAdjacencyElementNodeQuadrature(
    const Isize parent_gmsh_type_number, const Isize parent_index,
    const Isize adjacency_quadrature_node_sequence_in_parent,
    const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>&
        adjacency_element_quadrature_node_temporary_variable,
    Solver<SimulationControl>& solver) {
  if constexpr (SimulationControl::kDimension == 1) {
    solver.line_.element_(parent_index)
        .variable_adjacency_quadrature_.col(adjacency_quadrature_node_sequence_in_parent) =
        adjacency_element_quadrature_node_temporary_variable;
  } else if constexpr (SimulationControl::kDimension == 2) {
    if (parent_gmsh_type_number == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.triangle_.element_(parent_index)
          .variable_adjacency_quadrature_.col(adjacency_quadrature_node_sequence_in_parent) =
          adjacency_element_quadrature_node_temporary_variable;
    } else if (parent_gmsh_type_number == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.quadrangle_.element_(parent_index)
          .variable_adjacency_quadrature_.col(adjacency_quadrature_node_sequence_in_parent) =
          adjacency_element_quadrature_node_temporary_variable;
    }
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl, EquationModelEnum::NavierStokes>::
    storeAdjacencyElementNodeVolumeGardientQuadrature(
        const Isize parent_gmsh_type_number, const Isize parent_index,
        const Isize adjacency_quadrature_node_sequence_in_parent,
        const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>&
            adjacency_element_quadrature_node_temporary_variable,
        Solver<SimulationControl>& solver) {
  if constexpr (SimulationControl::kDimension == 2) {
    if (parent_gmsh_type_number == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.triangle_.element_(parent_index)
          .variable_gradient_volume_adjacency_quadrature_.col(adjacency_quadrature_node_sequence_in_parent) =
          adjacency_element_quadrature_node_temporary_variable;
    } else if (parent_gmsh_type_number == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.quadrangle_.element_(parent_index)
          .variable_gradient_volume_adjacency_quadrature_.col(adjacency_quadrature_node_sequence_in_parent) =
          adjacency_element_quadrature_node_temporary_variable;
    }
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl, EquationModelEnum::NavierStokes>::
    storeAdjacencyElementNodeInterfaceGardientQuadrature(
        const Isize parent_gmsh_type_number, const Isize parent_index, const Isize adjacency_sequence_in_parent,
        const Isize adjacency_quadrature_node_sequence_in_parent,
        const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>&
            adjacency_element_quadrature_node_temporary_variable,
        Solver<SimulationControl>& solver) {
  if constexpr (SimulationControl::kDimension == 2) {
    if (parent_gmsh_type_number == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.triangle_.element_(parent_index)
          .variable_gradient_interface_adjacency_quadrature_(adjacency_sequence_in_parent)
          .col(adjacency_quadrature_node_sequence_in_parent) = adjacency_element_quadrature_node_temporary_variable;
    } else if (parent_gmsh_type_number == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.quadrangle_.element_(parent_index)
          .variable_gradient_interface_adjacency_quadrature_(adjacency_sequence_in_parent)
          .col(adjacency_quadrature_node_sequence_in_parent) = adjacency_element_quadrature_node_temporary_variable;
    }
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl, EquationModelEnum::Euler>::
    calculateInteriorAdjacencyElementQuadrature(const Mesh<SimulationControl>& mesh,
                                                const ThermalModel<SimulationControl>& thermal_model,
                                                Solver<SimulationControl>& solver) {
  const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh =
      mesh.*(std::remove_reference<decltype(mesh)>::type::template getAdjacencyElement<AdjacencyElementTrait>());
  Variable<SimulationControl> left_quadrature_node_variable;
  Variable<SimulationControl> right_quadrature_node_variable;
  Flux<SimulationControl> convective_flux;
  Eigen::Vector<Isize, 2> adjacency_quadrature_node_sequence_in_parent;
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> adjacency_element_quadrature_node_temporary_variable;
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto)                        \
    shared(Eigen::Dynamic, mesh, adjacency_element_mesh, thermal_model, solver) private(    \
            left_quadrature_node_variable, right_quadrature_node_variable, convective_flux, \
                adjacency_quadrature_node_sequence_in_parent, adjacency_element_quadrature_node_temporary_variable)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < adjacency_element_mesh.interior_number_; i++) {
    for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
      const Eigen::Vector<Isize, 2>& parent_index_each_type =
          adjacency_element_mesh.element_(i).parent_index_each_type_;
      const Eigen::Vector<Isize, 2>& adjacency_sequence_in_parent =
          adjacency_element_mesh.element_(i).adjacency_sequence_in_parent_;
      const Eigen::Vector<Isize, 2>& parent_gmsh_type_number =
          adjacency_element_mesh.element_(i).parent_gmsh_type_number_;
      adjacency_quadrature_node_sequence_in_parent(0) =
          this->template getAdjacencyParentElementQuadratureNodeSequenceInParent<true>(
              parent_gmsh_type_number(0), adjacency_sequence_in_parent(0), j);
      adjacency_quadrature_node_sequence_in_parent(1) =
          this->template getAdjacencyParentElementQuadratureNodeSequenceInParent<false>(
              parent_gmsh_type_number(1), adjacency_sequence_in_parent(1), j);
      left_quadrature_node_variable.getFromParent(parent_gmsh_type_number(0), parent_index_each_type(0),
                                                  adjacency_quadrature_node_sequence_in_parent(0), mesh, solver);
      right_quadrature_node_variable.getFromParent(parent_gmsh_type_number(1), parent_index_each_type(1),
                                                   adjacency_quadrature_node_sequence_in_parent(1), mesh, solver);
      left_quadrature_node_variable.calculateComputationalFromConserved(thermal_model);
      right_quadrature_node_variable.calculateComputationalFromConserved(thermal_model);
      calculateConvectiveFlux(thermal_model, adjacency_element_mesh.element_(i).normal_vector_.col(j),
                              left_quadrature_node_variable, right_quadrature_node_variable, convective_flux);
      adjacency_element_quadrature_node_temporary_variable.noalias() =
          convective_flux.result_.normal_variable_ * adjacency_element_mesh.element_(i).jacobian_determinant_(j) *
          adjacency_element_mesh.quadrature_.weight_(j);
      this->storeAdjacencyElementNodeQuadrature(parent_gmsh_type_number(0), parent_index_each_type(0),
                                                adjacency_quadrature_node_sequence_in_parent(0),
                                                adjacency_element_quadrature_node_temporary_variable, solver);
      this->storeAdjacencyElementNodeQuadrature(parent_gmsh_type_number(1), parent_index_each_type(1),
                                                adjacency_quadrature_node_sequence_in_parent(1),
                                                -adjacency_element_quadrature_node_temporary_variable, solver);
    }
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl, EquationModelEnum::NavierStokes>::
    calculateInteriorAdjacencyElementQuadrature(const Mesh<SimulationControl>& mesh,
                                                const ThermalModel<SimulationControl>& thermal_model,
                                                Solver<SimulationControl>& solver) {
  const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh =
      mesh.*(std::remove_reference<decltype(mesh)>::type::template getAdjacencyElement<AdjacencyElementTrait>());
  Variable<SimulationControl> left_quadrature_node_variable;
  VariableGradient<SimulationControl> left_quadrature_node_variable_gradient;
  Variable<SimulationControl> right_quadrature_node_variable;
  VariableGradient<SimulationControl> right_quadrature_node_variable_gradient;
  Flux<SimulationControl> convective_flux;
  Flux<SimulationControl> viscous_flux;
  Eigen::Vector<Isize, 2> adjacency_quadrature_node_sequence_in_parent;
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> adjacency_element_quadrature_node_temporary_variable;
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto)                                               \
    shared(Eigen::Dynamic, mesh, adjacency_element_mesh, thermal_model, solver) private(                           \
            left_quadrature_node_variable, left_quadrature_node_variable_gradient, right_quadrature_node_variable, \
                right_quadrature_node_variable_gradient, convective_flux,                                          \
                adjacency_quadrature_node_sequence_in_parent, adjacency_element_quadrature_node_temporary_variable)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < adjacency_element_mesh.interior_number_; i++) {
    for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
      const Eigen::Vector<Isize, 2>& parent_index_each_type =
          adjacency_element_mesh.element_(i).parent_index_each_type_;
      const Eigen::Vector<Isize, 2>& adjacency_sequence_in_parent =
          adjacency_element_mesh.element_(i).adjacency_sequence_in_parent_;
      const Eigen::Vector<Isize, 2>& parent_gmsh_type_number =
          adjacency_element_mesh.element_(i).parent_gmsh_type_number_;
      adjacency_quadrature_node_sequence_in_parent(0) =
          this->template getAdjacencyParentElementQuadratureNodeSequenceInParent<true>(
              parent_gmsh_type_number(0), adjacency_sequence_in_parent(0), j);
      adjacency_quadrature_node_sequence_in_parent(1) =
          this->template getAdjacencyParentElementQuadratureNodeSequenceInParent<false>(
              parent_gmsh_type_number(1), adjacency_sequence_in_parent(1), j);
      left_quadrature_node_variable.getFromParent(parent_gmsh_type_number(0), parent_index_each_type(0),
                                                  adjacency_quadrature_node_sequence_in_parent(0), mesh, solver);
      right_quadrature_node_variable.getFromParent(parent_gmsh_type_number(1), parent_index_each_type(1),
                                                   adjacency_quadrature_node_sequence_in_parent(1), mesh, solver);
      left_quadrature_node_variable.calculateComputationalFromConserved(thermal_model);
      right_quadrature_node_variable.calculateComputationalFromConserved(thermal_model);
      left_quadrature_node_variable_gradient.template getFromParent<SimulationControl::kViscousFlux>(
          parent_gmsh_type_number(0), parent_index_each_type(0), adjacency_sequence_in_parent(0),
          adjacency_quadrature_node_sequence_in_parent(0), mesh, solver);
      right_quadrature_node_variable_gradient.template getFromParent<SimulationControl::kViscousFlux>(
          parent_gmsh_type_number(1), parent_index_each_type(1), adjacency_sequence_in_parent(1),
          adjacency_quadrature_node_sequence_in_parent(1), mesh, solver);
      left_quadrature_node_variable_gradient.calculatePrimitiveFromConserved(thermal_model,
                                                                             left_quadrature_node_variable);
      right_quadrature_node_variable_gradient.calculatePrimitiveFromConserved(thermal_model,
                                                                              right_quadrature_node_variable);
      calculateConvectiveFlux(thermal_model, adjacency_element_mesh.element_(i).normal_vector_.col(j),
                              left_quadrature_node_variable, right_quadrature_node_variable, convective_flux);
      calculateViscousFlux(thermal_model, adjacency_element_mesh.element_(i).normal_vector_.col(j),
                           left_quadrature_node_variable, left_quadrature_node_variable_gradient,
                           right_quadrature_node_variable, right_quadrature_node_variable_gradient, viscous_flux);
      adjacency_element_quadrature_node_temporary_variable.noalias() =
          (convective_flux.result_.normal_variable_ - viscous_flux.result_.normal_variable_) *
          adjacency_element_mesh.element_(i).jacobian_determinant_(j) * adjacency_element_mesh.quadrature_.weight_(j);
      this->storeAdjacencyElementNodeQuadrature(parent_gmsh_type_number(0), parent_index_each_type(0),
                                                adjacency_quadrature_node_sequence_in_parent(0),
                                                adjacency_element_quadrature_node_temporary_variable, solver);
      this->storeAdjacencyElementNodeQuadrature(parent_gmsh_type_number(1), parent_index_each_type(1),
                                                adjacency_quadrature_node_sequence_in_parent(1),
                                                -adjacency_element_quadrature_node_temporary_variable, solver);
    }
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl, EquationModelEnum::NavierStokes>::
    calculateInteriorAdjacencyElementGardientQuadrature(const Mesh<SimulationControl>& mesh,
                                                        Solver<SimulationControl>& solver) {
  const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh =
      mesh.*(std::remove_reference<decltype(mesh)>::type::template getAdjacencyElement<AdjacencyElementTrait>());
  Variable<SimulationControl> left_quadrature_node_variable;
  Variable<SimulationControl> right_quadrature_node_variable;
  FluxVariable<SimulationControl> gardient_flux;
  Eigen::Vector<Isize, 2> adjacency_quadrature_node_sequence_in_parent;
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>
      adjacency_element_quadrature_node_temporary_variable;
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto)                      \
    shared(Eigen::Dynamic, mesh, adjacency_element_mesh, solver) private(                 \
            left_quadrature_node_variable, right_quadrature_node_variable, gardient_flux, \
                adjacency_quadrature_node_sequence_in_parent, adjacency_element_quadrature_node_temporary_variable)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < adjacency_element_mesh.interior_number_; i++) {
    for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
      const Eigen::Vector<Isize, 2>& parent_index_each_type =
          adjacency_element_mesh.element_(i).parent_index_each_type_;
      const Eigen::Vector<Isize, 2>& adjacency_sequence_in_parent =
          adjacency_element_mesh.element_(i).adjacency_sequence_in_parent_;
      const Eigen::Vector<Isize, 2>& parent_gmsh_type_number =
          adjacency_element_mesh.element_(i).parent_gmsh_type_number_;
      adjacency_quadrature_node_sequence_in_parent(0) =
          this->template getAdjacencyParentElementQuadratureNodeSequenceInParent<true>(
              parent_gmsh_type_number(0), adjacency_sequence_in_parent(0), j);
      adjacency_quadrature_node_sequence_in_parent(1) =
          this->template getAdjacencyParentElementQuadratureNodeSequenceInParent<false>(
              parent_gmsh_type_number(1), adjacency_sequence_in_parent(1), j);
      left_quadrature_node_variable.getFromParent(parent_gmsh_type_number(0), parent_index_each_type(0),
                                                  adjacency_quadrature_node_sequence_in_parent(0), mesh, solver);
      right_quadrature_node_variable.getFromParent(parent_gmsh_type_number(1), parent_index_each_type(1),
                                                   adjacency_quadrature_node_sequence_in_parent(1), mesh, solver);
      calculateVolumeGardientFlux(adjacency_element_mesh.element_(i).normal_vector_.col(j),
                                  left_quadrature_node_variable, right_quadrature_node_variable, gardient_flux);
      adjacency_element_quadrature_node_temporary_variable.noalias() =
          (gardient_flux.variable_ * adjacency_element_mesh.element_(i).jacobian_determinant_(j) *
           adjacency_element_mesh.quadrature_.weight_(j))
              .reshaped(SimulationControl::kConservedVariableNumber * SimulationControl::kDimension, 1);
      this->storeAdjacencyElementNodeVolumeGardientQuadrature(
          parent_gmsh_type_number(0), parent_index_each_type(0), adjacency_quadrature_node_sequence_in_parent(0),
          adjacency_element_quadrature_node_temporary_variable, solver);
      this->storeAdjacencyElementNodeVolumeGardientQuadrature(
          parent_gmsh_type_number(1), parent_index_each_type(1), adjacency_quadrature_node_sequence_in_parent(1),
          -adjacency_element_quadrature_node_temporary_variable, solver);
      calculateInterfaceGardientFlux(adjacency_element_mesh.element_(i).normal_vector_.col(j),
                                     left_quadrature_node_variable, right_quadrature_node_variable, gardient_flux);
      adjacency_element_quadrature_node_temporary_variable.noalias() =
          (gardient_flux.variable_ * adjacency_element_mesh.element_(i).jacobian_determinant_(j) *
           adjacency_element_mesh.quadrature_.weight_(j))
              .reshaped(SimulationControl::kConservedVariableNumber * SimulationControl::kDimension, 1);
      this->storeAdjacencyElementNodeInterfaceGardientQuadrature(
          parent_gmsh_type_number(0), parent_index_each_type(0), adjacency_sequence_in_parent(0),
          adjacency_quadrature_node_sequence_in_parent(0), adjacency_element_quadrature_node_temporary_variable,
          solver);
      this->storeAdjacencyElementNodeInterfaceGardientQuadrature(
          parent_gmsh_type_number(1), parent_index_each_type(1), adjacency_sequence_in_parent(1),
          adjacency_quadrature_node_sequence_in_parent(1), -adjacency_element_quadrature_node_temporary_variable,
          solver);
    }
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl, EquationModelEnum::Euler>::
    calculateBoundaryAdjacencyElementQuadrature(
        const Mesh<SimulationControl>& mesh, const ThermalModel<SimulationControl>& thermal_model,
        const std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition,
        Solver<SimulationControl>& solver) {
  const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh =
      mesh.*(std::remove_reference<decltype(mesh)>::type::template getAdjacencyElement<AdjacencyElementTrait>());
  Variable<SimulationControl> left_quadrature_node_variable;
  Variable<SimulationControl> boundary_quadrature_node_variable;
  Flux<SimulationControl> convective_flux;
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> adjacency_element_quadrature_node_temporary_variable;
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto)                                         \
    shared(Eigen::Dynamic, mesh, adjacency_element_mesh, thermal_model, boundary_condition, solver) private( \
            left_quadrature_node_variable, boundary_quadrature_node_variable, convective_flux,               \
                adjacency_element_quadrature_node_temporary_variable)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = adjacency_element_mesh.interior_number_;
       i < adjacency_element_mesh.boundary_number_ + adjacency_element_mesh.interior_number_; i++) {
    for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
      const Isize parent_index_each_type = adjacency_element_mesh.element_(i).parent_index_each_type_(0);
      const Isize adjacency_sequence_in_parent = adjacency_element_mesh.element_(i).adjacency_sequence_in_parent_(0);
      const Isize parent_gmsh_type_number = adjacency_element_mesh.element_(i).parent_gmsh_type_number_(0);
      const Isize adjacency_quadrature_node_sequence_in_parent =
          this->template getAdjacencyParentElementQuadratureNodeSequenceInParent<true>(parent_gmsh_type_number,
                                                                                       adjacency_sequence_in_parent, j);
      left_quadrature_node_variable.getFromParent(parent_gmsh_type_number, parent_index_each_type,
                                                  adjacency_quadrature_node_sequence_in_parent, mesh, solver);
      left_quadrature_node_variable.calculateComputationalFromConserved(thermal_model);
      boundary_condition.at(adjacency_element_mesh.element_(i).gmsh_physical_index_)
          ->calculateBoundaryVariable(thermal_model, adjacency_element_mesh.element_(i).normal_vector_.col(j),
                                      left_quadrature_node_variable, boundary_quadrature_node_variable);
      calculateConvectiveNormalFlux(adjacency_element_mesh.element_(i).normal_vector_.col(j),
                                    boundary_quadrature_node_variable, convective_flux.result_);
      adjacency_element_quadrature_node_temporary_variable =
          convective_flux.result_.normal_variable_ * adjacency_element_mesh.element_(i).jacobian_determinant_(j) *
          adjacency_element_mesh.quadrature_.weight_(j);
      this->storeAdjacencyElementNodeQuadrature(parent_gmsh_type_number, parent_index_each_type,
                                                adjacency_quadrature_node_sequence_in_parent,
                                                adjacency_element_quadrature_node_temporary_variable, solver);
    }
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl, EquationModelEnum::NavierStokes>::
    calculateBoundaryAdjacencyElementQuadrature(
        const Mesh<SimulationControl>& mesh, const ThermalModel<SimulationControl>& thermal_model,
        const std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition,
        Solver<SimulationControl>& solver) {
  const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh =
      mesh.*(std::remove_reference<decltype(mesh)>::type::template getAdjacencyElement<AdjacencyElementTrait>());
  Variable<SimulationControl> left_quadrature_node_variable;
  VariableGradient<SimulationControl> left_quadrature_node_variable_gradient;
  Variable<SimulationControl> boundary_quadrature_node_variable;
  VariableGradient<SimulationControl> boundary_quadrature_node_variable_gradient;
  Flux<SimulationControl> convective_flux;
  Flux<SimulationControl> viscous_flux;
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> adjacency_element_quadrature_node_temporary_variable;
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto)                                                  \
    shared(Eigen::Dynamic, mesh, adjacency_element_mesh, thermal_model, boundary_condition, solver) private(          \
            left_quadrature_node_variable, left_quadrature_node_variable_gradient, boundary_quadrature_node_variable, \
                boundary_quadrature_node_variable_gradient, convective_flux, viscous_flux,                            \
                adjacency_element_quadrature_node_temporary_variable)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = adjacency_element_mesh.interior_number_;
       i < adjacency_element_mesh.boundary_number_ + adjacency_element_mesh.interior_number_; i++) {
    for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
      const Isize parent_index_each_type = adjacency_element_mesh.element_(i).parent_index_each_type_(0);
      const Isize adjacency_sequence_in_parent = adjacency_element_mesh.element_(i).adjacency_sequence_in_parent_(0);
      const Isize parent_gmsh_type_number = adjacency_element_mesh.element_(i).parent_gmsh_type_number_(0);
      const Isize adjacency_quadrature_node_sequence_in_parent =
          this->template getAdjacencyParentElementQuadratureNodeSequenceInParent<true>(parent_gmsh_type_number,
                                                                                       adjacency_sequence_in_parent, j);
      left_quadrature_node_variable.getFromParent(parent_gmsh_type_number, parent_index_each_type,
                                                  adjacency_quadrature_node_sequence_in_parent, mesh, solver);
      left_quadrature_node_variable.calculateComputationalFromConserved(thermal_model);
      left_quadrature_node_variable_gradient.template getFromParent<SimulationControl::kViscousFlux>(
          parent_gmsh_type_number, parent_index_each_type, adjacency_sequence_in_parent,
          adjacency_quadrature_node_sequence_in_parent, mesh, solver);
      left_quadrature_node_variable_gradient.calculatePrimitiveFromConserved(thermal_model,
                                                                             left_quadrature_node_variable);
      boundary_condition.at(adjacency_element_mesh.element_(i).gmsh_physical_index_)
          ->calculateBoundaryVariable(thermal_model, adjacency_element_mesh.element_(i).normal_vector_.col(j),
                                      left_quadrature_node_variable, boundary_quadrature_node_variable);
      boundary_condition.at(adjacency_element_mesh.element_(i).gmsh_physical_index_)
          ->calculateBoundaryGradientVariable(left_quadrature_node_variable_gradient,
                                              boundary_quadrature_node_variable_gradient);
      calculateConvectiveNormalFlux(adjacency_element_mesh.element_(i).normal_vector_.col(j),
                                    boundary_quadrature_node_variable, convective_flux.result_);
      boundary_condition.at(adjacency_element_mesh.element_(i).gmsh_physical_index_)
          ->calculateBoundaryLeftVariable(left_quadrature_node_variable, boundary_quadrature_node_variable);
      calculateViscousFlux(thermal_model, adjacency_element_mesh.element_(i).normal_vector_.col(j),
                           left_quadrature_node_variable, left_quadrature_node_variable_gradient,
                           boundary_quadrature_node_variable, boundary_quadrature_node_variable_gradient, viscous_flux);
      adjacency_element_quadrature_node_temporary_variable.noalias() =
          (convective_flux.result_.normal_variable_ - viscous_flux.result_.normal_variable_) *
          adjacency_element_mesh.element_(i).jacobian_determinant_(j) * adjacency_element_mesh.quadrature_.weight_(j);
      this->storeAdjacencyElementNodeQuadrature(parent_gmsh_type_number, parent_index_each_type,
                                                adjacency_quadrature_node_sequence_in_parent,
                                                adjacency_element_quadrature_node_temporary_variable, solver);
    }
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl, EquationModelEnum::NavierStokes>::
    calculateBoundaryAdjacencyElementGardientQuadrature(
        const Mesh<SimulationControl>& mesh, const ThermalModel<SimulationControl>& thermal_model,
        const std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition,
        Solver<SimulationControl>& solver) {
  const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh =
      mesh.*(std::remove_reference<decltype(mesh)>::type::template getAdjacencyElement<AdjacencyElementTrait>());
  Variable<SimulationControl> left_quadrature_node_variable;
  Variable<SimulationControl> boundary_quadrature_node_variable;
  FluxVariable<SimulationControl> gardient_flux;
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>
      adjacency_element_quadrature_node_temporary_variable;
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto)                         \
    shared(Eigen::Dynamic, mesh, adjacency_element_mesh, solver) private(                    \
            left_quadrature_node_variable, boundary_quadrature_node_variable, gardient_flux, \
                adjacency_element_quadrature_node_temporary_variable)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = adjacency_element_mesh.interior_number_;
       i < adjacency_element_mesh.boundary_number_ + adjacency_element_mesh.interior_number_; i++) {
    for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
      const Isize parent_index_each_type = adjacency_element_mesh.element_(i).parent_index_each_type_(0);
      const Isize adjacency_sequence_in_parent = adjacency_element_mesh.element_(i).adjacency_sequence_in_parent_(0);
      const Isize parent_gmsh_type_number = adjacency_element_mesh.element_(i).parent_gmsh_type_number_(0);
      const Isize adjacency_quadrature_node_sequence_in_parent =
          this->template getAdjacencyParentElementQuadratureNodeSequenceInParent<true>(parent_gmsh_type_number,
                                                                                       adjacency_sequence_in_parent, j);
      left_quadrature_node_variable.getFromParent(parent_gmsh_type_number, parent_index_each_type,
                                                  adjacency_quadrature_node_sequence_in_parent, mesh, solver);
      left_quadrature_node_variable.calculateComputationalFromConserved(thermal_model);
      boundary_condition.at(adjacency_element_mesh.element_(i).gmsh_physical_index_)
          ->calculateBoundaryVariable(thermal_model, adjacency_element_mesh.element_(i).normal_vector_.col(j),
                                      left_quadrature_node_variable, boundary_quadrature_node_variable);
      boundary_quadrature_node_variable.calculateConservedFromComputational();
      calculateGardientRawFlux(adjacency_element_mesh.element_(i).normal_vector_.col(j),
                               boundary_quadrature_node_variable, gardient_flux);
      adjacency_element_quadrature_node_temporary_variable =
          (gardient_flux.variable_ * adjacency_element_mesh.element_(i).jacobian_determinant_(j) *
           adjacency_element_mesh.quadrature_.weight_(j))
              .reshaped(SimulationControl::kConservedVariableNumber * SimulationControl::kDimension, 1);
      this->storeAdjacencyElementNodeVolumeGardientQuadrature(
          parent_gmsh_type_number, parent_index_each_type, adjacency_quadrature_node_sequence_in_parent,
          adjacency_element_quadrature_node_temporary_variable, solver);
      adjacency_element_quadrature_node_temporary_variable.setZero();
      this->storeAdjacencyElementNodeInterfaceGardientQuadrature(
          parent_gmsh_type_number, parent_index_each_type, adjacency_sequence_in_parent,
          adjacency_quadrature_node_sequence_in_parent, adjacency_element_quadrature_node_temporary_variable, solver);
    }
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::calculateAdjacencyQuadrature(
    const Mesh<SimulationControl>& mesh, const ThermalModel<SimulationControl>& thermal_model,
    const std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->point_.calculateInteriorAdjacencyElementQuadrature(mesh, thermal_model, *this);
    this->point_.calculateBoundaryAdjacencyElementQuadrature(mesh, thermal_model, boundary_condition, *this);
  } else if constexpr (SimulationControl::kDimension == 2) {
    this->line_.calculateInteriorAdjacencyElementQuadrature(mesh, thermal_model, *this);
    this->line_.calculateBoundaryAdjacencyElementQuadrature(mesh, thermal_model, boundary_condition, *this);
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::calculateAdjacencyGardientQuadrature(
    const Mesh<SimulationControl>& mesh, const ThermalModel<SimulationControl>& thermal_model,
    const std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition) {
  if constexpr (SimulationControl::kDimension == 2) {
    this->line_.calculateInteriorAdjacencyElementGardientQuadrature(mesh, *this);
    this->line_.calculateBoundaryAdjacencyElementGardientQuadrature(mesh, thermal_model, boundary_condition, *this);
  }
}

template <typename ElementTrait, typename SimulationControl>
inline void ElementSolverBase<ElementTrait, SimulationControl>::calculateElementResidual(
    const ElementMesh<ElementTrait>& element_mesh) {
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto) shared(Eigen::Dynamic, element_mesh)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < this->number_; i++) {
    // NOTE: Here we split the calculation to trigger eigen's noalias to avoid intermediate variables.
    this->element_(i).variable_residual_.noalias() =
        this->element_(i).variable_quadrature_ * element_mesh.basis_function_.gradient_value_;
    this->element_(i).variable_residual_.noalias() -=
        this->element_(i).variable_adjacency_quadrature_ * element_mesh.basis_function_.adjacency_value_;
  }
}

template <typename ElementTrait, typename SimulationControl>
inline void
ElementSolver<ElementTrait, SimulationControl, EquationModelEnum::NavierStokes>::calculateElementGardientResidual(
    const ElementMesh<ElementTrait>& element_mesh) {
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto) shared(Eigen::Dynamic, element_mesh)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < this->number_; i++) {
    // NOTE: Here we split the calculation to trigger eigen's noalias to avoid intermediate variables.
    this->element_(i).variable_gradient_volume_residual_.noalias() =
        this->element_(i).variable_gradient_volume_adjacency_quadrature_ *
        element_mesh.basis_function_.adjacency_value_;
    this->element_(i).variable_gradient_volume_residual_.noalias() -=
        this->element_(i).variable_gradient_volume_quadrature_ * element_mesh.basis_function_.gradient_value_;
    for (Isize j = 0; j < ElementTrait::kAdjacencyNumber; j++) {
      this->element_(i).variable_gradient_interface_residual_(j).noalias() =
          this->element_(i).variable_gradient_interface_adjacency_quadrature_(j) *
          element_mesh.basis_function_.adjacency_value_;
    }
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

template <typename SimulationControl>
inline void Solver<SimulationControl>::calculateGardientResidual(const Mesh<SimulationControl>& mesh) {
  if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.calculateElementGardientResidual(mesh.triangle_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.calculateElementGardientResidual(mesh.quadrangle_);
    }
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_SPATIAL_DISCRETE_HPP_
