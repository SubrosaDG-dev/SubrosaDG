/**
 * @file RawBinary.hpp
 * @brief The header file of SubrosaDG raw binary output.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-17
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_RAW_BINARY_HPP_
#define SUBROSA_DG_RAW_BINARY_HPP_

#include <Eigen/Core>
#include <fstream>

#include "Mesh/ReadControl.hpp"
#include "Solver/SimulationControl.hpp"
#include "Solver/SolveControl.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Concept.hpp"
#include "Utils/Enum.hpp"
#include "View/IOControl.hpp"

namespace SubrosaDG {

template <typename ElementTrait, typename SimulationControl, EquationModel EquationModelType>
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
inline void Solver<SimulationControl, 1>::writeRawBinary(std::fstream& fout) const {
  this->line_.writeElementRawBinary(fout);
}

template <typename SimulationControl>
inline void Solver<SimulationControl, 2>::writeRawBinary(std::fstream& fout) const {
  if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
    this->triangle_.writeElementRawBinary(fout);
  }
  if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
    this->quadrangle_.writeElementRawBinary(fout);
  }
}

template <typename ElementTrait, typename SimulationControl>
inline void ElementViewData<ElementTrait, SimulationControl>::readElementRawBinary(
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
    this->conserved_variable_(i) = conserved_variable_basis_function_coefficient * this->basis_function_value_;
  }
}

template <typename SimulationControl>
inline void ViewData<SimulationControl, 1>::readRawBinary(
    const Mesh<SimulationControl, SimulationControl::kDimension>& mesh, std::fstream& raw_binary_finout) {
  this->line_.readElementRawBinary(mesh.line_, raw_binary_finout);
}

template <typename SimulationControl>
inline void ViewData<SimulationControl, 2>::readRawBinary(
    const Mesh<SimulationControl, SimulationControl::kDimension>& mesh, std::fstream& raw_binary_finout) {
  if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
    this->triangle_.readElementRawBinary(mesh.triangle_, raw_binary_finout);
  }
  if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
    this->quadrangle_.readElementRawBinary(mesh.quadrangle_, raw_binary_finout);
  }
}

template <typename SimulationControl>
inline void ViewData<SimulationControl, 1>::calculateNodeConservedVariable(
    const Mesh<SimulationControl, SimulationControl::kDimension>& mesh) {
  this->node_conserved_variable_.setZero();
  for (Isize i = 0; i < mesh.line_.number_; i++) {
    for (Isize j = 0; j < LineTrait<SimulationControl::kPolynomialOrder>::kAllNodeNumber; j++) {
      this->node_conserved_variable_.col(mesh.line_.element_(i).node_tag_(j) - 1) +=
          this->line_.conserved_variable_(i).col(j);
    }
  }
  this->node_conserved_variable_.array().rowwise() /=
      mesh.node_element_number_.template cast<Real>().transpose().array();
}

template <typename SimulationControl>
inline void ViewData<SimulationControl, 2>::calculateNodeConservedVariable(
    const Mesh<SimulationControl, SimulationControl::kDimension>& mesh) {
  this->node_conserved_variable_.setZero();
  if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
    for (Isize i = 0; i < mesh.triangle_.number_; i++) {
      for (Isize j = 0; j < TriangleTrait<SimulationControl::kPolynomialOrder>::kAllNodeNumber; j++) {
        this->node_conserved_variable_.col(mesh.triangle_.element_(i).node_tag_(j) - 1) +=
            this->triangle_.conserved_variable_(i).col(j);
      }
    }
  }
  if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
    for (Isize i = 0; i < mesh.quadrangle_.number_; i++) {
      for (Isize j = 0; j < QuadrangleTrait<SimulationControl::kPolynomialOrder>::kAllNodeNumber; j++) {
        this->node_conserved_variable_.col(mesh.quadrangle_.element_(i).node_tag_(j) - 1) +=
            this->quadrangle_.conserved_variable_(i).col(j);
      }
    }
  }
  this->node_conserved_variable_.array().rowwise() /=
      mesh.node_element_number_.template cast<Real>().transpose().array();
}

template <typename SimulationControl>
inline void ViewData<SimulationControl, 1>::initializeViewData(
    const Mesh<SimulationControl, SimulationControl::kDimension>& mesh) {
  this->line_.conserved_variable_.resize(mesh.line_.number_);
  this->node_conserved_variable_.resize(Eigen::NoChange, mesh.node_coordinate_.cols());
}

template <typename SimulationControl>
inline void ViewData<SimulationControl, 2>::initializeViewData(
    const Mesh<SimulationControl, SimulationControl::kDimension>& mesh) {
  if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
    this->triangle_.conserved_variable_.resize(mesh.triangle_.number_);
  }
  if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
    this->quadrangle_.conserved_variable_.resize(mesh.quadrangle_.number_);
  }
  this->node_conserved_variable_.resize(Eigen::NoChange, mesh.node_coordinate_.cols());
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_RAW_BINARY_HPP_
