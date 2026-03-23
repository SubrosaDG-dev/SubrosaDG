/**
 * @file RawBinary.cpp
 * @brief The header file of SubrosaDG raw binary output.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-17
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_RAW_BINARY_CPP_
#define SUBROSA_DG_RAW_BINARY_CPP_

#include <zstd.h>

#include <Eigen/Core>
#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <sstream>
#include <string>
#include <vector>

#include "Mesh/ReadControl.cpp"
#include "Solver/SimulationControl.cpp"
#include "Solver/SolveControl.cpp"
#include "Utils/BasicDataType.cpp"
#include "Utils/Concept.cpp"
#include "Utils/Constant.cpp"
#include "Utils/Enum.cpp"
#include "View/IOControl.cpp"

namespace SubrosaDG {

struct RawBinaryCompress {
  static void write(const std::filesystem::path& raw_binary_path, std::stringstream& raw_binary_ss) {
    raw_binary_ss.seekg(0, std::ios::beg);
    raw_binary_ss.seekp(0, std::ios::beg);
    std::fstream raw_binary_fout(raw_binary_path, std::ios::out | std::ios::binary | std::ios::trunc);
    std::size_t compressed_size = ZSTD_compressBound(static_cast<std::size_t>(raw_binary_ss.str().size()));
    raw_binary_fout.write(reinterpret_cast<const char*>(&compressed_size),
                          static_cast<std::streamsize>(sizeof(std::size_t)));
    std::vector<char> compressed(compressed_size);
    std::size_t actual_size =
        ZSTD_compress(compressed.data(), compressed_size, raw_binary_ss.str().data(), raw_binary_ss.str().size(), 1);
    raw_binary_fout.write(compressed.data(), static_cast<std::streamsize>(actual_size));
    raw_binary_fout.close();
  }

  static void read(const std::filesystem::path& raw_binary_path, std::stringstream& raw_binary_ss) {
    raw_binary_ss.seekg(0, std::ios::beg);
    raw_binary_ss.seekp(0, std::ios::beg);
    std::fstream raw_binary_fin(raw_binary_path, std::ios::in | std::ios::binary);
    std::size_t original_size;
    raw_binary_fin.read(reinterpret_cast<char*>(&original_size), static_cast<std::streamsize>(sizeof(std::size_t)));
    raw_binary_fin.seekg(0, std::ios::end);
    std::streamoff actual_size = raw_binary_fin.tellg() - static_cast<std::streamoff>(sizeof(std::size_t));
    raw_binary_fin.seekg(static_cast<std::streamoff>(sizeof(std::size_t)));
    std::vector<char> compressed(static_cast<std::size_t>(actual_size));
    raw_binary_fin.read(compressed.data(), actual_size);
    std::vector<char> decompressed(original_size);
    ZSTD_decompress(decompressed.data(), original_size, compressed.data(), static_cast<std::size_t>(actual_size));
    raw_binary_ss << std::string(decompressed.data(), original_size);
    raw_binary_fin.close();
  }
};

template <typename VolumeElementTrait, typename SimulationControl>
inline void VolumeElementSolver<VolumeElementTrait, SimulationControl>::writeVolumeElementRawBinary(
    std::stringstream& raw_binary_ss) const {
  for (Isize i = 0; i < this->number_; i++) {
    raw_binary_ss.write(
        reinterpret_cast<const char*>(this->variable_basis_function_coefficient_(i).data()),
        SimulationControl::kConservedVariableNumber * VolumeElementTrait::kBasisFunctionNumber * kRealSize);
    if constexpr (IsEuler<SimulationControl::kEquationModel>) {
      raw_binary_ss.write(
          reinterpret_cast<const char*>(this->variable_volume_gradient_basis_function_coefficient_(i).data()),
          SimulationControl::kConservedVariableNumber * SimulationControl::kDimension *
              VolumeElementTrait::kBasisFunctionNumber * kRealSize);
    }
    if constexpr (IsNS<SimulationControl::kEquationModel>) {
      raw_binary_ss.write(reinterpret_cast<const char*>(this->variable_gradient_basis_function_coefficient_(i).data()),
                          SimulationControl::kConservedVariableNumber * SimulationControl::kDimension *
                              VolumeElementTrait::kBasisFunctionNumber * kRealSize);
    }
  }
}
template <typename AdjacencyElementTrait, typename SimulationControl>
template <typename VolumeElementTrait>
inline void AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl>::writeBoundaryAdjacencyPerElementRawBinary(
    const VolumeElementSolver<VolumeElementTrait, SimulationControl>& volume_element_solver,
    std::stringstream& raw_binary_ss, const Isize parent_index_each_type,
    [[maybe_unused]] const Isize adjacency_sequence_in_parent) const {
  raw_binary_ss.write(
      reinterpret_cast<const char*>(
          volume_element_solver.variable_basis_function_coefficient_(parent_index_each_type).data()),
      SimulationControl::kConservedVariableNumber * VolumeElementTrait::kBasisFunctionNumber * kRealSize);
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                VolumeElementTrait::kBasisFunctionNumber>
      variable_gradient_basis_function_coefficient;
  if constexpr (IsEuler<SimulationControl::kEquationModel>) {
    variable_gradient_basis_function_coefficient =
        volume_element_solver.variable_volume_gradient_basis_function_coefficient_(parent_index_each_type);
  }
  if constexpr (IsNS<SimulationControl::kEquationModel>) {
    if constexpr (SimulationControl::kViscousFlux == ViscousFluxEnum::BR1) {
      variable_gradient_basis_function_coefficient =
          volume_element_solver.variable_gradient_basis_function_coefficient_(parent_index_each_type);
    } else if constexpr (SimulationControl::kViscousFlux == ViscousFluxEnum::BR2) {
      variable_gradient_basis_function_coefficient =
          volume_element_solver.variable_volume_gradient_basis_function_coefficient_(parent_index_each_type) +
          volume_element_solver.variable_interface_gradient_basis_function_coefficient_(parent_index_each_type)(
              Eigen::placeholders::all,
              Eigen::seqN(adjacency_sequence_in_parent * VolumeElementTrait::kBasisFunctionNumber,
                          Eigen::fix<VolumeElementTrait::kBasisFunctionNumber>));
    }
  }
  raw_binary_ss.write(reinterpret_cast<const char*>(variable_gradient_basis_function_coefficient.data()),
                      SimulationControl::kConservedVariableNumber * SimulationControl::kDimension *
                          VolumeElementTrait::kBasisFunctionNumber * kRealSize);
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl>::writeBoundaryAdjacencyElementRawBinary(
    const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh, const Solver<SimulationControl>& solver,
    std::stringstream& raw_binary_ss) const {
  for (Isize i = adjacency_element_mesh.interior_number_;
       i < adjacency_element_mesh.boundary_number_ + adjacency_element_mesh.interior_number_; i++) {
    const Isize left_parent_index_each_type = adjacency_element_mesh.left_parent_index_each_type_(i);
    const Isize adjacency_sequence_in_left_parent = adjacency_element_mesh.adjacency_sequence_in_left_parent_(i);
    const Isize left_parent_gmsh_type_number = adjacency_element_mesh.left_parent_gmsh_type_number_(i);
    if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
      this->writeBoundaryAdjacencyPerElementRawBinary<VolumeLineTrait<SimulationControl::kPolynomialOrder>>(
          solver.line_, raw_binary_ss, left_parent_index_each_type, adjacency_sequence_in_left_parent);
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
      if (left_parent_gmsh_type_number == VolumeTriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->writeBoundaryAdjacencyPerElementRawBinary<VolumeTriangleTrait<SimulationControl::kPolynomialOrder>>(
            solver.triangle_, raw_binary_ss, left_parent_index_each_type, adjacency_sequence_in_left_parent);
      } else if (left_parent_gmsh_type_number ==
                 VolumeQuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->writeBoundaryAdjacencyPerElementRawBinary<VolumeQuadrangleTrait<SimulationControl::kPolynomialOrder>>(
            solver.quadrangle_, raw_binary_ss, left_parent_index_each_type, adjacency_sequence_in_left_parent);
      }
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
      if (left_parent_gmsh_type_number ==
          VolumeTetrahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->writeBoundaryAdjacencyPerElementRawBinary<VolumeTetrahedronTrait<SimulationControl::kPolynomialOrder>>(
            solver.tetrahedron_, raw_binary_ss, left_parent_index_each_type, adjacency_sequence_in_left_parent);
      } else if (left_parent_gmsh_type_number ==
                 VolumePyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->writeBoundaryAdjacencyPerElementRawBinary<VolumePyramidTrait<SimulationControl::kPolynomialOrder>>(
            solver.pyramid_, raw_binary_ss, left_parent_index_each_type, adjacency_sequence_in_left_parent);
      }
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
      if (left_parent_gmsh_type_number == VolumePyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->writeBoundaryAdjacencyPerElementRawBinary<VolumePyramidTrait<SimulationControl::kPolynomialOrder>>(
            solver.pyramid_, raw_binary_ss, left_parent_index_each_type, adjacency_sequence_in_left_parent);
      } else if (left_parent_gmsh_type_number ==
                 VolumeHexahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->writeBoundaryAdjacencyPerElementRawBinary<VolumeHexahedronTrait<SimulationControl::kPolynomialOrder>>(
            solver.hexahedron_, raw_binary_ss, left_parent_index_each_type, adjacency_sequence_in_left_parent);
      }
    }
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::writeRawBinary(const Mesh<SimulationControl>& mesh,
                                                      const std::filesystem::path& raw_binary_path) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.writeVolumeElementRawBinary(this->raw_binary_ss_);
    this->point_.writeBoundaryAdjacencyElementRawBinary(mesh.point_, *this, this->raw_binary_ss_);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.writeVolumeElementRawBinary(this->raw_binary_ss_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.writeVolumeElementRawBinary(this->raw_binary_ss_);
    }
    this->line_.writeBoundaryAdjacencyElementRawBinary(mesh.line_, *this, this->raw_binary_ss_);
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.writeVolumeElementRawBinary(this->raw_binary_ss_);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.writeVolumeElementRawBinary(this->raw_binary_ss_);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.writeVolumeElementRawBinary(this->raw_binary_ss_);
    }
    if constexpr (HasAdjacencyTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.writeBoundaryAdjacencyElementRawBinary(mesh.triangle_, *this, this->raw_binary_ss_);
    }
    if constexpr (HasAdjacencyQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.writeBoundaryAdjacencyElementRawBinary(mesh.quadrangle_, *this, this->raw_binary_ss_);
    }
  }
  this->write_raw_binary_future_ =
      std::async(std::launch::async, RawBinaryCompress::write, raw_binary_path, std::ref(this->raw_binary_ss_));
}

template <typename VolumeElementTrait, typename SimulationControl>
inline void VolumeElementViewSolver<VolumeElementTrait, SimulationControl>::computeVolumeElementViewVariable(
    const VolumeElementMesh<VolumeElementTrait>& volume_element_mesh, std::stringstream& raw_binary_ss) {
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, VolumeElementTrait::kBasisFunctionNumber>
      variable_basis_function_coefficient;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, VolumeElementTrait::kAllNodeNumber>
      all_conserved_variable;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                VolumeElementTrait::kBasisFunctionNumber>
      variable_gradient_basis_function_coefficient;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                VolumeElementTrait::kAllNodeNumber>
      all_conserved_gradient_variable;
  for (Isize i = 0; i < volume_element_mesh.number_; i++) {
    raw_binary_ss.read(
        reinterpret_cast<char*>(variable_basis_function_coefficient.data()),
        SimulationControl::kConservedVariableNumber * VolumeElementTrait::kBasisFunctionNumber * kRealSize);
    raw_binary_ss.read(reinterpret_cast<char*>(variable_gradient_basis_function_coefficient.data()),
                       SimulationControl::kConservedVariableNumber * SimulationControl::kDimension *
                           VolumeElementTrait::kBasisFunctionNumber * kRealSize);
    all_conserved_variable.noalias() = variable_basis_function_coefficient;
    this->view_variable_(i).convertComputationalFromConserved(all_conserved_variable);
    all_conserved_gradient_variable.noalias() = variable_gradient_basis_function_coefficient;
    this->view_variable_(i).convertPrimitiveGradientFromConservedGradient(all_conserved_gradient_variable);
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
template <typename VolumeElementTrait>
inline void
AdjacencyElementViewSolver<AdjacencyElementTrait, SimulationControl>::computeAdjacencyPerElementViewVariable(
    const VolumeElementViewSolver<VolumeElementTrait, SimulationControl>& volume_element_view_solver,
    std::stringstream& raw_binary_ss, const Isize parent_gmsh_type_number, const Isize adjacency_sequence_in_parent,
    const Isize element_index) {
  const std::array<int, AdjacencyElementTrait::kAllNodeNumber> adjacency_view_node_sequence_in_parent{
      getAdjacencyElementViewNodeSequenceInParent<AdjacencyElementTrait::kElementType,
                                                  SimulationControl::kPolynomialOrder>(
          static_cast<int>(parent_gmsh_type_number), static_cast<int>(adjacency_sequence_in_parent))};
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, VolumeElementTrait::kBasisFunctionNumber>
      variable_basis_function_coefficient;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, AdjacencyElementTrait::kAllNodeNumber>
      all_conserved_variable;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                VolumeElementTrait::kBasisFunctionNumber>
      variable_gradient_basis_function_coefficient;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                AdjacencyElementTrait::kAllNodeNumber>
      all_conserved_gradient_variable;
  raw_binary_ss.read(
      reinterpret_cast<char*>(variable_basis_function_coefficient.data()),
      SimulationControl::kConservedVariableNumber * VolumeElementTrait::kBasisFunctionNumber * kRealSize);
  raw_binary_ss.read(reinterpret_cast<char*>(variable_gradient_basis_function_coefficient.data()),
                     SimulationControl::kConservedVariableNumber * SimulationControl::kDimension *
                         VolumeElementTrait::kBasisFunctionNumber * kRealSize);
  for (Isize i = 0; i < AdjacencyElementTrait::kAllNodeNumber; i++) {
    all_conserved_variable.col(i).noalias() =
        variable_basis_function_coefficient.col(adjacency_view_node_sequence_in_parent[static_cast<Usize>(i)]);
    all_conserved_gradient_variable.col(i).noalias() =
        variable_gradient_basis_function_coefficient.col(adjacency_view_node_sequence_in_parent[static_cast<Usize>(i)]);
  }
  this->view_variable_(element_index).convertComputationalFromConserved(all_conserved_variable);
  this->view_variable_(element_index).convertPrimitiveGradientFromConservedGradient(all_conserved_gradient_variable);
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementViewSolver<AdjacencyElementTrait, SimulationControl>::computeAdjacencyElementViewVariable(
    const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
    const ViewSolver<SimulationControl>& view_solver, std::stringstream& raw_binary_ss) {
  for (Isize i = 0; i < adjacency_element_mesh.boundary_number_; i++) {
    const Isize adjacency_sequence_in_parent =
        adjacency_element_mesh.adjacency_sequence_in_left_parent_(i + adjacency_element_mesh.interior_number_);
    const Isize parent_gmsh_type_number =
        adjacency_element_mesh.left_parent_gmsh_type_number_(i + adjacency_element_mesh.interior_number_);
    if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
      this->computeAdjacencyPerElementViewVariable<VolumeLineTrait<SimulationControl::kPolynomialOrder>>(
          view_solver.line_, raw_binary_ss, parent_gmsh_type_number, adjacency_sequence_in_parent, i);
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
      if (parent_gmsh_type_number == VolumeTriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->computeAdjacencyPerElementViewVariable<VolumeTriangleTrait<SimulationControl::kPolynomialOrder>>(
            view_solver.triangle_, raw_binary_ss, parent_gmsh_type_number, adjacency_sequence_in_parent, i);
      } else if (parent_gmsh_type_number ==
                 VolumeQuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->computeAdjacencyPerElementViewVariable<VolumeQuadrangleTrait<SimulationControl::kPolynomialOrder>>(
            view_solver.quadrangle_, raw_binary_ss, parent_gmsh_type_number, adjacency_sequence_in_parent, i);
      }
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
      if (parent_gmsh_type_number == VolumeTetrahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->computeAdjacencyPerElementViewVariable<VolumeTetrahedronTrait<SimulationControl::kPolynomialOrder>>(
            view_solver.tetrahedron_, raw_binary_ss, parent_gmsh_type_number, adjacency_sequence_in_parent, i);
      } else if (parent_gmsh_type_number == VolumePyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->computeAdjacencyPerElementViewVariable<VolumePyramidTrait<SimulationControl::kPolynomialOrder>>(
            view_solver.pyramid_, raw_binary_ss, parent_gmsh_type_number, adjacency_sequence_in_parent, i);
      }
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
      if (parent_gmsh_type_number == VolumePyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->computeAdjacencyPerElementViewVariable<VolumePyramidTrait<SimulationControl::kPolynomialOrder>>(
            view_solver.pyramid_, raw_binary_ss, parent_gmsh_type_number, adjacency_sequence_in_parent, i);
      } else if (parent_gmsh_type_number ==
                 VolumeHexahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->computeAdjacencyPerElementViewVariable<VolumeHexahedronTrait<SimulationControl::kPolynomialOrder>>(
            view_solver.hexahedron_, raw_binary_ss, parent_gmsh_type_number, adjacency_sequence_in_parent, i);
      }
    }
  }
}

template <typename SimulationControl>
inline void ViewSolver<SimulationControl>::computeViewVariable(const Mesh<SimulationControl>& mesh) {
  RawBinaryCompress::read(this->raw_binary_path_, this->raw_binary_ss_);
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.computeVolumeElementViewVariable(mesh.line_, this->raw_binary_ss_);
    this->point_.computeAdjacencyElementViewVariable(mesh.point_, *this, this->raw_binary_ss_);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.computeVolumeElementViewVariable(mesh.triangle_, this->raw_binary_ss_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.computeVolumeElementViewVariable(mesh.quadrangle_, this->raw_binary_ss_);
    }
    this->line_.computeAdjacencyElementViewVariable(mesh.line_, *this, this->raw_binary_ss_);
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.computeVolumeElementViewVariable(mesh.tetrahedron_, this->raw_binary_ss_);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.computeVolumeElementViewVariable(mesh.pyramid_, this->raw_binary_ss_);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.computeVolumeElementViewVariable(mesh.hexahedron_, this->raw_binary_ss_);
    }
    if constexpr (HasAdjacencyTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.computeAdjacencyElementViewVariable(mesh.triangle_, *this, this->raw_binary_ss_);
    }
    if constexpr (HasAdjacencyQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.computeAdjacencyElementViewVariable(mesh.quadrangle_, *this, this->raw_binary_ss_);
    }
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_RAW_BINARY_CPP_
