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

#include <Eigen/Core>
#include <array>

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
  std::unreachable();
}

template <typename VolumeElementTrait, typename SimulationControl>
inline void VolumeElementSolver<VolumeElementTrait, SimulationControl>::computeVolumeElementQuadrature(
    const VolumeElementMesh<VolumeElementTrait>& volume_element_mesh,
    [[maybe_unused]] const SourceTerm<SimulationControl>& source_term) {
  tbb::parallel_for(tbb::blocked_range<Isize>(0, this->number_), [&](const tbb::blocked_range<Isize>& range) -> void {
    for (Isize i = range.begin(); i != range.end(); i++) {
      if constexpr (IsEuler<SimulationControl::kEquationModel>) {
        Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> quadrature_node_conserved_variable;
        Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber> quadrature_node_computational_variable;
        for (Isize j = 0; j < VolumeElementTrait::kQuadratureNumber; j++) {
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
      }
      if constexpr (IsNS<SimulationControl::kEquationModel>) {
        Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> quadrature_node_conserved_variable;
        Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber> quadrature_node_computational_variable;
        Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>
            quadrature_node_conserved_variable_gradient;
        Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>
            quadrature_node_primitive_variable_gradient;
        for (Isize j = 0; j < VolumeElementTrait::kQuadratureNumber; j++) {
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
inline void Solver<SimulationControl>::computeQuadrature(
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
        Device::StaticVector<Real, SimulationControl::kConservedVariableNumber> quadrature_node_conserved_variable;
        Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>
            quadrature_node_computational_variable;
        for (Isize j = 0; j < VolumeElementTrait::kQuadratureNumber; j++) {
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
      }
      if constexpr (IsNS<SimulationControl::kEquationModel>) {
        Device::StaticVector<Real, SimulationControl::kConservedVariableNumber> quadrature_node_conserved_variable;
        Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>
            quadrature_node_computational_variable;
        Device::StaticVector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>
            quadrature_node_conserved_variable_gradient;
        Device::StaticVector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>
            quadrature_node_primitive_variable_gradient;
        for (Isize j = 0; j < VolumeElementTrait::kQuadratureNumber; j++) {
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
inline void SolverDevice<SimulationControl>::computeQuadrature(
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
inline void Solver<SimulationControl>::computeGradientQuadrature(const Mesh<SimulationControl>& mesh) {
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
inline void SolverDevice<SimulationControl>::computeGradientQuadrature(const MeshDevice<SimulationControl>& mesh) {
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
  std::unreachable();
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
  std::unreachable();
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl>::computeInteriorAdjacencyElementQuadrature(
    const Mesh<SimulationControl>& mesh, Solver<SimulationControl>& solver) {
  const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh =
      mesh.*(Mesh<SimulationControl>::template getAdjacencyElement<AdjacencyElementTrait>());
  tbb::parallel_for(
      tbb::blocked_range<Isize>(0, this->interior_number_), [&](const tbb::blocked_range<Isize>& range) -> void {
        for (Isize i = range.begin(); i != range.end(); i++) {
          const std::array<int, AdjacencyElementTrait::kQuadratureNumber> adjacency_element_quadrature_sequence{
              getAdjacencyElementQuadratureSequence<AdjacencyElementTrait::kElementType,
                                                    SimulationControl::kPolynomialOrder>(
                  static_cast<int>(adjacency_element_mesh.adjacency_right_rotation_(i)))};
          const Isize left_parent_index_each_type = adjacency_element_mesh.left_parent_index_each_type_(i);
          const Isize right_parent_index_each_type = adjacency_element_mesh.right_parent_index_each_type_(i);
          const Isize adjacency_sequence_in_left_parent = adjacency_element_mesh.adjacency_sequence_in_left_parent_(i);
          const Isize adjacency_sequence_in_right_parent =
              adjacency_element_mesh.adjacency_sequence_in_right_parent_(i);
          const Isize left_parent_gmsh_type_number = adjacency_element_mesh.left_parent_gmsh_type_number_(i);
          const Isize right_parent_gmsh_type_number = adjacency_element_mesh.right_parent_gmsh_type_number_(i);
          const Isize left_adjacency_accumulate_quadrature_number =
              getAdjacencyElementAccumulateQuadratureNumber<AdjacencyElementTrait, SimulationControl>(
                  left_parent_gmsh_type_number, adjacency_sequence_in_left_parent);
          const Isize right_adjacency_accumulate_quadrature_number =
              getAdjacencyElementAccumulateQuadratureNumber<AdjacencyElementTrait, SimulationControl>(
                  right_parent_gmsh_type_number, adjacency_sequence_in_right_parent);
          if constexpr (IsEuler<SimulationControl::kEquationModel>) {
            Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> left_quadrature_node_conserved_variable;
            Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> right_quadrature_node_conserved_variable;
            Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>
                left_quadrature_node_computational_variable;
            Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>
                right_quadrature_node_computational_variable;
            for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
              AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl>::get(
                  mesh, solver, left_quadrature_node_conserved_variable, left_parent_gmsh_type_number,
                  left_parent_index_each_type, adjacency_sequence_in_left_parent, j);
              AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl>::get(
                  mesh, solver, right_quadrature_node_conserved_variable, right_parent_gmsh_type_number,
                  right_parent_index_each_type, adjacency_sequence_in_right_parent,
                  adjacency_element_quadrature_sequence[static_cast<Usize>(j)]);
              Variable<SimulationControl>::convertComputationalFromConserved(
                  left_quadrature_node_conserved_variable, left_quadrature_node_computational_variable);
              Variable<SimulationControl>::convertComputationalFromConserved(
                  right_quadrature_node_conserved_variable, right_quadrature_node_computational_variable);
              Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>>
                  left_quadrature_node_variable_adjacency_quadrature = this->getVariableAdjacencyQuadrature(
                      solver, left_parent_gmsh_type_number, left_parent_index_each_type,
                      left_adjacency_accumulate_quadrature_number + j);
              Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>>
                  right_quadrature_node_variable_adjacency_quadrature = this->getVariableAdjacencyQuadrature(
                      solver, right_parent_gmsh_type_number, right_parent_index_each_type,
                      right_adjacency_accumulate_quadrature_number +
                          adjacency_element_quadrature_sequence[static_cast<Usize>(j)]);
              const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector =
                  adjacency_element_mesh.normal_vector_(i).col(j);
              left_quadrature_node_variable_adjacency_quadrature.setZero();
              right_quadrature_node_variable_adjacency_quadrature.setZero();
              ConvectiveFlux<SimulationControl>::addVariableInteriorAdjacencyQuadratureNormalFlux(
                  normal_vector, left_quadrature_node_conserved_variable, left_quadrature_node_computational_variable,
                  right_quadrature_node_conserved_variable, right_quadrature_node_computational_variable,
                  left_quadrature_node_variable_adjacency_quadrature,
                  right_quadrature_node_variable_adjacency_quadrature,
                  adjacency_element_mesh.jacobian_determinant_multiply_weight_(i)(j));
            }
          }
          if constexpr (IsNS<SimulationControl::kEquationModel>) {
            Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> left_quadrature_node_conserved_variable;
            Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> right_quadrature_node_conserved_variable;
            Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>
                left_quadrature_node_computational_variable;
            Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>
                right_quadrature_node_computational_variable;
            Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>
                left_quadrature_node_conserved_variable_gradient;
            Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>
                right_quadrature_node_conserved_variable_gradient;
            Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>
                left_quadrature_node_primitive_variable_gradient;
            Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>
                right_quadrature_node_primitive_variable_gradient;
            for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
              AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl>::get(
                  mesh, solver, left_quadrature_node_conserved_variable, left_parent_gmsh_type_number,
                  left_parent_index_each_type, adjacency_sequence_in_left_parent, j);
              AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl>::get(
                  mesh, solver, right_quadrature_node_conserved_variable, right_parent_gmsh_type_number,
                  right_parent_index_each_type, adjacency_sequence_in_right_parent,
                  adjacency_element_quadrature_sequence[static_cast<Usize>(j)]);
              Variable<SimulationControl>::convertComputationalFromConserved(
                  left_quadrature_node_conserved_variable, left_quadrature_node_computational_variable);
              Variable<SimulationControl>::convertComputationalFromConserved(
                  right_quadrature_node_conserved_variable, right_quadrature_node_computational_variable);
              AdjacencyElementVariableGradient<AdjacencyElementTrait, SimulationControl>::template get<
                  SimulationControl::kViscousFlux>(mesh, solver, left_quadrature_node_conserved_variable_gradient,
                                                   left_parent_gmsh_type_number, left_parent_index_each_type,
                                                   adjacency_sequence_in_left_parent, j);
              AdjacencyElementVariableGradient<AdjacencyElementTrait, SimulationControl>::template get<
                  SimulationControl::kViscousFlux>(mesh, solver, right_quadrature_node_conserved_variable_gradient,
                                                   right_parent_gmsh_type_number, right_parent_index_each_type,
                                                   adjacency_sequence_in_right_parent,
                                                   adjacency_element_quadrature_sequence[static_cast<Usize>(j)]);
              VariableGradient<SimulationControl>::convertPrimitiveFromConserved(
                  left_quadrature_node_computational_variable, left_quadrature_node_conserved_variable_gradient,
                  left_quadrature_node_primitive_variable_gradient);
              VariableGradient<SimulationControl>::convertPrimitiveFromConserved(
                  right_quadrature_node_computational_variable, right_quadrature_node_conserved_variable_gradient,
                  right_quadrature_node_primitive_variable_gradient);
              Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>>
                  left_quadrature_node_variable_adjacency_quadrature = this->getVariableAdjacencyQuadrature(
                      solver, left_parent_gmsh_type_number, left_parent_index_each_type,
                      left_adjacency_accumulate_quadrature_number + j);
              Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>>
                  right_quadrature_node_variable_adjacency_quadrature = this->getVariableAdjacencyQuadrature(
                      solver, right_parent_gmsh_type_number, right_parent_index_each_type,
                      right_adjacency_accumulate_quadrature_number +
                          adjacency_element_quadrature_sequence[static_cast<Usize>(j)]);
              const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector =
                  adjacency_element_mesh.normal_vector_(i).col(j);
              left_quadrature_node_variable_adjacency_quadrature.setZero();
              right_quadrature_node_variable_adjacency_quadrature.setZero();
              ConvectiveFlux<SimulationControl>::addVariableInteriorAdjacencyQuadratureNormalFlux(
                  normal_vector, left_quadrature_node_conserved_variable, left_quadrature_node_computational_variable,
                  right_quadrature_node_conserved_variable, right_quadrature_node_computational_variable,
                  left_quadrature_node_variable_adjacency_quadrature,
                  right_quadrature_node_variable_adjacency_quadrature,
                  adjacency_element_mesh.jacobian_determinant_multiply_weight_(i)(j));
              ViscousFlux<SimulationControl>::minusVariableInteriorAdjacencyQuadratureNormalFlux(
                  normal_vector, left_quadrature_node_computational_variable,
                  left_quadrature_node_primitive_variable_gradient, right_quadrature_node_computational_variable,
                  right_quadrature_node_primitive_variable_gradient, left_quadrature_node_variable_adjacency_quadrature,
                  right_quadrature_node_variable_adjacency_quadrature,
                  adjacency_element_mesh.jacobian_determinant_multiply_weight_(i)(j));
            }
          }
        }
      });
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl>::computeBoundaryAdjacencyElementQuadrature(
    const Mesh<SimulationControl>& mesh, Solver<SimulationControl>& solver) {
  const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh =
      mesh.*(Mesh<SimulationControl>::template getAdjacencyElement<AdjacencyElementTrait>());
  tbb::parallel_for(
      tbb::blocked_range<Isize>(this->interior_number_, this->number_),
      [&](const tbb::blocked_range<Isize>& range) -> void {
        for (Isize i = range.begin(); i != range.end(); i++) {
          const Isize left_parent_index_each_type = adjacency_element_mesh.left_parent_index_each_type_(i);
          const Isize adjacency_sequence_in_left_parent = adjacency_element_mesh.adjacency_sequence_in_left_parent_(i);
          const Isize left_parent_gmsh_type_number = adjacency_element_mesh.left_parent_gmsh_type_number_(i);
          const Isize left_adjacency_accumulate_quadrature_number =
              getAdjacencyElementAccumulateQuadratureNumber<AdjacencyElementTrait, SimulationControl>(
                  left_parent_gmsh_type_number, adjacency_sequence_in_left_parent);
          const BoundaryConditionEnum boundary_condition_type = adjacency_element_mesh.boundary_condition_type_(i);
          if constexpr (IsEuler<SimulationControl::kEquationModel>) {
            Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> left_quadrature_node_conserved_variable;
            Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>
                left_quadrature_node_computational_variable;
            Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>
                boundary_quadrature_node_computational_variable;
            for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
              AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl>::get(
                  mesh, solver, left_quadrature_node_conserved_variable, left_parent_gmsh_type_number,
                  left_parent_index_each_type, adjacency_sequence_in_left_parent, j);
              Variable<SimulationControl>::convertComputationalFromConserved(
                  left_quadrature_node_conserved_variable, left_quadrature_node_computational_variable);
              const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
                  right_quadrature_node_computational_variable =
                      this->boundary_dummy_right_computational_variable_(i - adjacency_element_mesh.interior_number_)
                          .col(j);
              Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>>
                  left_quadrature_node_variable_adjacency_quadrature = this->getVariableAdjacencyQuadrature(
                      solver, left_parent_gmsh_type_number, left_parent_index_each_type,
                      left_adjacency_accumulate_quadrature_number + j);
              const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector =
                  adjacency_element_mesh.normal_vector_(i).col(j);
              left_quadrature_node_variable_adjacency_quadrature.setZero();
              ConvectiveFlux<SimulationControl>::addVariableBoundaryAdjacencyQuadratureNormalFlux(
                  normal_vector, left_quadrature_node_computational_variable,
                  right_quadrature_node_computational_variable, boundary_quadrature_node_computational_variable,
                  left_quadrature_node_variable_adjacency_quadrature, boundary_condition_type,
                  adjacency_element_mesh.jacobian_determinant_multiply_weight_(i)(j));
            }
          }
          if constexpr (IsNS<SimulationControl::kEquationModel>) {
            Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> left_quadrature_node_conserved_variable;
            Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>
                left_quadrature_node_computational_variable;
            Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>
                left_quadrature_node_conserved_variable_gradient;
            Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>
                left_quadrature_node_primitive_variable_gradient;
            Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>
                boundary_quadrature_node_computational_variable;
            for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
              AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl>::get(
                  mesh, solver, left_quadrature_node_conserved_variable, left_parent_gmsh_type_number,
                  left_parent_index_each_type, adjacency_sequence_in_left_parent, j);
              Variable<SimulationControl>::convertComputationalFromConserved(
                  left_quadrature_node_conserved_variable, left_quadrature_node_computational_variable);
              const Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber>&
                  right_quadrature_node_computational_variable =
                      this->boundary_dummy_right_computational_variable_(i - adjacency_element_mesh.interior_number_)
                          .col(j);
              AdjacencyElementVariableGradient<AdjacencyElementTrait, SimulationControl>::template get<
                  SimulationControl::kViscousFlux>(mesh, solver, left_quadrature_node_conserved_variable_gradient,
                                                   left_parent_gmsh_type_number, left_parent_index_each_type,
                                                   adjacency_sequence_in_left_parent, j);
              VariableGradient<SimulationControl>::convertPrimitiveFromConserved(
                  left_quadrature_node_computational_variable, left_quadrature_node_conserved_variable_gradient,
                  left_quadrature_node_primitive_variable_gradient);
              Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>>
                  left_quadrature_node_variable_adjacency_quadrature = this->getVariableAdjacencyQuadrature(
                      solver, left_parent_gmsh_type_number, left_parent_index_each_type,
                      left_adjacency_accumulate_quadrature_number + j);
              const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector =
                  adjacency_element_mesh.normal_vector_(i).col(j);
              left_quadrature_node_variable_adjacency_quadrature.setZero();
              ConvectiveFlux<SimulationControl>::addVariableBoundaryAdjacencyQuadratureNormalFlux(
                  normal_vector, left_quadrature_node_computational_variable,
                  right_quadrature_node_computational_variable, boundary_quadrature_node_computational_variable,
                  left_quadrature_node_variable_adjacency_quadrature, boundary_condition_type,
                  adjacency_element_mesh.jacobian_determinant_multiply_weight_(i)(j));
              ViscousFlux<SimulationControl>::minusVariableBoundaryAdjacencyQuadratureNormalFlux(
                  normal_vector, left_quadrature_node_computational_variable,
                  left_quadrature_node_primitive_variable_gradient, boundary_quadrature_node_computational_variable,
                  left_quadrature_node_variable_adjacency_quadrature, boundary_condition_type,
                  adjacency_element_mesh.jacobian_determinant_multiply_weight_(i)(j));
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
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasAdjacencyTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.computeInteriorAdjacencyElementQuadrature(mesh, *this);
      this->triangle_.computeBoundaryAdjacencyElementQuadrature(mesh, *this);
    }
    if constexpr (HasAdjacencyQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.computeInteriorAdjacencyElementQuadrature(mesh, *this);
      this->quadrangle_.computeBoundaryAdjacencyElementQuadrature(mesh, *this);
    }
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void
AdjacencyElementSolverDevice<AdjacencyElementTrait, SimulationControl>::computeInteriorAdjacencyElementQuadrature(
    const MeshDevice<SimulationControl>& mesh, SolverDevice<SimulationControl>& solver) {
  const AdjacencyElementMeshDevice<AdjacencyElementTrait>& adjacency_element_mesh =
      mesh.*(MeshDevice<SimulationControl>::template getAdjacencyElement<AdjacencyElementTrait>());
  queue.submit([&](sycl::handler& cgh) -> void {
    cgh.parallel_for(getNdRange(this->interior_number_), [=, this](sycl::nd_item<1> index) -> void {
      const auto i = static_cast<Isize>(index.get_global_id(0));
      if (i >= this->interior_number_) {
        return;
      }
      const std::array<int, AdjacencyElementTrait::kQuadratureNumber> adjacency_element_quadrature_sequence{
          getAdjacencyElementQuadratureSequence<AdjacencyElementTrait::kElementType,
                                                SimulationControl::kPolynomialOrder>(
              static_cast<int>(adjacency_element_mesh.adjacency_right_rotation_(i)))};
      const Isize left_parent_index_each_type = adjacency_element_mesh.left_parent_index_each_type_(i);
      const Isize right_parent_index_each_type = adjacency_element_mesh.right_parent_index_each_type_(i);
      const Isize adjacency_sequence_in_left_parent = adjacency_element_mesh.adjacency_sequence_in_left_parent_(i);
      const Isize adjacency_sequence_in_right_parent = adjacency_element_mesh.adjacency_sequence_in_right_parent_(i);
      const Isize left_parent_gmsh_type_number = adjacency_element_mesh.left_parent_gmsh_type_number_(i);
      const Isize right_parent_gmsh_type_number = adjacency_element_mesh.right_parent_gmsh_type_number_(i);
      const Isize left_adjacency_accumulate_quadrature_number =
          getAdjacencyElementAccumulateQuadratureNumber<AdjacencyElementTrait, SimulationControl>(
              left_parent_gmsh_type_number, adjacency_sequence_in_left_parent);
      const Isize right_adjacency_accumulate_quadrature_number =
          getAdjacencyElementAccumulateQuadratureNumber<AdjacencyElementTrait, SimulationControl>(
              right_parent_gmsh_type_number, adjacency_sequence_in_right_parent);
      if constexpr (IsEuler<SimulationControl::kEquationModel>) {
        Device::StaticVector<Real, SimulationControl::kConservedVariableNumber> left_quadrature_node_conserved_variable;
        Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>
            right_quadrature_node_conserved_variable;
        Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>
            left_quadrature_node_computational_variable;
        Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>
            right_quadrature_node_computational_variable;
        for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
          AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::get(
              mesh, solver, left_quadrature_node_conserved_variable, left_parent_gmsh_type_number,
              left_parent_index_each_type, adjacency_sequence_in_left_parent, j);
          AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::get(
              mesh, solver, right_quadrature_node_conserved_variable, right_parent_gmsh_type_number,
              right_parent_index_each_type, adjacency_sequence_in_right_parent,
              adjacency_element_quadrature_sequence[static_cast<Usize>(j)]);
          VariableDevice<SimulationControl>::convertComputationalFromConserved(
              left_quadrature_node_conserved_variable, left_quadrature_node_computational_variable);
          VariableDevice<SimulationControl>::convertComputationalFromConserved(
              right_quadrature_node_conserved_variable, right_quadrature_node_computational_variable);
          Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber>>
              left_quadrature_node_variable_adjacency_quadrature = this->getVariableAdjacencyQuadrature(
                  solver, left_parent_gmsh_type_number, left_parent_index_each_type,
                  left_adjacency_accumulate_quadrature_number + j);
          Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber>>
              right_quadrature_node_variable_adjacency_quadrature = this->getVariableAdjacencyQuadrature(
                  solver, right_parent_gmsh_type_number, right_parent_index_each_type,
                  right_adjacency_accumulate_quadrature_number +
                      adjacency_element_quadrature_sequence[static_cast<Usize>(j)]);
          const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector =
              adjacency_element_mesh.normal_vector_.slice(i, adjacency_element_mesh.number_,
                                                          Device::Slice<SimulationControl::kDimension>::all(),
                                                          Device::Slice<1>::seqN(j));
          const Device::View<const Device::Vector<Real, AdjacencyElementTrait::kQuadratureNumber>>
              jacobian_determinant_multiply_weight =
                  adjacency_element_mesh.jacobian_determinant_multiply_weight_.view(i, adjacency_element_mesh.number_);
          left_quadrature_node_variable_adjacency_quadrature.setZero();
          right_quadrature_node_variable_adjacency_quadrature.setZero();
          ConvectiveFluxDevice<SimulationControl>::addVariableInteriorAdjacencyQuadratureNormalFlux(
              normal_vector, left_quadrature_node_conserved_variable, left_quadrature_node_computational_variable,
              right_quadrature_node_conserved_variable, right_quadrature_node_computational_variable,
              left_quadrature_node_variable_adjacency_quadrature, right_quadrature_node_variable_adjacency_quadrature,
              jacobian_determinant_multiply_weight(j));
        }
      }
      if constexpr (IsNS<SimulationControl::kEquationModel>) {
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
        for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
          AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::get(
              mesh, solver, left_quadrature_node_conserved_variable, left_parent_gmsh_type_number,
              left_parent_index_each_type, adjacency_sequence_in_left_parent, j);
          AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::get(
              mesh, solver, right_quadrature_node_conserved_variable, right_parent_gmsh_type_number,
              right_parent_index_each_type, adjacency_sequence_in_right_parent,
              adjacency_element_quadrature_sequence[static_cast<Usize>(j)]);
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
                                               adjacency_sequence_in_right_parent,
                                               adjacency_element_quadrature_sequence[static_cast<Usize>(j)]);
          VariableGradientDevice<SimulationControl>::convertPrimitiveFromConserved(
              left_quadrature_node_computational_variable, left_quadrature_node_conserved_variable_gradient,
              left_quadrature_node_primitive_variable_gradient);
          VariableGradientDevice<SimulationControl>::convertPrimitiveFromConserved(
              right_quadrature_node_computational_variable, right_quadrature_node_conserved_variable_gradient,
              right_quadrature_node_primitive_variable_gradient);
          Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber>>
              left_quadrature_node_variable_adjacency_quadrature = this->getVariableAdjacencyQuadrature(
                  solver, left_parent_gmsh_type_number, left_parent_index_each_type,
                  left_adjacency_accumulate_quadrature_number + j);
          Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber>>
              right_quadrature_node_variable_adjacency_quadrature = this->getVariableAdjacencyQuadrature(
                  solver, right_parent_gmsh_type_number, right_parent_index_each_type,
                  right_adjacency_accumulate_quadrature_number +
                      adjacency_element_quadrature_sequence[static_cast<Usize>(j)]);
          const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector =
              adjacency_element_mesh.normal_vector_.slice(i, adjacency_element_mesh.number_,
                                                          Device::Slice<SimulationControl::kDimension>::all(),
                                                          Device::Slice<1>::seqN(j));
          const Device::View<const Device::Vector<Real, AdjacencyElementTrait::kQuadratureNumber>>
              jacobian_determinant_multiply_weight =
                  adjacency_element_mesh.jacobian_determinant_multiply_weight_.view(i, adjacency_element_mesh.number_);
          left_quadrature_node_variable_adjacency_quadrature.setZero();
          right_quadrature_node_variable_adjacency_quadrature.setZero();
          ConvectiveFluxDevice<SimulationControl>::addVariableInteriorAdjacencyQuadratureNormalFlux(
              normal_vector, left_quadrature_node_conserved_variable, left_quadrature_node_computational_variable,
              right_quadrature_node_conserved_variable, right_quadrature_node_computational_variable,
              left_quadrature_node_variable_adjacency_quadrature, right_quadrature_node_variable_adjacency_quadrature,
              jacobian_determinant_multiply_weight(j));
          ViscousFluxDevice<SimulationControl>::minusVariableInteriorAdjacencyQuadratureNormalFlux(
              normal_vector, left_quadrature_node_computational_variable,
              left_quadrature_node_primitive_variable_gradient, right_quadrature_node_computational_variable,
              right_quadrature_node_primitive_variable_gradient, left_quadrature_node_variable_adjacency_quadrature,
              right_quadrature_node_variable_adjacency_quadrature, jacobian_determinant_multiply_weight(j));
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
      mesh.*(MeshDevice<SimulationControl>::template getAdjacencyElement<AdjacencyElementTrait>());
  queue.submit([&](sycl::handler& cgh) -> void {
    cgh.parallel_for(getNdRange(this->boundary_number_), [=, this](sycl::nd_item<1> index) -> void {
      const auto i = this->interior_number_ + static_cast<Isize>(index.get_global_id(0));
      if (i >= this->number_) {
        return;
      }
      const AdjacencyElementSolverDevice<AdjacencyElementTrait, SimulationControl>* self = this;
      const Isize left_parent_index_each_type = adjacency_element_mesh.left_parent_index_each_type_(i);
      const Isize adjacency_sequence_in_left_parent = adjacency_element_mesh.adjacency_sequence_in_left_parent_(i);
      const Isize left_parent_gmsh_type_number = adjacency_element_mesh.left_parent_gmsh_type_number_(i);
      const Isize left_adjacency_accumulate_quadrature_number =
          getAdjacencyElementAccumulateQuadratureNumber<AdjacencyElementTrait, SimulationControl>(
              left_parent_gmsh_type_number, adjacency_sequence_in_left_parent);
      const BoundaryConditionEnum boundary_condition_type = adjacency_element_mesh.boundary_condition_type_(i);
      if constexpr (IsEuler<SimulationControl::kEquationModel>) {
        Device::StaticVector<Real, SimulationControl::kConservedVariableNumber> left_quadrature_node_conserved_variable;
        Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>
            left_quadrature_node_computational_variable;
        Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>
            right_quadrature_node_computational_variable;
        Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>
            boundary_quadrature_node_computational_variable;
        for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
          AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::get(
              mesh, solver, left_quadrature_node_conserved_variable, left_parent_gmsh_type_number,
              left_parent_index_each_type, adjacency_sequence_in_left_parent, j);
          VariableDevice<SimulationControl>::convertComputationalFromConserved(
              left_quadrature_node_conserved_variable, left_quadrature_node_computational_variable);
          const Device::View<const Device::Vector<Real, SimulationControl::kComputationalVariableNumber>>
              boundary_dummy_right_computational_variable = self->boundary_dummy_right_computational_variable_.slice(
                  i - adjacency_element_mesh.interior_number_, adjacency_element_mesh.boundary_number_,
                  Device::Slice<SimulationControl::kComputationalVariableNumber>::all(), Device::Slice<1>::seqN(j));
          for (Isize m = 0; m < SimulationControl::kComputationalVariableNumber; m++) {
            right_quadrature_node_computational_variable(m) = boundary_dummy_right_computational_variable(m);
          }
          Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber>>
              left_quadrature_node_variable_adjacency_quadrature = this->getVariableAdjacencyQuadrature(
                  solver, left_parent_gmsh_type_number, left_parent_index_each_type,
                  left_adjacency_accumulate_quadrature_number + j);
          const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector =
              adjacency_element_mesh.normal_vector_.slice(i, adjacency_element_mesh.number_,
                                                          Device::Slice<SimulationControl::kDimension>::all(),
                                                          Device::Slice<1>::seqN(j));
          const Device::View<const Device::Vector<Real, AdjacencyElementTrait::kQuadratureNumber>>
              jacobian_determinant_multiply_weight =
                  adjacency_element_mesh.jacobian_determinant_multiply_weight_.view(i, adjacency_element_mesh.number_);
          left_quadrature_node_variable_adjacency_quadrature.setZero();
          ConvectiveFluxDevice<SimulationControl>::addVariableBoundaryAdjacencyQuadratureNormalFlux(
              normal_vector, left_quadrature_node_computational_variable, right_quadrature_node_computational_variable,
              boundary_quadrature_node_computational_variable, left_quadrature_node_variable_adjacency_quadrature,
              boundary_condition_type, jacobian_determinant_multiply_weight(j));
        }
      }
      if constexpr (IsNS<SimulationControl::kEquationModel>) {
        Device::StaticVector<Real, SimulationControl::kConservedVariableNumber> left_quadrature_node_conserved_variable;
        Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>
            left_quadrature_node_computational_variable;
        Device::StaticVector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>
            left_quadrature_node_conserved_variable_gradient;
        Device::StaticVector<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension>
            left_quadrature_node_primitive_variable_gradient;
        Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>
            right_quadrature_node_computational_variable;
        Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>
            boundary_quadrature_node_computational_variable;
        for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
          AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::get(
              mesh, solver, left_quadrature_node_conserved_variable, left_parent_gmsh_type_number,
              left_parent_index_each_type, adjacency_sequence_in_left_parent, j);
          VariableDevice<SimulationControl>::convertComputationalFromConserved(
              left_quadrature_node_conserved_variable, left_quadrature_node_computational_variable);
          const Device::View<const Device::Vector<Real, SimulationControl::kComputationalVariableNumber>>
              boundary_dummy_right_computational_variable = self->boundary_dummy_right_computational_variable_.slice(
                  i - adjacency_element_mesh.interior_number_, adjacency_element_mesh.boundary_number_,
                  Device::Slice<SimulationControl::kComputationalVariableNumber>::all(), Device::Slice<1>::seqN(j));
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
          Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber>>
              left_quadrature_node_variable_adjacency_quadrature = this->getVariableAdjacencyQuadrature(
                  solver, left_parent_gmsh_type_number, left_parent_index_each_type,
                  left_adjacency_accumulate_quadrature_number + j);
          const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector =
              adjacency_element_mesh.normal_vector_.slice(i, adjacency_element_mesh.number_,
                                                          Device::Slice<SimulationControl::kDimension>::all(),
                                                          Device::Slice<1>::seqN(j));
          const Device::View<const Device::Vector<Real, AdjacencyElementTrait::kQuadratureNumber>>
              jacobian_determinant_multiply_weight =
                  adjacency_element_mesh.jacobian_determinant_multiply_weight_.view(i, adjacency_element_mesh.number_);
          left_quadrature_node_variable_adjacency_quadrature.setZero();
          ConvectiveFluxDevice<SimulationControl>::addVariableBoundaryAdjacencyQuadratureNormalFlux(
              normal_vector, left_quadrature_node_computational_variable, right_quadrature_node_computational_variable,
              boundary_quadrature_node_computational_variable, left_quadrature_node_variable_adjacency_quadrature,
              boundary_condition_type, jacobian_determinant_multiply_weight(j));
          ViscousFluxDevice<SimulationControl>::minusVariableBoundaryAdjacencyQuadratureNormalFlux(
              normal_vector, left_quadrature_node_computational_variable,
              left_quadrature_node_primitive_variable_gradient, boundary_quadrature_node_computational_variable,
              left_quadrature_node_variable_adjacency_quadrature, boundary_condition_type,
              jacobian_determinant_multiply_weight(j));
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
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasAdjacencyTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.computeInteriorAdjacencyElementQuadrature(mesh, *this);
      this->triangle_.computeBoundaryAdjacencyElementQuadrature(mesh, *this);
    }
    if constexpr (HasAdjacencyQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.computeInteriorAdjacencyElementQuadrature(mesh, *this);
      this->quadrangle_.computeBoundaryAdjacencyElementQuadrature(mesh, *this);
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
  std::unreachable();
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
  std::unreachable();
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
  std::unreachable();
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
  std::unreachable();
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void
AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl>::computeInteriorAdjacencyElementGradientQuadrature(
    const Mesh<SimulationControl>& mesh, Solver<SimulationControl>& solver) {
  const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh =
      mesh.*(Mesh<SimulationControl>::template getAdjacencyElement<AdjacencyElementTrait>());
  tbb::parallel_for(
      tbb::blocked_range<Isize>(0, this->interior_number_), [&](const tbb::blocked_range<Isize>& range) -> void {
        for (Isize i = range.begin(); i != range.end(); i++) {
          const std::array<int, AdjacencyElementTrait::kQuadratureNumber> adjacency_element_quadrature_sequence{
              getAdjacencyElementQuadratureSequence<AdjacencyElementTrait::kElementType,
                                                    SimulationControl::kPolynomialOrder>(
                  static_cast<int>(adjacency_element_mesh.adjacency_right_rotation_(i)))};
          const Isize left_parent_index_each_type = adjacency_element_mesh.left_parent_index_each_type_(i);
          const Isize right_parent_index_each_type = adjacency_element_mesh.right_parent_index_each_type_(i);
          const Isize adjacency_sequence_in_left_parent = adjacency_element_mesh.adjacency_sequence_in_left_parent_(i);
          const Isize adjacency_sequence_in_right_parent =
              adjacency_element_mesh.adjacency_sequence_in_right_parent_(i);
          const Isize left_parent_gmsh_type_number = adjacency_element_mesh.left_parent_gmsh_type_number_(i);
          const Isize right_parent_gmsh_type_number_number = adjacency_element_mesh.right_parent_gmsh_type_number_(i);
          const Isize left_adjacency_accumulate_quadrature_number =
              getAdjacencyElementAccumulateQuadratureNumber<AdjacencyElementTrait, SimulationControl>(
                  left_parent_gmsh_type_number, adjacency_sequence_in_left_parent);
          const Isize right_adjacency_accumulate_quadrature_number =
              getAdjacencyElementAccumulateQuadratureNumber<AdjacencyElementTrait, SimulationControl>(
                  right_parent_gmsh_type_number_number, adjacency_sequence_in_right_parent);
          Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> left_quadrature_node_conserved_variable;
          Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> right_quadrature_node_conserved_variable;
          for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
            AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl>::get(
                mesh, solver, left_quadrature_node_conserved_variable, left_parent_gmsh_type_number,
                left_parent_index_each_type, adjacency_sequence_in_left_parent, j);
            AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl>::get(
                mesh, solver, right_quadrature_node_conserved_variable, right_parent_gmsh_type_number_number,
                right_parent_index_each_type, adjacency_sequence_in_right_parent,
                adjacency_element_quadrature_sequence[static_cast<Usize>(j)]);
            Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
                left_quadrature_node_variable_volume_gradient_adjacency_quadrature =
                    this->getVariableVolumeGradientAdjacencyQuadrature(solver, left_parent_gmsh_type_number,
                                                                       left_parent_index_each_type,
                                                                       left_adjacency_accumulate_quadrature_number + j);
            Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
                right_quadrature_node_variable_volume_gradient_adjacency_quadrature =
                    this->getVariableVolumeGradientAdjacencyQuadrature(
                        solver, right_parent_gmsh_type_number_number, right_parent_index_each_type,
                        right_adjacency_accumulate_quadrature_number +
                            adjacency_element_quadrature_sequence[static_cast<Usize>(j)]);
            const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector =
                adjacency_element_mesh.normal_vector_(i).col(j);
            GradientFlux<SimulationControl>::computeVariableVolumeGradientInteriorAdjacencyQuadratureNormalFlux(
                normal_vector, left_quadrature_node_conserved_variable, right_quadrature_node_conserved_variable,
                left_quadrature_node_variable_volume_gradient_adjacency_quadrature,
                right_quadrature_node_variable_volume_gradient_adjacency_quadrature,
                adjacency_element_mesh.jacobian_determinant_multiply_weight_(i)(j));
            if constexpr (IsNS<SimulationControl::kEquationModel>) {
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
                          right_adjacency_accumulate_quadrature_number +
                              adjacency_element_quadrature_sequence[static_cast<Usize>(j)]);
              GradientFlux<SimulationControl>::computeVariableInterfaceGradientInteriorAdjacencyQuadratureNormalFlux(
                  normal_vector, left_quadrature_node_conserved_variable, right_quadrature_node_conserved_variable,
                  left_quadrature_node_variable_interface_gradient_adjacency_quadrature,
                  right_quadrature_node_variable_interface_gradient_adjacency_quadrature,
                  adjacency_element_mesh.jacobian_determinant_multiply_weight_(i)(j));
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
      mesh.*(Mesh<SimulationControl>::template getAdjacencyElement<AdjacencyElementTrait>());
  tbb::parallel_for(
      tbb::blocked_range<Isize>(this->interior_number_, this->number_),
      [&](const tbb::blocked_range<Isize>& range) -> void {
        for (Isize i = range.begin(); i != range.end(); i++) {
          const Isize left_parent_index_each_type = adjacency_element_mesh.left_parent_index_each_type_(i);
          const Isize adjacency_sequence_in_left_parent = adjacency_element_mesh.adjacency_sequence_in_left_parent_(i);
          const Isize left_parent_gmsh_type_number = adjacency_element_mesh.left_parent_gmsh_type_number_(i);
          const Isize left_adjacency_accumulate_quadrature_number =
              getAdjacencyElementAccumulateQuadratureNumber<AdjacencyElementTrait, SimulationControl>(
                  left_parent_gmsh_type_number, adjacency_sequence_in_left_parent);
          const BoundaryConditionEnum boundary_condition_type = adjacency_element_mesh.boundary_condition_type_(i);
          Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> left_quadrature_node_conserved_variable;
          // NOTE: Here, the variable for the interface gradient is stored because its calculation requires the
          // boundary variable, which is costly. Therefore, it is calculated and stored, even in the Euler equations,
          // for possible subsequent use in the NS equations.
          Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>
              boundary_quadrature_node_interface_gradient_variable;
          for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
            AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl>::get(
                mesh, solver, left_quadrature_node_conserved_variable, left_parent_gmsh_type_number,
                left_parent_index_each_type, adjacency_sequence_in_left_parent, j);
            const Eigen::Vector<
                Real, SimulationControl::kComputationalVariableNumber>& right_quadrature_node_computational_variable =
                this->boundary_dummy_right_computational_variable_(i - adjacency_element_mesh.interior_number_).col(j);
            Eigen::Ref<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
                left_quadrature_node_variable_volume_gradient_adjacency_quadrature =
                    this->getVariableVolumeGradientAdjacencyQuadrature(solver, left_parent_gmsh_type_number,
                                                                       left_parent_index_each_type,
                                                                       left_adjacency_accumulate_quadrature_number + j);
            const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector =
                adjacency_element_mesh.normal_vector_(i).col(j);
            GradientFlux<SimulationControl>::computeVariableVolumeGradientBoundaryAdjacencyQuadratureNormalFlux(
                normal_vector, left_quadrature_node_conserved_variable, right_quadrature_node_computational_variable,
                boundary_quadrature_node_interface_gradient_variable,
                left_quadrature_node_variable_volume_gradient_adjacency_quadrature, boundary_condition_type,
                adjacency_element_mesh.jacobian_determinant_multiply_weight_(i)(j));
            if constexpr (IsNS<SimulationControl::kEquationModel>) {
              Eigen::Ref<
                  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
                  left_quadrature_node_variable_interface_gradient_adjacency_quadrature =
                      this->getVariableInterfaceGradientAdjacencyQuadrature(
                          solver, left_parent_gmsh_type_number, left_parent_index_each_type,
                          left_adjacency_accumulate_quadrature_number + j);
              GradientFlux<SimulationControl>::computeVariableInterfaceGradientBoundaryAdjacencyQuadratureNormalFlux(
                  normal_vector, boundary_quadrature_node_interface_gradient_variable,
                  left_quadrature_node_variable_interface_gradient_adjacency_quadrature,
                  adjacency_element_mesh.jacobian_determinant_multiply_weight_(i)(j));
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
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasAdjacencyTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.computeInteriorAdjacencyElementGradientQuadrature(mesh, *this);
      this->triangle_.computeBoundaryAdjacencyElementGradientQuadrature(mesh, *this);
    }
    if constexpr (HasAdjacencyQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.computeInteriorAdjacencyElementGradientQuadrature(mesh, *this);
      this->quadrangle_.computeBoundaryAdjacencyElementGradientQuadrature(mesh, *this);
    }
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementSolverDevice<AdjacencyElementTrait, SimulationControl>::
    computeInteriorAdjacencyElementGradientQuadrature(const MeshDevice<SimulationControl>& mesh,
                                                      SolverDevice<SimulationControl>& solver) {
  const AdjacencyElementMeshDevice<AdjacencyElementTrait>& adjacency_element_mesh =
      mesh.*(MeshDevice<SimulationControl>::template getAdjacencyElement<AdjacencyElementTrait>());
  queue.submit([&](sycl::handler& cgh) -> void {
    cgh.parallel_for(getNdRange(this->interior_number_), [=, this](sycl::nd_item<1> index) -> void {
      const auto i = static_cast<Isize>(index.get_global_id(0));
      if (i >= this->interior_number_) {
        return;
      }
      const std::array<int, AdjacencyElementTrait::kQuadratureNumber> adjacency_element_quadrature_sequence{
          getAdjacencyElementQuadratureSequence<AdjacencyElementTrait::kElementType,
                                                SimulationControl::kPolynomialOrder>(
              static_cast<int>(adjacency_element_mesh.adjacency_right_rotation_(i)))};
      const Isize left_parent_index_each_type = adjacency_element_mesh.left_parent_index_each_type_(i);
      const Isize right_parent_index_each_type = adjacency_element_mesh.right_parent_index_each_type_(i);
      const Isize adjacency_sequence_in_left_parent = adjacency_element_mesh.adjacency_sequence_in_left_parent_(i);
      const Isize adjacency_sequence_in_right_parent = adjacency_element_mesh.adjacency_sequence_in_right_parent_(i);
      const Isize left_parent_gmsh_type_number = adjacency_element_mesh.left_parent_gmsh_type_number_(i);
      const Isize right_parent_gmsh_type_number_number = adjacency_element_mesh.right_parent_gmsh_type_number_(i);
      const Isize left_adjacency_accumulate_quadrature_number =
          getAdjacencyElementAccumulateQuadratureNumber<AdjacencyElementTrait, SimulationControl>(
              left_parent_gmsh_type_number, adjacency_sequence_in_left_parent);
      const Isize right_adjacency_accumulate_quadrature_number =
          getAdjacencyElementAccumulateQuadratureNumber<AdjacencyElementTrait, SimulationControl>(
              right_parent_gmsh_type_number_number, adjacency_sequence_in_right_parent);
      Device::StaticVector<Real, SimulationControl::kConservedVariableNumber> left_quadrature_node_conserved_variable;
      Device::StaticVector<Real, SimulationControl::kConservedVariableNumber> right_quadrature_node_conserved_variable;
      for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
        AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::get(
            mesh, solver, left_quadrature_node_conserved_variable, left_parent_gmsh_type_number,
            left_parent_index_each_type, adjacency_sequence_in_left_parent, j);
        AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::get(
            mesh, solver, right_quadrature_node_conserved_variable, right_parent_gmsh_type_number_number,
            right_parent_index_each_type, adjacency_sequence_in_right_parent,
            adjacency_element_quadrature_sequence[static_cast<Usize>(j)]);
        Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
            left_quadrature_node_variable_volume_gradient_adjacency_quadrature =
                this->getVariableVolumeGradientAdjacencyQuadrature(solver, left_parent_gmsh_type_number,
                                                                   left_parent_index_each_type,
                                                                   left_adjacency_accumulate_quadrature_number + j);
        Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
            right_quadrature_node_variable_volume_gradient_adjacency_quadrature =
                this->getVariableVolumeGradientAdjacencyQuadrature(
                    solver, right_parent_gmsh_type_number_number, right_parent_index_each_type,
                    right_adjacency_accumulate_quadrature_number +
                        adjacency_element_quadrature_sequence[static_cast<Usize>(j)]);
        const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector =
            adjacency_element_mesh.normal_vector_.slice(i, adjacency_element_mesh.number_,
                                                        Device::Slice<SimulationControl::kDimension>::all(),
                                                        Device::Slice<1>::seqN(j));
        const Device::View<const Device::Vector<Real, AdjacencyElementTrait::kQuadratureNumber>>
            jacobian_determinant_multiply_weight =
                adjacency_element_mesh.jacobian_determinant_multiply_weight_.view(i, adjacency_element_mesh.number_);
        GradientFluxDevice<SimulationControl>::computeVariableVolumeGradientInteriorAdjacencyQuadratureNormalFlux(
            normal_vector, left_quadrature_node_conserved_variable, right_quadrature_node_conserved_variable,
            left_quadrature_node_variable_volume_gradient_adjacency_quadrature,
            right_quadrature_node_variable_volume_gradient_adjacency_quadrature,
            jacobian_determinant_multiply_weight(j));
        if constexpr (IsNS<SimulationControl::kEquationModel>) {
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
                      right_adjacency_accumulate_quadrature_number +
                          adjacency_element_quadrature_sequence[static_cast<Usize>(j)]);
          GradientFluxDevice<SimulationControl>::computeVariableInterfaceGradientInteriorAdjacencyQuadratureNormalFlux(
              normal_vector, left_quadrature_node_conserved_variable, right_quadrature_node_conserved_variable,
              left_quadrature_node_variable_interface_gradient_adjacency_quadrature,
              right_quadrature_node_variable_interface_gradient_adjacency_quadrature,
              jacobian_determinant_multiply_weight(j));
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
      mesh.*(MeshDevice<SimulationControl>::template getAdjacencyElement<AdjacencyElementTrait>());
  queue.submit([&](sycl::handler& cgh) -> void {
    cgh.parallel_for(getNdRange(this->boundary_number_), [=, this](sycl::nd_item<1> index) -> void {
      const auto i = this->interior_number_ + static_cast<Isize>(index.get_global_id(0));
      if (i >= this->number_) {
        return;
      }
      const AdjacencyElementSolverDevice<AdjacencyElementTrait, SimulationControl>* self = this;
      const Isize left_parent_index_each_type = adjacency_element_mesh.left_parent_index_each_type_(i);
      const Isize adjacency_sequence_in_left_parent = adjacency_element_mesh.adjacency_sequence_in_left_parent_(i);
      const Isize left_parent_gmsh_type_number = adjacency_element_mesh.left_parent_gmsh_type_number_(i);
      const Isize left_adjacency_accumulate_quadrature_number =
          getAdjacencyElementAccumulateQuadratureNumber<AdjacencyElementTrait, SimulationControl>(
              left_parent_gmsh_type_number, adjacency_sequence_in_left_parent);
      const BoundaryConditionEnum boundary_condition_type = adjacency_element_mesh.boundary_condition_type_(i);
      Device::StaticVector<Real, SimulationControl::kConservedVariableNumber> left_quadrature_node_conserved_variable;
      Device::StaticVector<Real, SimulationControl::kComputationalVariableNumber>
          right_quadrature_node_computational_variable;
      Device::StaticVector<Real, SimulationControl::kConservedVariableNumber>
          boundary_quadrature_node_interface_gradient_variable;
      for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
        AdjacencyElementVariableDevice<AdjacencyElementTrait, SimulationControl>::get(
            mesh, solver, left_quadrature_node_conserved_variable, left_parent_gmsh_type_number,
            left_parent_index_each_type, adjacency_sequence_in_left_parent, j);
        const Device::View<const Device::Vector<Real, SimulationControl::kComputationalVariableNumber>>
            boundary_dummy_right_computational_variable = self->boundary_dummy_right_computational_variable_.slice(
                i - adjacency_element_mesh.interior_number_, adjacency_element_mesh.boundary_number_,
                Device::Slice<SimulationControl::kComputationalVariableNumber>::all(), Device::Slice<1>::seqN(j));
        for (Isize m = 0; m < SimulationControl::kComputationalVariableNumber; m++) {
          right_quadrature_node_computational_variable(m) = boundary_dummy_right_computational_variable(m);
        }
        Device::View<Device::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
            left_quadrature_node_variable_volume_gradient_adjacency_quadrature =
                this->getVariableVolumeGradientAdjacencyQuadrature(solver, left_parent_gmsh_type_number,
                                                                   left_parent_index_each_type,
                                                                   left_adjacency_accumulate_quadrature_number + j);
        const Device::View<const Device::Vector<Real, SimulationControl::kDimension>> normal_vector =
            adjacency_element_mesh.normal_vector_.slice(i, adjacency_element_mesh.number_,
                                                        Device::Slice<SimulationControl::kDimension>::all(),
                                                        Device::Slice<1>::seqN(j));
        const Device::View<const Device::Vector<Real, AdjacencyElementTrait::kQuadratureNumber>>
            jacobian_determinant_multiply_weight =
                adjacency_element_mesh.jacobian_determinant_multiply_weight_.view(i, adjacency_element_mesh.number_);
        GradientFluxDevice<SimulationControl>::computeVariableVolumeGradientBoundaryAdjacencyQuadratureNormalFlux(
            normal_vector, left_quadrature_node_conserved_variable, right_quadrature_node_computational_variable,
            boundary_quadrature_node_interface_gradient_variable,
            left_quadrature_node_variable_volume_gradient_adjacency_quadrature, boundary_condition_type,
            jacobian_determinant_multiply_weight(j));
        if constexpr (IsNS<SimulationControl::kEquationModel>) {
          Device::View<
              Device::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>>
              left_quadrature_node_variable_interface_gradient_adjacency_quadrature =
                  this->getVariableInterfaceGradientAdjacencyQuadrature(
                      solver, left_parent_gmsh_type_number, left_parent_index_each_type,
                      left_adjacency_accumulate_quadrature_number + j);
          GradientFluxDevice<SimulationControl>::computeVariableInterfaceGradientBoundaryAdjacencyQuadratureNormalFlux(
              normal_vector, boundary_quadrature_node_interface_gradient_variable,
              left_quadrature_node_variable_interface_gradient_adjacency_quadrature,
              jacobian_determinant_multiply_weight(j));
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
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasAdjacencyTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.computeInteriorAdjacencyElementGradientQuadrature(mesh, *this);
      this->triangle_.computeBoundaryAdjacencyElementGradientQuadrature(mesh, *this);
    }
    if constexpr (HasAdjacencyQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.computeInteriorAdjacencyElementGradientQuadrature(mesh, *this);
      this->quadrangle_.computeBoundaryAdjacencyElementGradientQuadrature(mesh, *this);
    }
  }
  queue.wait();
}

template <typename VolumeElementTrait, typename SimulationControl>
inline void VolumeElementSolver<VolumeElementTrait, SimulationControl>::computeVolumeElementResidual(
    const VolumeElementMesh<VolumeElementTrait>& volume_element_mesh) {
  tbb::parallel_for(tbb::blocked_range<Isize>(0, this->number_), [&](const tbb::blocked_range<Isize>& range) -> void {
    for (Isize i = range.begin(); i != range.end(); i++) {
      // NOTE: Here we split the calculation to trigger eigen's noalias to avoid intermediate variables.
      this->variable_residual_(i).noalias() =
          this->variable_quadrature_(i) * volume_element_mesh.nodal_gradient_basis_function_;
      this->variable_residual_(i).noalias() -=
          this->variable_adjacency_quadrature_(i) * volume_element_mesh.nodal_adjacency_basis_function_;
      if constexpr (SimulationControl::kSourceTerm != SourceTermEnum::None) {
        this->variable_residual_(i).noalias() +=
            this->variable_source_quadrature_(i) * volume_element_mesh.nodal_basis_function_;
      }
    }
  });
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::computeResidual(const Mesh<SimulationControl>& mesh) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.computeVolumeElementResidual(mesh.line_);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.computeVolumeElementResidual(mesh.triangle_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.computeVolumeElementResidual(mesh.quadrangle_);
    }
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.computeVolumeElementResidual(mesh.tetrahedron_);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.computeVolumeElementResidual(mesh.pyramid_);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.computeVolumeElementResidual(mesh.hexahedron_);
    }
  }
}

template <typename VolumeElementTrait, typename SimulationControl>
inline void VolumeElementSolverDevice<VolumeElementTrait, SimulationControl>::computeVolumeElementResidual(
    const VolumeElementMeshDevice<VolumeElementTrait>& volume_element_mesh) {
  queue.submit([&](sycl::handler& cgh) -> void {
    cgh.parallel_for(getNdRange(this->number_), [=, this](sycl::nd_item<1> index) -> void {
      const auto i = static_cast<Isize>(index.get_global_id(0));
      if (i >= this->number_) {
        return;
      }
      const VolumeElementSolverDevice<VolumeElementTrait, SimulationControl>* self = this;
      const Device::View<const Device::Matrix<Real, SimulationControl::kConservedVariableNumber,
                                              VolumeElementTrait::kQuadratureNumber * SimulationControl::kDimension>>
          variable_quadrature = self->variable_quadrature_.view(i, this->number_);
      const Device::View<const Device::Matrix<Real, SimulationControl::kConservedVariableNumber,
                                              VolumeElementTrait::kAllAdjacencyQuadratureNumber>>
          variable_adjacency_quadrature = self->variable_adjacency_quadrature_.view(i, this->number_);
      Device::View<
          Device::Matrix<Real, SimulationControl::kConservedVariableNumber, VolumeElementTrait::kBasisFunctionNumber>>
          variable_residual = this->variable_residual_.view(i, this->number_);
      for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
        for (Isize n = 0; n < VolumeElementTrait::kBasisFunctionNumber; n++) {
          Real sum = 0.0_r;
          for (Isize k = 0; k < VolumeElementTrait::kQuadratureNumber * SimulationControl::kDimension; k++) {
            sum += variable_quadrature(m, k) * volume_element_mesh.nodal_gradient_basis_function_(k, n);
          }
          variable_residual(m, n) = sum;
        }
      }
      for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
        for (Isize n = 0; n < VolumeElementTrait::kBasisFunctionNumber; n++) {
          Real sum = 0.0_r;
          for (Isize k = 0; k < VolumeElementTrait::kAllAdjacencyQuadratureNumber; k++) {
            sum += variable_adjacency_quadrature(m, k) * volume_element_mesh.nodal_adjacency_basis_function_(k, n);
          }
          variable_residual(m, n) -= sum;
        }
      }
      if constexpr (SimulationControl::kSourceTerm != SourceTermEnum::None) {
        const Device::View<const Device::Matrix<Real, SimulationControl::kConservedVariableNumber,
                                                VolumeElementTrait::kQuadratureNumber>>
            variable_source_quadrature = self->variable_source_quadrature_.view(i, this->number_);
        for (Isize m = 0; m < SimulationControl::kConservedVariableNumber; m++) {
          for (Isize n = 0; n < VolumeElementTrait::kBasisFunctionNumber; n++) {
            Real sum = 0.0_r;
            for (Isize k = 0; k < VolumeElementTrait::kQuadratureNumber; k++) {
              sum += variable_source_quadrature(m, k) * volume_element_mesh.nodal_basis_function_(k, n);
            }
            variable_residual(m, n) += sum;
          }
        }
      }
    });
  });
  queue.wait_and_throw();
}

template <typename SimulationControl>
inline void SolverDevice<SimulationControl>::computeResidual(const MeshDevice<SimulationControl>& mesh) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.computeVolumeElementResidual(mesh.line_);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.computeVolumeElementResidual(mesh.triangle_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.computeVolumeElementResidual(mesh.quadrangle_);
    }
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.computeVolumeElementResidual(mesh.tetrahedron_);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.computeVolumeElementResidual(mesh.pyramid_);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.computeVolumeElementResidual(mesh.hexahedron_);
    }
  }
  queue.wait();
}

template <typename VolumeElementTrait, typename SimulationControl>
inline void VolumeElementSolver<VolumeElementTrait, SimulationControl>::computeVolumeElementGradientResidual(
    const VolumeElementMesh<VolumeElementTrait>& volume_element_mesh) {
  constexpr std::array<int, VolumeElementTrait::kAdjacencyNumber> kAdjacencyQuadratureNumber{
      getVolumeElementPerAdjacencyQuadratureNumber<VolumeElementTrait::kElementType,
                                                   SimulationControl::kPolynomialOrder>()};
  constexpr std::array<int, VolumeElementTrait::kAdjacencyNumber + 1> kAdjacencyQuadratureSequence{
      getVolumeElementAdjacencyQuadratureSequence<VolumeElementTrait::kElementType,
                                                  SimulationControl::kPolynomialOrder>()};
  tbb::parallel_for(tbb::blocked_range<Isize>(0, this->number_), [&](const tbb::blocked_range<Isize>& range) -> void {
    for (Isize i = range.begin(); i != range.end(); i++) {
      this->variable_volume_gradient_residual_(i).noalias() =
          this->variable_volume_gradient_adjacency_quadrature_(i) * volume_element_mesh.nodal_adjacency_basis_function_;
      this->variable_volume_gradient_residual_(i).noalias() -=
          this->variable_volume_gradient_quadrature_(i) * volume_element_mesh.nodal_gradient_basis_function_;
      if constexpr (IsNS<SimulationControl::kEquationModel>) {
        if constexpr (SimulationControl::kViscousFlux == ViscousFluxEnum::BR1) {
          this->variable_interface_gradient_residual_(i).noalias() =
              this->variable_interface_gradient_adjacency_quadrature_(i) *
              volume_element_mesh.nodal_adjacency_basis_function_;
        } else if constexpr (SimulationControl::kViscousFlux == ViscousFluxEnum::BR2) {
          for (Isize j = 0; j < VolumeElementTrait::kAdjacencyNumber; j++) {
            const Isize adjacency_quadrature_start = kAdjacencyQuadratureSequence[static_cast<Usize>(j)];
            const Isize adjacency_quadrature_number = kAdjacencyQuadratureNumber[static_cast<Usize>(j)];
            this->variable_interface_gradient_residual_(i)(
                Eigen::placeholders::all, Eigen::seqN(j * VolumeElementTrait::kBasisFunctionNumber,
                                                      Eigen::fix<VolumeElementTrait::kBasisFunctionNumber>)) =
                this->variable_interface_gradient_adjacency_quadrature_(i)(
                    Eigen::placeholders::all, Eigen::seqN(adjacency_quadrature_start, adjacency_quadrature_number)) *
                volume_element_mesh.nodal_adjacency_basis_function_(
                    Eigen::seqN(adjacency_quadrature_start, adjacency_quadrature_number), Eigen::placeholders::all);
          }
        }
      }
    }
  });
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::computeGradientResidual(const Mesh<SimulationControl>& mesh) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.computeVolumeElementGradientResidual(mesh.line_);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.computeVolumeElementGradientResidual(mesh.triangle_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.computeVolumeElementGradientResidual(mesh.quadrangle_);
    }
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.computeVolumeElementGradientResidual(mesh.tetrahedron_);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.computeVolumeElementGradientResidual(mesh.pyramid_);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.computeVolumeElementGradientResidual(mesh.hexahedron_);
    }
  }
}

template <typename VolumeElementTrait, typename SimulationControl>
inline void VolumeElementSolverDevice<VolumeElementTrait, SimulationControl>::computeVolumeElementGradientResidual(
    const VolumeElementMeshDevice<VolumeElementTrait>& volume_element_mesh) {
  constexpr std::array<int, VolumeElementTrait::kAdjacencyNumber> kAdjacencyQuadratureNumber{
      getVolumeElementPerAdjacencyQuadratureNumber<VolumeElementTrait::kElementType,
                                                   SimulationControl::kPolynomialOrder>()};
  constexpr std::array<int, VolumeElementTrait::kAdjacencyNumber + 1> kAdjacencyQuadratureSequence{
      getVolumeElementAdjacencyQuadratureSequence<VolumeElementTrait::kElementType,
                                                  SimulationControl::kPolynomialOrder>()};
  queue.submit([&](sycl::handler& cgh) -> void {
    cgh.parallel_for(getNdRange(this->number_), [=, this](sycl::nd_item<1> index) -> void {
      const auto i = static_cast<Isize>(index.get_global_id(0));
      if (i >= this->number_) {
        return;
      }
      const VolumeElementSolverDevice<VolumeElementTrait, SimulationControl>* self = this;
      const Device::View<
          const Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                               VolumeElementTrait::kAllAdjacencyQuadratureNumber>>
          variable_volume_gradient_adjacency_quadrature =
              self->variable_volume_gradient_adjacency_quadrature_.view(i, this->number_);
      Device::View<Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                                  VolumeElementTrait::kBasisFunctionNumber>>
          variable_volume_gradient_residual = this->variable_volume_gradient_residual_.view(i, this->number_);
      for (Isize m = 0; m < SimulationControl::kConservedVariableNumber * SimulationControl::kDimension; m++) {
        for (Isize n = 0; n < VolumeElementTrait::kBasisFunctionNumber; n++) {
          Real sum = 0.0_r;
          for (Isize k = 0; k < VolumeElementTrait::kAllAdjacencyQuadratureNumber; k++) {
            sum += variable_volume_gradient_adjacency_quadrature(m, k) *
                   volume_element_mesh.nodal_adjacency_basis_function_(k, n);
          }
          variable_volume_gradient_residual(m, n) = sum;
        }
      }
      const Device::View<
          const Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                               VolumeElementTrait::kQuadratureNumber * SimulationControl::kDimension>>
          variable_volume_gradient_quadrature = self->variable_volume_gradient_quadrature_.view(i, this->number_);
      for (Isize m = 0; m < SimulationControl::kConservedVariableNumber * SimulationControl::kDimension; m++) {
        for (Isize n = 0; n < VolumeElementTrait::kBasisFunctionNumber; n++) {
          Real sum = 0.0_r;
          for (Isize k = 0; k < VolumeElementTrait::kQuadratureNumber * SimulationControl::kDimension; k++) {
            sum += variable_volume_gradient_quadrature(m, k) * volume_element_mesh.nodal_gradient_basis_function_(k, n);
          }
          variable_volume_gradient_residual(m, n) -= sum;
        }
      }
      if constexpr (IsNS<SimulationControl::kEquationModel>) {
        if constexpr (SimulationControl::kViscousFlux == ViscousFluxEnum::BR1) {
          const Device::View<
              Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                             VolumeElementTrait::kAllAdjacencyQuadratureNumber>>
              variable_interface_gradient_adjacency_quadrature =
                  self->variable_interface_gradient_adjacency_quadrature_.view(i, this->number_);
          Device::View<Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                                      VolumeElementTrait::kBasisFunctionNumber>>
              variable_interface_gradient_residual = this->variable_interface_gradient_residual_.view(i, this->number_);
          for (Isize m = 0; m < SimulationControl::kConservedVariableNumber * SimulationControl::kDimension; m++) {
            for (Isize n = 0; n < VolumeElementTrait::kBasisFunctionNumber; n++) {
              Real sum = 0.0_r;
              for (Isize k = 0; k < VolumeElementTrait::kAllAdjacencyQuadratureNumber; k++) {
                sum += variable_interface_gradient_adjacency_quadrature(m, k) *
                       volume_element_mesh.nodal_adjacency_basis_function_(k, n);
              }
              variable_interface_gradient_residual(m, n) = sum;
            }
          }
        } else if constexpr (SimulationControl::kViscousFlux == ViscousFluxEnum::BR2) {
          const Device::View<
              const Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                                   VolumeElementTrait::kAllAdjacencyQuadratureNumber>>
              variable_interface_gradient_adjacency_quadrature =
                  self->variable_interface_gradient_adjacency_quadrature_.view(i, this->number_);
          for (Isize j = 0; j < VolumeElementTrait::kAdjacencyNumber; j++) {
            const Isize adjacency_quadrature_start = kAdjacencyQuadratureSequence[static_cast<Usize>(j)];
            const Isize adjacency_quadrature_number = kAdjacencyQuadratureNumber[static_cast<Usize>(j)];
            Device::View<
                Device::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                               VolumeElementTrait::kBasisFunctionNumber>>
                variable_interface_gradient_residual = this->variable_interface_gradient_residual_.slice(
                    i, this->number_,
                    Device::Slice<SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>::all(),
                    Device::Slice<VolumeElementTrait::kBasisFunctionNumber>::seqN(
                        j * VolumeElementTrait::kBasisFunctionNumber));
            for (Isize m = 0; m < SimulationControl::kConservedVariableNumber * SimulationControl::kDimension; m++) {
              for (Isize n = 0; n < VolumeElementTrait::kBasisFunctionNumber; n++) {
                Real sum = 0.0_r;
                for (Isize k = 0; k < adjacency_quadrature_number; k++) {
                  sum += variable_interface_gradient_adjacency_quadrature(m, adjacency_quadrature_start + k) *
                         volume_element_mesh.nodal_adjacency_basis_function_(adjacency_quadrature_start + k, n);
                }
                variable_interface_gradient_residual(m, n) = sum;
              }
            }
          }
        }
      }
    });
  });
}

template <typename SimulationControl>
inline void SolverDevice<SimulationControl>::computeGradientResidual(const MeshDevice<SimulationControl>& mesh) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.computeVolumeElementGradientResidual(mesh.line_);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.computeVolumeElementGradientResidual(mesh.triangle_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.computeVolumeElementGradientResidual(mesh.quadrangle_);
    }
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.computeVolumeElementGradientResidual(mesh.tetrahedron_);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.computeVolumeElementGradientResidual(mesh.pyramid_);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.computeVolumeElementGradientResidual(mesh.hexahedron_);
    }
  }
  queue.wait();
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_SPATIAL_DISCRETE_CPP_
