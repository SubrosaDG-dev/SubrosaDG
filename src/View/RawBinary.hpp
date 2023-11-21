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

#include <fstream>

#include "Mesh/ReadControl.hpp"
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
inline void Solver<SimulationControl, 2>::writeRawBinary(std::fstream& fout) const {
  if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
    this->triangle_.writeElementRawBinary(fout);
  }
  if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
    this->quadrangle_.writeElementRawBinary(fout);
  }
}

template <typename ElementTrait, typename SimulationControl>
void ElementViewData<ElementTrait, SimulationControl>::readElementRawBinary(
    const ElementMesh<ElementTrait>& element_mesh, std::fstream& raw_binary_finout) {
  this->conserved_variable_basis_function_coefficient_.resize(element_mesh.number_);
  for (Isize i = 0; i < element_mesh.number_; i++) {
#ifdef SUBROSA_DG_DEVELOP
    for (Isize j = 0; j < SimulationControl::kConservedVariableNumber; j++) {
      for (Isize k = 0; k < ElementTrait::kBasisFunctionNumber; k++) {
        raw_binary_finout >> this->conserved_variable_basis_function_coefficient_(i)(j, k);
      }
    }
#else
    raw_binary_finout.read(reinterpret_cast<char*>(this->conserved_variable_basis_function_coefficient_(i).data()),
                           SimulationControl::kConservedVariableNumber * ElementTrait::kBasisFunctionNumber *
                               static_cast<std::streamsize>(sizeof(Real)));
#endif
  }
}

template <typename SimulationControl>
void ViewData<SimulationControl, 2>::readRawBinary(const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
                                                   std::fstream& raw_binary_finout) {
  if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
    this->triangle_.readElementRawBinary(mesh.triangle_, raw_binary_finout);
  }
  if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
    this->quadrangle_.readElementRawBinary(mesh.quadrangle_, raw_binary_finout);
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_RAW_BINARY_HPP_
