/**
 * @file RawBinary.hpp
 * @brief The header file of SubrosaDG raw binary output.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-17
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_RAW_BINARY_HPP_
#define SUBROSA_DG_RAW_BINARY_HPP_

#include <zconf.h>
#include <zlib.h>

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

#include "Mesh/ReadControl.hpp"
#include "Solver/PhysicalModel.hpp"
#include "Solver/SimulationControl.hpp"
#include "Solver/SolveControl.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Concept.hpp"
#include "Utils/Constant.hpp"
#include "Utils/Enum.hpp"
#include "View/IOControl.hpp"

namespace SubrosaDG {

struct RawBinaryCompress {
  inline static void write(const std::filesystem::path& raw_binary_path, std::stringstream& raw_binary_ss) {
    raw_binary_ss.seekg(0, std::ios::beg);
    raw_binary_ss.seekp(0, std::ios::beg);
    std::fstream raw_binary_fout(raw_binary_path, std::ios::out | std::ios::binary | std::ios::trunc);
    uLongf dest_len = compressBound(static_cast<uLong>(raw_binary_ss.str().size()));
    raw_binary_fout.write(reinterpret_cast<const char*>(&dest_len), static_cast<std::streamsize>(sizeof(uLongf)));
    std::vector<Bytef> dest(dest_len);
    compress(dest.data(), &dest_len, reinterpret_cast<const Bytef*>(raw_binary_ss.str().c_str()),
             static_cast<uLong>(raw_binary_ss.str().size()));
    raw_binary_fout.write(reinterpret_cast<const char*>(dest.data()), static_cast<std::streamsize>(dest_len));
    raw_binary_fout.close();
  }

  inline static void read(const std::filesystem::path& raw_binary_path, std::stringstream& raw_binary_ss) {
    raw_binary_ss.seekg(0, std::ios::beg);
    raw_binary_ss.seekp(0, std::ios::beg);
    std::fstream raw_binary_fin(raw_binary_path, std::ios::in | std::ios::binary);
    uLongf dest_len;
    raw_binary_fin.read(reinterpret_cast<char*>(&dest_len), static_cast<std::streamsize>(sizeof(uLongf)));
    raw_binary_fin.seekg(0, std::ios::end);
    auto size = raw_binary_fin.tellg() - static_cast<std::streamoff>(sizeof(uLongf));
    raw_binary_fin.seekg(static_cast<std::streamoff>(sizeof(uLongf)));
    std::vector<Bytef> source(static_cast<std::size_t>(size));
    raw_binary_fin.read(reinterpret_cast<char*>(source.data()), size);
    std::vector<Bytef> dest(dest_len);
    uncompress(dest.data(), &dest_len, source.data(), static_cast<uLong>(size));
    raw_binary_ss << std::string(reinterpret_cast<const char*>(dest.data()), dest_len);
    raw_binary_fin.close();
  }
};

template <typename ElementTrait, typename SimulationControl>
inline void ElementSolver<ElementTrait, SimulationControl>::writeElementRawBinary(
    std::stringstream& raw_binary_ss) const {
  for (Isize i = 0; i < this->number_; i++) {
    raw_binary_ss.write(reinterpret_cast<const char*>(this->element_(i).variable_basis_function_coefficient_(1).data()),
                        SimulationControl::kConservedVariableNumber * ElementTrait::kBasisFunctionNumber * kRealSize);
    if constexpr (SimulationControl::kEquationModel == EquationModelEnum::CompresibleNS ||
                  SimulationControl::kEquationModel == EquationModelEnum::IncompresibleNS) {
      raw_binary_ss.write(
          reinterpret_cast<const char*>(this->element_(i).variable_gradient_basis_function_coefficient_.data()),
          SimulationControl::kConservedVariableNumber * SimulationControl::kDimension *
              ElementTrait::kBasisFunctionNumber * kRealSize);
    }
  }
}
template <typename AdjacencyElementTrait, typename SimulationControl>
template <typename ElementTrait>
inline void AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl>::writeBoundaryAdjacencyPerElementRawBinary(
    const ElementSolver<ElementTrait, SimulationControl>& element_solver, std::stringstream& raw_binary_ss,
    const Isize parent_index_each_type, [[maybe_unused]] const Isize adjacency_sequence_in_parent) const {
  raw_binary_ss.write(
      reinterpret_cast<const char*>(
          element_solver.element_(parent_index_each_type).variable_basis_function_coefficient_(1).data()),
      SimulationControl::kConservedVariableNumber * ElementTrait::kBasisFunctionNumber * kRealSize);
  if constexpr (SimulationControl::kEquationModel == EquationModelEnum::CompresibleNS ||
                SimulationControl::kEquationModel == EquationModelEnum::IncompresibleNS) {
    Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                  ElementTrait::kBasisFunctionNumber>
        variable_gradient_basis_function_coefficient;
    if constexpr (SimulationControl::kViscousFlux == ViscousFluxEnum::BR1) {
      variable_gradient_basis_function_coefficient =
          element_solver.element_(parent_index_each_type).variable_gradient_basis_function_coefficient_;
    } else if constexpr (SimulationControl::kViscousFlux == ViscousFluxEnum::BR2) {
      variable_gradient_basis_function_coefficient =
          element_solver.element_(parent_index_each_type).variable_volume_gradient_basis_function_coefficient_ +
          element_solver.element_(parent_index_each_type)
              .variable_interface_gradient_basis_function_coefficient_(adjacency_sequence_in_parent);
    }
    raw_binary_ss.write(reinterpret_cast<const char*>(variable_gradient_basis_function_coefficient.data()),
                        SimulationControl::kConservedVariableNumber * SimulationControl::kDimension *
                            ElementTrait::kBasisFunctionNumber * kRealSize);
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl>::writeBoundaryAdjacencyElementRawBinary(
    const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh, const Solver<SimulationControl>& solver,
    std::stringstream& raw_binary_ss) const {
  for (Isize i = adjacency_element_mesh.interior_number_;
       i < adjacency_element_mesh.boundary_number_ + adjacency_element_mesh.interior_number_; i++) {
    const Isize parent_index_each_type = adjacency_element_mesh.element_(i).parent_index_each_type_(0);
    const Isize adjacency_sequence_in_parent = adjacency_element_mesh.element_(i).adjacency_sequence_in_parent_(0);
    const Isize parent_gmsh_type_number = adjacency_element_mesh.element_(i).parent_gmsh_type_number_(0);
    if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
      this->writeBoundaryAdjacencyPerElementRawBinary<LineTrait<SimulationControl::kPolynomialOrder>>(
          solver.line_, raw_binary_ss, parent_index_each_type, adjacency_sequence_in_parent);
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
      if (parent_gmsh_type_number == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->writeBoundaryAdjacencyPerElementRawBinary<TriangleTrait<SimulationControl::kPolynomialOrder>>(
            solver.triangle_, raw_binary_ss, parent_index_each_type, adjacency_sequence_in_parent);
      } else if (parent_gmsh_type_number == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->writeBoundaryAdjacencyPerElementRawBinary<QuadrangleTrait<SimulationControl::kPolynomialOrder>>(
            solver.quadrangle_, raw_binary_ss, parent_index_each_type, adjacency_sequence_in_parent);
      }
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
      if (parent_gmsh_type_number == TetrahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->writeBoundaryAdjacencyPerElementRawBinary<TetrahedronTrait<SimulationControl::kPolynomialOrder>>(
            solver.tetrahedron_, raw_binary_ss, parent_index_each_type, adjacency_sequence_in_parent);
      } else if (parent_gmsh_type_number == PyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->writeBoundaryAdjacencyPerElementRawBinary<PyramidTrait<SimulationControl::kPolynomialOrder>>(
            solver.pyramid_, raw_binary_ss, parent_index_each_type, adjacency_sequence_in_parent);
      }
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
      if (parent_gmsh_type_number == PyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->writeBoundaryAdjacencyPerElementRawBinary<PyramidTrait<SimulationControl::kPolynomialOrder>>(
            solver.pyramid_, raw_binary_ss, parent_index_each_type, adjacency_sequence_in_parent);
      } else if (parent_gmsh_type_number == HexahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->writeBoundaryAdjacencyPerElementRawBinary<HexahedronTrait<SimulationControl::kPolynomialOrder>>(
            solver.hexahedron_, raw_binary_ss, parent_index_each_type, adjacency_sequence_in_parent);
      }
    }
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::writeRawBinary(const Mesh<SimulationControl>& mesh,
                                                      const std::filesystem::path& raw_binary_path) {
  this->raw_binary_ss_.write(reinterpret_cast<const char*>(this->node_artificial_viscosity_.data()),
                             mesh.node_number_ * kRealSize);
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.writeElementRawBinary(this->raw_binary_ss_);
    this->point_.writeBoundaryAdjacencyElementRawBinary(mesh.point_, *this, this->raw_binary_ss_);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.writeElementRawBinary(this->raw_binary_ss_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.writeElementRawBinary(this->raw_binary_ss_);
    }
    this->line_.writeBoundaryAdjacencyElementRawBinary(mesh.line_, *this, this->raw_binary_ss_);
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.writeElementRawBinary(this->raw_binary_ss_);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.writeElementRawBinary(this->raw_binary_ss_);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.writeElementRawBinary(this->raw_binary_ss_);
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

template <typename ElementTrait, typename SimulationControl>
inline void ElementViewSolver<ElementTrait, SimulationControl>::calcluateElementViewVariable(
    const ElementMesh<ElementTrait>& element_mesh, const PhysicalModel<SimulationControl>& physical_model,
    const Eigen::Vector<Real, Eigen::Dynamic>& node_artificial_viscosity, std::stringstream& raw_binary_ss) {
  if constexpr (SimulationControl::kEquationModel == EquationModelEnum::CompresibleEuler ||
                SimulationControl::kEquationModel == EquationModelEnum::IncompresibleEuler) {
    Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kBasisFunctionNumber>
        variable_basis_function_coefficient;
    Eigen::Vector<Real, ElementTrait::kBasicNodeNumber> variable_artificial_viscosity;
    for (Isize i = 0; i < element_mesh.number_; i++) {
      raw_binary_ss.read(reinterpret_cast<char*>(variable_basis_function_coefficient.data()),
                         SimulationControl::kConservedVariableNumber * ElementTrait::kBasisFunctionNumber * kRealSize);
      this->view_variable_(i).variable_.conserved_.noalias() =
          variable_basis_function_coefficient * this->basis_function_.modal_value_;
      this->view_variable_(i).variable_.calculateComputationalFromConserved(physical_model);
      for (Isize j = 0; j < ElementTrait::kBasicNodeNumber; j++) {
        variable_artificial_viscosity(j) = node_artificial_viscosity(element_mesh.element_(i).node_tag_(j) - 1);
      }
      this->view_variable_(i).artificial_viscosity_.noalias() =
          this->basis_function_.nodal_value_.transpose() * variable_artificial_viscosity;
    }
  }
  if constexpr (SimulationControl::kEquationModel == EquationModelEnum::CompresibleNS ||
                SimulationControl::kEquationModel == EquationModelEnum::IncompresibleNS) {
    Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kBasisFunctionNumber>
        variable_basis_function_coefficient;
    Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                  ElementTrait::kBasisFunctionNumber>
        variable_gradient_basis_function_coefficient;
    Eigen::Vector<Real, ElementTrait::kBasicNodeNumber> variable_artificial_viscosity;
    for (Isize i = 0; i < element_mesh.number_; i++) {
      raw_binary_ss.read(reinterpret_cast<char*>(variable_basis_function_coefficient.data()),
                         SimulationControl::kConservedVariableNumber * ElementTrait::kBasisFunctionNumber * kRealSize);
      raw_binary_ss.read(reinterpret_cast<char*>(variable_gradient_basis_function_coefficient.data()),
                         SimulationControl::kConservedVariableNumber * SimulationControl::kDimension *
                             ElementTrait::kBasisFunctionNumber * kRealSize);
      this->view_variable_(i).variable_.conserved_.noalias() =
          variable_basis_function_coefficient * this->basis_function_.modal_value_;
      this->view_variable_(i).variable_.calculateComputationalFromConserved(physical_model);
      this->view_variable_(i).variable_gradient_.conserved_.noalias() =
          variable_gradient_basis_function_coefficient * this->basis_function_.modal_value_;
      this->view_variable_(i).variable_gradient_.calculatePrimitiveFromConserved(physical_model,
                                                                                 this->view_variable_(i).variable_);
      for (Isize j = 0; j < ElementTrait::kBasicNodeNumber; j++) {
        variable_artificial_viscosity(j) = node_artificial_viscosity(element_mesh.element_(i).node_tag_(j) - 1);
      }
      this->view_variable_(i).artificial_viscosity_.noalias() =
          this->basis_function_.nodal_value_.transpose() * variable_artificial_viscosity;
    }
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
template <typename ElementTrait>
inline void
AdjacencyElementViewSolver<AdjacencyElementTrait, SimulationControl>::calcluateAdjacencyPerElementViewVariable(
    const PhysicalModel<SimulationControl>& physical_model,
    const ElementViewSolver<ElementTrait, SimulationControl>& element_view_solver, std::stringstream& raw_binary_ss,
    const Isize adjacency_sequence_in_parent, const Isize parent_gmsh_type_number, const Isize column) {
  const std::array<
      int, getElementBasisFunctionNumber<AdjacencyElementTrait::kElementType, SimulationControl::kPolynomialOrder>()>
      adjacency_element_view_node_parent_sequence{
          getAdjacencyElementViewNodeParentSequence<AdjacencyElementTrait::kElementType,
                                                    SimulationControl::kPolynomialOrder>(
              static_cast<int>(parent_gmsh_type_number), static_cast<int>(adjacency_sequence_in_parent))};
  if constexpr (SimulationControl::kEquationModel == EquationModelEnum::CompresibleEuler ||
                SimulationControl::kEquationModel == EquationModelEnum::IncompresibleEuler) {
    Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kBasisFunctionNumber>
        variable_basis_function_coefficient;
    raw_binary_ss.read(reinterpret_cast<char*>(variable_basis_function_coefficient.data()),
                       SimulationControl::kConservedVariableNumber * ElementTrait::kBasisFunctionNumber * kRealSize);
    for (Isize i = 0; i < AdjacencyElementTrait::kAllNodeNumber; i++) {
      this->view_variable_(column).variable_.conserved_.col(i).noalias() =
          variable_basis_function_coefficient * element_view_solver.basis_function_.modal_value_.col(
                                                    adjacency_element_view_node_parent_sequence[static_cast<Usize>(i)]);
    }
    this->view_variable_(column).variable_.calculateComputationalFromConserved(physical_model);
  }
  if constexpr (SimulationControl::kEquationModel == EquationModelEnum::CompresibleNS ||
                SimulationControl::kEquationModel == EquationModelEnum::IncompresibleNS) {
    Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kBasisFunctionNumber>
        variable_basis_function_coefficient;
    Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                  ElementTrait::kBasisFunctionNumber>
        variable_gradient_basis_function_coefficient;
    raw_binary_ss.read(reinterpret_cast<char*>(variable_basis_function_coefficient.data()),
                       SimulationControl::kConservedVariableNumber * ElementTrait::kBasisFunctionNumber * kRealSize);
    raw_binary_ss.read(reinterpret_cast<char*>(variable_gradient_basis_function_coefficient.data()),
                       SimulationControl::kConservedVariableNumber * SimulationControl::kDimension *
                           ElementTrait::kBasisFunctionNumber * kRealSize);
    for (Isize i = 0; i < AdjacencyElementTrait::kAllNodeNumber; i++) {
      this->view_variable_(column).variable_.conserved_.col(i).noalias() =
          variable_basis_function_coefficient * element_view_solver.basis_function_.modal_value_.col(
                                                    adjacency_element_view_node_parent_sequence[static_cast<Usize>(i)]);
      this->view_variable_(column).variable_gradient_.conserved_.col(i).noalias() =
          variable_gradient_basis_function_coefficient *
          element_view_solver.basis_function_.modal_value_.col(
              adjacency_element_view_node_parent_sequence[static_cast<Usize>(i)]);
    }
    this->view_variable_(column).variable_.calculateComputationalFromConserved(physical_model);
    this->view_variable_(column).variable_gradient_.calculatePrimitiveFromConserved(
        physical_model, this->view_variable_(column).variable_);
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementViewSolver<AdjacencyElementTrait, SimulationControl>::calcluateAdjacencyElementViewVariable(
    const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
    const PhysicalModel<SimulationControl>& physical_model, const ViewSolver<SimulationControl>& view_solver,
    const Eigen::Vector<Real, Eigen::Dynamic>& node_artificial_viscosity, std::stringstream& raw_binary_ss) {
  Eigen::Vector<Real, AdjacencyElementTrait::kBasicNodeNumber> variable_artificial_viscosity;
  for (Isize i = 0; i < adjacency_element_mesh.boundary_number_; i++) {
    const Isize adjacency_sequence_in_parent =
        adjacency_element_mesh.element_(i + adjacency_element_mesh.interior_number_).adjacency_sequence_in_parent_(0);
    const Isize parent_gmsh_type_number =
        adjacency_element_mesh.element_(i + adjacency_element_mesh.interior_number_).parent_gmsh_type_number_(0);
    if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
      this->calcluateAdjacencyPerElementViewVariable<LineTrait<SimulationControl::kPolynomialOrder>>(
          physical_model, view_solver.line_, raw_binary_ss, adjacency_sequence_in_parent, parent_gmsh_type_number, i);
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
      if (parent_gmsh_type_number == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->calcluateAdjacencyPerElementViewVariable<TriangleTrait<SimulationControl::kPolynomialOrder>>(
            physical_model, view_solver.triangle_, raw_binary_ss, adjacency_sequence_in_parent, parent_gmsh_type_number,
            i);
      } else if (parent_gmsh_type_number == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->calcluateAdjacencyPerElementViewVariable<QuadrangleTrait<SimulationControl::kPolynomialOrder>>(
            physical_model, view_solver.quadrangle_, raw_binary_ss, adjacency_sequence_in_parent,
            parent_gmsh_type_number, i);
      }
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
      if (parent_gmsh_type_number == TetrahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->calcluateAdjacencyPerElementViewVariable<TetrahedronTrait<SimulationControl::kPolynomialOrder>>(
            physical_model, view_solver.tetrahedron_, raw_binary_ss, adjacency_sequence_in_parent,
            parent_gmsh_type_number, i);
      } else if (parent_gmsh_type_number == PyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->calcluateAdjacencyPerElementViewVariable<PyramidTrait<SimulationControl::kPolynomialOrder>>(
            physical_model, view_solver.pyramid_, raw_binary_ss, adjacency_sequence_in_parent, parent_gmsh_type_number,
            i);
      }
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
      if (parent_gmsh_type_number == PyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->calcluateAdjacencyPerElementViewVariable<PyramidTrait<SimulationControl::kPolynomialOrder>>(
            physical_model, view_solver.pyramid_, raw_binary_ss, adjacency_sequence_in_parent, parent_gmsh_type_number,
            i);
      } else if (parent_gmsh_type_number == HexahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->calcluateAdjacencyPerElementViewVariable<HexahedronTrait<SimulationControl::kPolynomialOrder>>(
            physical_model, view_solver.hexahedron_, raw_binary_ss, adjacency_sequence_in_parent,
            parent_gmsh_type_number, i);
      }
    }
    for (Isize j = 0; j < AdjacencyElementTrait::kBasicNodeNumber; j++) {
      variable_artificial_viscosity(j) = node_artificial_viscosity(adjacency_element_mesh.element_(i).node_tag_(j) - 1);
    }
    this->view_variable_(i).artificial_viscosity_.noalias() =
        this->basis_function_.nodal_value_.transpose() * variable_artificial_viscosity;
  }
}

template <typename SimulationControl>
inline void ViewSolver<SimulationControl>::calcluateViewVariable(const Mesh<SimulationControl>& mesh,
                                                                 const PhysicalModel<SimulationControl>& physical_model,
                                                                 const std::filesystem::path& raw_binary_path,
                                                                 std::stringstream& raw_binary_ss) {
  RawBinaryCompress::read(raw_binary_path, raw_binary_ss);
  Eigen::Vector<Real, Eigen::Dynamic> node_artificial_viscosity(mesh.node_number_);
  raw_binary_ss.read(reinterpret_cast<char*>(node_artificial_viscosity.data()), mesh.node_number_ * kRealSize);
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.calcluateElementViewVariable(mesh.line_, physical_model, node_artificial_viscosity, raw_binary_ss);
    this->point_.calcluateAdjacencyElementViewVariable(mesh.point_, physical_model, *this, node_artificial_viscosity,
                                                       raw_binary_ss);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.calcluateElementViewVariable(mesh.triangle_, physical_model, node_artificial_viscosity,
                                                   raw_binary_ss);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.calcluateElementViewVariable(mesh.quadrangle_, physical_model, node_artificial_viscosity,
                                                     raw_binary_ss);
    }
    this->line_.calcluateAdjacencyElementViewVariable(mesh.line_, physical_model, *this, node_artificial_viscosity,
                                                      raw_binary_ss);
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.calcluateElementViewVariable(mesh.tetrahedron_, physical_model, node_artificial_viscosity,
                                                      raw_binary_ss);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.calcluateElementViewVariable(mesh.pyramid_, physical_model, node_artificial_viscosity,
                                                  raw_binary_ss);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.calcluateElementViewVariable(mesh.hexahedron_, physical_model, node_artificial_viscosity,
                                                     raw_binary_ss);
    }
    if constexpr (HasAdjacencyTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.calcluateAdjacencyElementViewVariable(mesh.triangle_, physical_model, *this,
                                                            node_artificial_viscosity, raw_binary_ss);
    }
    if constexpr (HasAdjacencyQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.calcluateAdjacencyElementViewVariable(mesh.quadrangle_, physical_model, *this,
                                                              node_artificial_viscosity, raw_binary_ss);
    }
  }
}

template <typename SimulationControl>
inline void ViewSolver<SimulationControl>::initialViewSolver(const Mesh<SimulationControl>& mesh) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.view_variable_.resize(mesh.line_.number_);
    this->point_.view_variable_.resize(mesh.point_.boundary_number_);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.view_variable_.resize(mesh.triangle_.number_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.view_variable_.resize(mesh.quadrangle_.number_);
    }
    this->line_.view_variable_.resize(mesh.line_.boundary_number_);
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.view_variable_.resize(mesh.tetrahedron_.number_);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.view_variable_.resize(mesh.pyramid_.number_);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.view_variable_.resize(mesh.hexahedron_.number_);
    }
    if constexpr (HasAdjacencyTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.view_variable_.resize(mesh.triangle_.boundary_number_);
    }
    if constexpr (HasAdjacencyQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.view_variable_.resize(mesh.quadrangle_.boundary_number_);
    }
  }
}

template <typename SimulationControl>
inline void ViewSolver<SimulationControl>::initialViewSolver(const ViewSolver<SimulationControl>& view_solver) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.view_variable_.resize(view_solver.line_.view_variable_.size());
    this->point_.view_variable_.resize(view_solver.point_.view_variable_.size());
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.view_variable_.resize(view_solver.triangle_.view_variable_.size());
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.view_variable_.resize(view_solver.quadrangle_.view_variable_.size());
    }
    this->line_.view_variable_.resize(view_solver.line_.view_variable_.size());
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.view_variable_.resize(view_solver.tetrahedron_.view_variable_.size());
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.view_variable_.resize(view_solver.pyramid_.view_variable_.size());
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.view_variable_.resize(view_solver.hexahedron_.view_variable_.size());
    }
    if constexpr (HasAdjacencyTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.view_variable_.resize(view_solver.triangle_.view_variable_.size());
    }
    if constexpr (HasAdjacencyQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.view_variable_.resize(view_solver.quadrangle_.view_variable_.size());
    }
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_RAW_BINARY_HPP_
