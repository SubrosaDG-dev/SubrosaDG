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
#include "Solver/SourceTerm.hpp"
#include "Solver/ThermalModel.hpp"
#include "Solver/VariableConvertor.hpp"
#include "Solver/ViscousFlux.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Concept.hpp"
#include "Utils/Enum.hpp"

namespace SubrosaDG {

template <typename ElementTrait, typename SimulationControl>
inline void ElementSolver<ElementTrait, SimulationControl, EquationModelEnum::Euler>::calculateElementQuadrature(
    const ElementMesh<ElementTrait>& element_mesh, [[maybe_unused]] const SourceTerm<SimulationControl>& source_term,
    const ThermalModel<SimulationControl>& thermal_model) {
  ElementVariable<ElementTrait, SimulationControl> quadrature_node_variable;
  FluxVariable<SimulationControl> convective_raw_flux;
  [[maybe_unused]] FluxNormalVariable<SimulationControl> source_flux;
  Eigen::Matrix<Real, ElementTrait::kDimension, ElementTrait::kDimension> quadrature_node_jacobian_transpose_inverse;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, SimulationControl::kDimension>
      element_quadrature_node_temporary_variable;
  [[maybe_unused]] Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>
      element_source_quadrature_node_temporary_variable;
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto)                                         \
    shared(Eigen::all, Eigen::fix<SimulationControl::kDimension>, Eigen::Dynamic, element_mesh, source_term, \
               thermal_model) private(quadrature_node_variable, convective_raw_flux, source_flux,            \
                                          quadrature_node_jacobian_transpose_inverse,                        \
                                          element_quadrature_node_temporary_variable,                        \
                                          element_source_quadrature_node_temporary_variable)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < element_mesh.number_; i++) {
    quadrature_node_variable.get(element_mesh, *this, i);
    quadrature_node_variable.calculateComputationalFromConserved(thermal_model);
    for (Isize j = 0; j < ElementTrait::kQuadratureNumber; j++) {
      calculateConvectiveRawFlux(quadrature_node_variable, convective_raw_flux, j);
      quadrature_node_jacobian_transpose_inverse.noalias() =
          element_mesh.element_(i).jacobian_transpose_inverse_.col(j).reshaped(ElementTrait::kDimension,
                                                                               ElementTrait::kDimension);
      element_quadrature_node_temporary_variable.noalias() =
          convective_raw_flux.variable_.transpose() * quadrature_node_jacobian_transpose_inverse *
          element_mesh.element_(i).jacobian_determinant_(j) * element_mesh.quadrature_.weight_(j);
      this->element_(i).variable_quadrature_(
          Eigen::all, Eigen::seqN(j * SimulationControl::kDimension, Eigen::fix<SimulationControl::kDimension>)) =
          element_quadrature_node_temporary_variable;
      if constexpr (SimulationControl::kSourceTerm != SourceTermEnum::None) {
        source_term.calculateSourceTerm(quadrature_node_variable, source_flux, j);
        element_source_quadrature_node_temporary_variable.noalias() =
            source_flux.normal_variable_ * element_mesh.element_(i).jacobian_determinant_(j) *
            element_mesh.quadrature_.weight_(j);
        this->element_(i).variable_source_quadrature_.col(j) = element_source_quadrature_node_temporary_variable;
      }
    }
  }
}

template <typename ElementTrait, typename SimulationControl>
inline void ElementSolver<ElementTrait, SimulationControl, EquationModelEnum::NavierStokes>::calculateElementQuadrature(
    const ElementMesh<ElementTrait>& element_mesh, [[maybe_unused]] const SourceTerm<SimulationControl>& source_term,
    const ThermalModel<SimulationControl>& thermal_model) {
  ElementVariable<ElementTrait, SimulationControl> quadrature_node_variable;
  ElementVariableGradient<ElementTrait, SimulationControl> quadrature_node_variable_gradient;
  FluxVariable<SimulationControl> convective_raw_flux;
  FluxVariable<SimulationControl> viscous_raw_flux;
  [[maybe_unused]] FluxNormalVariable<SimulationControl> source_flux;
  Eigen::Matrix<Real, ElementTrait::kDimension, ElementTrait::kDimension> quadrature_node_jacobian_transpose_inverse;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, SimulationControl::kDimension>
      element_quadrature_node_temporary_variable;
  [[maybe_unused]] Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>
      element_source_quadrature_node_temporary_variable;
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto) shared(                                         \
        Eigen::all, Eigen::fix<SimulationControl::kDimension>, Eigen::Dynamic, element_mesh, source_term,            \
            thermal_model) private(quadrature_node_variable, quadrature_node_variable_gradient, convective_raw_flux, \
                                       viscous_raw_flux, source_flux, quadrature_node_jacobian_transpose_inverse,    \
                                       element_quadrature_node_temporary_variable,                                   \
                                       element_source_quadrature_node_temporary_variable)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < element_mesh.number_; i++) {
    quadrature_node_variable.get(element_mesh, *this, i);
    quadrature_node_variable.calculateComputationalFromConserved(thermal_model);
    quadrature_node_variable_gradient.get(element_mesh, *this, i);
    quadrature_node_variable_gradient.calculatePrimitiveFromConserved(thermal_model, quadrature_node_variable);
    for (Isize j = 0; j < ElementTrait::kQuadratureNumber; j++) {
      calculateConvectiveRawFlux(quadrature_node_variable, convective_raw_flux, j);
      calculateViscousRawFlux(thermal_model, quadrature_node_variable, quadrature_node_variable_gradient,
                              viscous_raw_flux, j);
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
      if constexpr (SimulationControl::kSourceTerm != SourceTermEnum::None) {
        source_term.calculateSourceTerm(quadrature_node_variable, source_flux, j);
        element_source_quadrature_node_temporary_variable.noalias() =
            source_flux.normal_variable_ * element_mesh.element_(i).jacobian_determinant_(j) *
            element_mesh.quadrature_.weight_(j);
        this->element_(i).variable_source_quadrature_.col(j) = element_source_quadrature_node_temporary_variable;
      }
    }
  }
}

template <typename ElementTrait, typename SimulationControl>
inline void
ElementSolver<ElementTrait, SimulationControl, EquationModelEnum::NavierStokes>::calculateElementGardientQuadrature(
    const ElementMesh<ElementTrait>& element_mesh) {
  ElementVariable<ElementTrait, SimulationControl> quadrature_node_variable;
  Eigen::Matrix<Real, ElementTrait::kDimension, ElementTrait::kDimension> quadrature_node_jacobian_transpose_inverse;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                SimulationControl::kDimension>
      element_quadrature_node_temporary_variable;
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto)                                     \
    shared(Eigen::all, Eigen::fix<SimulationControl::kDimension>, Eigen::Dynamic, element_mesh) private( \
            quadrature_node_variable, quadrature_node_jacobian_transpose_inverse,                        \
                element_quadrature_node_temporary_variable)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < element_mesh.number_; i++) {
    quadrature_node_variable.get(element_mesh, *this, i);
    for (Isize j = 0; j < ElementTrait::kQuadratureNumber; j++) {
      quadrature_node_jacobian_transpose_inverse.noalias() =
          element_mesh.element_(i).jacobian_transpose_inverse_.col(j).reshaped(ElementTrait::kDimension,
                                                                               ElementTrait::kDimension);
      for (Isize k = 0; k < SimulationControl::kConservedVariableNumber; k++) {
        element_quadrature_node_temporary_variable(
            Eigen::seqN(k * SimulationControl::kDimension, Eigen::fix<SimulationControl::kDimension>), Eigen::all) =
            quadrature_node_variable.conserved_(k, j) * quadrature_node_jacobian_transpose_inverse *
            element_mesh.element_(i).jacobian_determinant_(j) * element_mesh.quadrature_.weight_(j);
      }
      this->element_(i).variable_gradient_volume_quadrature_(
          Eigen::all, Eigen::seqN(j * SimulationControl::kDimension, Eigen::fix<SimulationControl::kDimension>)) =
          element_quadrature_node_temporary_variable;
    }
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::calculateQuadrature(
    const Mesh<SimulationControl>& mesh, [[maybe_unused]] const SourceTerm<SimulationControl>& source_term,
    const ThermalModel<SimulationControl>& thermal_model) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.calculateElementQuadrature(mesh.line_, source_term, thermal_model);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.calculateElementQuadrature(mesh.triangle_, source_term, thermal_model);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.calculateElementQuadrature(mesh.quadrangle_, source_term, thermal_model);
    }
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.calculateElementQuadrature(mesh.tetrahedron_, source_term, thermal_model);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.calculateElementQuadrature(mesh.pyramid_, source_term, thermal_model);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.calculateElementQuadrature(mesh.hexahedron_, source_term, thermal_model);
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
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.calculateElementGardientQuadrature(mesh.tetrahedron_);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.calculateElementGardientQuadrature(mesh.pyramid_);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.calculateElementGardientQuadrature(mesh.hexahedron_);
    }
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
[[nodiscard]] inline Isize AdjacencyElementSolverBase<AdjacencyElementTrait, SimulationControl>::
    getAdjacencyParentElementAccumulateAdjacencyQuadratureNumber([[maybe_unused]] const Isize parent_gmsh_type_number,
                                                                 const Isize adjacency_sequence_in_parent) {
  if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
    constexpr std::array<int, LineTrait<SimulationControl::kPolynomialOrder>::kAdjacencyNumber + 1>
        kElementAccumulateAdjacencyQuadratureNumber{
            getElementAccumulateAdjacencyQuadratureNumber<ElementEnum::Line, SimulationControl::kPolynomialOrder>()};
    return kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent)];
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
    if (parent_gmsh_type_number == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      constexpr std::array<int, TriangleTrait<SimulationControl::kPolynomialOrder>::kAdjacencyNumber + 1>
          kElementAccumulateAdjacencyQuadratureNumber{
              getElementAccumulateAdjacencyQuadratureNumber<ElementEnum::Triangle,
                                                            SimulationControl::kPolynomialOrder>()};
      return kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent)];
    }
    if (parent_gmsh_type_number == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      constexpr std::array<int, QuadrangleTrait<SimulationControl::kPolynomialOrder>::kAdjacencyNumber + 1>
          kElementAccumulateAdjacencyQuadratureNumber{
              getElementAccumulateAdjacencyQuadratureNumber<ElementEnum::Quadrangle,
                                                            SimulationControl::kPolynomialOrder>()};
      return kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent)];
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
    if (parent_gmsh_type_number == TetrahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      constexpr std::array<int, TetrahedronTrait<SimulationControl::kPolynomialOrder>::kAdjacencyNumber + 1>
          kElementAccumulateAdjacencyQuadratureNumber{
              getElementAccumulateAdjacencyQuadratureNumber<ElementEnum::Tetrahedron,
                                                            SimulationControl::kPolynomialOrder>()};
      return kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent)];
    }
    if (parent_gmsh_type_number == PyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      constexpr std::array<int, PyramidTrait<SimulationControl::kPolynomialOrder>::kAdjacencyNumber + 1>
          kElementAccumulateAdjacencyQuadratureNumber{
              getElementAccumulateAdjacencyQuadratureNumber<ElementEnum::Pyramid,
                                                            SimulationControl::kPolynomialOrder>()};
      return kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent)];
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
    if (parent_gmsh_type_number == PyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      constexpr std::array<int, PyramidTrait<SimulationControl::kPolynomialOrder>::kAdjacencyNumber + 1>
          kElementAccumulateAdjacencyQuadratureNumber{
              getElementAccumulateAdjacencyQuadratureNumber<ElementEnum::Pyramid,
                                                            SimulationControl::kPolynomialOrder>()};
      return kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent)];
    }
    if (parent_gmsh_type_number == HexahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      constexpr std::array<int, HexahedronTrait<SimulationControl::kPolynomialOrder>::kAdjacencyNumber + 1>
          kElementAccumulateAdjacencyQuadratureNumber{
              getElementAccumulateAdjacencyQuadratureNumber<ElementEnum::Hexahedron,
                                                            SimulationControl::kPolynomialOrder>()};
      return kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent)];
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
  if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
    solver.line_.element_(parent_index)
        .variable_adjacency_quadrature_.col(adjacency_quadrature_node_sequence_in_parent) =
        adjacency_element_quadrature_node_temporary_variable;
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
    if (parent_gmsh_type_number == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.triangle_.element_(parent_index)
          .variable_adjacency_quadrature_.col(adjacency_quadrature_node_sequence_in_parent) =
          adjacency_element_quadrature_node_temporary_variable;
    } else if (parent_gmsh_type_number == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.quadrangle_.element_(parent_index)
          .variable_adjacency_quadrature_.col(adjacency_quadrature_node_sequence_in_parent) =
          adjacency_element_quadrature_node_temporary_variable;
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
    if (parent_gmsh_type_number == TetrahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.tetrahedron_.element_(parent_index)
          .variable_adjacency_quadrature_.col(adjacency_quadrature_node_sequence_in_parent) =
          adjacency_element_quadrature_node_temporary_variable;
    } else if (parent_gmsh_type_number == PyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.pyramid_.element_(parent_index)
          .variable_adjacency_quadrature_.col(adjacency_quadrature_node_sequence_in_parent) =
          adjacency_element_quadrature_node_temporary_variable;
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
    if (parent_gmsh_type_number == PyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.pyramid_.element_(parent_index)
          .variable_adjacency_quadrature_.col(adjacency_quadrature_node_sequence_in_parent) =
          adjacency_element_quadrature_node_temporary_variable;
    } else if (parent_gmsh_type_number == HexahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.hexahedron_.element_(parent_index)
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
  if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
    if (parent_gmsh_type_number == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.triangle_.element_(parent_index)
          .variable_gradient_volume_adjacency_quadrature_.col(adjacency_quadrature_node_sequence_in_parent) =
          adjacency_element_quadrature_node_temporary_variable;
    } else if (parent_gmsh_type_number == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.quadrangle_.element_(parent_index)
          .variable_gradient_volume_adjacency_quadrature_.col(adjacency_quadrature_node_sequence_in_parent) =
          adjacency_element_quadrature_node_temporary_variable;
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
    if (parent_gmsh_type_number == TetrahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.tetrahedron_.element_(parent_index)
          .variable_gradient_volume_adjacency_quadrature_.col(adjacency_quadrature_node_sequence_in_parent) =
          adjacency_element_quadrature_node_temporary_variable;
    } else if (parent_gmsh_type_number == PyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.pyramid_.element_(parent_index)
          .variable_gradient_volume_adjacency_quadrature_.col(adjacency_quadrature_node_sequence_in_parent) =
          adjacency_element_quadrature_node_temporary_variable;
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
    if (parent_gmsh_type_number == PyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.pyramid_.element_(parent_index)
          .variable_gradient_volume_adjacency_quadrature_.col(adjacency_quadrature_node_sequence_in_parent) =
          adjacency_element_quadrature_node_temporary_variable;
    } else if (parent_gmsh_type_number == HexahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.hexahedron_.element_(parent_index)
          .variable_gradient_volume_adjacency_quadrature_.col(adjacency_quadrature_node_sequence_in_parent) =
          adjacency_element_quadrature_node_temporary_variable;
    }
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl, EquationModelEnum::NavierStokes>::
    storeAdjacencyElementNodeInterfaceGardientQuadrature(
        const Isize parent_gmsh_type_number, const Isize parent_index,
        const Isize adjacency_quadrature_node_sequence_in_parent,
        const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>&
            adjacency_element_quadrature_node_temporary_variable,
        Solver<SimulationControl>& solver) {
  if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
    if (parent_gmsh_type_number == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.triangle_.element_(parent_index)
          .variable_gradient_interface_adjacency_quadrature_.col(adjacency_quadrature_node_sequence_in_parent) =
          adjacency_element_quadrature_node_temporary_variable;
    } else if (parent_gmsh_type_number == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.quadrangle_.element_(parent_index)
          .variable_gradient_interface_adjacency_quadrature_.col(adjacency_quadrature_node_sequence_in_parent) =
          adjacency_element_quadrature_node_temporary_variable;
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
    if (parent_gmsh_type_number == TetrahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.tetrahedron_.element_(parent_index)
          .variable_gradient_interface_adjacency_quadrature_.col(adjacency_quadrature_node_sequence_in_parent) =
          adjacency_element_quadrature_node_temporary_variable;
    } else if (parent_gmsh_type_number == PyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.pyramid_.element_(parent_index)
          .variable_gradient_interface_adjacency_quadrature_.col(adjacency_quadrature_node_sequence_in_parent) =
          adjacency_element_quadrature_node_temporary_variable;
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
    if (parent_gmsh_type_number == PyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.pyramid_.element_(parent_index)
          .variable_gradient_interface_adjacency_quadrature_.col(adjacency_quadrature_node_sequence_in_parent) =
          adjacency_element_quadrature_node_temporary_variable;
    } else if (parent_gmsh_type_number == HexahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.hexahedron_.element_(parent_index)
          .variable_gradient_interface_adjacency_quadrature_.col(adjacency_quadrature_node_sequence_in_parent) =
          adjacency_element_quadrature_node_temporary_variable;
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
  AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl> left_quadrature_node_variable;
  AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl> right_quadrature_node_variable;
  Flux<SimulationControl> convective_flux;
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> adjacency_element_quadrature_node_temporary_variable;
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto)                        \
    shared(Eigen::Dynamic, mesh, adjacency_element_mesh, thermal_model, solver) private(    \
            left_quadrature_node_variable, right_quadrature_node_variable, convective_flux, \
                adjacency_element_quadrature_node_temporary_variable)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < adjacency_element_mesh.interior_number_; i++) {
    const Isize adjacency_right_rotation = adjacency_element_mesh.element_(i).adjacency_right_rotation_;
    const std::array<int, AdjacencyElementTrait::kQuadratureNumber> adjacency_element_quadrature_sequence{
        getAdjacencyElementQuadratureSequence<AdjacencyElementTrait::kElementType, SimulationControl::kPolynomialOrder>(
            static_cast<int>(adjacency_right_rotation))};
    const Eigen::Vector<Isize, 2>& parent_index_each_type = adjacency_element_mesh.element_(i).parent_index_each_type_;
    const Eigen::Vector<Isize, 2>& adjacency_sequence_in_parent =
        adjacency_element_mesh.element_(i).adjacency_sequence_in_parent_;
    const Eigen::Vector<Isize, 2>& parent_gmsh_type_number =
        adjacency_element_mesh.element_(i).parent_gmsh_type_number_;
    const Isize left_adjacency_accumulate_quadrature_number =
        this->getAdjacencyParentElementAccumulateAdjacencyQuadratureNumber(parent_gmsh_type_number(0),
                                                                           adjacency_sequence_in_parent(0));
    const Isize right_adjacency_accumulate_quadrature_number =
        this->getAdjacencyParentElementAccumulateAdjacencyQuadratureNumber(parent_gmsh_type_number(1),
                                                                           adjacency_sequence_in_parent(1));
    left_quadrature_node_variable.get(mesh, solver, parent_gmsh_type_number(0), parent_index_each_type(0),
                                      adjacency_sequence_in_parent(0));
    right_quadrature_node_variable.get(mesh, solver, parent_gmsh_type_number(1), parent_index_each_type(1),
                                       adjacency_sequence_in_parent(1));
    left_quadrature_node_variable.calculateComputationalFromConserved(thermal_model);
    right_quadrature_node_variable.calculateComputationalFromConserved(thermal_model);
    for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
      calculateConvectiveFlux(thermal_model, adjacency_element_mesh.element_(i).normal_vector_.col(j),
                              left_quadrature_node_variable, right_quadrature_node_variable, convective_flux, j,
                              adjacency_element_quadrature_sequence[static_cast<Usize>(j)]);
      adjacency_element_quadrature_node_temporary_variable.noalias() =
          convective_flux.result_.normal_variable_ * adjacency_element_mesh.element_(i).jacobian_determinant_(j) *
          adjacency_element_mesh.quadrature_.weight_(j);
      this->storeAdjacencyElementNodeQuadrature(parent_gmsh_type_number(0), parent_index_each_type(0),
                                                left_adjacency_accumulate_quadrature_number + j,
                                                adjacency_element_quadrature_node_temporary_variable, solver);
      this->storeAdjacencyElementNodeQuadrature(
          parent_gmsh_type_number(1), parent_index_each_type(1),
          right_adjacency_accumulate_quadrature_number + adjacency_element_quadrature_sequence[static_cast<Usize>(j)],
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
  AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl> left_quadrature_node_variable;
  AdjacencyElementVariableGradient<AdjacencyElementTrait, SimulationControl> left_quadrature_node_variable_gradient;
  AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl> right_quadrature_node_variable;
  AdjacencyElementVariableGradient<AdjacencyElementTrait, SimulationControl> right_quadrature_node_variable_gradient;
  Flux<SimulationControl> convective_flux;
  Flux<SimulationControl> viscous_flux;
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> adjacency_element_quadrature_node_temporary_variable;
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto)                                               \
    shared(Eigen::Dynamic, mesh, adjacency_element_mesh, thermal_model, solver) private(                           \
            left_quadrature_node_variable, left_quadrature_node_variable_gradient, right_quadrature_node_variable, \
                right_quadrature_node_variable_gradient, convective_flux, viscous_flux,                            \
                adjacency_element_quadrature_node_temporary_variable)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < adjacency_element_mesh.interior_number_; i++) {
    const Isize adjacency_right_rotation = adjacency_element_mesh.element_(i).adjacency_right_rotation_;
    const std::array<int, AdjacencyElementTrait::kQuadratureNumber> adjacency_element_quadrature_sequence{
        getAdjacencyElementQuadratureSequence<AdjacencyElementTrait::kElementType, SimulationControl::kPolynomialOrder>(
            static_cast<int>(adjacency_right_rotation))};
    const Eigen::Vector<Isize, 2>& parent_index_each_type = adjacency_element_mesh.element_(i).parent_index_each_type_;
    const Eigen::Vector<Isize, 2>& adjacency_sequence_in_parent =
        adjacency_element_mesh.element_(i).adjacency_sequence_in_parent_;
    const Eigen::Vector<Isize, 2>& parent_gmsh_type_number =
        adjacency_element_mesh.element_(i).parent_gmsh_type_number_;
    const Isize left_adjacency_accumulate_quadrature_number =
        this->getAdjacencyParentElementAccumulateAdjacencyQuadratureNumber(parent_gmsh_type_number(0),
                                                                           adjacency_sequence_in_parent(0));
    const Isize right_adjacency_accumulate_quadrature_number =
        this->getAdjacencyParentElementAccumulateAdjacencyQuadratureNumber(parent_gmsh_type_number(1),
                                                                           adjacency_sequence_in_parent(1));
    left_quadrature_node_variable.get(mesh, solver, parent_gmsh_type_number(0), parent_index_each_type(0),
                                      adjacency_sequence_in_parent(0));
    right_quadrature_node_variable.get(mesh, solver, parent_gmsh_type_number(1), parent_index_each_type(1),
                                       adjacency_sequence_in_parent(1));
    left_quadrature_node_variable.calculateComputationalFromConserved(thermal_model);
    right_quadrature_node_variable.calculateComputationalFromConserved(thermal_model);
    left_quadrature_node_variable_gradient.template get<SimulationControl::kViscousFlux>(
        mesh, solver, parent_gmsh_type_number(0), parent_index_each_type(0), adjacency_sequence_in_parent(0));
    right_quadrature_node_variable_gradient.template get<SimulationControl::kViscousFlux>(
        mesh, solver, parent_gmsh_type_number(1), parent_index_each_type(1), adjacency_sequence_in_parent(1));
    left_quadrature_node_variable_gradient.calculatePrimitiveFromConserved(thermal_model,
                                                                           left_quadrature_node_variable);
    right_quadrature_node_variable_gradient.calculatePrimitiveFromConserved(thermal_model,
                                                                            right_quadrature_node_variable);
    for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
      calculateConvectiveFlux(thermal_model, adjacency_element_mesh.element_(i).normal_vector_.col(j),
                              left_quadrature_node_variable, right_quadrature_node_variable, convective_flux, j,
                              adjacency_element_quadrature_sequence[static_cast<Usize>(j)]);
      calculateViscousFlux(thermal_model, adjacency_element_mesh.element_(i).normal_vector_.col(j),
                           left_quadrature_node_variable, left_quadrature_node_variable_gradient,
                           right_quadrature_node_variable, right_quadrature_node_variable_gradient, viscous_flux, j,
                           adjacency_element_quadrature_sequence[static_cast<Usize>(j)]);
      adjacency_element_quadrature_node_temporary_variable.noalias() =
          (convective_flux.result_.normal_variable_ - viscous_flux.result_.normal_variable_) *
          adjacency_element_mesh.element_(i).jacobian_determinant_(j) * adjacency_element_mesh.quadrature_.weight_(j);
      this->storeAdjacencyElementNodeQuadrature(parent_gmsh_type_number(0), parent_index_each_type(0),
                                                left_adjacency_accumulate_quadrature_number + j,
                                                adjacency_element_quadrature_node_temporary_variable, solver);
      this->storeAdjacencyElementNodeQuadrature(
          parent_gmsh_type_number(1), parent_index_each_type(1),
          right_adjacency_accumulate_quadrature_number + adjacency_element_quadrature_sequence[static_cast<Usize>(j)],
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
  AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl> left_quadrature_node_variable;
  AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl> right_quadrature_node_variable;
  FluxVariable<SimulationControl> gardient_flux;
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>
      adjacency_element_quadrature_node_temporary_variable;
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto)                      \
    shared(Eigen::Dynamic, mesh, adjacency_element_mesh, solver) private(                 \
            left_quadrature_node_variable, right_quadrature_node_variable, gardient_flux, \
                adjacency_element_quadrature_node_temporary_variable)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < adjacency_element_mesh.interior_number_; i++) {
    const Isize adjacency_right_rotation = adjacency_element_mesh.element_(i).adjacency_right_rotation_;
    const std::array<int, AdjacencyElementTrait::kQuadratureNumber> adjacency_element_quadrature_sequence{
        getAdjacencyElementQuadratureSequence<AdjacencyElementTrait::kElementType, SimulationControl::kPolynomialOrder>(
            static_cast<int>(adjacency_right_rotation))};
    const Eigen::Vector<Isize, 2>& parent_index_each_type = adjacency_element_mesh.element_(i).parent_index_each_type_;
    const Eigen::Vector<Isize, 2>& adjacency_sequence_in_parent =
        adjacency_element_mesh.element_(i).adjacency_sequence_in_parent_;
    const Eigen::Vector<Isize, 2>& parent_gmsh_type_number =
        adjacency_element_mesh.element_(i).parent_gmsh_type_number_;
    const Isize left_adjacency_accumulate_quadrature_number =
        this->getAdjacencyParentElementAccumulateAdjacencyQuadratureNumber(parent_gmsh_type_number(0),
                                                                           adjacency_sequence_in_parent(0));
    const Isize right_adjacency_accumulate_quadrature_number =
        this->getAdjacencyParentElementAccumulateAdjacencyQuadratureNumber(parent_gmsh_type_number(1),
                                                                           adjacency_sequence_in_parent(1));
    left_quadrature_node_variable.get(mesh, solver, parent_gmsh_type_number(0), parent_index_each_type(0),
                                      adjacency_sequence_in_parent(0));
    right_quadrature_node_variable.get(mesh, solver, parent_gmsh_type_number(1), parent_index_each_type(1),
                                       adjacency_sequence_in_parent(1));
    for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
      calculateVolumeGardientFlux(adjacency_element_mesh.element_(i).normal_vector_.col(j),
                                  left_quadrature_node_variable, right_quadrature_node_variable, gardient_flux, j,
                                  adjacency_element_quadrature_sequence[static_cast<Usize>(j)]);
      adjacency_element_quadrature_node_temporary_variable.noalias() =
          (gardient_flux.variable_ * adjacency_element_mesh.element_(i).jacobian_determinant_(j) *
           adjacency_element_mesh.quadrature_.weight_(j))
              .reshaped();
      this->storeAdjacencyElementNodeVolumeGardientQuadrature(
          parent_gmsh_type_number(0), parent_index_each_type(0), left_adjacency_accumulate_quadrature_number + j,
          adjacency_element_quadrature_node_temporary_variable, solver);
      this->storeAdjacencyElementNodeVolumeGardientQuadrature(
          parent_gmsh_type_number(1), parent_index_each_type(1),
          right_adjacency_accumulate_quadrature_number + adjacency_element_quadrature_sequence[static_cast<Usize>(j)],
          -adjacency_element_quadrature_node_temporary_variable, solver);
      calculateInterfaceGardientFlux(adjacency_element_mesh.element_(i).normal_vector_.col(j),
                                     left_quadrature_node_variable, right_quadrature_node_variable, gardient_flux, j,
                                     adjacency_element_quadrature_sequence[static_cast<Usize>(j)]);
      adjacency_element_quadrature_node_temporary_variable.noalias() =
          (gardient_flux.variable_ * adjacency_element_mesh.element_(i).jacobian_determinant_(j) *
           adjacency_element_mesh.quadrature_.weight_(j))
              .reshaped();
      this->storeAdjacencyElementNodeInterfaceGardientQuadrature(
          parent_gmsh_type_number(0), parent_index_each_type(0), left_adjacency_accumulate_quadrature_number + j,
          adjacency_element_quadrature_node_temporary_variable, solver);
      this->storeAdjacencyElementNodeInterfaceGardientQuadrature(
          parent_gmsh_type_number(1), parent_index_each_type(1),
          right_adjacency_accumulate_quadrature_number + adjacency_element_quadrature_sequence[static_cast<Usize>(j)],
          adjacency_element_quadrature_node_temporary_variable, solver);
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
  AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl> left_quadrature_node_variable;
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
    const Isize parent_index_each_type = adjacency_element_mesh.element_(i).parent_index_each_type_(0);
    const Isize adjacency_sequence_in_parent = adjacency_element_mesh.element_(i).adjacency_sequence_in_parent_(0);
    const Isize parent_gmsh_type_number = adjacency_element_mesh.element_(i).parent_gmsh_type_number_(0);
    const Isize left_adjacency_accumulate_quadrature_number =
        this->getAdjacencyParentElementAccumulateAdjacencyQuadratureNumber(parent_gmsh_type_number,
                                                                           adjacency_sequence_in_parent);
    left_quadrature_node_variable.get(mesh, solver, parent_gmsh_type_number, parent_index_each_type,
                                      adjacency_sequence_in_parent);
    left_quadrature_node_variable.calculateComputationalFromConserved(thermal_model);
    for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
      boundary_condition.at(adjacency_element_mesh.element_(i).gmsh_physical_index_)
          ->calculateBoundaryVariable(thermal_model, adjacency_element_mesh.element_(i).normal_vector_.col(j),
                                      left_quadrature_node_variable,
                                      this->boundary_dummy_variable_(i - adjacency_element_mesh.interior_number_),
                                      boundary_quadrature_node_variable, j);
      calculateConvectiveNormalFlux(adjacency_element_mesh.element_(i).normal_vector_.col(j),
                                    boundary_quadrature_node_variable, convective_flux.result_, 0);
      adjacency_element_quadrature_node_temporary_variable =
          convective_flux.result_.normal_variable_ * adjacency_element_mesh.element_(i).jacobian_determinant_(j) *
          adjacency_element_mesh.quadrature_.weight_(j);
      this->storeAdjacencyElementNodeQuadrature(parent_gmsh_type_number, parent_index_each_type,
                                                left_adjacency_accumulate_quadrature_number + j,
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
  AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl> left_quadrature_node_variable;
  AdjacencyElementVariableGradient<AdjacencyElementTrait, SimulationControl> left_quadrature_node_variable_gradient;
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
    const Isize parent_index_each_type = adjacency_element_mesh.element_(i).parent_index_each_type_(0);
    const Isize adjacency_sequence_in_parent = adjacency_element_mesh.element_(i).adjacency_sequence_in_parent_(0);
    const Isize parent_gmsh_type_number = adjacency_element_mesh.element_(i).parent_gmsh_type_number_(0);
    const Isize left_adjacency_accumulate_quadrature_number =
        this->getAdjacencyParentElementAccumulateAdjacencyQuadratureNumber(parent_gmsh_type_number,
                                                                           adjacency_sequence_in_parent);
    left_quadrature_node_variable.get(mesh, solver, parent_gmsh_type_number, parent_index_each_type,
                                      adjacency_sequence_in_parent);
    left_quadrature_node_variable.calculateComputationalFromConserved(thermal_model);
    left_quadrature_node_variable_gradient.template get<SimulationControl::kViscousFlux>(
        mesh, solver, parent_gmsh_type_number, parent_index_each_type, adjacency_sequence_in_parent);
    left_quadrature_node_variable_gradient.calculatePrimitiveFromConserved(thermal_model,
                                                                           left_quadrature_node_variable);
    for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
      boundary_condition.at(adjacency_element_mesh.element_(i).gmsh_physical_index_)
          ->calculateBoundaryVariable(thermal_model, adjacency_element_mesh.element_(i).normal_vector_.col(j),
                                      left_quadrature_node_variable,
                                      this->boundary_dummy_variable_(i - adjacency_element_mesh.interior_number_),
                                      boundary_quadrature_node_variable, j);
      calculateConvectiveNormalFlux(adjacency_element_mesh.element_(i).normal_vector_.col(j),
                                    boundary_quadrature_node_variable, convective_flux.result_, 0);
      boundary_condition.at(adjacency_element_mesh.element_(i).gmsh_physical_index_)
          ->calculateBoundaryVariableGradient(left_quadrature_node_variable, left_quadrature_node_variable_gradient,
                                              boundary_quadrature_node_variable,
                                              boundary_quadrature_node_variable_gradient, j);
      calculateViscousFlux(thermal_model, adjacency_element_mesh.element_(i).normal_vector_.col(j),
                           left_quadrature_node_variable, left_quadrature_node_variable_gradient,
                           boundary_quadrature_node_variable, boundary_quadrature_node_variable_gradient, viscous_flux,
                           j, 0);
      adjacency_element_quadrature_node_temporary_variable.noalias() =
          (convective_flux.result_.normal_variable_ - viscous_flux.result_.normal_variable_) *
          adjacency_element_mesh.element_(i).jacobian_determinant_(j) * adjacency_element_mesh.quadrature_.weight_(j);
      this->storeAdjacencyElementNodeQuadrature(parent_gmsh_type_number, parent_index_each_type,
                                                left_adjacency_accumulate_quadrature_number + j,
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
  AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl> left_quadrature_node_variable;
  Variable<SimulationControl> boundary_quadrature_node_volume_gradient_variable;
  Variable<SimulationControl> boundary_quadrature_node_interface_gradient_variable;
  FluxVariable<SimulationControl> gardient_flux;
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>
      adjacency_element_quadrature_node_temporary_variable;
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto)                                         \
    shared(Eigen::Dynamic, mesh, thermal_model, boundary_condition, adjacency_element_mesh, solver) private( \
            left_quadrature_node_variable, boundary_quadrature_node_volume_gradient_variable,                \
                boundary_quadrature_node_interface_gradient_variable, gardient_flux,                         \
                adjacency_element_quadrature_node_temporary_variable)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = adjacency_element_mesh.interior_number_;
       i < adjacency_element_mesh.boundary_number_ + adjacency_element_mesh.interior_number_; i++) {
    const Isize parent_index_each_type = adjacency_element_mesh.element_(i).parent_index_each_type_(0);
    const Isize adjacency_sequence_in_parent = adjacency_element_mesh.element_(i).adjacency_sequence_in_parent_(0);
    const Isize parent_gmsh_type_number = adjacency_element_mesh.element_(i).parent_gmsh_type_number_(0);
    const Isize left_adjacency_accumulate_quadrature_number =
        this->getAdjacencyParentElementAccumulateAdjacencyQuadratureNumber(parent_gmsh_type_number,
                                                                           adjacency_sequence_in_parent);
    left_quadrature_node_variable.get(mesh, solver, parent_gmsh_type_number, parent_index_each_type,
                                      adjacency_sequence_in_parent);
    left_quadrature_node_variable.calculateComputationalFromConserved(thermal_model);
    for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
      boundary_condition.at(adjacency_element_mesh.element_(i).gmsh_physical_index_)
          ->calculateBoundaryGradientVariable(
              thermal_model, adjacency_element_mesh.element_(i).normal_vector_.col(j), left_quadrature_node_variable,
              this->boundary_dummy_variable_(i - adjacency_element_mesh.interior_number_),
              boundary_quadrature_node_volume_gradient_variable, boundary_quadrature_node_interface_gradient_variable,
              j);
      calculateGardientRawFlux(adjacency_element_mesh.element_(i).normal_vector_.col(j),
                               boundary_quadrature_node_volume_gradient_variable, gardient_flux, 0);
      adjacency_element_quadrature_node_temporary_variable.noalias() =
          (gardient_flux.variable_ * adjacency_element_mesh.element_(i).jacobian_determinant_(j) *
           adjacency_element_mesh.quadrature_.weight_(j))
              .reshaped();
      this->storeAdjacencyElementNodeVolumeGardientQuadrature(
          parent_gmsh_type_number, parent_index_each_type, left_adjacency_accumulate_quadrature_number + j,
          adjacency_element_quadrature_node_temporary_variable, solver);
      calculateGardientRawFlux(adjacency_element_mesh.element_(i).normal_vector_.col(j),
                               boundary_quadrature_node_interface_gradient_variable, gardient_flux, 0);
      adjacency_element_quadrature_node_temporary_variable.noalias() =
          (gardient_flux.variable_ * adjacency_element_mesh.element_(i).jacobian_determinant_(j) *
           adjacency_element_mesh.quadrature_.weight_(j))
              .reshaped();
      this->storeAdjacencyElementNodeInterfaceGardientQuadrature(
          parent_gmsh_type_number, parent_index_each_type, left_adjacency_accumulate_quadrature_number + j,
          adjacency_element_quadrature_node_temporary_variable, solver);
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
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasAdjacencyTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.calculateInteriorAdjacencyElementQuadrature(mesh, thermal_model, *this);
      this->triangle_.calculateBoundaryAdjacencyElementQuadrature(mesh, thermal_model, boundary_condition, *this);
    }
    if constexpr (HasAdjacencyQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.calculateInteriorAdjacencyElementQuadrature(mesh, thermal_model, *this);
      this->quadrangle_.calculateBoundaryAdjacencyElementQuadrature(mesh, thermal_model, boundary_condition, *this);
    }
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::calculateAdjacencyGardientQuadrature(
    const Mesh<SimulationControl>& mesh, const ThermalModel<SimulationControl>& thermal_model,
    const std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition) {
  if constexpr (SimulationControl::kDimension == 2) {
    this->line_.calculateInteriorAdjacencyElementGardientQuadrature(mesh, *this);
    this->line_.calculateBoundaryAdjacencyElementGardientQuadrature(mesh, thermal_model, boundary_condition, *this);
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasAdjacencyTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.calculateInteriorAdjacencyElementGardientQuadrature(mesh, *this);
      this->triangle_.calculateBoundaryAdjacencyElementGardientQuadrature(mesh, thermal_model, boundary_condition,
                                                                          *this);
    }
    if constexpr (HasAdjacencyQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.calculateInteriorAdjacencyElementGardientQuadrature(mesh, *this);
      this->quadrangle_.calculateBoundaryAdjacencyElementGardientQuadrature(mesh, thermal_model, boundary_condition,
                                                                            *this);
    }
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
    if constexpr (SimulationControl::kSourceTerm != SourceTermEnum::None) {
      this->element_(i).variable_residual_.noalias() +=
          this->element_(i).variable_source_quadrature_ * element_mesh.basis_function_.value_;
    }
  }
}

template <typename ElementTrait, typename SimulationControl>
inline void
ElementSolver<ElementTrait, SimulationControl, EquationModelEnum::NavierStokes>::calculateElementGardientResidual(
    const ElementMesh<ElementTrait>& element_mesh) {
  [[maybe_unused]] constexpr std::array<int, ElementTrait::kAdjacencyNumber + 1>
      kElementAccumulateAdjacencyQuadratureNumber{
          getElementAccumulateAdjacencyQuadratureNumber<ElementTrait::kElementType,
                                                        SimulationControl::kPolynomialOrder>()};
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto) \
    shared(Eigen::Dynamic, Eigen::all, element_mesh, kElementAccumulateAdjacencyQuadratureNumber)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < this->number_; i++) {
    this->element_(i).variable_gradient_volume_residual_.noalias() =
        this->element_(i).variable_gradient_volume_adjacency_quadrature_ *
        element_mesh.basis_function_.adjacency_value_;
    this->element_(i).variable_gradient_volume_residual_.noalias() -=
        this->element_(i).variable_gradient_volume_quadrature_ * element_mesh.basis_function_.gradient_value_;
    if constexpr (SimulationControl::kViscousFlux == ViscousFluxEnum::BR1) {
      this->element_(i).variable_gradient_interface_residual_.noalias() =
          this->element_(i).variable_gradient_interface_adjacency_quadrature_ *
          element_mesh.basis_function_.adjacency_value_;
    } else if constexpr (SimulationControl::kViscousFlux == ViscousFluxEnum::BR2) {
      for (Isize j = 0; j < ElementTrait::kAdjacencyNumber; j++) {
        this->element_(i).variable_gradient_interface_residual_(j).noalias() =
            this->element_(i).variable_gradient_interface_adjacency_quadrature_(
                Eigen::all, Eigen::seq(kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(j)],
                                       kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(j) + 1] - 1)) *
            element_mesh.basis_function_.adjacency_value_(
                Eigen::seq(kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(j)],
                           kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(j) + 1] - 1),
                Eigen::all);
      }
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
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.calculateElementResidual(mesh.tetrahedron_);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.calculateElementResidual(mesh.pyramid_);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.calculateElementResidual(mesh.hexahedron_);
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
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.calculateElementGardientResidual(mesh.tetrahedron_);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.calculateElementGardientResidual(mesh.pyramid_);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.calculateElementGardientResidual(mesh.hexahedron_);
    }
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_SPATIAL_DISCRETE_HPP_
