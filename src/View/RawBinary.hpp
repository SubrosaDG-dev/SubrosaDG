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
#include <sstream>
#include <string>

#include "Mesh/ReadControl.hpp"
#include "Solver/SolveControl.hpp"
#include "Solver/ThermalModel.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Concept.hpp"
#include "Utils/Enum.hpp"
#include "View/IOControl.hpp"

namespace SubrosaDG {

template <typename ElementTrait, typename SimulationControl>
inline void ElementSolver<ElementTrait, SimulationControl, EquationModelEnum::Euler>::writeElementRawBinary(
    std::fstream& raw_binary_fout) const {
  for (Isize i = 0; i < this->number_; i++) {
#ifdef SUBROSA_DG_DEVELOP
    raw_binary_fout << this->element_(i).variable_basis_function_coefficient_(1) << '\n';
#else
    raw_binary_fout.write(
        reinterpret_cast<const char*>(this->element_(i).variable_basis_function_coefficient_(1).data()),
        SimulationControl::kConservedVariableNumber * ElementTrait::kBasisFunctionNumber *
            static_cast<std::streamsize>(sizeof(Real)));
#endif
  }
}

template <typename ElementTrait, typename SimulationControl>
inline void ElementSolver<ElementTrait, SimulationControl, EquationModelEnum::NavierStokes>::writeElementRawBinary(
    std::fstream& raw_binary_fout) const {
  for (Isize i = 0; i < this->number_; i++) {
#ifdef SUBROSA_DG_DEVELOP
    raw_binary_fout << this->element_(i).variable_basis_function_coefficient_(1) << '\n'
                    << this->element_(i).variable_gradient_basis_function_coefficient_ << '\n';
#else
    raw_binary_fout.write(
        reinterpret_cast<const char*>(this->element_(i).variable_basis_function_coefficient_(1).data()),
        SimulationControl::kConservedVariableNumber * ElementTrait::kBasisFunctionNumber *
            static_cast<std::streamsize>(sizeof(Real)));
    raw_binary_fout.write(
        reinterpret_cast<const char*>(this->element_(i).variable_gradient_basis_function_coefficient_.data()),
        SimulationControl::kConservedVariableNumber * SimulationControl::kDimension *
            ElementTrait::kBasisFunctionNumber * static_cast<std::streamsize>(sizeof(Real)));
#endif
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::writeRawBinary() {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.writeElementRawBinary(this->raw_binary_fout_);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.writeElementRawBinary(this->raw_binary_fout_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.writeElementRawBinary(this->raw_binary_fout_);
    }
  }
}

template <typename ElementTrait, typename SimulationControl>
inline void ElementViewSolver<ElementTrait, SimulationControl, EquationModelEnum::Euler>::calcluateElementViewVariable(
    const ElementMesh<ElementTrait>& element_mesh, const ThermalModel<SimulationControl>& thermal_model,
    std::fstream& raw_binary_fin) {
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kBasisFunctionNumber>
      variable_basis_function_coefficient;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kAllNodeNumber> variable;
  for (Isize i = 0; i < element_mesh.number_; i++) {
#ifdef SUBROSA_DG_DEVELOP
    for (Isize j = 0; j < SimulationControl::kConservedVariableNumber; j++) {
      for (Isize k = 0; k < ElementTrait::kBasisFunctionNumber; k++) {
        raw_binary_fin >> variable_basis_function_coefficient(j, k);
      }
    }
#else
    raw_binary_fin.read(reinterpret_cast<char*>(variable_basis_function_coefficient.data()),
                        SimulationControl::kConservedVariableNumber * ElementTrait::kBasisFunctionNumber *
                            static_cast<std::streamsize>(sizeof(Real)));
#endif
    variable.noalias() = variable_basis_function_coefficient * this->basis_function_value_;
    for (Isize j = 0; j < ElementTrait::kAllNodeNumber; j++) {
      this->view_variable_(j, i).variable_.conserved_ = variable.col(j);
      this->view_variable_(j, i).variable_.calculateComputationalFromConserved(thermal_model);
    }
  }
}

template <typename ElementTrait, typename SimulationControl>
inline void
ElementViewSolver<ElementTrait, SimulationControl, EquationModelEnum::NavierStokes>::calcluateElementViewVariable(
    const ElementMesh<ElementTrait>& element_mesh, const ThermalModel<SimulationControl>& thermal_model,
    std::fstream& raw_binary_fin) {
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kBasisFunctionNumber>
      variable_basis_function_coefficient;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kAllNodeNumber> variable;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                ElementTrait::kBasisFunctionNumber>
      variable_gradient_basis_function_coefficient;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                ElementTrait::kAllNodeNumber>
      variable_gradient;
  for (Isize i = 0; i < element_mesh.number_; i++) {
#ifdef SUBROSA_DG_DEVELOP
    for (Isize j = 0; j < SimulationControl::kConservedVariableNumber; j++) {
      for (Isize k = 0; k < ElementTrait::kBasisFunctionNumber; k++) {
        raw_binary_fin >> variable_basis_function_coefficient(j, k);
      }
    }
    for (Isize j = 0; j < SimulationControl::kConservedVariableNumber * SimulationControl::kDimension; j++) {
      for (Isize k = 0; k < ElementTrait::kBasisFunctionNumber; k++) {
        raw_binary_fin >> variable_gradient_basis_function_coefficient(j, k);
      }
    }
#else
    raw_binary_fin.read(reinterpret_cast<char*>(variable_basis_function_coefficient.data()),
                        SimulationControl::kConservedVariableNumber * ElementTrait::kBasisFunctionNumber *
                            static_cast<std::streamsize>(sizeof(Real)));
    raw_binary_fin.read(reinterpret_cast<char*>(variable_gradient_basis_function_coefficient.data()),
                        SimulationControl::kConservedVariableNumber * SimulationControl::kDimension *
                            ElementTrait::kBasisFunctionNumber * static_cast<std::streamsize>(sizeof(Real)));
#endif
    variable.noalias() = variable_basis_function_coefficient * this->basis_function_value_;
    variable_gradient.noalias() = variable_gradient_basis_function_coefficient * this->basis_function_value_;
    for (Isize j = 0; j < ElementTrait::kAllNodeNumber; j++) {
      this->view_variable_(j, i).variable_.conserved_ = variable.col(j);
      this->view_variable_(j, i).variable_.calculateComputationalFromConserved(thermal_model);
      this->view_variable_(j, i).variable_gradient_.conserved_ =
          variable_gradient.col(j).reshaped(SimulationControl::kDimension, SimulationControl::kConservedVariableNumber);
      this->view_variable_(j, i).variable_gradient_.calculatePrimitiveFromConserved(
          thermal_model, this->view_variable_(j, i).variable_);
    }
  }
}

template <typename SimulationControl>
inline void ViewSolver<SimulationControl>::calcluateViewVariable(const Mesh<SimulationControl>& mesh,
                                                                 const ThermalModel<SimulationControl>& thermal_model,
                                                                 std::fstream& raw_binary_fin) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.calcluateElementViewVariable(mesh.line_, thermal_model, raw_binary_fin);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.calcluateElementViewVariable(mesh.triangle_, thermal_model, raw_binary_fin);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.calcluateElementViewVariable(mesh.quadrangle_, thermal_model, raw_binary_fin);
    }
  }
}

template <typename SimulationControl>
inline void ViewSolver<SimulationControl>::initialViewSolver(const Mesh<SimulationControl>& mesh) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.view_variable_.resize(Eigen::NoChange, mesh.line_.number_);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.view_variable_.resize(Eigen::NoChange, mesh.triangle_.number_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.view_variable_.resize(Eigen::NoChange, mesh.quadrangle_.number_);
    }
  }
}

template <typename SimulationControl>
inline void ViewSolver<SimulationControl>::initialViewSolver(const ViewSolver<SimulationControl>& view_solver) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.view_variable_.resize(Eigen::NoChange, view_solver.line_.view_variable_.cols());
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.view_variable_.resize(Eigen::NoChange, view_solver.triangle_.view_variable_.cols());
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.view_variable_.resize(Eigen::NoChange, view_solver.quadrangle_.view_variable_.cols());
    }
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_RAW_BINARY_HPP_
