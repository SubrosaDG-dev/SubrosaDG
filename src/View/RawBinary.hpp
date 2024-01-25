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

#include <Eigen/Core>
#include <fstream>
#include <magic_enum.hpp>
#include <sstream>
#include <string>

#include "Mesh/ReadControl.hpp"
#include "Solver/SolveControl.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Concept.hpp"
#include "Utils/Enum.hpp"
#include "View/IOControl.hpp"

namespace SubrosaDG {

template <typename ElementTrait, typename SimulationControl, EquationModelEnum EquationModelType>
inline void ElementSolver<ElementTrait, SimulationControl, EquationModelType>::writeElementRawBinary(
    std::fstream& fout) const {
  for (Isize i = 0; i < this->number_; i++) {
#ifdef SUBROSA_DG_DEVELOP
    fout << this->element_(i).conserved_variable_basis_function_coefficient_(1) << '\n';
#else
    fout.write(
        reinterpret_cast<const char*>(this->element_(i).conserved_variable_basis_function_coefficient_(1).data()),
        SimulationControl::kConservedVariableNumber * ElementTrait::kBasisFunctionNumber *
            static_cast<std::streamsize>(sizeof(Real)));
#endif
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::writeRawBinary(std::fstream& fout) const {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.writeElementRawBinary(fout);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.writeElementRawBinary(fout);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.writeElementRawBinary(fout);
    }
  }
}

template <typename ElementTrait, typename SimulationControl>
inline void ElementViewVariable<ElementTrait, SimulationControl>::readElementRawBinary(
    const ElementMesh<ElementTrait>& element_mesh, std::fstream& raw_binary_finout) {
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kBasisFunctionNumber>
      conserved_variable_basis_function_coefficient;
  for (Isize i = 0; i < element_mesh.number_; i++) {
#ifdef SUBROSA_DG_DEVELOP
    for (Isize j = 0; j < SimulationControl::kConservedVariableNumber; j++) {
      for (Isize k = 0; k < ElementTrait::kBasisFunctionNumber; k++) {
        raw_binary_finout >> conserved_variable_basis_function_coefficient(j, k);
      }
    }
#else
    raw_binary_finout.read(reinterpret_cast<char*>(conserved_variable_basis_function_coefficient.data()),
                           SimulationControl::kConservedVariableNumber * ElementTrait::kBasisFunctionNumber *
                               static_cast<std::streamsize>(sizeof(Real)));
#endif
    this->conserved_variable_(i).noalias() =
        conserved_variable_basis_function_coefficient * this->basis_function_value_;
  }
}

template <typename ElementTrait, typename SimulationControl>
inline void ElementViewVariable<ElementTrait, SimulationControl>::addNodeConservedVariable(
    const ElementMesh<ElementTrait>& element_mesh,
    Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, Eigen::Dynamic>& node_conserved_variable) {
#if defined(SUBROSA_DG_WITH_OPENMP) && !defined(SUBROSA_DG_DEVELOP)
#pragma omp parallel for collapse(2) default(none) schedule(auto) \
    shared(Eigen::Dynamic, element_mesh, node_conserved_variable)
#endif  // SUBROSA_DG_WITH_OPENMP && !SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < element_mesh.number_; i++) {
    for (Isize j = 0; j < ElementTrait::kAllNodeNumber; j++) {
      node_conserved_variable.col(element_mesh.element_(i).node_tag_(j) - 1) += this->conserved_variable_(i).col(j);
    }
  }
}

template <typename SimulationControl>
inline void ViewVariable<SimulationControl>::readRawBinary(const Mesh<SimulationControl>& mesh,
                                                           const ViewConfigEnum config_enum,
                                                           std::fstream& raw_binary_finout) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.readElementRawBinary(mesh.line_, raw_binary_finout);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.readElementRawBinary(mesh.triangle_, raw_binary_finout);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.readElementRawBinary(mesh.quadrangle_, raw_binary_finout);
    }
  }
  if ((config_enum & ViewConfigEnum::SolverSmoothness) == ViewConfigEnum::SolverSmoothness) {
    this->calculateNodeConservedVariable(mesh);
  }
}

template <typename SimulationControl>
inline void ViewVariable<SimulationControl>::readTimeValue(const int iteration_number, std::fstream& error_finout) {
  std::string line;
  std::getline(error_finout, line);
  for (int i = 0; i < iteration_number; i++) {
    std::getline(error_finout, line);
    std::stringstream ss(line);
    ss.ignore(2) >> this->time_value_(i);
  }
}

template <typename SimulationControl>
inline void ViewVariable<SimulationControl>::calculateNodeConservedVariable(const Mesh<SimulationControl>& mesh) {
  this->node_conserved_variable_.setZero();
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.addNodeConservedVariable(mesh.line_, this->node_conserved_variable_);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.addNodeConservedVariable(mesh.triangle_, this->node_conserved_variable_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.addNodeConservedVariable(mesh.quadrangle_, this->node_conserved_variable_);
    }
  }
  this->node_conserved_variable_.array().rowwise() /=
      mesh.node_element_number_.template cast<Real>().transpose().array();
}

template <typename SimulationControl>
inline void ViewVariable<SimulationControl>::initializeViewVariable(const int iteration_number,
                                                                    const Mesh<SimulationControl>& mesh) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.conserved_variable_.resize(mesh.line_.number_);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.conserved_variable_.resize(mesh.triangle_.number_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.conserved_variable_.resize(mesh.quadrangle_.number_);
    }
  }
  this->time_value_.resize(iteration_number);
  this->node_conserved_variable_.resize(Eigen::NoChange, mesh.node_coordinate_.cols());
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_RAW_BINARY_HPP_
