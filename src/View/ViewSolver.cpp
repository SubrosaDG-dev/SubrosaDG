/**
 * @file ViewSolver.cpp
 * @brief Implementation of the ViewSolver class for handling view-related computations.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2026-11-23
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_VIEW_SOLVER_CPP_
#define SUBROSA_DG_VIEW_SOLVER_CPP_

#include <Eigen/Core>
#include <array>
#include <cmath>
#include <filesystem>

#include "Solver/VariableConvertor.cpp"
#include "Solver/ViscousFlux.cpp"
#include "Utils/BasicDataType.cpp"
#include "Utils/Concept.cpp"
#include "Utils/Enum.cpp"
#include "View/IOControl.cpp"

namespace SubrosaDG {

template <typename ElementTrait, typename SimulationControl>
struct ViewVariable {
  Eigen::Matrix<Real, SimulationControl::kComputationalVariableNumber, ElementTrait::kAllNodeNumber> computational_;
  Eigen::Matrix<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension,
                ElementTrait::kAllNodeNumber>
      primitive_gradient_;

  void convertComputationalFromConserved(const Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber,
                                                             ElementTrait::kAllNodeNumber>& all_conserved_variable) {
    for (Isize i = 0; i < ElementTrait::kAllNodeNumber; i++) {
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& conserved_variable =
          all_conserved_variable.col(i);
      Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber> computational_variable;
      Variable<SimulationControl>::convertComputationalFromConserved(conserved_variable, computational_variable);
      this->computational_.col(i) = computational_variable;
    }
  }

  void convertPrimitiveGradientFromConservedGradient(
      const Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                          ElementTrait::kAllNodeNumber>& all_conserved_variable_gradient) {
    for (Isize i = 0; i < ElementTrait::kAllNodeNumber; i++) {
      const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>& computational_variable =
          this->computational_.col(i);
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>&
          conserved_variable_gradient = all_conserved_variable_gradient.col(i);
      Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>
          primitive_variable_gradient;
      VariableGradient<SimulationControl>::convertPrimitiveFromConserved(
          computational_variable, conserved_variable_gradient, primitive_variable_gradient);
      this->primitive_gradient_.col(i) = primitive_variable_gradient;
    }
  }

  [[nodiscard]] Real get(const ViewVariableEnum variable_type, const Isize column) const {
    const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>& computational_variable =
        this->computational_.col(column);
    const Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>&
        primitive_variable_gradient = this->primitive_gradient_.col(column);
    switch (variable_type) {
    case ViewVariableEnum::Density:
      return Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
          computational_variable);
    case ViewVariableEnum::Velocity:
      return Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
                 computational_variable)
          .norm();
    case ViewVariableEnum::Temperature:
      return PhysicalModel<SimulationControl, PhysicalModelData>::computeTemperatureFromInternalEnergy(
          Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::InternalEnergy>(
              computational_variable));
    case ViewVariableEnum::Pressure:
      return Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
          computational_variable);
    case ViewVariableEnum::SoundSpeed:
      return PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(
          Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(computational_variable),
          Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(computational_variable));
    case ViewVariableEnum::MachNumber:
      return Variable<SimulationControl>::template getVector<ComputationalVariableEnum::Velocity>(
                 computational_variable)
                 .norm() /
             PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(
                 Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
                     computational_variable),
                 Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
                     computational_variable));
    case ViewVariableEnum::Entropy:
      if constexpr (IsCompressible<SimulationControl::kEquationModel>) {
        return PhysicalModel<SimulationControl, PhysicalModelData>::computeEntropyFromDensityPressure(
            Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(computational_variable),
            Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
                computational_variable));
      } else {
        return 0.0_r;
      }
    case ViewVariableEnum::Vorticity:
      if constexpr (SimulationControl::kDimension == 2) {
        // vorticity = dv/dx - du/dy
        return VariableGradient<SimulationControl>::template getScalar<PrimitiveVariableEnum::VelocityY,
                                                                       VariableGradientEnum::X>(
                   primitive_variable_gradient) -
               VariableGradient<SimulationControl>::template getScalar<PrimitiveVariableEnum::VelocityX,
                                                                       VariableGradientEnum::Y>(
                   primitive_variable_gradient);
      } else if constexpr (SimulationControl::kDimension == 3) {
        // vorticity = sqrt( (dw/dy - dv/dz)^2 + (du/dz - dw/dx)^2 + (dv/dx - du/dy)^2 )
        const Real partial_velocity_z_over_partial_y_minus_partial_velocity_y_over_partial_z =
            VariableGradient<SimulationControl>::template getScalar<PrimitiveVariableEnum::VelocityZ,
                                                                    VariableGradientEnum::Y>(
                primitive_variable_gradient) -
            VariableGradient<SimulationControl>::template getScalar<PrimitiveVariableEnum::VelocityY,
                                                                    VariableGradientEnum::Z>(
                primitive_variable_gradient);
        const Real partial_velocity_x_over_partial_z_minus_partial_velocity_z_over_partial_x =
            VariableGradient<SimulationControl>::template getScalar<PrimitiveVariableEnum::VelocityX,
                                                                    VariableGradientEnum::Z>(
                primitive_variable_gradient) -
            VariableGradient<SimulationControl>::template getScalar<PrimitiveVariableEnum::VelocityZ,
                                                                    VariableGradientEnum::X>(
                primitive_variable_gradient);
        const Real partial_velocity_y_over_partial_x_minus_partial_velocity_x_over_partial_y =
            VariableGradient<SimulationControl>::template getScalar<PrimitiveVariableEnum::VelocityY,
                                                                    VariableGradientEnum::X>(
                primitive_variable_gradient) -
            VariableGradient<SimulationControl>::template getScalar<PrimitiveVariableEnum::VelocityX,
                                                                    VariableGradientEnum::Y>(
                primitive_variable_gradient);
        return std::sqrt(partial_velocity_z_over_partial_y_minus_partial_velocity_y_over_partial_z *
                             partial_velocity_z_over_partial_y_minus_partial_velocity_y_over_partial_z +
                         partial_velocity_x_over_partial_z_minus_partial_velocity_z_over_partial_x *
                             partial_velocity_x_over_partial_z_minus_partial_velocity_z_over_partial_x +
                         partial_velocity_y_over_partial_x_minus_partial_velocity_x_over_partial_y *
                             partial_velocity_y_over_partial_x_minus_partial_velocity_x_over_partial_y);
      }
    case ViewVariableEnum::VelocityX:
      return Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::VelocityX>(
          computational_variable);
    case ViewVariableEnum::VelocityY:
      return Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::VelocityY>(
          computational_variable);
    case ViewVariableEnum::VelocityZ:
      return Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::VelocityZ>(
          computational_variable);
    case ViewVariableEnum::MachNumberX:
      return Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::VelocityX>(
                 computational_variable) /
             PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(
                 Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
                     computational_variable),
                 Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
                     computational_variable));
    case ViewVariableEnum::MachNumberY:
      return Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::VelocityY>(
                 computational_variable) /
             PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(
                 Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
                     computational_variable),
                 Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
                     computational_variable));
    case ViewVariableEnum::MachNumberZ:
      return Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::VelocityZ>(
                 computational_variable) /
             PhysicalModel<SimulationControl, PhysicalModelData>::computeSoundSpeedFromDensityPressure(
                 Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Density>(
                     computational_variable),
                 Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
                     computational_variable));
    case ViewVariableEnum::VorticityX:
      return VariableGradient<SimulationControl>::template getScalar<PrimitiveVariableEnum::VelocityZ,
                                                                     VariableGradientEnum::Y>(
                 primitive_variable_gradient) -
             VariableGradient<SimulationControl>::template getScalar<PrimitiveVariableEnum::VelocityY,
                                                                     VariableGradientEnum::Z>(
                 primitive_variable_gradient);
    case ViewVariableEnum::VorticityY:
      return VariableGradient<SimulationControl>::template getScalar<PrimitiveVariableEnum::VelocityX,
                                                                     VariableGradientEnum::Z>(
                 primitive_variable_gradient) -
             VariableGradient<SimulationControl>::template getScalar<PrimitiveVariableEnum::VelocityZ,
                                                                     VariableGradientEnum::X>(
                 primitive_variable_gradient);
    case ViewVariableEnum::VorticityZ:
      return VariableGradient<SimulationControl>::template getScalar<PrimitiveVariableEnum::VelocityY,
                                                                     VariableGradientEnum::X>(
                 primitive_variable_gradient) -
             VariableGradient<SimulationControl>::template getScalar<PrimitiveVariableEnum::VelocityX,
                                                                     VariableGradientEnum::Y>(
                 primitive_variable_gradient);
    case ViewVariableEnum::HeatFluxX:
      return VariableGradient<SimulationControl>::template getScalar<PrimitiveVariableEnum::Temperature,
                                                                     VariableGradientEnum::X>(
          primitive_variable_gradient);
    case ViewVariableEnum::HeatFluxY:
      return VariableGradient<SimulationControl>::template getScalar<PrimitiveVariableEnum::Temperature,
                                                                     VariableGradientEnum::Y>(
          primitive_variable_gradient);
    case ViewVariableEnum::HeatFluxZ:
      return VariableGradient<SimulationControl>::template getScalar<PrimitiveVariableEnum::Temperature,
                                                                     VariableGradientEnum::Z>(
          primitive_variable_gradient);
    case ViewVariableEnum::QCriterion: {
      const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>& velocity_gradient =
          VariableGradient<SimulationControl>::template getVectorGradient<PrimitiveVariableEnum::Velocity>(
              primitive_variable_gradient);
      return (velocity_gradient.trace() * velocity_gradient.trace() - (velocity_gradient * velocity_gradient).trace()) *
             0.5_r;
    }
    case ViewVariableEnum::Lambda2:
      if constexpr (SimulationControl::kDimension == 3) {
        // const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>& velocity_gradient =
        //     VariableGradient<SimulationControl>::template getVectorGradient<PrimitiveVariableEnum::Velocity>(
        //         primitive_variable_gradient);
        // const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension> symmetric_part =
        //     (velocity_gradient + velocity_gradient.transpose()) * 0.5_r;
        // const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension> antisymmetric_part =
        //     (velocity_gradient - velocity_gradient.transpose()) * 0.5_r;
        // const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension> eigenvalue_matrix =
        //     symmetric_part * symmetric_part + antisymmetric_part.transpose() * antisymmetric_part;
        // Eigen::SelfAdjointEigenSolver<Eigen::Matrix<Real, SimulationControl::kDimension,
        // SimulationControl::kDimension>>
        //     eigen_solver(eigenvalue_matrix, Eigen::EigenvaluesOnly);
        // return eigen_solver.eigenvalues()(1);
      }
    default:
      return 0.0_r;
    }
  }
};

template <typename VolumeElementTrait, typename SimulationControl>
inline void VolumeElementViewSolver<VolumeElementTrait, SimulationControl>::computeVolumeElementViewVariable(
    const VolumeElementSolver<VolumeElementTrait, SimulationControl>& volume_element_solver) {
  for (Isize i = 0; i < volume_element_solver.number_; i++) {
    this->view_variable_(i).convertComputationalFromConserved(
        volume_element_solver.variable_basis_function_coefficient_(i));
    if constexpr (IsEuler<SimulationControl::kEquationModel>) {
      this->view_variable_(i).convertPrimitiveGradientFromConservedGradient(
          volume_element_solver.variable_volume_gradient_basis_function_coefficient_(i));
    } else if constexpr (IsNS<SimulationControl::kEquationModel>) {
      this->view_variable_(i).convertPrimitiveGradientFromConservedGradient(
          volume_element_solver.variable_gradient_basis_function_coefficient_(i));
    }
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline Eigen::Vector<Real, SimulationControl::kDimension> computeWallForce(
    const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh, const Isize element_index,
    const Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, AdjacencyElementTrait::kAllNodeNumber>&
        all_conserved_variable,
    const Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                        AdjacencyElementTrait::kAllNodeNumber>& all_conserved_variable_gradient) {
  Eigen::Vector<Real, SimulationControl::kDimension> force = Eigen::Vector<Real, SimulationControl::kDimension>::Zero();
  for (Isize i = 0; i < AdjacencyElementTrait::kQuadratureNumber; i++) {
    const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> quadrature_node_conserved_variable =
        all_conserved_variable * adjacency_element_mesh.nodal_basis_function_.row(i).transpose();
    Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber> quadrature_node_computational_variable;
    Variable<SimulationControl>::convertComputationalFromConserved(quadrature_node_conserved_variable,
                                                                   quadrature_node_computational_variable);
    const Eigen::Vector<Real, SimulationControl::kDimension> normal_vector =
        adjacency_element_mesh.normal_vector_(element_index).col(i);
    Eigen::Vector<Real, SimulationControl::kDimension> traction =
        Variable<SimulationControl>::template getScalar<ComputationalVariableEnum::Pressure>(
            quadrature_node_computational_variable) *
        normal_vector;
    if constexpr (IsNS<SimulationControl::kEquationModel>) {
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>
          quadrature_node_conserved_variable_gradient =
              all_conserved_variable_gradient * adjacency_element_mesh.nodal_basis_function_.row(i).transpose();
      Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>
          quadrature_node_primitive_variable_gradient;
      VariableGradient<SimulationControl>::convertPrimitiveFromConserved(quadrature_node_computational_variable,
                                                                         quadrature_node_conserved_variable_gradient,
                                                                         quadrature_node_primitive_variable_gradient);
      Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kConservedVariableNumber> viscous_raw_flux;
      ViscousFlux<SimulationControl>::computeRawFlux(quadrature_node_computational_variable,
                                                     quadrature_node_primitive_variable_gradient, viscous_raw_flux);
      traction.noalias() -=
          RawFlux<SimulationControl>::template getVectorDimension<ConservedVariableEnum::Momentum>(viscous_raw_flux) *
          normal_vector;
    }
    force += traction * adjacency_element_mesh.jacobian_determinant_multiply_weight_(element_index)(i);
  }
  return force;
}

template <typename AdjacencyElementTrait, typename SimulationControl>
template <typename VolumeElementTrait>
inline void
AdjacencyElementViewSolver<AdjacencyElementTrait, SimulationControl>::computeAdjacencyPerElementViewVariable(
    const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh, const Solver<SimulationControl>& solver,
    const Isize element_local_index) {
  const Isize element_index = adjacency_element_mesh.interior_number_ + element_local_index;
  const Isize parent_index = adjacency_element_mesh.left_parent_index_each_type_(element_index);
  const Isize adjacency_sequence = adjacency_element_mesh.adjacency_sequence_in_left_parent_(element_index);
  const VolumeElementSolver<VolumeElementTrait, SimulationControl>& volume_element_solver =
      solver.*Solver<SimulationControl>::template getVolumeElement<VolumeElementTrait>();
  const std::array<int, AdjacencyElementTrait::kAllNodeNumber> adjacency_view_node_sequence_in_parent{
      getAdjacencyElementViewNodeSequenceInParent<AdjacencyElementTrait::kElementType,
                                                  SimulationControl::kPolynomialOrder>(
          VolumeElementTrait::kGmshTypeNumber, static_cast<int>(adjacency_sequence))};
  const Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, VolumeElementTrait::kAllBasisFunctionNumber>&
      variable_basis_function_coefficient = volume_element_solver.variable_basis_function_coefficient_(parent_index);
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, AdjacencyElementTrait::kAllNodeNumber>
      all_conserved_variable;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                VolumeElementTrait::kAllBasisFunctionNumber>
      variable_gradient_basis_function_coefficient;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                AdjacencyElementTrait::kAllNodeNumber>
      all_conserved_variable_gradient;
  if constexpr (IsEuler<SimulationControl::kEquationModel>) {
    variable_gradient_basis_function_coefficient =
        volume_element_solver.variable_volume_gradient_basis_function_coefficient_(parent_index);
  } else if constexpr (IsNS<SimulationControl::kEquationModel>) {
    if constexpr (SimulationControl::kViscousFlux == ViscousFluxEnum::BR1) {
      variable_gradient_basis_function_coefficient =
          volume_element_solver.variable_gradient_basis_function_coefficient_(parent_index);
    } else if constexpr (SimulationControl::kViscousFlux == ViscousFluxEnum::BR2) {
      variable_gradient_basis_function_coefficient =
          volume_element_solver.variable_volume_gradient_basis_function_coefficient_(parent_index) +
          volume_element_solver.variable_interface_gradient_basis_function_coefficient_(parent_index)(
              Eigen::placeholders::all, Eigen::seqN(adjacency_sequence * VolumeElementTrait::kAllBasisFunctionNumber,
                                                    Eigen::fix<VolumeElementTrait::kAllBasisFunctionNumber>));
    }
  }
  for (Isize i = 0; i < AdjacencyElementTrait::kAllNodeNumber; i++) {
    all_conserved_variable.col(i) =
        variable_basis_function_coefficient.col(adjacency_view_node_sequence_in_parent[static_cast<Usize>(i)]);
    all_conserved_variable_gradient.col(i) =
        variable_gradient_basis_function_coefficient.col(adjacency_view_node_sequence_in_parent[static_cast<Usize>(i)]);
  }
  this->view_variable_(element_local_index).convertComputationalFromConserved(all_conserved_variable);
  this->view_variable_(element_local_index)
      .convertPrimitiveGradientFromConservedGradient(all_conserved_variable_gradient);
  if constexpr (SimulationControl::kDimension >= 2) {
    if (isWall(adjacency_element_mesh.boundary_condition_type_(element_index))) {
      this->force_.col(element_local_index) = computeWallForce<AdjacencyElementTrait, SimulationControl>(
          adjacency_element_mesh, element_index, all_conserved_variable, all_conserved_variable_gradient);
    }
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementViewSolver<AdjacencyElementTrait, SimulationControl>::computeAdjacencyElementViewVariable(
    const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
    const Solver<SimulationControl>& solver) {
  if constexpr (SimulationControl::kDimension >= 2) {
    this->force_.resize(Eigen::NoChange, adjacency_element_mesh.boundary_number_);
  }
  for (Isize i = 0; i < adjacency_element_mesh.boundary_number_; i++) {
    const Isize element_index = i + adjacency_element_mesh.interior_number_;
    const Isize parent_gmsh_type_number = adjacency_element_mesh.left_parent_gmsh_type_number_(element_index);
    if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
      this->computeAdjacencyPerElementViewVariable<VolumeLineTrait<SimulationControl::kPolynomialOrder>>(
          adjacency_element_mesh, solver, i);
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
      if (parent_gmsh_type_number == VolumeTriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->computeAdjacencyPerElementViewVariable<VolumeTriangleTrait<SimulationControl::kPolynomialOrder>>(
            adjacency_element_mesh, solver, i);
      } else if (parent_gmsh_type_number ==
                 VolumeQuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->computeAdjacencyPerElementViewVariable<VolumeQuadrangleTrait<SimulationControl::kPolynomialOrder>>(
            adjacency_element_mesh, solver, i);
      }
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
      if (parent_gmsh_type_number == VolumeTetrahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->computeAdjacencyPerElementViewVariable<VolumeTetrahedronTrait<SimulationControl::kPolynomialOrder>>(
            adjacency_element_mesh, solver, i);
      } else if (parent_gmsh_type_number == VolumePyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->computeAdjacencyPerElementViewVariable<VolumePyramidTrait<SimulationControl::kPolynomialOrder>>(
            adjacency_element_mesh, solver, i);
      }
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
      if (parent_gmsh_type_number == VolumePyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->computeAdjacencyPerElementViewVariable<VolumePyramidTrait<SimulationControl::kPolynomialOrder>>(
            adjacency_element_mesh, solver, i);
      } else if (parent_gmsh_type_number ==
                 VolumeHexahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->computeAdjacencyPerElementViewVariable<VolumeHexahedronTrait<SimulationControl::kPolynomialOrder>>(
            adjacency_element_mesh, solver, i);
      }
    }
  }
}

template <typename SimulationControl>
inline void ViewSolver<SimulationControl>::computeViewVariable(const Mesh<SimulationControl>& mesh,
                                                               const Solver<SimulationControl>& solver) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.computeVolumeElementViewVariable(solver.line_);
    this->point_.computeAdjacencyElementViewVariable(mesh.point_, solver);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.computeVolumeElementViewVariable(solver.triangle_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.computeVolumeElementViewVariable(solver.quadrangle_);
    }
    this->line_.computeAdjacencyElementViewVariable(mesh.line_, solver);
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.computeVolumeElementViewVariable(solver.tetrahedron_);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.computeVolumeElementViewVariable(solver.pyramid_);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.computeVolumeElementViewVariable(solver.hexahedron_);
    }
    if constexpr (HasAdjacencyTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.computeAdjacencyElementViewVariable(mesh.triangle_, solver);
    }
    if constexpr (HasAdjacencyQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.computeAdjacencyElementViewVariable(mesh.quadrangle_, solver);
    }
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::stepView(Mesh<SimulationControl>& mesh,
                                                ViewSolver<SimulationControl>& view_solver,
                                                const std::filesystem::path& raw_binary_path) {
  this->readRawBinary(raw_binary_path, view_solver.time_);
  this->rotateMesh(mesh, view_solver.time_);
  this->updateBoundaryVariable(mesh, view_solver.time_);
  this->updateInterfaceVariable(mesh);
  this->computeVolumeGradientQuadrature(mesh);
  this->computeAdjacencyGradientQuadrature(mesh);
  this->updateGradientBasisFunctionCoefficient(mesh);
  view_solver.computeViewVariable(mesh, *this);
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_VIEW_SOLVER_CPP_
