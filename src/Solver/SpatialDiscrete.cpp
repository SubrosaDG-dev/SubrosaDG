/**
 * @file SpatialDiscrete.cpp
 * @brief The header file of SubrosaDG spatial discrete.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-16
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_SPATIAL_DISCRETE_CPP_
#define SUBROSA_DG_SPATIAL_DISCRETE_CPP_

#include <oneapi/tbb.h>

#include <Eigen/Core>
#include <array>
#include <sycl/sycl.hpp>

#include "Mesh/ReadControl.cpp"
#include "Solver/ConvectiveFlux.cpp"
#include "Solver/SimulationControl.cpp"
#include "Solver/SolveControl.cpp"
#include "Solver/SourceTerm.cpp"
#include "Solver/VariableConvertor.cpp"
#include "Solver/ViscousFlux.cpp"
#include "Utils/BasicDataType.cpp"
#include "Utils/Concept.cpp"
#include "Utils/Constant.cpp"
#include "Utils/Enum.cpp"

namespace SubrosaDG {

template <typename AdjacencyElementTrait, typename SimulationControl>
[[nodiscard]] inline Isize getAdjacencyElementAccumulateQuadratureNumber(
    [[maybe_unused]] const Isize parent_gmsh_type_number, const Isize adjacency_sequence_in_parent) {
  if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
    constexpr std::array<int, VolumeLineTrait<SimulationControl::kPolynomialOrder>::kAdjacencyNumber + 1>
        kAdjacencyQuadratureSequence{
            getVolumeElementAdjacencyQuadratureSequence<ElementEnum::Line, SimulationControl::kPolynomialOrder>()};
    return kAdjacencyQuadratureSequence[static_cast<Usize>(adjacency_sequence_in_parent)];
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
    if (parent_gmsh_type_number == VolumeTriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      constexpr std::array<int, VolumeTriangleTrait<SimulationControl::kPolynomialOrder>::kAdjacencyNumber + 1>
          kAdjacencyQuadratureSequence{
              getVolumeElementAdjacencyQuadratureSequence<ElementEnum::Triangle,
                                                          SimulationControl::kPolynomialOrder>()};
      return kAdjacencyQuadratureSequence[static_cast<Usize>(adjacency_sequence_in_parent)];
    }
    if (parent_gmsh_type_number == VolumeQuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      constexpr std::array<int, VolumeQuadrangleTrait<SimulationControl::kPolynomialOrder>::kAdjacencyNumber + 1>
          kAdjacencyQuadratureSequence{
              getVolumeElementAdjacencyQuadratureSequence<ElementEnum::Quadrangle,
                                                          SimulationControl::kPolynomialOrder>()};
      return kAdjacencyQuadratureSequence[static_cast<Usize>(adjacency_sequence_in_parent)];
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
    if (parent_gmsh_type_number == VolumeTetrahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      constexpr std::array<int, VolumeTetrahedronTrait<SimulationControl::kPolynomialOrder>::kAdjacencyNumber + 1>
          kAdjacencyQuadratureSequence{
              getVolumeElementAdjacencyQuadratureSequence<ElementEnum::Tetrahedron,
                                                          SimulationControl::kPolynomialOrder>()};
      return kAdjacencyQuadratureSequence[static_cast<Usize>(adjacency_sequence_in_parent)];
    }
    if (parent_gmsh_type_number == VolumePyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      constexpr std::array<int, VolumePyramidTrait<SimulationControl::kPolynomialOrder>::kAdjacencyNumber + 1>
          kAdjacencyQuadratureSequence{
              getVolumeElementAdjacencyQuadratureSequence<ElementEnum::Pyramid, SimulationControl::kPolynomialOrder>()};
      return kAdjacencyQuadratureSequence[static_cast<Usize>(adjacency_sequence_in_parent)];
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
    if (parent_gmsh_type_number == VolumePyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      constexpr std::array<int, VolumePyramidTrait<SimulationControl::kPolynomialOrder>::kAdjacencyNumber + 1>
          kAdjacencyQuadratureSequence{
              getVolumeElementAdjacencyQuadratureSequence<ElementEnum::Pyramid, SimulationControl::kPolynomialOrder>()};
      return kAdjacencyQuadratureSequence[static_cast<Usize>(adjacency_sequence_in_parent)];
    }
    if (parent_gmsh_type_number == VolumeHexahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      constexpr std::array<int, VolumeHexahedronTrait<SimulationControl::kPolynomialOrder>::kAdjacencyNumber + 1>
          kAdjacencyQuadratureSequence{
              getVolumeElementAdjacencyQuadratureSequence<ElementEnum::Hexahedron,
                                                          SimulationControl::kPolynomialOrder>()};
      return kAdjacencyQuadratureSequence[static_cast<Usize>(adjacency_sequence_in_parent)];
    }
  }
}

template <typename VolumeElementTrait, typename SimulationControl>
inline void VolumeElementSolver<VolumeElementTrait, SimulationControl>::computeVolumeElementQuadrature(
    const VolumeElementMesh<VolumeElementTrait>& volume_element_mesh,
    [[maybe_unused]] const SourceTerm<SimulationControl>& source_term) {
  tbb::parallel_for(tbb::blocked_range<Isize>(0, this->number_), [&](const tbb::blocked_range<Isize>& range) -> void {
    for (Isize i = range.begin(); i != range.end(); i++) {
      if constexpr (IsEuler<SimulationControl::kEquationModel>) {
        Eigen::Vector<Real, SimulationControl::kDimension> quadrature_node_rotation_velocity;
        Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> quadrature_node_conserved_variable;
        Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber> quadrature_node_computational_variable;
        for (Isize j = 0; j < VolumeElementTrait::kQuadratureNumber; j++) {
          VolumeElementVariable<VolumeElementTrait, SimulationControl>::getRotationVelocity(
              volume_element_mesh, *this, quadrature_node_rotation_velocity, i, j);
          VolumeElementVariable<VolumeElementTrait, SimulationControl>::get(volume_element_mesh, *this,
                                                                            quadrature_node_conserved_variable, i, j);
          Variable<SimulationControl>::convertComputationalFromConserved(quadrature_node_conserved_variable,
                                                                         quadrature_node_computational_variable);
          const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>&
              jacobian_transpose_inverse_multiply_determinate_and_weight =
                  volume_element_mesh.jacobian_transpose_inverse_multiply_determinate_and_weight_(i)(
                      Eigen::placeholders::all,
                      Eigen::seqN(j * SimulationControl::kDimension, Eigen::fix<SimulationControl::kDimension>));
          Eigen::Ref<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, SimulationControl::kDimension>>
              quadrature_node_variable_quadrature = this->variable_quadrature_(i)(
                  Eigen::placeholders::all,
                  Eigen::seqN(j * SimulationControl::kDimension, Eigen::fix<SimulationControl::kDimension>));
          quadrature_node_variable_quadrature.setZero();
          ConvectiveFlux<SimulationControl>::addVariableQuadratureRawFlux(
              quadrature_node_rotation_velocity, quadrature_node_conserved_variable,
              quadrature_node_computational_variable, jacobian_transpose_inverse_multiply_determinate_and_weight,
              quadrature_node_variable_quadrature);
          if constexpr (SimulationControl::kSourceTerm != SourceTermEnum::None) {
            Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>>
                quadrature_node_source_quadrature = this->variable_source_quadrature_(i).col(j);
            source_term.computeSourceQuadrature(quadrature_node_computational_variable,
                                                quadrature_node_source_quadrature,
                                                volume_element_mesh.jacobian_determinant_multiply_weight_(i)(j));
          }
        }
      } else if constexpr (IsNS<SimulationControl::kEquationModel>) {
        Eigen::Vector<Real, SimulationControl::kDimension> quadrature_node_rotation_velocity;
        Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> quadrature_node_conserved_variable;
        Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber> quadrature_node_computational_variable;
        Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>
            quadrature_node_conserved_variable_gradient;
        Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>
            quadrature_node_primitive_variable_gradient;
        for (Isize j = 0; j < VolumeElementTrait::kQuadratureNumber; j++) {
          VolumeElementVariable<VolumeElementTrait, SimulationControl>::getRotationVelocity(
              volume_element_mesh, *this, quadrature_node_rotation_velocity, i, j);
          VolumeElementVariable<VolumeElementTrait, SimulationControl>::get(volume_element_mesh, *this,
                                                                            quadrature_node_conserved_variable, i, j);
          Variable<SimulationControl>::convertComputationalFromConserved(quadrature_node_conserved_variable,
                                                                         quadrature_node_computational_variable);
          VolumeElementVariableGradient<VolumeElementTrait, SimulationControl>::template get<
              SimulationControl::kViscousFlux>(volume_element_mesh, *this, quadrature_node_conserved_variable_gradient,
                                               i, j);
          VariableGradient<SimulationControl>::convertPrimitiveFromConserved(
              quadrature_node_computational_variable, quadrature_node_conserved_variable_gradient,
              quadrature_node_primitive_variable_gradient);
          const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>&
              jacobian_transpose_inverse_multiply_determinate_and_weight =
                  volume_element_mesh.jacobian_transpose_inverse_multiply_determinate_and_weight_(i)(
                      Eigen::placeholders::all,
                      Eigen::seqN(j * SimulationControl::kDimension, Eigen::fix<SimulationControl::kDimension>));
          Eigen::Ref<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, SimulationControl::kDimension>>
              quadrature_node_variable_quadrature = this->variable_quadrature_(i)(
                  Eigen::placeholders::all,
                  Eigen::seqN(j * SimulationControl::kDimension, Eigen::fix<SimulationControl::kDimension>));
          quadrature_node_variable_quadrature.setZero();
          ConvectiveFlux<SimulationControl>::addVariableQuadratureRawFlux(
              quadrature_node_rotation_velocity, quadrature_node_conserved_variable,
              quadrature_node_computational_variable, jacobian_transpose_inverse_multiply_determinate_and_weight,
              quadrature_node_variable_quadrature);
          ViscousFlux<SimulationControl>::minusVariableQuadratureRawFlux(
              quadrature_node_computational_variable, quadrature_node_primitive_variable_gradient,
              jacobian_transpose_inverse_multiply_determinate_and_weight, quadrature_node_variable_quadrature);
          if constexpr (SimulationControl::kSourceTerm != SourceTermEnum::None) {
            Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>>
                quadrature_node_source_quadrature = this->variable_source_quadrature_(i).col(j);
            source_term.computeSourceQuadrature(quadrature_node_computational_variable,
                                                quadrature_node_source_quadrature,
                                                volume_element_mesh.jacobian_determinant_multiply_weight_(i)(j));
          }
        }
      }
    }
  });
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::computeVolumeQuadrature(
    const Mesh<SimulationControl>& mesh, [[maybe_unused]] const SourceTerm<SimulationControl>& source_term) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.computeVolumeElementQuadrature(mesh.line_, source_term);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.computeVolumeElementQuadrature(mesh.triangle_, source_term);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.computeVolumeElementQuadrature(mesh.quadrangle_, source_term);
    }
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.computeVolumeElementQuadrature(mesh.tetrahedron_, source_term);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.computeVolumeElementQuadrature(mesh.pyramid_, source_term);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.computeVolumeElementQuadrature(mesh.hexahedron_, source_term);
    }
  }
}

template <typename VolumeElementTrait, typename SimulationControl>
inline void VolumeElementSolverDevice<VolumeElementTrait, SimulationControl>::computeVolumeElementQuadrature(
    const VolumeElementMeshDevice<VolumeElementTrait>& volume_element_mesh,
    [[maybe_unused]] const SourceTermDevice<SimulationControl>& source_term) {
  queue.submit([&](sycl::handler& cgh) -> void {
    cgh.parallel_for(getNdRange(this->number_), [=, this](sycl::nd_item<1> index) -> void {
      const auto i = static_cast<Isize>(index.get_global_id(0));
      if (i >= this->number_) {
        return;
      }
      if constexpr (IsEuler<SimulationControl::kEquationModel>) {
        Device::StaticVector<Real, SimulationControl::kDimension> quadrature_node_rotation_velocity;
        Device::StaticVector<Real, SimulationControl::kConservedVariableNumber> quadrature_node_conserved_variable;
        Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>
            quadrature_node_computational_variable;
        for (Isize j = 0; j < VolumeElementTrait::kQuadratureNumber; j++) {
          VolumeElementVariableDevice<VolumeElementTrait, SimulationControl>::getRotationVelocity(
              volume_element_mesh, *this, quadrature_node_rotation_velocity, i, j);
          VolumeElementVariableDevice<VolumeElementTrait, SimulationControl>::get(
              volume_element_mesh, *this, quadrature_node_conserved_variable, i, j);
          VariableDevice<SimulationControl>::convertComputationalFromConserved(quadrature_node_conserved_variable,
                                                                               quadrature_node_computational_variable);
          const Device::View<const Device::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>>
              jacobian_transpose_inverse_multiply_determinate_and_weight =
                  volume_element_mesh.jacobian_transpose_inverse_multiply_determinate_and_weight_.slice(
                      i, volume_element_mesh.number_, Device::Slice<SimulationControl::kDimension>::all(),
                      Device::Slice<SimulationControl::kDimension>::seqN(j * SimulationControl::kDimension));
          Device::View<Device::Matrix<Real, SimulationControl::kConservedVariableNumber, SimulationControl::kDimension>>
              quadrature_node_variable_quadrature = this->variable_quadrature_.slice(
                  i, this->number_, Device::Slice<SimulationControl::kConservedVariableNumber>::all(),
                  Device::Slice<SimulationControl::kDimension>::seqN(j * SimulationControl::kDimension));
          quadrature_node_variable_quadrature.setZero();
          ConvectiveFluxDevice<SimulationControl>::addVariableQuadratureRawFlux(
              quadrature_node_rotation_velocity, quadrature_node_conserved_variable,
              quadrature_node_computational_variable, jacobian_transpose_inverse_multiply_determinate_and_weight,
              quadrature_node_variable_quadrature);
          if constexpr (SimulationControl::kSourceTerm != SourceTermEnum::None) {
            Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber>>
                quadrature_node_source_quadrature = this->variable_source_quadrature_.slice(
                    i, this->number_, Device::Slice<SimulationControl::kConservedVariableNumber>::all(),
                    Device::Slice<1>::seqN(j));
            const Device::View<const Device::Vector<Real, VolumeElementTrait::kQuadratureNumber>>
                jacobian_determinant_multiply_weight =
                    volume_element_mesh.jacobian_determinant_multiply_weight_.view(i, volume_element_mesh.number_);
            source_term.computeSourceQuadrature(quadrature_node_computational_variable,
                                                quadrature_node_source_quadrature,
                                                jacobian_determinant_multiply_weight(j));
          }
        }
      } else if constexpr (IsNS<SimulationControl::kEquationModel>) {
        Device::StaticVector<Real, SimulationControl::kDimension> quadrature_node_rotation_velocity;
        Device::StaticVector<Real, SimulationControl::kConservedVariableNumber> quadrature_node_conserved_variable;
        Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>
            quadrature_node_computational_variable;
        Device::StaticVector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>
            quadrature_node_conserved_variable_gradient;
        Device::StaticVector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>
            quadrature_node_primitive_variable_gradient;
        for (Isize j = 0; j < VolumeElementTrait::kQuadratureNumber; j++) {
          VolumeElementVariableDevice<VolumeElementTrait, SimulationControl>::getRotationVelocity(
              volume_element_mesh, *this, quadrature_node_rotation_velocity, i, j);
          VolumeElementVariableDevice<VolumeElementTrait, SimulationControl>::get(
              volume_element_mesh, *this, quadrature_node_conserved_variable, i, j);
          VariableDevice<SimulationControl>::convertComputationalFromConserved(quadrature_node_conserved_variable,
                                                                               quadrature_node_computational_variable);
          VolumeElementVariableGradientDevice<VolumeElementTrait, SimulationControl>::template get<
              SimulationControl::kViscousFlux>(volume_element_mesh, *this, quadrature_node_conserved_variable_gradient,
                                               i, j);
          VariableGradientDevice<SimulationControl>::convertPrimitiveFromConserved(
              quadrature_node_computational_variable, quadrature_node_conserved_variable_gradient,
              quadrature_node_primitive_variable_gradient);
          const Device::View<const Device::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>>
              jacobian_transpose_inverse_multiply_determinate_and_weight =
                  volume_element_mesh.jacobian_transpose_inverse_multiply_determinate_and_weight_.slice(
                      i, volume_element_mesh.number_, Device::Slice<SimulationControl::kDimension>::all(),
                      Device::Slice<SimulationControl::kDimension>::seqN(j * SimulationControl::kDimension));
          Device::View<Device::Matrix<Real, SimulationControl::kConservedVariableNumber, SimulationControl::kDimension>>
              quadrature_node_variable_quadrature = this->variable_quadrature_.slice(
                  i, this->number_, Device::Slice<SimulationControl::kConservedVariableNumber>::all(),
                  Device::Slice<SimulationControl::kDimension>::seqN(j * SimulationControl::kDimension));
          quadrature_node_variable_quadrature.setZero();
          ConvectiveFluxDevice<SimulationControl>::addVariableQuadratureRawFlux(
              quadrature_node_rotation_velocity, quadrature_node_conserved_variable,
              quadrature_node_computational_variable, jacobian_transpose_inverse_multiply_determinate_and_weight,
              quadrature_node_variable_quadrature);
          ViscousFluxDevice<SimulationControl>::minusVariableQuadratureRawFlux(
              quadrature_node_computational_variable, quadrature_node_primitive_variable_gradient,
              jacobian_transpose_inverse_multiply_determinate_and_weight, quadrature_node_variable_quadrature);
          if constexpr (SimulationControl::kSourceTerm != SourceTermEnum::None) {
            Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber>>
                quadrature_node_source_quadrature = this->variable_source_quadrature_.slice(
                    i, this->number_, Device::Slice<SimulationControl::kConservedVariableNumber>::all(),
                    Device::Slice<1>::seqN(j));
            const Device::View<const Device::Vector<Real, VolumeElementTrait::kQuadratureNumber>>
                jacobian_determinant_multiply_weight =
                    volume_element_mesh.jacobian_determinant_multiply_weight_.view(i, volume_element_mesh.number_);
            source_term.computeSourceQuadrature(quadrature_node_computational_variable,
                                                quadrature_node_source_quadrature,
                                                jacobian_determinant_multiply_weight(j));
          }
        }
      }
    });
  });
}

template <typename SimulationControl>
inline void SolverDevice<SimulationControl>::computeVolumeQuadrature(
    const MeshDevice<SimulationControl>& mesh,
    [[maybe_unused]] const SourceTermDevice<SimulationControl>& source_term) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.computeVolumeElementQuadrature(mesh.line_, source_term);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.computeVolumeElementQuadrature(mesh.triangle_, source_term);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.computeVolumeElementQuadrature(mesh.quadrangle_, source_term);
    }
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.computeVolumeElementQuadrature(mesh.tetrahedron_, source_term);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.computeVolumeElementQuadrature(mesh.pyramid_, source_term);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.computeVolumeElementQuadrature(mesh.hexahedron_, source_term);
    }
  }
  queue.wait();
}

template <typename VolumeElementTrait, typename SimulationControl>
inline void VolumeElementSolver<VolumeElementTrait, SimulationControl>::computeVolumeElementGradientQuadrature(
    const VolumeElementMesh<VolumeElementTrait>& volume_element_mesh) {
  tbb::parallel_for(tbb::blocked_range<Isize>(0, this->number_), [&](const tbb::blocked_range<Isize>& range) -> void {
    for (Isize i = range.begin(); i != range.end(); i++) {
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> quadrature_node_conserved_variable;
      for (Isize j = 0; j < VolumeElementTrait::kQuadratureNumber; j++) {
        VolumeElementVariable<VolumeElementTrait, SimulationControl>::get(volume_element_mesh, *this,
                                                                          quadrature_node_conserved_variable, i, j);
        const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>&
            jacobian_transpose_inverse_multiply_determinate_and_weight =
                volume_element_mesh.jacobian_transpose_inverse_multiply_determinate_and_weight_(i)(
                    Eigen::placeholders::all,
                    Eigen::seqN(j * SimulationControl::kDimension, Eigen::fix<SimulationControl::kDimension>));
        for (Isize k = 0; k < SimulationControl::kConservedVariableNumber; k++) {
          this->variable_volume_gradient_quadrature_(i)(
              Eigen::seqN(k * SimulationControl::kDimension, Eigen::fix<SimulationControl::kDimension>),
              Eigen::seqN(j * SimulationControl::kDimension, Eigen::fix<SimulationControl::kDimension>)) =
              quadrature_node_conserved_variable(k) * jacobian_transpose_inverse_multiply_determinate_and_weight;
        }
      }
    }
  });
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::computeVolumeGradientQuadrature(const Mesh<SimulationControl>& mesh) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.computeVolumeElementGradientQuadrature(mesh.line_);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.computeVolumeElementGradientQuadrature(mesh.triangle_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.computeVolumeElementGradientQuadrature(mesh.quadrangle_);
    }
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.computeVolumeElementGradientQuadrature(mesh.tetrahedron_);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.computeVolumeElementGradientQuadrature(mesh.pyramid_);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.computeVolumeElementGradientQuadrature(mesh.hexahedron_);
    }
  }
}

template <typename VolumeElementTrait, typename SimulationControl>
inline void VolumeElementSolverDevice<VolumeElementTrait, SimulationControl>::computeVolumeElementGradientQuadrature(
    const VolumeElementMeshDevice<VolumeElementTrait>& volume_element_mesh) {
  queue.submit([&](sycl::handler& cgh) -> void {
    cgh.parallel_for(getNdRange(this->number_), [=, this](sycl::nd_item<1> index) -> void {
      const auto i = static_cast<Isize>(index.get_global_id(0));
      if (i >= this->number_) {
        return;
      }
      Device::StaticVector<Real, SimulationControl::kConservedVariableNumber> quadrature_node_conserved_variable;
      for (Isize j = 0; j < VolumeElementTrait::kQuadratureNumber; j++) {
        VolumeElementVariableDevice<VolumeElementTrait, SimulationControl>::get(
            volume_element_mesh, *this, quadrature_node_conserved_variable, i, j);
        const Device::View<const Device::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>>
            jacobian_transpose_inverse_multiply_determinate_and_weight =
                volume_element_mesh.jacobian_transpose_inverse_multiply_determinate_and_weight_.slice(
                    i, volume_element_mesh.number_, Device::Slice<SimulationControl::kDimension>::all(),
                    Device::Slice<SimulationControl::kDimension>::seqN(j * SimulationControl::kDimension));
        for (Isize k = 0; k < SimulationControl::kConservedVariableNumber; k++) {
          Device::View<Device::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>>
              quadrature_node_variable_volume_gradient_quadrature = this->variable_volume_gradient_quadrature_.slice(
                  i, this->number_,
                  Device::Slice<SimulationControl::kDimension>::seqN(k * SimulationControl::kDimension),
                  Device::Slice<SimulationControl::kDimension>::seqN(j * SimulationControl::kDimension));
          for (Isize m = 0; m < SimulationControl::kDimension; m++) {
            for (Isize n = 0; n < SimulationControl::kDimension; n++) {
              quadrature_node_variable_volume_gradient_quadrature(m, n) =
                  quadrature_node_conserved_variable(k) *
                  jacobian_transpose_inverse_multiply_determinate_and_weight(m, n);
            }
          }
        }
      }
    });
  });
}

template <typename SimulationControl>
inline void SolverDevice<SimulationControl>::computeVolumeGradientQuadrature(
    const MeshDevice<SimulationControl>& mesh) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.computeVolumeElementGradientQuadrature(mesh.line_);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.computeVolumeElementGradientQuadrature(mesh.triangle_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.computeVolumeElementGradientQuadrature(mesh.quadrangle_);
    }
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.computeVolumeElementGradientQuadrature(mesh.tetrahedron_);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.computeVolumeElementGradientQuadrature(mesh.pyramid_);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.computeVolumeElementGradientQuadrature(mesh.hexahedron_);
    }
  }
  queue.wait();
}

template <typename AdjacencyElementTrait, typename SimulationControl>
[[nodiscard]] inline Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>>
AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl>::getVariableAdjacencyQuadrature(
    Solver<SimulationControl>& solver, const Isize parent_gmsh_type_number, const Isize parent_index_each_type,
    const Isize quadrature_node_sequence_in_parent) {
  if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
    return solver.line_.variable_adjacency_quadrature_(parent_index_each_type).col(quadrature_node_sequence_in_parent);
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
    if (parent_gmsh_type_number == VolumeTriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      return solver.triangle_.variable_adjacency_quadrature_(parent_index_each_type)
          .col(quadrature_node_sequence_in_parent);
    }
    if (parent_gmsh_type_number == VolumeQuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      return solver.quadrangle_.variable_adjacency_quadrature_(parent_index_each_type)
          .col(quadrature_node_sequence_in_parent);
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
    if (parent_gmsh_type_number == VolumeTetrahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      return solver.tetrahedron_.variable_adjacency_quadrature_(parent_index_each_type)
          .col(quadrature_node_sequence_in_parent);
    }
    if (parent_gmsh_type_number == VolumePyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      return solver.pyramid_.variable_adjacency_quadrature_(parent_index_each_type)
          .col(quadrature_node_sequence_in_parent);
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
    if (parent_gmsh_type_number == VolumePyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      return solver.pyramid_.variable_adjacency_quadrature_(parent_index_each_type)
          .col(quadrature_node_sequence_in_parent);
    }
    if (parent_gmsh_type_number == VolumeHexahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      return solver.hexahedron_.variable_adjacency_quadrature_(parent_index_each_type)
          .col(quadrature_node_sequence_in_parent);
    }
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
[[nodiscard]] inline Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber>>
AdjacencyElementSolverDevice<AdjacencyElementTrait, SimulationControl>::getVariableAdjacencyQuadrature(
    SolverDevice<SimulationControl> solver, const Isize parent_gmsh_type_number, const Isize parent_index_each_type,
    const Isize quadrature_node_sequence_in_parent) {
  if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
    return solver.line_.variable_adjacency_quadrature_.slice(
        parent_index_each_type, solver.line_.number_, Device::Slice<SimulationControl::kConservedVariableNumber>::all(),
        Device::Slice<1>::seqN(quadrature_node_sequence_in_parent));
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
    if (parent_gmsh_type_number == VolumeTriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      return solver.triangle_.variable_adjacency_quadrature_.slice(
          parent_index_each_type, solver.triangle_.number_,
          Device::Slice<SimulationControl::kConservedVariableNumber>::all(),
          Device::Slice<1>::seqN(quadrature_node_sequence_in_parent));
    }
    if (parent_gmsh_type_number == VolumeQuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      return solver.quadrangle_.variable_adjacency_quadrature_.slice(
          parent_index_each_type, solver.quadrangle_.number_,
          Device::Slice<SimulationControl::kConservedVariableNumber>::all(),
          Device::Slice<1>::seqN(quadrature_node_sequence_in_parent));
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
    if (parent_gmsh_type_number == VolumeTetrahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      return solver.tetrahedron_.variable_adjacency_quadrature_.slice(
          parent_index_each_type, solver.tetrahedron_.number_,
          Device::Slice<SimulationControl::kConservedVariableNumber>::all(),
          Device::Slice<1>::seqN(quadrature_node_sequence_in_parent));
    }
    if (parent_gmsh_type_number == VolumePyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      return solver.pyramid_.variable_adjacency_quadrature_.slice(
          parent_index_each_type, solver.pyramid_.number_,
          Device::Slice<SimulationControl::kConservedVariableNumber>::all(),
          Device::Slice<1>::seqN(quadrature_node_sequence_in_parent));
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
    if (parent_gmsh_type_number == VolumePyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      return solver.pyramid_.variable_adjacency_quadrature_.slice(
          parent_index_each_type, solver.pyramid_.number_,
          Device::Slice<SimulationControl::kConservedVariableNumber>::all(),
          Device::Slice<1>::seqN(quadrature_node_sequence_in_parent));
    }
    if (parent_gmsh_type_number == VolumeHexahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      return solver.hexahedron_.variable_adjacency_quadrature_.slice(
          parent_index_each_type, solver.hexahedron_.number_,
          Device::Slice<SimulationControl::kConservedVariableNumber>::all(),
          Device::Slice<1>::seqN(quadrature_node_sequence_in_parent));
    }
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl>::computeInteriorAdjacencyElementQuadrature(
    const Mesh<SimulationControl>& mesh, Solver<SimulationControl>& solver) {
  const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh =
      mesh.*Mesh<SimulationControl>::template getAdjacencyElement<AdjacencyElementTrait>();
  tbb::parallel_for(
      tbb::blocked_range<Isize>(0, this->interior_number_), [&](const tbb::blocked_range<Isize>& range) -> void {
        for (Isize i = range.begin(); i != range.end(); i++) {
          const Isize element_index = i;
          const std::array<int, AdjacencyElementTrait::kQuadratureNumber> adjacency_element_quadrature_sequence{
              getAdjacencyElementQuadratureSequence<AdjacencyElementTrait::kElementType,
                                                    SimulationControl::kPolynomialOrder>(
                  static_cast<int>(adjacency_element_mesh.adjacency_right_rotation_(element_index)))};
          const Isize left_parent_index_each_type = adjacency_element_mesh.left_parent_index_each_type_(element_index);
          const Isize right_parent_index_each_type =
              adjacency_element_mesh.right_parent_index_each_type_(element_index);
          const Isize adjacency_sequence_in_left_parent =
              adjacency_element_mesh.adjacency_sequence_in_left_parent_(element_index);
          const Isize adjacency_sequence_in_right_parent =
              adjacency_element_mesh.adjacency_sequence_in_right_parent_(element_index);
          const Isize left_parent_gmsh_type_number =
              adjacency_element_mesh.left_parent_gmsh_type_number_(element_index);
          const Isize right_parent_gmsh_type_number =
              adjacency_element_mesh.right_parent_gmsh_type_number_(element_index);
          const Isize left_adjacency_accumulate_quadrature_number =
              getAdjacencyElementAccumulateQuadratureNumber<AdjacencyElementTrait, SimulationControl>(
                  left_parent_gmsh_type_number, adjacency_sequence_in_left_parent);
          const Isize right_adjacency_accumulate_quadrature_number =
              getAdjacencyElementAccumulateQuadratureNumber<AdjacencyElementTrait, SimulationControl>(
                  right_parent_gmsh_type_number, adjacency_sequence_in_right_parent);
          Eigen::Matrix<Real, SimulationControl::kDimension, AdjacencyElementTrait::kQuadratureNumber>
              rotation_velocity;
          AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl>::getRotationVelocity(
              mesh, solver, rotation_velocity, left_parent_gmsh_type_number, left_parent_index_each_type,
              adjacency_sequence_in_left_parent);
          Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, AdjacencyElementTrait::kQuadratureNumber>
              left_conserved_variable;
          AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl>::get(
              mesh, solver, left_conserved_variable, left_parent_gmsh_type_number, left_parent_index_each_type,
              adjacency_sequence_in_left_parent);
          Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, AdjacencyElementTrait::kQuadratureNumber>
              right_conserved_variable;
          AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl>::get(
              mesh, solver, right_conserved_variable, right_parent_gmsh_type_number, right_parent_index_each_type,
              adjacency_sequence_in_right_parent);
          if constexpr (IsEuler<SimulationControl::kEquationModel>) {
            Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>
                left_quadrature_node_computational_variable;
            Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>
                right_quadrature_node_computational_variable;
            Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>
                quadrature_node_variable_adjacency_quadrature;
            for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
              const Isize right_quadrature_sequence = adjacency_element_quadrature_sequence[static_cast<Usize>(j)];
              Variable<SimulationControl>::convertComputationalFromConserved(
                  left_conserved_variable.col(j), left_quadrature_node_computational_variable);
              Variable<SimulationControl>::convertComputationalFromConserved(
                  right_conserved_variable.col(right_quadrature_sequence),
                  right_quadrature_node_computational_variable);
              const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector =
                  adjacency_element_mesh.normal_vector_(element_index).col(j);
              const Real jacobian_determinant_multiply_weight =
                  adjacency_element_mesh.jacobian_determinant_multiply_weight_(element_index)(j);
              quadrature_node_variable_adjacency_quadrature.setZero();
              ConvectiveFlux<SimulationControl>::addVariableInteriorAdjacencyQuadratureNormalFlux(
                  normal_vector, rotation_velocity.col(j), left_conserved_variable.col(j),
                  left_quadrature_node_computational_variable, right_conserved_variable.col(right_quadrature_sequence),
                  right_quadrature_node_computational_variable, quadrature_node_variable_adjacency_quadrature,
                  jacobian_determinant_multiply_weight);
              Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>>
                  left_quadrature_node_variable_adjacency_quadrature = this->getVariableAdjacencyQuadrature(
                      solver, left_parent_gmsh_type_number, left_parent_index_each_type,
                      left_adjacency_accumulate_quadrature_number + j);
              Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>>
                  right_quadrature_node_variable_adjacency_quadrature = this->getVariableAdjacencyQuadrature(
                      solver, right_parent_gmsh_type_number, right_parent_index_each_type,
                      right_adjacency_accumulate_quadrature_number + right_quadrature_sequence);
              left_quadrature_node_variable_adjacency_quadrature = quadrature_node_variable_adjacency_quadrature;
              right_quadrature_node_variable_adjacency_quadrature = -quadrature_node_variable_adjacency_quadrature;
            }
          } else if constexpr (IsNS<SimulationControl::kEquationModel>) {
            Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>
                left_quadrature_node_computational_variable;
            Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>
                right_quadrature_node_computational_variable;
            Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>
                left_quadrature_node_primitive_variable_gradient;
            Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>
                right_quadrature_node_primitive_variable_gradient;
            Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>
                quadrature_node_variable_adjacency_quadrature;
            Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                          AdjacencyElementTrait::kQuadratureNumber>
                left_conserved_variable_gradient;
            AdjacencyElementVariableGradient<AdjacencyElementTrait, SimulationControl>::template get<
                SimulationControl::kViscousFlux>(mesh, solver, left_conserved_variable_gradient,
                                                 left_parent_gmsh_type_number, left_parent_index_each_type,
                                                 adjacency_sequence_in_left_parent);
            Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                          AdjacencyElementTrait::kQuadratureNumber>
                right_conserved_variable_gradient;
            AdjacencyElementVariableGradient<AdjacencyElementTrait, SimulationControl>::template get<
                SimulationControl::kViscousFlux>(mesh, solver, right_conserved_variable_gradient,
                                                 right_parent_gmsh_type_number, right_parent_index_each_type,
                                                 adjacency_sequence_in_right_parent);
            for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
              const Isize right_quadrature_sequence = adjacency_element_quadrature_sequence[static_cast<Usize>(j)];
              Variable<SimulationControl>::convertComputationalFromConserved(
                  left_conserved_variable.col(j), left_quadrature_node_computational_variable);
              Variable<SimulationControl>::convertComputationalFromConserved(
                  right_conserved_variable.col(right_quadrature_sequence),
                  right_quadrature_node_computational_variable);
              VariableGradient<SimulationControl>::convertPrimitiveFromConserved(
                  left_quadrature_node_computational_variable, left_conserved_variable_gradient.col(j),
                  left_quadrature_node_primitive_variable_gradient);
              VariableGradient<SimulationControl>::convertPrimitiveFromConserved(
                  right_quadrature_node_computational_variable,
                  right_conserved_variable_gradient.col(right_quadrature_sequence),
                  right_quadrature_node_primitive_variable_gradient);
              const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector =
                  adjacency_element_mesh.normal_vector_(element_index).col(j);
              const Real jacobian_determinant_multiply_weight =
                  adjacency_element_mesh.jacobian_determinant_multiply_weight_(element_index)(j);
              quadrature_node_variable_adjacency_quadrature.setZero();
              ConvectiveFlux<SimulationControl>::addVariableInteriorAdjacencyQuadratureNormalFlux(
                  normal_vector, rotation_velocity.col(j), left_conserved_variable.col(j),
                  left_quadrature_node_computational_variable, right_conserved_variable.col(right_quadrature_sequence),
                  right_quadrature_node_computational_variable, quadrature_node_variable_adjacency_quadrature,
                  jacobian_determinant_multiply_weight);
              ViscousFlux<SimulationControl>::minusVariableInteriorAdjacencyQuadratureNormalFlux(
                  normal_vector, left_quadrature_node_computational_variable,
                  left_quadrature_node_primitive_variable_gradient, right_quadrature_node_computational_variable,
                  right_quadrature_node_primitive_variable_gradient, quadrature_node_variable_adjacency_quadrature,
                  jacobian_determinant_multiply_weight);
              Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>>
                  left_quadrature_node_variable_adjacency_quadrature = this->getVariableAdjacencyQuadrature(
                      solver, left_parent_gmsh_type_number, left_parent_index_each_type,
                      left_adjacency_accumulate_quadrature_number + j);
              Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>>
                  right_quadrature_node_variable_adjacency_quadrature = this->getVariableAdjacencyQuadrature(
                      solver, right_parent_gmsh_type_number, right_parent_index_each_type,
                      right_adjacency_accumulate_quadrature_number + right_quadrature_sequence);
              left_quadrature_node_variable_adjacency_quadrature = quadrature_node_variable_adjacency_quadrature;
              right_quadrature_node_variable_adjacency_quadrature = -quadrature_node_variable_adjacency_quadrature;
            }
          }
        }
      });
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl>::computeBoundaryAdjacencyElementQuadrature(
    const Mesh<SimulationControl>& mesh, Solver<SimulationControl>& solver) {
  const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh =
      mesh.*Mesh<SimulationControl>::template getAdjacencyElement<AdjacencyElementTrait>();
  tbb::parallel_for(
      tbb::blocked_range<Isize>(0, this->boundary_number_), [&](const tbb::blocked_range<Isize>& range) -> void {
        for (Isize i = range.begin(); i != range.end(); i++) {
          const Isize element_index = i + this->interior_number_;
          const Isize left_parent_index_each_type = adjacency_element_mesh.left_parent_index_each_type_(element_index);
          const Isize adjacency_sequence_in_left_parent =
              adjacency_element_mesh.adjacency_sequence_in_left_parent_(element_index);
          const Isize left_parent_gmsh_type_number =
              adjacency_element_mesh.left_parent_gmsh_type_number_(element_index);
          const Isize left_adjacency_accumulate_quadrature_number =
              getAdjacencyElementAccumulateQuadratureNumber<AdjacencyElementTrait, SimulationControl>(
                  left_parent_gmsh_type_number, adjacency_sequence_in_left_parent);
          const BoundaryConditionEnum boundary_condition_type =
              adjacency_element_mesh.boundary_condition_type_(element_index);
          Eigen::Matrix<Real, SimulationControl::kDimension, AdjacencyElementTrait::kQuadratureNumber>
              rotation_velocity;
          AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl>::getRotationVelocity(
              mesh, solver, rotation_velocity, left_parent_gmsh_type_number, left_parent_index_each_type,
              adjacency_sequence_in_left_parent);
          Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, AdjacencyElementTrait::kQuadratureNumber>
              left_conserved_variable;
          AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl>::get(
              mesh, solver, left_conserved_variable, left_parent_gmsh_type_number, left_parent_index_each_type,
              adjacency_sequence_in_left_parent);
          if constexpr (IsEuler<SimulationControl::kEquationModel>) {
            Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>
                left_quadrature_node_computational_variable;
            Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>
                boundary_quadrature_node_computational_variable;
            Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>
                quadrature_node_variable_adjacency_quadrature;
            for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
              Variable<SimulationControl>::convertComputationalFromConserved(
                  left_conserved_variable.col(j), left_quadrature_node_computational_variable);
              const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
                  right_quadrature_node_computational_variable =
                      this->boundary_dummy_right_computational_variable_(i).col(
                          j);  // the index use i not element_index
              const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector =
                  adjacency_element_mesh.normal_vector_(element_index).col(j);
              const Real jacobian_determinant_multiply_weight =
                  adjacency_element_mesh.jacobian_determinant_multiply_weight_(element_index)(j);
              quadrature_node_variable_adjacency_quadrature.setZero();
              ConvectiveFlux<SimulationControl>::addVariableBoundaryAdjacencyQuadratureNormalFlux(
                  normal_vector, rotation_velocity.col(j), left_quadrature_node_computational_variable,
                  right_quadrature_node_computational_variable, boundary_quadrature_node_computational_variable,
                  quadrature_node_variable_adjacency_quadrature, boundary_condition_type,
                  jacobian_determinant_multiply_weight);
              Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>>
                  left_quadrature_node_variable_adjacency_quadrature = this->getVariableAdjacencyQuadrature(
                      solver, left_parent_gmsh_type_number, left_parent_index_each_type,
                      left_adjacency_accumulate_quadrature_number + j);
              left_quadrature_node_variable_adjacency_quadrature = quadrature_node_variable_adjacency_quadrature;
            }
          } else if constexpr (IsNS<SimulationControl::kEquationModel>) {
            Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>
                left_quadrature_node_computational_variable;
            Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>
                left_quadrature_node_primitive_variable_gradient;
            Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>
                boundary_quadrature_node_computational_variable;
            Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>
                quadrature_node_variable_adjacency_quadrature;
            Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                          AdjacencyElementTrait::kQuadratureNumber>
                left_conserved_variable_gradient;
            AdjacencyElementVariableGradient<AdjacencyElementTrait, SimulationControl>::template get<
                SimulationControl::kViscousFlux>(mesh, solver, left_conserved_variable_gradient,
                                                 left_parent_gmsh_type_number, left_parent_index_each_type,
                                                 adjacency_sequence_in_left_parent);
            for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
              Variable<SimulationControl>::convertComputationalFromConserved(
                  left_conserved_variable.col(j), left_quadrature_node_computational_variable);
              const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
                  right_quadrature_node_computational_variable =
                      this->boundary_dummy_right_computational_variable_(i).col(
                          j);  // the index use i not element_index
              VariableGradient<SimulationControl>::convertPrimitiveFromConserved(
                  left_quadrature_node_computational_variable, left_conserved_variable_gradient.col(j),
                  left_quadrature_node_primitive_variable_gradient);
              const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector =
                  adjacency_element_mesh.normal_vector_(element_index).col(j);
              const Real jacobian_determinant_multiply_weight =
                  adjacency_element_mesh.jacobian_determinant_multiply_weight_(element_index)(j);
              quadrature_node_variable_adjacency_quadrature.setZero();
              ConvectiveFlux<SimulationControl>::addVariableBoundaryAdjacencyQuadratureNormalFlux(
                  normal_vector, rotation_velocity.col(j), left_quadrature_node_computational_variable,
                  right_quadrature_node_computational_variable, boundary_quadrature_node_computational_variable,
                  quadrature_node_variable_adjacency_quadrature, boundary_condition_type,
                  jacobian_determinant_multiply_weight);
              ViscousFlux<SimulationControl>::minusVariableBoundaryAdjacencyQuadratureNormalFlux(
                  normal_vector, left_quadrature_node_computational_variable,
                  left_quadrature_node_primitive_variable_gradient, boundary_quadrature_node_computational_variable,
                  quadrature_node_variable_adjacency_quadrature, boundary_condition_type,
                  jacobian_determinant_multiply_weight);
              Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>>
                  left_quadrature_node_variable_adjacency_quadrature = this->getVariableAdjacencyQuadrature(
                      solver, left_parent_gmsh_type_number, left_parent_index_each_type,
                      left_adjacency_accumulate_quadrature_number + j);
              left_quadrature_node_variable_adjacency_quadrature = quadrature_node_variable_adjacency_quadrature;
            }
          }
        }
      });
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void
AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl>::computeInterfaceAdjacencyElementQuadrature(
    const Mesh<SimulationControl>& mesh, Solver<SimulationControl>& solver) {
  const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh =
      mesh.*Mesh<SimulationControl>::template getAdjacencyElement<AdjacencyElementTrait>();
  tbb::parallel_for(
      tbb::blocked_range<Isize>(0, 2 * this->interface_number_), [&](const tbb::blocked_range<Isize>& range) -> void {
        for (Isize i = range.begin(); i != range.end(); i++) {
          const Isize element_index = i + this->interior_number_ + this->boundary_number_;
          const Isize left_parent_index_each_type = adjacency_element_mesh.left_parent_index_each_type_(element_index);
          const Isize adjacency_sequence_in_left_parent =
              adjacency_element_mesh.adjacency_sequence_in_left_parent_(element_index);
          const Isize left_parent_gmsh_type_number =
              adjacency_element_mesh.left_parent_gmsh_type_number_(element_index);
          const Isize left_adjacency_accumulate_quadrature_number =
              getAdjacencyElementAccumulateQuadratureNumber<AdjacencyElementTrait, SimulationControl>(
                  left_parent_gmsh_type_number, adjacency_sequence_in_left_parent);
          Eigen::Matrix<Real, SimulationControl::kDimension, AdjacencyElementTrait::kQuadratureNumber>
              rotation_velocity;
          AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl>::getRotationVelocity(
              mesh, solver, rotation_velocity, left_parent_gmsh_type_number, left_parent_index_each_type,
              adjacency_sequence_in_left_parent);
          Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, AdjacencyElementTrait::kQuadratureNumber>
              left_conserved_variable;
          AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl>::get(
              mesh, solver, left_conserved_variable, left_parent_gmsh_type_number, left_parent_index_each_type,
              adjacency_sequence_in_left_parent);
          if constexpr (IsEuler<SimulationControl::kEquationModel>) {
            Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>
                left_quadrature_node_computational_variable;
            Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>
                right_quadrature_node_computational_variable;
            Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>
                quadrature_node_variable_adjacency_quadrature;
            for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
              Variable<SimulationControl>::convertComputationalFromConserved(
                  left_conserved_variable.col(j), left_quadrature_node_computational_variable);
              const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>&
                  right_quadrature_node_conserved_variable =
                      this->interface_right_conserved_variable_(i).col(j);  // the index use i not element_index
              Variable<SimulationControl>::convertComputationalFromConserved(
                  right_quadrature_node_conserved_variable, right_quadrature_node_computational_variable);
              const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector =
                  adjacency_element_mesh.normal_vector_(element_index).col(j);
              const Real jacobian_determinant_multiply_weight =
                  adjacency_element_mesh.jacobian_determinant_multiply_weight_(element_index)(j);
              quadrature_node_variable_adjacency_quadrature.setZero();
              ConvectiveFlux<SimulationControl>::addVariableInteriorAdjacencyQuadratureNormalFlux(
                  normal_vector, rotation_velocity.col(j), left_conserved_variable.col(j),
                  left_quadrature_node_computational_variable, right_quadrature_node_conserved_variable,
                  right_quadrature_node_computational_variable, quadrature_node_variable_adjacency_quadrature,
                  jacobian_determinant_multiply_weight);
              Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>>
                  left_quadrature_node_variable_adjacency_quadrature = this->getVariableAdjacencyQuadrature(
                      solver, left_parent_gmsh_type_number, left_parent_index_each_type,
                      left_adjacency_accumulate_quadrature_number + j);
              left_quadrature_node_variable_adjacency_quadrature = quadrature_node_variable_adjacency_quadrature;
            }
          } else if constexpr (IsNS<SimulationControl::kEquationModel>) {
            Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>
                left_quadrature_node_computational_variable;
            Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>
                right_quadrature_node_computational_variable;
            Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>
                left_quadrature_node_primitive_variable_gradient;
            Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>
                right_quadrature_node_primitive_variable_gradient;
            Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>
                quadrature_node_variable_adjacency_quadrature;
            Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                          AdjacencyElementTrait::kQuadratureNumber>
                left_conserved_variable_gradient;
            AdjacencyElementVariableGradient<AdjacencyElementTrait, SimulationControl>::template get<
                SimulationControl::kViscousFlux>(mesh, solver, left_conserved_variable_gradient,
                                                 left_parent_gmsh_type_number, left_parent_index_each_type,
                                                 adjacency_sequence_in_left_parent);
            for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
              const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>&
                  right_quadrature_node_conserved_variable =
                      this->interface_right_conserved_variable_(i).col(j);  // the index use i not element_index
              Variable<SimulationControl>::convertComputationalFromConserved(
                  left_conserved_variable.col(j), left_quadrature_node_computational_variable);
              Variable<SimulationControl>::convertComputationalFromConserved(
                  right_quadrature_node_conserved_variable, right_quadrature_node_computational_variable);
              const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>&
                  right_quadrature_node_conserved_variable_gradient =
                      this->interface_right_conserved_variable_gradient_(i).col(
                          j);  // the index use i not element_index
              VariableGradient<SimulationControl>::convertPrimitiveFromConserved(
                  left_quadrature_node_computational_variable, left_conserved_variable_gradient.col(j),
                  left_quadrature_node_primitive_variable_gradient);
              VariableGradient<SimulationControl>::convertPrimitiveFromConserved(
                  right_quadrature_node_computational_variable, right_quadrature_node_conserved_variable_gradient,
                  right_quadrature_node_primitive_variable_gradient);
              const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector =
                  adjacency_element_mesh.normal_vector_(element_index).col(j);
              const Real jacobian_determinant_multiply_weight =
                  adjacency_element_mesh.jacobian_determinant_multiply_weight_(element_index)(j);
              quadrature_node_variable_adjacency_quadrature.setZero();
              ConvectiveFlux<SimulationControl>::addVariableInteriorAdjacencyQuadratureNormalFlux(
                  normal_vector, rotation_velocity.col(j), left_conserved_variable.col(j),
                  left_quadrature_node_computational_variable, right_quadrature_node_conserved_variable,
                  right_quadrature_node_computational_variable, quadrature_node_variable_adjacency_quadrature,
                  jacobian_determinant_multiply_weight);
              ViscousFlux<SimulationControl>::minusVariableInteriorAdjacencyQuadratureNormalFlux(
                  normal_vector, left_quadrature_node_computational_variable,
                  left_quadrature_node_primitive_variable_gradient, right_quadrature_node_computational_variable,
                  right_quadrature_node_primitive_variable_gradient, quadrature_node_variable_adjacency_quadrature,
                  jacobian_determinant_multiply_weight);
              Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>>
                  left_quadrature_node_variable_adjacency_quadrature = this->getVariableAdjacencyQuadrature(
                      solver, left_parent_gmsh_type_number, left_parent_index_each_type,
                      left_adjacency_accumulate_quadrature_number + j);
              left_quadrature_node_variable_adjacency_quadrature = quadrature_node_variable_adjacency_quadrature;
            }
          }
        }
      });
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::computeAdjacencyQuadrature(const Mesh<SimulationControl>& mesh) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->point_.computeInteriorAdjacencyElementQuadrature(mesh, *this);
    this->point_.computeBoundaryAdjacencyElementQuadrature(mesh, *this);
  } else if constexpr (SimulationControl::kDimension == 2) {
    this->line_.computeInteriorAdjacencyElementQuadrature(mesh, *this);
    this->line_.computeBoundaryAdjacencyElementQuadrature(mesh, *this);
    this->line_.computeInterfaceAdjacencyElementQuadrature(mesh, *this);
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasAdjacencyTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.computeInteriorAdjacencyElementQuadrature(mesh, *this);
      this->triangle_.computeBoundaryAdjacencyElementQuadrature(mesh, *this);
    }
    if constexpr (HasAdjacencyQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.computeInteriorAdjacencyElementQuadrature(mesh, *this);
      this->quadrangle_.computeBoundaryAdjacencyElementQuadrature(mesh, *this);
      this->quadrangle_.computeInterfaceAdjacencyElementQuadrature(mesh, *this);
    }
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void
AdjacencyElementSolverDevice<AdjacencyElementTrait, SimulationControl>::computeInteriorAdjacencyElementQuadrature(
    const MeshDevice<SimulationControl>& mesh, SolverDevice<SimulationControl>& solver) {
  const AdjacencyElementMeshDevice<AdjacencyElementTrait>& adjacency_element_mesh =
      mesh.*MeshDevice<SimulationControl>::template getAdjacencyElement<AdjacencyElementTrait>();
  queue.submit([&](sycl::handler& cgh) -> void {
    cgh.parallel_for(getNdRange(this->interior_number_), [=, this](sycl::nd_item<1> index) -> void {
      const auto i = static_cast<Isize>(index.get_global_id(0));
      if (i >= this->interior_number_) {
        return;
      }
      const Isize element_index = i;
      const std::array<int, AdjacencyElementTrait::kQuadratureNumber> adjacency_element_quadrature_sequence{
          getAdjacencyElementQuadratureSequence<AdjacencyElementTrait::kElementType,
                                                SimulationControl::kPolynomialOrder>(
              static_cast<int>(adjacency_element_mesh.adjacency_right_rotation_(element_index)))};
      const Isize left_parent_index_each_type = adjacency_element_mesh.left_parent_index_each_type_(element_index);
      const Isize right_parent_index_each_type = adjacency_element_mesh.right_parent_index_each_type_(element_index);
      const Isize adjacency_sequence_in_left_parent =
          adjacency_element_mesh.adjacency_sequence_in_left_parent_(element_index);
      const Isize adjacency_sequence_in_right_parent =
          adjacency_element_mesh.adjacency_sequence_in_right_parent_(element_index);
      const Isize left_parent_gmsh_type_number = adjacency_element_mesh.left_parent_gmsh_type_number_(element_index);
      const Isize right_parent_gmsh_type_number = adjacency_element_mesh.right_parent_gmsh_type_number_(element_index);
      const Isize left_adjacency_accumulate_quadrature_number =
          getAdjacencyElementAccumulateQuadratureNumber<AdjacencyElementTrait, SimulationControl>(
              left_parent_gmsh_type_number, adjacency_sequence_in_left_parent);
      const Isize right_adjacency_accumulate_quadrature_number =
          getAdjacencyElementAccumulateQuadratureNumber<AdjacencyElementTrait, SimulationControl>(
              right_parent_gmsh_type_number, adjacency_sequence_in_right_parent);
      if constexpr (IsEuler<SimulationControl::kEquationModel>) {
        Device::StaticVector<Real, SimulationControl::kDimension> quadrature_node_rotation_velocity;
        Device::StaticVector<Real, SimulationControl::kConservedVariableNumber> left_quadrature_node_conserved_variable;
        Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>
            right_quadrature_node_conserved_variable;
        Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>
            left_quadrature_node_computational_variable;
        Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>
            right_quadrature_node_computational_variable;
        Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>
            quadrature_node_variable_adjacency_quadrature;
        for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
          const Isize right_quadrature_sequence = adjacency_element_quadrature_sequence[static_cast<Usize>(j)];
          AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::getRotationVelocity(
              mesh, solver, quadrature_node_rotation_velocity, left_parent_gmsh_type_number,
              left_parent_index_each_type, adjacency_sequence_in_left_parent, j);
          AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::get(
              mesh, solver, left_quadrature_node_conserved_variable, left_parent_gmsh_type_number,
              left_parent_index_each_type, adjacency_sequence_in_left_parent, j);
          AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::get(
              mesh, solver, right_quadrature_node_conserved_variable, right_parent_gmsh_type_number,
              right_parent_index_each_type, adjacency_sequence_in_right_parent, right_quadrature_sequence);
          VariableDevice<SimulationControl>::convertComputationalFromConserved(
              left_quadrature_node_conserved_variable, left_quadrature_node_computational_variable);
          VariableDevice<SimulationControl>::convertComputationalFromConserved(
              right_quadrature_node_conserved_variable, right_quadrature_node_computational_variable);
          const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector =
              adjacency_element_mesh.normal_vector_.slice(element_index, adjacency_element_mesh.number_,
                                                          Device::Slice<SimulationControl::kDimension>::all(),
                                                          Device::Slice<1>::seqN(j));
          const Real jacobian_determinant_multiply_weight =
              adjacency_element_mesh.jacobian_determinant_multiply_weight_.view(element_index,
                                                                                adjacency_element_mesh.number_)(j);
          quadrature_node_variable_adjacency_quadrature.setZero();
          ConvectiveFluxDevice<SimulationControl>::addVariableInteriorAdjacencyQuadratureNormalFlux(
              normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_conserved_variable,
              left_quadrature_node_computational_variable, right_quadrature_node_conserved_variable,
              right_quadrature_node_computational_variable, quadrature_node_variable_adjacency_quadrature,
              jacobian_determinant_multiply_weight);
          Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber>>
              left_quadrature_node_variable_adjacency_quadrature = this->getVariableAdjacencyQuadrature(
                  solver, left_parent_gmsh_type_number, left_parent_index_each_type,
                  left_adjacency_accumulate_quadrature_number + j);
          Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber>>
              right_quadrature_node_variable_adjacency_quadrature = this->getVariableAdjacencyQuadrature(
                  solver, right_parent_gmsh_type_number, right_parent_index_each_type,
                  right_adjacency_accumulate_quadrature_number + right_quadrature_sequence);
          for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
            left_quadrature_node_variable_adjacency_quadrature(m) = quadrature_node_variable_adjacency_quadrature(m);
            right_quadrature_node_variable_adjacency_quadrature(m) = -quadrature_node_variable_adjacency_quadrature(m);
          }
        }
      } else if constexpr (IsNS<SimulationControl::kEquationModel>) {
        Device::StaticVector<Real, SimulationControl::kDimension> quadrature_node_rotation_velocity;
        Device::StaticVector<Real, SimulationControl::kConservedVariableNumber> left_quadrature_node_conserved_variable;
        Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>
            right_quadrature_node_conserved_variable;
        Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>
            left_quadrature_node_computational_variable;
        Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>
            right_quadrature_node_computational_variable;
        Device::StaticVector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>
            left_quadrature_node_conserved_variable_gradient;
        Device::StaticVector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>
            right_quadrature_node_conserved_variable_gradient;
        Device::StaticVector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>
            left_quadrature_node_primitive_variable_gradient;
        Device::StaticVector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>
            right_quadrature_node_primitive_variable_gradient;
        Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>
            quadrature_node_variable_adjacency_quadrature;
        for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
          const Isize right_quadrature_sequence = adjacency_element_quadrature_sequence[static_cast<Usize>(j)];
          AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::getRotationVelocity(
              mesh, solver, quadrature_node_rotation_velocity, left_parent_gmsh_type_number,
              left_parent_index_each_type, adjacency_sequence_in_left_parent, j);
          AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::get(
              mesh, solver, left_quadrature_node_conserved_variable, left_parent_gmsh_type_number,
              left_parent_index_each_type, adjacency_sequence_in_left_parent, j);
          AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::get(
              mesh, solver, right_quadrature_node_conserved_variable, right_parent_gmsh_type_number,
              right_parent_index_each_type, adjacency_sequence_in_right_parent, right_quadrature_sequence);
          VariableDevice<SimulationControl>::convertComputationalFromConserved(
              left_quadrature_node_conserved_variable, left_quadrature_node_computational_variable);
          VariableDevice<SimulationControl>::convertComputationalFromConserved(
              right_quadrature_node_conserved_variable, right_quadrature_node_computational_variable);
          AdjacencyElementVariableGradientDevice<AdjacencyElementTrait, SimulationControl>::template get<
              SimulationControl::kViscousFlux>(mesh, solver, left_quadrature_node_conserved_variable_gradient,
                                               left_parent_gmsh_type_number, left_parent_index_each_type,
                                               adjacency_sequence_in_left_parent, j);
          AdjacencyElementVariableGradientDevice<AdjacencyElementTrait, SimulationControl>::template get<
              SimulationControl::kViscousFlux>(mesh, solver, right_quadrature_node_conserved_variable_gradient,
                                               right_parent_gmsh_type_number, right_parent_index_each_type,
                                               adjacency_sequence_in_right_parent, right_quadrature_sequence);
          VariableGradientDevice<SimulationControl>::convertPrimitiveFromConserved(
              left_quadrature_node_computational_variable, left_quadrature_node_conserved_variable_gradient,
              left_quadrature_node_primitive_variable_gradient);
          VariableGradientDevice<SimulationControl>::convertPrimitiveFromConserved(
              right_quadrature_node_computational_variable, right_quadrature_node_conserved_variable_gradient,
              right_quadrature_node_primitive_variable_gradient);
          const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector =
              adjacency_element_mesh.normal_vector_.slice(element_index, adjacency_element_mesh.number_,
                                                          Device::Slice<SimulationControl::kDimension>::all(),
                                                          Device::Slice<1>::seqN(j));
          const Real jacobian_determinant_multiply_weight =
              adjacency_element_mesh.jacobian_determinant_multiply_weight_.view(element_index,
                                                                                adjacency_element_mesh.number_)(j);
          quadrature_node_variable_adjacency_quadrature.setZero();
          ConvectiveFluxDevice<SimulationControl>::addVariableInteriorAdjacencyQuadratureNormalFlux(
              normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_conserved_variable,
              left_quadrature_node_computational_variable, right_quadrature_node_conserved_variable,
              right_quadrature_node_computational_variable, quadrature_node_variable_adjacency_quadrature,
              jacobian_determinant_multiply_weight);
          ViscousFluxDevice<SimulationControl>::minusVariableInteriorAdjacencyQuadratureNormalFlux(
              normal_vector, left_quadrature_node_computational_variable,
              left_quadrature_node_primitive_variable_gradient, right_quadrature_node_computational_variable,
              right_quadrature_node_primitive_variable_gradient, quadrature_node_variable_adjacency_quadrature,
              jacobian_determinant_multiply_weight);
          Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber>>
              left_quadrature_node_variable_adjacency_quadrature = this->getVariableAdjacencyQuadrature(
                  solver, left_parent_gmsh_type_number, left_parent_index_each_type,
                  left_adjacency_accumulate_quadrature_number + j);
          Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber>>
              right_quadrature_node_variable_adjacency_quadrature = this->getVariableAdjacencyQuadrature(
                  solver, right_parent_gmsh_type_number, right_parent_index_each_type,
                  right_adjacency_accumulate_quadrature_number + right_quadrature_sequence);
          for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
            left_quadrature_node_variable_adjacency_quadrature(m) = quadrature_node_variable_adjacency_quadrature(m);
            right_quadrature_node_variable_adjacency_quadrature(m) = -quadrature_node_variable_adjacency_quadrature(m);
          }
        }
      }
    });
  });
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void
AdjacencyElementSolverDevice<AdjacencyElementTrait, SimulationControl>::computeBoundaryAdjacencyElementQuadrature(
    const MeshDevice<SimulationControl>& mesh, SolverDevice<SimulationControl>& solver) {
  const AdjacencyElementMeshDevice<AdjacencyElementTrait>& adjacency_element_mesh =
      mesh.*MeshDevice<SimulationControl>::template getAdjacencyElement<AdjacencyElementTrait>();
  queue.submit([&](sycl::handler& cgh) -> void {
    cgh.parallel_for(
        getNdRange(this->boundary_number_, AdjacencyElementTrait::kQuadratureNumber),
        [=, this](sycl::nd_item<2> index) -> void {
          const auto i = static_cast<Isize>(index.get_global_id(0));
          const auto j = static_cast<Isize>(index.get_global_id(1));
          if (i >= this->boundary_number_ || j >= AdjacencyElementTrait::kQuadratureNumber) {
            return;
          }
          const Isize element_index = i + this->interior_number_;
          const AdjacencyElementSolverDevice<AdjacencyElementTrait, SimulationControl>* self = this;
          const Isize left_parent_index_each_type = adjacency_element_mesh.left_parent_index_each_type_(element_index);
          const Isize adjacency_sequence_in_left_parent =
              adjacency_element_mesh.adjacency_sequence_in_left_parent_(element_index);
          const Isize left_parent_gmsh_type_number =
              adjacency_element_mesh.left_parent_gmsh_type_number_(element_index);
          const Isize left_adjacency_accumulate_quadrature_number =
              getAdjacencyElementAccumulateQuadratureNumber<AdjacencyElementTrait, SimulationControl>(
                  left_parent_gmsh_type_number, adjacency_sequence_in_left_parent);
          const BoundaryConditionEnum boundary_condition_type =
              adjacency_element_mesh.boundary_condition_type_(element_index);
          if constexpr (IsEuler<SimulationControl::kEquationModel>) {
            Device::StaticVector<Real, SimulationControl::kDimension> quadrature_node_rotation_velocity;
            Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>
                left_quadrature_node_conserved_variable;
            Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>
                left_quadrature_node_computational_variable;
            Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>
                right_quadrature_node_computational_variable;
            Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>
                boundary_quadrature_node_computational_variable;
            Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>
                quadrature_node_variable_adjacency_quadrature;
            AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::getRotationVelocity(
                mesh, solver, quadrature_node_rotation_velocity, left_parent_gmsh_type_number,
                left_parent_index_each_type, adjacency_sequence_in_left_parent, j);
            AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::get(
                mesh, solver, left_quadrature_node_conserved_variable, left_parent_gmsh_type_number,
                left_parent_index_each_type, adjacency_sequence_in_left_parent, j);
            VariableDevice<SimulationControl>::convertComputationalFromConserved(
                left_quadrature_node_conserved_variable, left_quadrature_node_computational_variable);
            const Device::View<const Device::Vector<Real, SimulationControl::kComputationalVariableNumber>>
                boundary_dummy_right_computational_variable = self->boundary_dummy_right_computational_variable_.slice(
                    i, this->boundary_number_, Device::Slice<SimulationControl::kComputationalVariableNumber>::all(),
                    Device::Slice<1>::seqN(j));  // the index use i not element_index
            for (Isize m = 0; m < SimulationControl::kComputationalVariableNumber; m++) {
              right_quadrature_node_computational_variable(m) = boundary_dummy_right_computational_variable(m);
            }
            const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector =
                adjacency_element_mesh.normal_vector_.slice(element_index, adjacency_element_mesh.number_,
                                                            Device::Slice<SimulationControl::kDimension>::all(),
                                                            Device::Slice<1>::seqN(j));
            const Real jacobian_determinant_multiply_weight =
                adjacency_element_mesh.jacobian_determinant_multiply_weight_.view(element_index,
                                                                                  adjacency_element_mesh.number_)(j);
            quadrature_node_variable_adjacency_quadrature.setZero();
            ConvectiveFluxDevice<SimulationControl>::addVariableBoundaryAdjacencyQuadratureNormalFlux(
                normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_computational_variable,
                right_quadrature_node_computational_variable, boundary_quadrature_node_computational_variable,
                quadrature_node_variable_adjacency_quadrature, boundary_condition_type,
                jacobian_determinant_multiply_weight);
            Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber>>
                left_quadrature_node_variable_adjacency_quadrature = this->getVariableAdjacencyQuadrature(
                    solver, left_parent_gmsh_type_number, left_parent_index_each_type,
                    left_adjacency_accumulate_quadrature_number + j);
            for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
              left_quadrature_node_variable_adjacency_quadrature(m) = quadrature_node_variable_adjacency_quadrature(m);
            }
          } else if constexpr (IsNS<SimulationControl::kEquationModel>) {
            Device::StaticVector<Real, SimulationControl::kDimension> quadrature_node_rotation_velocity;
            Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>
                left_quadrature_node_conserved_variable;
            Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>
                left_quadrature_node_computational_variable;
            Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>
                right_quadrature_node_computational_variable;
            Device::StaticVector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>
                left_quadrature_node_conserved_variable_gradient;
            Device::StaticVector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>
                left_quadrature_node_primitive_variable_gradient;
            Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>
                boundary_quadrature_node_computational_variable;
            Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>
                quadrature_node_variable_adjacency_quadrature;
            AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::getRotationVelocity(
                mesh, solver, quadrature_node_rotation_velocity, left_parent_gmsh_type_number,
                left_parent_index_each_type, adjacency_sequence_in_left_parent, j);
            AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::get(
                mesh, solver, left_quadrature_node_conserved_variable, left_parent_gmsh_type_number,
                left_parent_index_each_type, adjacency_sequence_in_left_parent, j);
            VariableDevice<SimulationControl>::convertComputationalFromConserved(
                left_quadrature_node_conserved_variable, left_quadrature_node_computational_variable);
            const Device::View<const Device::Vector<Real, SimulationControl::kComputationalVariableNumber>>
                boundary_dummy_right_computational_variable = self->boundary_dummy_right_computational_variable_.slice(
                    i, this->boundary_number_, Device::Slice<SimulationControl::kComputationalVariableNumber>::all(),
                    Device::Slice<1>::seqN(j));  // the index use i not element_index
            for (Isize m = 0; m < SimulationControl::kComputationalVariableNumber; m++) {
              right_quadrature_node_computational_variable(m) = boundary_dummy_right_computational_variable(m);
            }
            AdjacencyElementVariableGradientDevice<AdjacencyElementTrait, SimulationControl>::template get<
                SimulationControl::kViscousFlux>(mesh, solver, left_quadrature_node_conserved_variable_gradient,
                                                 left_parent_gmsh_type_number, left_parent_index_each_type,
                                                 adjacency_sequence_in_left_parent, j);
            VariableGradientDevice<SimulationControl>::convertPrimitiveFromConserved(
                left_quadrature_node_computational_variable, left_quadrature_node_conserved_variable_gradient,
                left_quadrature_node_primitive_variable_gradient);
            const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector =
                adjacency_element_mesh.normal_vector_.slice(element_index, adjacency_element_mesh.number_,
                                                            Device::Slice<SimulationControl::kDimension>::all(),
                                                            Device::Slice<1>::seqN(j));
            const Real jacobian_determinant_multiply_weight =
                adjacency_element_mesh.jacobian_determinant_multiply_weight_.view(element_index,
                                                                                  adjacency_element_mesh.number_)(j);
            quadrature_node_variable_adjacency_quadrature.setZero();
            ConvectiveFluxDevice<SimulationControl>::addVariableBoundaryAdjacencyQuadratureNormalFlux(
                normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_computational_variable,
                right_quadrature_node_computational_variable, boundary_quadrature_node_computational_variable,
                quadrature_node_variable_adjacency_quadrature, boundary_condition_type,
                jacobian_determinant_multiply_weight);
            ViscousFluxDevice<SimulationControl>::minusVariableBoundaryAdjacencyQuadratureNormalFlux(
                normal_vector, left_quadrature_node_computational_variable,
                left_quadrature_node_primitive_variable_gradient, boundary_quadrature_node_computational_variable,
                quadrature_node_variable_adjacency_quadrature, boundary_condition_type,
                jacobian_determinant_multiply_weight);
            Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber>>
                left_quadrature_node_variable_adjacency_quadrature = this->getVariableAdjacencyQuadrature(
                    solver, left_parent_gmsh_type_number, left_parent_index_each_type,
                    left_adjacency_accumulate_quadrature_number + j);
            for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
              left_quadrature_node_variable_adjacency_quadrature(m) = quadrature_node_variable_adjacency_quadrature(m);
            }
          }
        });
  });
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void
AdjacencyElementSolverDevice<AdjacencyElementTrait, SimulationControl>::computeInterfaceAdjacencyElementQuadrature(
    const MeshDevice<SimulationControl>& mesh, SolverDevice<SimulationControl>& solver) {
  const AdjacencyElementMeshDevice<AdjacencyElementTrait>& adjacency_element_mesh =
      mesh.*MeshDevice<SimulationControl>::template getAdjacencyElement<AdjacencyElementTrait>();
  queue.submit([&](sycl::handler& cgh) -> void {
    cgh.parallel_for(
        getNdRange(2 * this->interface_number_, AdjacencyElementTrait::kQuadratureNumber),
        [=, this](sycl::nd_item<2> index) -> void {
          const auto i = static_cast<Isize>(index.get_global_id(0));
          const auto j = static_cast<Isize>(index.get_global_id(1));
          if (i >= 2 * this->interface_number_ || j >= AdjacencyElementTrait::kQuadratureNumber) {
            return;
          }
          const Isize element_index = i + this->interior_number_ + this->boundary_number_;
          const AdjacencyElementSolverDevice<AdjacencyElementTrait, SimulationControl>* self = this;
          const Isize left_parent_index_each_type = adjacency_element_mesh.left_parent_index_each_type_(element_index);
          const Isize adjacency_sequence_in_left_parent =
              adjacency_element_mesh.adjacency_sequence_in_left_parent_(element_index);
          const Isize left_parent_gmsh_type_number =
              adjacency_element_mesh.left_parent_gmsh_type_number_(element_index);
          const Isize left_adjacency_accumulate_quadrature_number =
              getAdjacencyElementAccumulateQuadratureNumber<AdjacencyElementTrait, SimulationControl>(
                  left_parent_gmsh_type_number, adjacency_sequence_in_left_parent);
          if constexpr (IsEuler<SimulationControl::kEquationModel>) {
            Device::StaticVector<Real, SimulationControl::kDimension> quadrature_node_rotation_velocity;
            Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>
                left_quadrature_node_conserved_variable;
            Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>
                right_quadrature_node_conserved_variable;
            Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>
                left_quadrature_node_computational_variable;
            Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>
                right_quadrature_node_computational_variable;
            Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>
                quadrature_node_variable_adjacency_quadrature;
            AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::getRotationVelocity(
                mesh, solver, quadrature_node_rotation_velocity, left_parent_gmsh_type_number,
                left_parent_index_each_type, adjacency_sequence_in_left_parent, j);
            AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::get(
                mesh, solver, left_quadrature_node_conserved_variable, left_parent_gmsh_type_number,
                left_parent_index_each_type, adjacency_sequence_in_left_parent, j);
            const Device::View<const Device::Vector<Real, SimulationControl::kConservedVariableNumber>>
                interface_right_conserved_variable = self->interface_right_conserved_variable_.slice(
                    i, 2 * this->interface_number_, Device::Slice<SimulationControl::kConservedVariableNumber>::all(),
                    Device::Slice<1>::seqN(j));  // the index use i not element_index
            for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
              right_quadrature_node_conserved_variable(m) = interface_right_conserved_variable(m);
            }
            VariableDevice<SimulationControl>::convertComputationalFromConserved(
                left_quadrature_node_conserved_variable, left_quadrature_node_computational_variable);
            VariableDevice<SimulationControl>::convertComputationalFromConserved(
                right_quadrature_node_conserved_variable, right_quadrature_node_computational_variable);
            const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector =
                adjacency_element_mesh.normal_vector_.slice(element_index, adjacency_element_mesh.number_,
                                                            Device::Slice<SimulationControl::kDimension>::all(),
                                                            Device::Slice<1>::seqN(j));
            const Real jacobian_determinant_multiply_weight =
                adjacency_element_mesh.jacobian_determinant_multiply_weight_.view(element_index,
                                                                                  adjacency_element_mesh.number_)(j);
            quadrature_node_variable_adjacency_quadrature.setZero();
            ConvectiveFluxDevice<SimulationControl>::addVariableInteriorAdjacencyQuadratureNormalFlux(
                normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_conserved_variable,
                left_quadrature_node_computational_variable, right_quadrature_node_conserved_variable,
                right_quadrature_node_computational_variable, quadrature_node_variable_adjacency_quadrature,
                jacobian_determinant_multiply_weight);
            Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber>>
                left_quadrature_node_variable_adjacency_quadrature = this->getVariableAdjacencyQuadrature(
                    solver, left_parent_gmsh_type_number, left_parent_index_each_type,
                    left_adjacency_accumulate_quadrature_number + j);
            for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
              left_quadrature_node_variable_adjacency_quadrature(m) = quadrature_node_variable_adjacency_quadrature(m);
            }
          } else if constexpr (IsNS<SimulationControl::kEquationModel>) {
            Device::StaticVector<Real, SimulationControl::kDimension> quadrature_node_rotation_velocity;
            Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>
                left_quadrature_node_conserved_variable;
            Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>
                right_quadrature_node_conserved_variable;
            Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>
                left_quadrature_node_computational_variable;
            Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>
                right_quadrature_node_computational_variable;
            Device::StaticVector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>
                left_quadrature_node_conserved_variable_gradient;
            Device::StaticVector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>
                right_quadrature_node_conserved_variable_gradient;
            Device::StaticVector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>
                left_quadrature_node_primitive_variable_gradient;
            Device::StaticVector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>
                right_quadrature_node_primitive_variable_gradient;
            Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>
                quadrature_node_variable_adjacency_quadrature;
            AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::getRotationVelocity(
                mesh, solver, quadrature_node_rotation_velocity, left_parent_gmsh_type_number,
                left_parent_index_each_type, adjacency_sequence_in_left_parent, j);
            AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::get(
                mesh, solver, left_quadrature_node_conserved_variable, left_parent_gmsh_type_number,
                left_parent_index_each_type, adjacency_sequence_in_left_parent, j);
            const Device::View<const Device::Vector<Real, SimulationControl::kConservedVariableNumber>>
                interface_right_conserved_variable = self->interface_right_conserved_variable_.slice(
                    i, 2 * this->interface_number_, Device::Slice<SimulationControl::kConservedVariableNumber>::all(),
                    Device::Slice<1>::seqN(j));  // the index use i not element_index
            for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
              right_quadrature_node_conserved_variable(m) = interface_right_conserved_variable(m);
            }
            VariableDevice<SimulationControl>::convertComputationalFromConserved(
                left_quadrature_node_conserved_variable, left_quadrature_node_computational_variable);
            VariableDevice<SimulationControl>::convertComputationalFromConserved(
                right_quadrature_node_conserved_variable, right_quadrature_node_computational_variable);
            AdjacencyElementVariableGradientDevice<AdjacencyElementTrait, SimulationControl>::template get<
                SimulationControl::kViscousFlux>(mesh, solver, left_quadrature_node_conserved_variable_gradient,
                                                 left_parent_gmsh_type_number, left_parent_index_each_type,
                                                 adjacency_sequence_in_left_parent, j);
            const Device::View<
                const Device::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
                interface_right_conserved_variable_gradient = self->interface_right_conserved_variable_gradient_.slice(
                    i, 2 * this->interface_number_,
                    Device::Slice<SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>::all(),
                    Device::Slice<1>::seqN(j));  // the index use i not element_index
            for (Isize m = 0; m < SimulationControl::kConservedVariableNumber * SimulationControl::kDimension; m++) {
              right_quadrature_node_conserved_variable_gradient(m) = interface_right_conserved_variable_gradient(m);
            }
            VariableGradientDevice<SimulationControl>::convertPrimitiveFromConserved(
                left_quadrature_node_computational_variable, left_quadrature_node_conserved_variable_gradient,
                left_quadrature_node_primitive_variable_gradient);
            VariableGradientDevice<SimulationControl>::convertPrimitiveFromConserved(
                right_quadrature_node_computational_variable, right_quadrature_node_conserved_variable_gradient,
                right_quadrature_node_primitive_variable_gradient);
            const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector =
                adjacency_element_mesh.normal_vector_.slice(element_index, adjacency_element_mesh.number_,
                                                            Device::Slice<SimulationControl::kDimension>::all(),
                                                            Device::Slice<1>::seqN(j));
            const Real jacobian_determinant_multiply_weight =
                adjacency_element_mesh.jacobian_determinant_multiply_weight_.view(element_index,
                                                                                  adjacency_element_mesh.number_)(j);
            quadrature_node_variable_adjacency_quadrature.setZero();
            ConvectiveFluxDevice<SimulationControl>::addVariableInteriorAdjacencyQuadratureNormalFlux(
                normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_conserved_variable,
                left_quadrature_node_computational_variable, right_quadrature_node_conserved_variable,
                right_quadrature_node_computational_variable, quadrature_node_variable_adjacency_quadrature,
                jacobian_determinant_multiply_weight);
            ViscousFluxDevice<SimulationControl>::minusVariableInteriorAdjacencyQuadratureNormalFlux(
                normal_vector, left_quadrature_node_computational_variable,
                left_quadrature_node_primitive_variable_gradient, right_quadrature_node_computational_variable,
                right_quadrature_node_primitive_variable_gradient, quadrature_node_variable_adjacency_quadrature,
                jacobian_determinant_multiply_weight);
            Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber>>
                left_quadrature_node_variable_adjacency_quadrature = this->getVariableAdjacencyQuadrature(
                    solver, left_parent_gmsh_type_number, left_parent_index_each_type,
                    left_adjacency_accumulate_quadrature_number + j);
            for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
              left_quadrature_node_variable_adjacency_quadrature(m) = quadrature_node_variable_adjacency_quadrature(m);
            }
          }
        });
  });
}

template <typename SimulationControl>
inline void SolverDevice<SimulationControl>::computeAdjacencyQuadrature(const MeshDevice<SimulationControl>& mesh) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->point_.computeInteriorAdjacencyElementQuadrature(mesh, *this);
    this->point_.computeBoundaryAdjacencyElementQuadrature(mesh, *this);
  } else if constexpr (SimulationControl::kDimension == 2) {
    this->line_.computeInteriorAdjacencyElementQuadrature(mesh, *this);
    this->line_.computeBoundaryAdjacencyElementQuadrature(mesh, *this);
    this->line_.computeInterfaceAdjacencyElementQuadrature(mesh, *this);
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasAdjacencyTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.computeInteriorAdjacencyElementQuadrature(mesh, *this);
      this->triangle_.computeBoundaryAdjacencyElementQuadrature(mesh, *this);
    }
    if constexpr (HasAdjacencyQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.computeInteriorAdjacencyElementQuadrature(mesh, *this);
      this->quadrangle_.computeBoundaryAdjacencyElementQuadrature(mesh, *this);
      this->quadrangle_.computeInterfaceAdjacencyElementQuadrature(mesh, *this);
    }
  }
  queue.wait();
}

template <typename AdjacencyElementTrait, typename SimulationControl>
[[nodiscard]] inline Eigen::Ref<
    Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl>::getVariableVolumeGradientAdjacencyQuadrature(
    Solver<SimulationControl>& solver, const Isize parent_gmsh_type_number, const Isize parent_index_each_type,
    const Isize quadrature_node_sequence_in_parent) {
  if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
    return solver.line_.variable_volume_gradient_adjacency_quadrature_(parent_index_each_type)
        .col(quadrature_node_sequence_in_parent);
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
    if (parent_gmsh_type_number == VolumeTriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      return solver.triangle_.variable_volume_gradient_adjacency_quadrature_(parent_index_each_type)
          .col(quadrature_node_sequence_in_parent);
    }
    if (parent_gmsh_type_number == VolumeQuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      return solver.quadrangle_.variable_volume_gradient_adjacency_quadrature_(parent_index_each_type)
          .col(quadrature_node_sequence_in_parent);
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
    if (parent_gmsh_type_number == VolumeTetrahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      return solver.tetrahedron_.variable_volume_gradient_adjacency_quadrature_(parent_index_each_type)
          .col(quadrature_node_sequence_in_parent);
    }
    if (parent_gmsh_type_number == VolumePyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      return solver.pyramid_.variable_volume_gradient_adjacency_quadrature_(parent_index_each_type)
          .col(quadrature_node_sequence_in_parent);
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
    if (parent_gmsh_type_number == VolumePyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      return solver.pyramid_.variable_volume_gradient_adjacency_quadrature_(parent_index_each_type)
          .col(quadrature_node_sequence_in_parent);
    }
    if (parent_gmsh_type_number == VolumeHexahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      return solver.hexahedron_.variable_volume_gradient_adjacency_quadrature_(parent_index_each_type)
          .col(quadrature_node_sequence_in_parent);
    }
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
[[nodiscard]] inline Device::View<
    Device::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
AdjacencyElementSolverDevice<AdjacencyElementTrait, SimulationControl>::getVariableVolumeGradientAdjacencyQuadrature(
    SolverDevice<SimulationControl> solver, const Isize parent_gmsh_type_number, const Isize parent_index_each_type,
    const Isize quadrature_node_sequence_in_parent) {
  if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
    return solver.line_.variable_volume_gradient_adjacency_quadrature_.slice(
        parent_index_each_type, solver.line_.number_,
        Device::Slice<SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>::all(),
        Device::Slice<1>::seqN(quadrature_node_sequence_in_parent));
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
    if (parent_gmsh_type_number == VolumeTriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      return solver.triangle_.variable_volume_gradient_adjacency_quadrature_.slice(
          parent_index_each_type, solver.triangle_.number_,
          Device::Slice<SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>::all(),
          Device::Slice<1>::seqN(quadrature_node_sequence_in_parent));
    }
    if (parent_gmsh_type_number == VolumeQuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      return solver.quadrangle_.variable_volume_gradient_adjacency_quadrature_.slice(
          parent_index_each_type, solver.quadrangle_.number_,
          Device::Slice<SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>::all(),
          Device::Slice<1>::seqN(quadrature_node_sequence_in_parent));
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
    if (parent_gmsh_type_number == VolumeTetrahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      return solver.tetrahedron_.variable_volume_gradient_adjacency_quadrature_.slice(
          parent_index_each_type, solver.tetrahedron_.number_,
          Device::Slice<SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>::all(),
          Device::Slice<1>::seqN(quadrature_node_sequence_in_parent));
    }
    if (parent_gmsh_type_number == VolumePyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      return solver.pyramid_.variable_volume_gradient_adjacency_quadrature_.slice(
          parent_index_each_type, solver.pyramid_.number_,
          Device::Slice<SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>::all(),
          Device::Slice<1>::seqN(quadrature_node_sequence_in_parent));
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
    if (parent_gmsh_type_number == VolumePyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      return solver.pyramid_.variable_volume_gradient_adjacency_quadrature_.slice(
          parent_index_each_type, solver.pyramid_.number_,
          Device::Slice<SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>::all(),
          Device::Slice<1>::seqN(quadrature_node_sequence_in_parent));
    }
    if (parent_gmsh_type_number == VolumeHexahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      return solver.hexahedron_.variable_volume_gradient_adjacency_quadrature_.slice(
          parent_index_each_type, solver.hexahedron_.number_,
          Device::Slice<SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>::all(),
          Device::Slice<1>::seqN(quadrature_node_sequence_in_parent));
    }
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
[[nodiscard]] inline Eigen::Ref<
    Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl>::getVariableInterfaceGradientAdjacencyQuadrature(
    Solver<SimulationControl>& solver, const Isize parent_gmsh_type_number, const Isize parent_index_each_type,
    const Isize quadrature_node_sequence_in_parent) {
  if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
    return solver.line_.variable_interface_gradient_adjacency_quadrature_(parent_index_each_type)
        .col(quadrature_node_sequence_in_parent);
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
    if (parent_gmsh_type_number == VolumeTriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      return solver.triangle_.variable_interface_gradient_adjacency_quadrature_(parent_index_each_type)
          .col(quadrature_node_sequence_in_parent);
    }
    if (parent_gmsh_type_number == VolumeQuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      return solver.quadrangle_.variable_interface_gradient_adjacency_quadrature_(parent_index_each_type)
          .col(quadrature_node_sequence_in_parent);
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
    if (parent_gmsh_type_number == VolumeTetrahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      return solver.tetrahedron_.variable_interface_gradient_adjacency_quadrature_(parent_index_each_type)
          .col(quadrature_node_sequence_in_parent);
    }
    if (parent_gmsh_type_number == VolumePyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      return solver.pyramid_.variable_interface_gradient_adjacency_quadrature_(parent_index_each_type)
          .col(quadrature_node_sequence_in_parent);
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
    if (parent_gmsh_type_number == VolumePyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      return solver.pyramid_.variable_interface_gradient_adjacency_quadrature_(parent_index_each_type)
          .col(quadrature_node_sequence_in_parent);
    }
    if (parent_gmsh_type_number == VolumeHexahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      return solver.hexahedron_.variable_interface_gradient_adjacency_quadrature_(parent_index_each_type)
          .col(quadrature_node_sequence_in_parent);
    }
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
[[nodiscard]] inline Device::View<
    Device::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
AdjacencyElementSolverDevice<AdjacencyElementTrait, SimulationControl>::getVariableInterfaceGradientAdjacencyQuadrature(
    SolverDevice<SimulationControl> solver, const Isize parent_gmsh_type_number, const Isize parent_index_each_type,
    const Isize quadrature_node_sequence_in_parent) {
  if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
    return solver.line_.variable_interface_gradient_adjacency_quadrature_.slice(
        parent_index_each_type, solver.line_.number_,
        Device::Slice<SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>::all(),
        Device::Slice<1>::seqN(quadrature_node_sequence_in_parent));
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
    if (parent_gmsh_type_number == VolumeTriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      return solver.triangle_.variable_interface_gradient_adjacency_quadrature_.slice(
          parent_index_each_type, solver.triangle_.number_,
          Device::Slice<SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>::all(),
          Device::Slice<1>::seqN(quadrature_node_sequence_in_parent));
    }
    if (parent_gmsh_type_number == VolumeQuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      return solver.quadrangle_.variable_interface_gradient_adjacency_quadrature_.slice(
          parent_index_each_type, solver.quadrangle_.number_,
          Device::Slice<SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>::all(),
          Device::Slice<1>::seqN(quadrature_node_sequence_in_parent));
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
    if (parent_gmsh_type_number == VolumeTetrahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      return solver.tetrahedron_.variable_interface_gradient_adjacency_quadrature_.slice(
          parent_index_each_type, solver.tetrahedron_.number_,
          Device::Slice<SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>::all(),
          Device::Slice<1>::seqN(quadrature_node_sequence_in_parent));
    }
    if (parent_gmsh_type_number == VolumePyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      return solver.pyramid_.variable_interface_gradient_adjacency_quadrature_.slice(
          parent_index_each_type, solver.pyramid_.number_,
          Device::Slice<SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>::all(),
          Device::Slice<1>::seqN(quadrature_node_sequence_in_parent));
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
    if (parent_gmsh_type_number == VolumePyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      return solver.pyramid_.variable_interface_gradient_adjacency_quadrature_.slice(
          parent_index_each_type, solver.pyramid_.number_,
          Device::Slice<SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>::all(),
          Device::Slice<1>::seqN(quadrature_node_sequence_in_parent));
    }
    if (parent_gmsh_type_number == VolumeHexahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      return solver.hexahedron_.variable_interface_gradient_adjacency_quadrature_.slice(
          parent_index_each_type, solver.hexahedron_.number_,
          Device::Slice<SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>::all(),
          Device::Slice<1>::seqN(quadrature_node_sequence_in_parent));
    }
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void
AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl>::computeInteriorAdjacencyElementGradientQuadrature(
    const Mesh<SimulationControl>& mesh, Solver<SimulationControl>& solver) {
  const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh =
      mesh.*Mesh<SimulationControl>::template getAdjacencyElement<AdjacencyElementTrait>();
  tbb::parallel_for(
      tbb::blocked_range<Isize>(0, this->interior_number_), [&](const tbb::blocked_range<Isize>& range) -> void {
        for (Isize i = range.begin(); i != range.end(); i++) {
          const Isize element_index = i;
          const std::array<int, AdjacencyElementTrait::kQuadratureNumber> adjacency_element_quadrature_sequence{
              getAdjacencyElementQuadratureSequence<AdjacencyElementTrait::kElementType,
                                                    SimulationControl::kPolynomialOrder>(
                  static_cast<int>(adjacency_element_mesh.adjacency_right_rotation_(element_index)))};
          const Isize left_parent_index_each_type = adjacency_element_mesh.left_parent_index_each_type_(element_index);
          const Isize right_parent_index_each_type =
              adjacency_element_mesh.right_parent_index_each_type_(element_index);
          const Isize adjacency_sequence_in_left_parent =
              adjacency_element_mesh.adjacency_sequence_in_left_parent_(element_index);
          const Isize adjacency_sequence_in_right_parent =
              adjacency_element_mesh.adjacency_sequence_in_right_parent_(element_index);
          const Isize left_parent_gmsh_type_number =
              adjacency_element_mesh.left_parent_gmsh_type_number_(element_index);
          const Isize right_parent_gmsh_type_number_number =
              adjacency_element_mesh.right_parent_gmsh_type_number_(element_index);
          const Isize left_adjacency_accumulate_quadrature_number =
              getAdjacencyElementAccumulateQuadratureNumber<AdjacencyElementTrait, SimulationControl>(
                  left_parent_gmsh_type_number, adjacency_sequence_in_left_parent);
          const Isize right_adjacency_accumulate_quadrature_number =
              getAdjacencyElementAccumulateQuadratureNumber<AdjacencyElementTrait, SimulationControl>(
                  right_parent_gmsh_type_number_number, adjacency_sequence_in_right_parent);
          Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>
              quadrature_node_variable_gradient_adjacency_quadrature;
          Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, AdjacencyElementTrait::kQuadratureNumber>
              left_conserved_variable;
          AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl>::get(
              mesh, solver, left_conserved_variable, left_parent_gmsh_type_number, left_parent_index_each_type,
              adjacency_sequence_in_left_parent);
          Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, AdjacencyElementTrait::kQuadratureNumber>
              right_conserved_variable;
          AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl>::get(
              mesh, solver, right_conserved_variable, right_parent_gmsh_type_number_number,
              right_parent_index_each_type, adjacency_sequence_in_right_parent);
          for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
            const Isize right_quadrature_sequence = adjacency_element_quadrature_sequence[static_cast<Usize>(j)];
            const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector =
                adjacency_element_mesh.normal_vector_(element_index).col(j);
            const Real jacobian_determinant_multiply_weight =
                adjacency_element_mesh.jacobian_determinant_multiply_weight_(element_index)(j);
            quadrature_node_variable_gradient_adjacency_quadrature.setZero();
            GradientFlux<SimulationControl>::computeVariableVolumeGradientInteriorAdjacencyQuadratureNormalFlux(
                normal_vector, left_conserved_variable.col(j), right_conserved_variable.col(right_quadrature_sequence),
                quadrature_node_variable_gradient_adjacency_quadrature, jacobian_determinant_multiply_weight);
            Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
                left_quadrature_node_variable_volume_gradient_adjacency_quadrature =
                    this->getVariableVolumeGradientAdjacencyQuadrature(solver, left_parent_gmsh_type_number,
                                                                       left_parent_index_each_type,
                                                                       left_adjacency_accumulate_quadrature_number + j);
            Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
                right_quadrature_node_variable_volume_gradient_adjacency_quadrature =
                    this->getVariableVolumeGradientAdjacencyQuadrature(
                        solver, right_parent_gmsh_type_number_number, right_parent_index_each_type,
                        right_adjacency_accumulate_quadrature_number + right_quadrature_sequence);
            left_quadrature_node_variable_volume_gradient_adjacency_quadrature =
                quadrature_node_variable_gradient_adjacency_quadrature;
            right_quadrature_node_variable_volume_gradient_adjacency_quadrature =
                -quadrature_node_variable_gradient_adjacency_quadrature;
            if constexpr (IsNS<SimulationControl::kEquationModel>) {
              quadrature_node_variable_gradient_adjacency_quadrature.setZero();
              GradientFlux<SimulationControl>::computeVariableInterfaceGradientInteriorAdjacencyQuadratureNormalFlux(
                  normal_vector, left_conserved_variable.col(j),
                  right_conserved_variable.col(right_quadrature_sequence),
                  quadrature_node_variable_gradient_adjacency_quadrature, jacobian_determinant_multiply_weight);
              Eigen::Ref<
                  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
                  left_quadrature_node_variable_interface_gradient_adjacency_quadrature =
                      this->getVariableInterfaceGradientAdjacencyQuadrature(
                          solver, left_parent_gmsh_type_number, left_parent_index_each_type,
                          left_adjacency_accumulate_quadrature_number + j);
              Eigen::Ref<
                  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
                  right_quadrature_node_variable_interface_gradient_adjacency_quadrature =
                      this->getVariableInterfaceGradientAdjacencyQuadrature(
                          solver, right_parent_gmsh_type_number_number, right_parent_index_each_type,
                          right_adjacency_accumulate_quadrature_number + right_quadrature_sequence);
              // NOTE: Here h_int(u^-,u^+;n)=(u^+ - u^-)n/2, so the interface gradient flux on the left and right have
              // the same sign
              left_quadrature_node_variable_interface_gradient_adjacency_quadrature =
                  quadrature_node_variable_gradient_adjacency_quadrature;
              right_quadrature_node_variable_interface_gradient_adjacency_quadrature =
                  quadrature_node_variable_gradient_adjacency_quadrature;
            }
          }
        }
      });
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void
AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl>::computeBoundaryAdjacencyElementGradientQuadrature(
    const Mesh<SimulationControl>& mesh, Solver<SimulationControl>& solver) {
  const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh =
      mesh.*Mesh<SimulationControl>::template getAdjacencyElement<AdjacencyElementTrait>();
  tbb::parallel_for(
      tbb::blocked_range<Isize>(0, this->boundary_number_), [&](const tbb::blocked_range<Isize>& range) -> void {
        for (Isize i = range.begin(); i != range.end(); i++) {
          const Isize element_index = i + this->interior_number_;
          const Isize left_parent_index_each_type = adjacency_element_mesh.left_parent_index_each_type_(element_index);
          const Isize adjacency_sequence_in_left_parent =
              adjacency_element_mesh.adjacency_sequence_in_left_parent_(element_index);
          const Isize left_parent_gmsh_type_number =
              adjacency_element_mesh.left_parent_gmsh_type_number_(element_index);
          const Isize left_adjacency_accumulate_quadrature_number =
              getAdjacencyElementAccumulateQuadratureNumber<AdjacencyElementTrait, SimulationControl>(
                  left_parent_gmsh_type_number, adjacency_sequence_in_left_parent);
          const BoundaryConditionEnum boundary_condition_type =
              adjacency_element_mesh.boundary_condition_type_(element_index);
          // NOTE: Here, the variable for the interface gradient is stored because its calculation requires the
          // boundary variable, which is costly. Therefore, it is calculated and stored, even in the Euler equations,
          // for possible subsequent use in the NS equations.
          Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>
              boundary_quadrature_node_interface_gradient_variable;
          Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>
              quadrature_node_variable_gradient_adjacency_quadrature;
          Eigen::Matrix<Real, SimulationControl::kDimension, AdjacencyElementTrait::kQuadratureNumber>
              rotation_velocity;
          AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl>::getRotationVelocity(
              mesh, solver, rotation_velocity, left_parent_gmsh_type_number, left_parent_index_each_type,
              adjacency_sequence_in_left_parent);
          Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, AdjacencyElementTrait::kQuadratureNumber>
              left_conserved_variable;
          AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl>::get(
              mesh, solver, left_conserved_variable, left_parent_gmsh_type_number, left_parent_index_each_type,
              adjacency_sequence_in_left_parent);
          for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
            const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
                right_quadrature_node_computational_variable =
                    this->boundary_dummy_right_computational_variable_(i).col(j);  // the index use i not element_index
            const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector =
                adjacency_element_mesh.normal_vector_(element_index).col(j);
            const Real jacobian_determinant_multiply_weight =
                adjacency_element_mesh.jacobian_determinant_multiply_weight_(element_index)(j);
            quadrature_node_variable_gradient_adjacency_quadrature.setZero();
            GradientFlux<SimulationControl>::computeVariableVolumeGradientBoundaryAdjacencyQuadratureNormalFlux(
                normal_vector, rotation_velocity.col(j), left_conserved_variable.col(j),
                right_quadrature_node_computational_variable, boundary_quadrature_node_interface_gradient_variable,
                quadrature_node_variable_gradient_adjacency_quadrature, boundary_condition_type,
                jacobian_determinant_multiply_weight);
            Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
                left_quadrature_node_variable_volume_gradient_adjacency_quadrature =
                    this->getVariableVolumeGradientAdjacencyQuadrature(solver, left_parent_gmsh_type_number,
                                                                       left_parent_index_each_type,
                                                                       left_adjacency_accumulate_quadrature_number + j);
            left_quadrature_node_variable_volume_gradient_adjacency_quadrature =
                quadrature_node_variable_gradient_adjacency_quadrature;
            if constexpr (IsNS<SimulationControl::kEquationModel>) {
              quadrature_node_variable_gradient_adjacency_quadrature.setZero();
              GradientFlux<SimulationControl>::computeVariableInterfaceGradientBoundaryAdjacencyQuadratureNormalFlux(
                  normal_vector, boundary_quadrature_node_interface_gradient_variable,
                  quadrature_node_variable_gradient_adjacency_quadrature, jacobian_determinant_multiply_weight);
              Eigen::Ref<
                  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
                  left_quadrature_node_variable_interface_gradient_adjacency_quadrature =
                      this->getVariableInterfaceGradientAdjacencyQuadrature(
                          solver, left_parent_gmsh_type_number, left_parent_index_each_type,
                          left_adjacency_accumulate_quadrature_number + j);
              left_quadrature_node_variable_interface_gradient_adjacency_quadrature =
                  quadrature_node_variable_gradient_adjacency_quadrature;
            }
          }
        }
      });
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void
AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl>::computeInterfaceAdjacencyElementGradientQuadrature(
    const Mesh<SimulationControl>& mesh, Solver<SimulationControl>& solver) {
  const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh =
      mesh.*Mesh<SimulationControl>::template getAdjacencyElement<AdjacencyElementTrait>();
  tbb::parallel_for(
      tbb::blocked_range<Isize>(0, 2 * this->interface_number_), [&](const tbb::blocked_range<Isize>& range) -> void {
        for (Isize i = range.begin(); i != range.end(); i++) {
          const Isize element_index = i + this->interior_number_ + this->boundary_number_;
          const Isize left_parent_index_each_type = adjacency_element_mesh.left_parent_index_each_type_(element_index);
          const Isize adjacency_sequence_in_left_parent =
              adjacency_element_mesh.adjacency_sequence_in_left_parent_(element_index);
          const Isize left_parent_gmsh_type_number =
              adjacency_element_mesh.left_parent_gmsh_type_number_(element_index);
          const Isize left_adjacency_accumulate_quadrature_number =
              getAdjacencyElementAccumulateQuadratureNumber<AdjacencyElementTrait, SimulationControl>(
                  left_parent_gmsh_type_number, adjacency_sequence_in_left_parent);
          Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>
              quadrature_node_variable_gradient_adjacency_quadrature;
          Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, AdjacencyElementTrait::kQuadratureNumber>
              left_conserved_variable;
          AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl>::get(
              mesh, solver, left_conserved_variable, left_parent_gmsh_type_number, left_parent_index_each_type,
              adjacency_sequence_in_left_parent);
          for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
            const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>&
                right_quadrature_node_conserved_variable =
                    this->interface_right_conserved_variable_(i).col(j);  // the index use i not element_index

            const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector =
                adjacency_element_mesh.normal_vector_(element_index).col(j);
            const Real jacobian_determinant_multiply_weight =
                adjacency_element_mesh.jacobian_determinant_multiply_weight_(element_index)(j);
            quadrature_node_variable_gradient_adjacency_quadrature.setZero();
            GradientFlux<SimulationControl>::computeVariableVolumeGradientInteriorAdjacencyQuadratureNormalFlux(
                normal_vector, left_conserved_variable.col(j), right_quadrature_node_conserved_variable,
                quadrature_node_variable_gradient_adjacency_quadrature, jacobian_determinant_multiply_weight);
            Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
                left_quadrature_node_variable_volume_gradient_adjacency_quadrature =
                    this->getVariableVolumeGradientAdjacencyQuadrature(solver, left_parent_gmsh_type_number,
                                                                       left_parent_index_each_type,
                                                                       left_adjacency_accumulate_quadrature_number + j);
            left_quadrature_node_variable_volume_gradient_adjacency_quadrature =
                quadrature_node_variable_gradient_adjacency_quadrature;
            if constexpr (IsNS<SimulationControl::kEquationModel>) {
              quadrature_node_variable_gradient_adjacency_quadrature.setZero();
              GradientFlux<SimulationControl>::computeVariableInterfaceGradientInteriorAdjacencyQuadratureNormalFlux(
                  normal_vector, left_conserved_variable.col(j), right_quadrature_node_conserved_variable,
                  quadrature_node_variable_gradient_adjacency_quadrature, jacobian_determinant_multiply_weight);
              Eigen::Ref<
                  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
                  left_quadrature_node_variable_interface_gradient_adjacency_quadrature =
                      this->getVariableInterfaceGradientAdjacencyQuadrature(
                          solver, left_parent_gmsh_type_number, left_parent_index_each_type,
                          left_adjacency_accumulate_quadrature_number + j);
              left_quadrature_node_variable_interface_gradient_adjacency_quadrature =
                  quadrature_node_variable_gradient_adjacency_quadrature;
            }
          }
        }
      });
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::computeAdjacencyGradientQuadrature(const Mesh<SimulationControl>& mesh) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->point_.computeInteriorAdjacencyElementGradientQuadrature(mesh, *this);
    this->point_.computeBoundaryAdjacencyElementGradientQuadrature(mesh, *this);
  } else if constexpr (SimulationControl::kDimension == 2) {
    this->line_.computeInteriorAdjacencyElementGradientQuadrature(mesh, *this);
    this->line_.computeBoundaryAdjacencyElementGradientQuadrature(mesh, *this);
    this->line_.computeInterfaceAdjacencyElementGradientQuadrature(mesh, *this);
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasAdjacencyTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.computeInteriorAdjacencyElementGradientQuadrature(mesh, *this);
      this->triangle_.computeBoundaryAdjacencyElementGradientQuadrature(mesh, *this);
    }
    if constexpr (HasAdjacencyQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.computeInteriorAdjacencyElementGradientQuadrature(mesh, *this);
      this->quadrangle_.computeBoundaryAdjacencyElementGradientQuadrature(mesh, *this);
      this->quadrangle_.computeInterfaceAdjacencyElementGradientQuadrature(mesh, *this);
    }
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementSolverDevice<AdjacencyElementTrait, SimulationControl>::
    computeInteriorAdjacencyElementGradientQuadrature(const MeshDevice<SimulationControl>& mesh,
                                                      SolverDevice<SimulationControl>& solver) {
  const AdjacencyElementMeshDevice<AdjacencyElementTrait>& adjacency_element_mesh =
      mesh.*MeshDevice<SimulationControl>::template getAdjacencyElement<AdjacencyElementTrait>();
  queue.submit([&](sycl::handler& cgh) -> void {
    cgh.parallel_for(getNdRange(this->interior_number_), [=, this](sycl::nd_item<1> index) -> void {
      const auto i = static_cast<Isize>(index.get_global_id(0));
      if (i >= this->interior_number_) {
        return;
      }
      const Isize element_index = i;
      const std::array<int, AdjacencyElementTrait::kQuadratureNumber> adjacency_element_quadrature_sequence{
          getAdjacencyElementQuadratureSequence<AdjacencyElementTrait::kElementType,
                                                SimulationControl::kPolynomialOrder>(
              static_cast<int>(adjacency_element_mesh.adjacency_right_rotation_(element_index)))};
      const Isize left_parent_index_each_type = adjacency_element_mesh.left_parent_index_each_type_(element_index);
      const Isize right_parent_index_each_type = adjacency_element_mesh.right_parent_index_each_type_(element_index);
      const Isize adjacency_sequence_in_left_parent =
          adjacency_element_mesh.adjacency_sequence_in_left_parent_(element_index);
      const Isize adjacency_sequence_in_right_parent =
          adjacency_element_mesh.adjacency_sequence_in_right_parent_(element_index);
      const Isize left_parent_gmsh_type_number = adjacency_element_mesh.left_parent_gmsh_type_number_(element_index);
      const Isize right_parent_gmsh_type_number_number =
          adjacency_element_mesh.right_parent_gmsh_type_number_(element_index);
      const Isize left_adjacency_accumulate_quadrature_number =
          getAdjacencyElementAccumulateQuadratureNumber<AdjacencyElementTrait, SimulationControl>(
              left_parent_gmsh_type_number, adjacency_sequence_in_left_parent);
      const Isize right_adjacency_accumulate_quadrature_number =
          getAdjacencyElementAccumulateQuadratureNumber<AdjacencyElementTrait, SimulationControl>(
              right_parent_gmsh_type_number_number, adjacency_sequence_in_right_parent);
      Device::StaticVector<Real, SimulationControl::kConservedVariableNumber> left_quadrature_node_conserved_variable;
      Device::StaticVector<Real, SimulationControl::kConservedVariableNumber> right_quadrature_node_conserved_variable;
      Device::StaticVector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>
          quadrature_node_variable_gradient_adjacency_quadrature;
      for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
        const Isize right_quadrature_sequence = adjacency_element_quadrature_sequence[static_cast<Usize>(j)];
        AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::get(
            mesh, solver, left_quadrature_node_conserved_variable, left_parent_gmsh_type_number,
            left_parent_index_each_type, adjacency_sequence_in_left_parent, j);
        AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::get(
            mesh, solver, right_quadrature_node_conserved_variable, right_parent_gmsh_type_number_number,
            right_parent_index_each_type, adjacency_sequence_in_right_parent, right_quadrature_sequence);
        const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector =
            adjacency_element_mesh.normal_vector_.slice(element_index, adjacency_element_mesh.number_,
                                                        Device::Slice<SimulationControl::kDimension>::all(),
                                                        Device::Slice<1>::seqN(j));
        const Real jacobian_determinant_multiply_weight =
            adjacency_element_mesh.jacobian_determinant_multiply_weight_.view(element_index,
                                                                              adjacency_element_mesh.number_)(j);
        quadrature_node_variable_gradient_adjacency_quadrature.setZero();
        GradientFluxDevice<SimulationControl>::computeVariableVolumeGradientInteriorAdjacencyQuadratureNormalFlux(
            normal_vector, left_quadrature_node_conserved_variable, right_quadrature_node_conserved_variable,
            quadrature_node_variable_gradient_adjacency_quadrature, jacobian_determinant_multiply_weight);
        Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
            left_quadrature_node_variable_volume_gradient_adjacency_quadrature =
                this->getVariableVolumeGradientAdjacencyQuadrature(solver, left_parent_gmsh_type_number,
                                                                   left_parent_index_each_type,
                                                                   left_adjacency_accumulate_quadrature_number + j);
        Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
            right_quadrature_node_variable_volume_gradient_adjacency_quadrature =
                this->getVariableVolumeGradientAdjacencyQuadrature(
                    solver, right_parent_gmsh_type_number_number, right_parent_index_each_type,
                    right_adjacency_accumulate_quadrature_number + right_quadrature_sequence);
        for (Isize m = 0; m < SimulationControl::kConservedVariableNumber * SimulationControl::kDimension; m++) {
          left_quadrature_node_variable_volume_gradient_adjacency_quadrature(m) =
              quadrature_node_variable_gradient_adjacency_quadrature(m);
          right_quadrature_node_variable_volume_gradient_adjacency_quadrature(m) =
              -quadrature_node_variable_gradient_adjacency_quadrature(m);
        }
        if constexpr (IsNS<SimulationControl::kEquationModel>) {
          quadrature_node_variable_gradient_adjacency_quadrature.setZero();
          GradientFluxDevice<SimulationControl>::computeVariableInterfaceGradientInteriorAdjacencyQuadratureNormalFlux(
              normal_vector, left_quadrature_node_conserved_variable, right_quadrature_node_conserved_variable,
              quadrature_node_variable_gradient_adjacency_quadrature, jacobian_determinant_multiply_weight);
          Device::View<
              Device::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
              left_quadrature_node_variable_interface_gradient_adjacency_quadrature =
                  this->getVariableInterfaceGradientAdjacencyQuadrature(
                      solver, left_parent_gmsh_type_number, left_parent_index_each_type,
                      left_adjacency_accumulate_quadrature_number + j);
          Device::View<
              Device::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
              right_quadrature_node_variable_interface_gradient_adjacency_quadrature =
                  this->getVariableInterfaceGradientAdjacencyQuadrature(
                      solver, right_parent_gmsh_type_number_number, right_parent_index_each_type,
                      right_adjacency_accumulate_quadrature_number + right_quadrature_sequence);
          // NOTE: Here h_int(u^-,u^+;n)=(u^+ - u^-)n/2, so the interface gradient flux on the left and right have the
          // same sign
          for (Isize m = 0; m < SimulationControl::kConservedVariableNumber * SimulationControl::kDimension; m++) {
            left_quadrature_node_variable_interface_gradient_adjacency_quadrature(m) =
                quadrature_node_variable_gradient_adjacency_quadrature(m);
            right_quadrature_node_variable_interface_gradient_adjacency_quadrature(m) =
                quadrature_node_variable_gradient_adjacency_quadrature(m);
          }
        }
      }
    });
  });
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementSolverDevice<AdjacencyElementTrait, SimulationControl>::
    computeBoundaryAdjacencyElementGradientQuadrature(const MeshDevice<SimulationControl>& mesh,
                                                      SolverDevice<SimulationControl>& solver) {
  const AdjacencyElementMeshDevice<AdjacencyElementTrait>& adjacency_element_mesh =
      mesh.*MeshDevice<SimulationControl>::template getAdjacencyElement<AdjacencyElementTrait>();
  queue.submit([&](sycl::handler& cgh) -> void {
    cgh.parallel_for(
        getNdRange(this->boundary_number_, AdjacencyElementTrait::kQuadratureNumber),
        [=, this](sycl::nd_item<2> index) -> void {
          const auto i = static_cast<Isize>(index.get_global_id(0));
          const auto j = static_cast<Isize>(index.get_global_id(1));
          if (i >= this->boundary_number_ || j >= AdjacencyElementTrait::kQuadratureNumber) {
            return;
          }
          const Isize element_index = i + this->interior_number_;
          const AdjacencyElementSolverDevice<AdjacencyElementTrait, SimulationControl>* self = this;
          const Isize left_parent_index_each_type = adjacency_element_mesh.left_parent_index_each_type_(element_index);
          const Isize adjacency_sequence_in_left_parent =
              adjacency_element_mesh.adjacency_sequence_in_left_parent_(element_index);
          const Isize left_parent_gmsh_type_number =
              adjacency_element_mesh.left_parent_gmsh_type_number_(element_index);
          const Isize left_adjacency_accumulate_quadrature_number =
              getAdjacencyElementAccumulateQuadratureNumber<AdjacencyElementTrait, SimulationControl>(
                  left_parent_gmsh_type_number, adjacency_sequence_in_left_parent);
          const BoundaryConditionEnum boundary_condition_type =
              adjacency_element_mesh.boundary_condition_type_(element_index);
          Device::StaticVector<Real, SimulationControl::kDimension> quadrature_node_rotation_velocity;
          Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>
              left_quadrature_node_conserved_variable;
          Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>
              right_quadrature_node_computational_variable;
          Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>
              boundary_quadrature_node_interface_gradient_variable;
          Device::StaticVector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>
              quadrature_node_variable_gradient_adjacency_quadrature;
          AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::getRotationVelocity(
              mesh, solver, quadrature_node_rotation_velocity, left_parent_gmsh_type_number,
              left_parent_index_each_type, adjacency_sequence_in_left_parent, j);
          AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::get(
              mesh, solver, left_quadrature_node_conserved_variable, left_parent_gmsh_type_number,
              left_parent_index_each_type, adjacency_sequence_in_left_parent, j);
          const Device::View<const Device::Vector<Real, SimulationControl::kComputationalVariableNumber>>
              boundary_dummy_right_computational_variable = self->boundary_dummy_right_computational_variable_.slice(
                  i, this->boundary_number_, Device::Slice<SimulationControl::kComputationalVariableNumber>::all(),
                  Device::Slice<1>::seqN(j));  // the index use i not element_index
          for (Isize m = 0; m < SimulationControl::kComputationalVariableNumber; m++) {
            right_quadrature_node_computational_variable(m) = boundary_dummy_right_computational_variable(m);
          }
          const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector =
              adjacency_element_mesh.normal_vector_.slice(element_index, adjacency_element_mesh.number_,
                                                          Device::Slice<SimulationControl::kDimension>::all(),
                                                          Device::Slice<1>::seqN(j));
          const Real jacobian_determinant_multiply_weight =
              adjacency_element_mesh.jacobian_determinant_multiply_weight_.view(element_index,
                                                                                adjacency_element_mesh.number_)(j);
          quadrature_node_variable_gradient_adjacency_quadrature.setZero();
          GradientFluxDevice<SimulationControl>::computeVariableVolumeGradientBoundaryAdjacencyQuadratureNormalFlux(
              normal_vector, quadrature_node_rotation_velocity, left_quadrature_node_conserved_variable,
              right_quadrature_node_computational_variable, boundary_quadrature_node_interface_gradient_variable,
              quadrature_node_variable_gradient_adjacency_quadrature, boundary_condition_type,
              jacobian_determinant_multiply_weight);
          Device::View<
              Device::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
              left_quadrature_node_variable_volume_gradient_adjacency_quadrature =
                  this->getVariableVolumeGradientAdjacencyQuadrature(solver, left_parent_gmsh_type_number,
                                                                     left_parent_index_each_type,
                                                                     left_adjacency_accumulate_quadrature_number + j);
          for (Isize m = 0; m < SimulationControl::kConservedVariableNumber * SimulationControl::kDimension; m++) {
            left_quadrature_node_variable_volume_gradient_adjacency_quadrature(m) =
                quadrature_node_variable_gradient_adjacency_quadrature(m);
          }
          if constexpr (IsNS<SimulationControl::kEquationModel>) {
            quadrature_node_variable_gradient_adjacency_quadrature.setZero();
            GradientFluxDevice<SimulationControl>::
                computeVariableInterfaceGradientBoundaryAdjacencyQuadratureNormalFlux(
                    normal_vector, boundary_quadrature_node_interface_gradient_variable,
                    quadrature_node_variable_gradient_adjacency_quadrature, jacobian_determinant_multiply_weight);
            Device::View<
                Device::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
                left_quadrature_node_variable_interface_gradient_adjacency_quadrature =
                    this->getVariableInterfaceGradientAdjacencyQuadrature(
                        solver, left_parent_gmsh_type_number, left_parent_index_each_type,
                        left_adjacency_accumulate_quadrature_number + j);
            for (Isize m = 0; m < SimulationControl::kConservedVariableNumber * SimulationControl::kDimension; m++) {
              left_quadrature_node_variable_interface_gradient_adjacency_quadrature(m) =
                  quadrature_node_variable_gradient_adjacency_quadrature(m);
            }
          }
        });
  });
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementSolverDevice<AdjacencyElementTrait, SimulationControl>::
    computeInterfaceAdjacencyElementGradientQuadrature(const MeshDevice<SimulationControl>& mesh,
                                                       SolverDevice<SimulationControl>& solver) {
  const AdjacencyElementMeshDevice<AdjacencyElementTrait>& adjacency_element_mesh =
      mesh.*MeshDevice<SimulationControl>::template getAdjacencyElement<AdjacencyElementTrait>();
  queue.submit([&](sycl::handler& cgh) -> void {
    cgh.parallel_for(
        getNdRange(2 * this->interface_number_, AdjacencyElementTrait::kQuadratureNumber),
        [=, this](sycl::nd_item<2> index) -> void {
          const auto i = static_cast<Isize>(index.get_global_id(0));
          const auto j = static_cast<Isize>(index.get_global_id(1));
          if (i >= 2 * this->interface_number_ || j >= AdjacencyElementTrait::kQuadratureNumber) {
            return;
          }
          const Isize element_index = i + this->interior_number_ + this->boundary_number_;
          const AdjacencyElementSolverDevice<AdjacencyElementTrait, SimulationControl>* self = this;
          const Isize left_parent_index_each_type = adjacency_element_mesh.left_parent_index_each_type_(element_index);
          const Isize adjacency_sequence_in_left_parent =
              adjacency_element_mesh.adjacency_sequence_in_left_parent_(element_index);
          const Isize left_parent_gmsh_type_number =
              adjacency_element_mesh.left_parent_gmsh_type_number_(element_index);
          const Isize left_adjacency_accumulate_quadrature_number =
              getAdjacencyElementAccumulateQuadratureNumber<AdjacencyElementTrait, SimulationControl>(
                  left_parent_gmsh_type_number, adjacency_sequence_in_left_parent);
          Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>
              left_quadrature_node_conserved_variable;
          Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>
              right_quadrature_node_conserved_variable;
          Device::StaticVector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>
              quadrature_node_variable_gradient_adjacency_quadrature;
          AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::get(
              mesh, solver, left_quadrature_node_conserved_variable, left_parent_gmsh_type_number,
              left_parent_index_each_type, adjacency_sequence_in_left_parent, j);
          const Device::View<const Device::Vector<Real, SimulationControl::kConservedVariableNumber>>
              interface_right_conserved_variable = self->interface_right_conserved_variable_.slice(
                  i, 2 * this->interface_number_, Device::Slice<SimulationControl::kConservedVariableNumber>::all(),
                  Device::Slice<1>::seqN(j));  // the index use i not element_index
          for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
            right_quadrature_node_conserved_variable(m) = interface_right_conserved_variable(m);
          }
          const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector =
              adjacency_element_mesh.normal_vector_.slice(element_index, adjacency_element_mesh.number_,
                                                          Device::Slice<SimulationControl::kDimension>::all(),
                                                          Device::Slice<1>::seqN(j));
          const Real jacobian_determinant_multiply_weight =
              adjacency_element_mesh.jacobian_determinant_multiply_weight_.view(element_index,
                                                                                adjacency_element_mesh.number_)(j);
          quadrature_node_variable_gradient_adjacency_quadrature.setZero();
          GradientFluxDevice<SimulationControl>::computeVariableVolumeGradientInteriorAdjacencyQuadratureNormalFlux(
              normal_vector, left_quadrature_node_conserved_variable, right_quadrature_node_conserved_variable,
              quadrature_node_variable_gradient_adjacency_quadrature, jacobian_determinant_multiply_weight);
          Device::View<
              Device::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
              left_quadrature_node_variable_volume_gradient_adjacency_quadrature =
                  this->getVariableVolumeGradientAdjacencyQuadrature(solver, left_parent_gmsh_type_number,
                                                                     left_parent_index_each_type,
                                                                     left_adjacency_accumulate_quadrature_number + j);
          for (Isize m = 0; m < SimulationControl::kConservedVariableNumber * SimulationControl::kDimension; m++) {
            left_quadrature_node_variable_volume_gradient_adjacency_quadrature(m) =
                quadrature_node_variable_gradient_adjacency_quadrature(m);
          }
          if constexpr (IsNS<SimulationControl::kEquationModel>) {
            quadrature_node_variable_gradient_adjacency_quadrature.setZero();
            GradientFluxDevice<SimulationControl>::
                computeVariableInterfaceGradientInteriorAdjacencyQuadratureNormalFlux(
                    normal_vector, left_quadrature_node_conserved_variable, right_quadrature_node_conserved_variable,
                    quadrature_node_variable_gradient_adjacency_quadrature, jacobian_determinant_multiply_weight);
            Device::View<
                Device::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
                left_quadrature_node_variable_interface_gradient_adjacency_quadrature =
                    this->getVariableInterfaceGradientAdjacencyQuadrature(
                        solver, left_parent_gmsh_type_number, left_parent_index_each_type,
                        left_adjacency_accumulate_quadrature_number + j);
            for (Isize m = 0; m < SimulationControl::kConservedVariableNumber * SimulationControl::kDimension; m++) {
              left_quadrature_node_variable_interface_gradient_adjacency_quadrature(m) =
                  quadrature_node_variable_gradient_adjacency_quadrature(m);
            }
          }
        });
  });
}

template <typename SimulationControl>
inline void SolverDevice<SimulationControl>::computeAdjacencyGradientQuadrature(
    const MeshDevice<SimulationControl>& mesh) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->point_.computeInteriorAdjacencyElementGradientQuadrature(mesh, *this);
    this->point_.computeBoundaryAdjacencyElementGradientQuadrature(mesh, *this);
  } else if constexpr (SimulationControl::kDimension == 2) {
    this->line_.computeInteriorAdjacencyElementGradientQuadrature(mesh, *this);
    this->line_.computeBoundaryAdjacencyElementGradientQuadrature(mesh, *this);
    this->line_.computeInterfaceAdjacencyElementGradientQuadrature(mesh, *this);
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasAdjacencyTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.computeInteriorAdjacencyElementGradientQuadrature(mesh, *this);
      this->triangle_.computeBoundaryAdjacencyElementGradientQuadrature(mesh, *this);
    }
    if constexpr (HasAdjacencyQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.computeInteriorAdjacencyElementGradientQuadrature(mesh, *this);
      this->quadrangle_.computeBoundaryAdjacencyElementGradientQuadrature(mesh, *this);
      this->quadrangle_.computeInterfaceAdjacencyElementGradientQuadrature(mesh, *this);
    }
  }
  queue.wait();
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_SPATIAL_DISCRETE_CPP_
