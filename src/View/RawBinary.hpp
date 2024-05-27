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
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <sstream>
#include <string>
#include <vector>

#include "Mesh/ReadControl.hpp"
#include "Solver/SolveControl.hpp"
#include "Solver/ThermalModel.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Concept.hpp"
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
inline void ElementSolver<ElementTrait, SimulationControl, EquationModelEnum::Euler>::writeElementRawBinary(
    std::stringstream& raw_binary_ss) const {
  for (Isize i = 0; i < this->number_; i++) {
    raw_binary_ss.write(reinterpret_cast<const char*>(this->element_(i).variable_basis_function_coefficient_(1).data()),
                        SimulationControl::kConservedVariableNumber * ElementTrait::kBasisFunctionNumber *
                            static_cast<std::streamsize>(sizeof(Real)));
  }
}

template <typename ElementTrait, typename SimulationControl>
inline void ElementSolver<ElementTrait, SimulationControl, EquationModelEnum::NavierStokes>::writeElementRawBinary(
    std::stringstream& raw_binary_ss) const {
  for (Isize i = 0; i < this->number_; i++) {
    raw_binary_ss.write(reinterpret_cast<const char*>(this->element_(i).variable_basis_function_coefficient_(1).data()),
                        SimulationControl::kConservedVariableNumber * ElementTrait::kBasisFunctionNumber *
                            static_cast<std::streamsize>(sizeof(Real)));
    raw_binary_ss.write(
        reinterpret_cast<const char*>(this->element_(i).variable_gradient_basis_function_coefficient_.data()),
        SimulationControl::kConservedVariableNumber * SimulationControl::kDimension *
            ElementTrait::kBasisFunctionNumber * static_cast<std::streamsize>(sizeof(Real)));
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::writeRawBinary(const std::filesystem::path& raw_binary_path) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.writeElementRawBinary(this->raw_binary_ss_);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.writeElementRawBinary(this->raw_binary_ss_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.writeElementRawBinary(this->raw_binary_ss_);
    }
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
  }
  this->write_raw_binary_future_ =
      std::async(std::launch::async, RawBinaryCompress::write, raw_binary_path, std::ref(this->raw_binary_ss_));
}

template <typename ElementTrait, typename SimulationControl>
inline void ElementViewSolver<ElementTrait, SimulationControl, EquationModelEnum::Euler>::calcluateElementViewVariable(
    const ElementMesh<ElementTrait>& element_mesh, const ThermalModel<SimulationControl>& thermal_model,
    std::stringstream& raw_binary_ss) {
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kBasisFunctionNumber>
      variable_basis_function_coefficient;
  for (Isize i = 0; i < element_mesh.number_; i++) {
    raw_binary_ss.read(reinterpret_cast<char*>(variable_basis_function_coefficient.data()),
                       SimulationControl::kConservedVariableNumber * ElementTrait::kBasisFunctionNumber *
                           static_cast<std::streamsize>(sizeof(Real)));
    this->view_variable_(i).variable_.conserved_.noalias() =
        variable_basis_function_coefficient * this->basis_function_value_;
    this->view_variable_(i).variable_.calculateComputationalFromConserved(thermal_model);
  }
}

template <typename ElementTrait, typename SimulationControl>
inline void
ElementViewSolver<ElementTrait, SimulationControl, EquationModelEnum::NavierStokes>::calcluateElementViewVariable(
    const ElementMesh<ElementTrait>& element_mesh, const ThermalModel<SimulationControl>& thermal_model,
    std::stringstream& raw_binary_ss) {
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kBasisFunctionNumber>
      variable_basis_function_coefficient;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                ElementTrait::kBasisFunctionNumber>
      variable_gradient_basis_function_coefficient;
  for (Isize i = 0; i < element_mesh.number_; i++) {
    raw_binary_ss.read(reinterpret_cast<char*>(variable_basis_function_coefficient.data()),
                       SimulationControl::kConservedVariableNumber * ElementTrait::kBasisFunctionNumber *
                           static_cast<std::streamsize>(sizeof(Real)));
    raw_binary_ss.read(reinterpret_cast<char*>(variable_gradient_basis_function_coefficient.data()),
                       SimulationControl::kConservedVariableNumber * SimulationControl::kDimension *
                           ElementTrait::kBasisFunctionNumber * static_cast<std::streamsize>(sizeof(Real)));
    this->view_variable_(i).variable_.conserved_.noalias() =
        variable_basis_function_coefficient * this->basis_function_value_;
    this->view_variable_(i).variable_.calculateComputationalFromConserved(thermal_model);
    this->view_variable_(i).variable_gradient_.conserved_.noalias() =
        variable_gradient_basis_function_coefficient * this->basis_function_value_;
    this->view_variable_(i).variable_gradient_.calculatePrimitiveFromConserved(thermal_model,
                                                                               this->view_variable_(i).variable_);
  }
}

template <typename SimulationControl>
inline void ViewSolver<SimulationControl>::calcluateViewVariable(const Mesh<SimulationControl>& mesh,
                                                                 const ThermalModel<SimulationControl>& thermal_model,
                                                                 const std::filesystem::path& raw_binary_path,
                                                                 std::stringstream& raw_binary_ss) {
  RawBinaryCompress::read(raw_binary_path, raw_binary_ss);
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.calcluateElementViewVariable(mesh.line_, thermal_model, raw_binary_ss);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.calcluateElementViewVariable(mesh.triangle_, thermal_model, raw_binary_ss);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.calcluateElementViewVariable(mesh.quadrangle_, thermal_model, raw_binary_ss);
    }
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.calcluateElementViewVariable(mesh.tetrahedron_, thermal_model, raw_binary_ss);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.calcluateElementViewVariable(mesh.pyramid_, thermal_model, raw_binary_ss);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.calcluateElementViewVariable(mesh.hexahedron_, thermal_model, raw_binary_ss);
    }
  }
}

template <typename SimulationControl>
inline void ViewSolver<SimulationControl>::initialViewSolver(const Mesh<SimulationControl>& mesh) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.view_variable_.resize(mesh.line_.number_);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.view_variable_.resize(mesh.triangle_.number_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.view_variable_.resize(mesh.quadrangle_.number_);
    }
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
  }
}

template <typename SimulationControl>
inline void ViewSolver<SimulationControl>::initialViewSolver(const ViewSolver<SimulationControl>& view_solver) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.view_variable_.resize(view_solver.line_.view_variable_.size());
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.view_variable_.resize(view_solver.triangle_.view_variable_.size());
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.view_variable_.resize(view_solver.quadrangle_.view_variable_.size());
    }
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
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_RAW_BINARY_HPP_
