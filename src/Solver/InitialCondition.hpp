/**
 * @file InitialCondition.hpp
 * @brief The header file of SubrosaDG initial condition.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-09
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_INITIAL_CONDITION_HPP_
#define SUBROSA_DG_INITIAL_CONDITION_HPP_

#include <Eigen/Cholesky>
#include <Eigen/Core>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "Mesh/BasisFunction.hpp"
#include "Mesh/Quadrature.hpp"
#include "Mesh/ReadControl.hpp"
#include "Solver/BoundaryCondition.hpp"
#include "Solver/SimulationControl.hpp"
#include "Solver/SolveControl.hpp"
#include "Solver/ThermalModel.hpp"
#include "Solver/VariableConvertor.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Concept.hpp"
#include "Utils/Enum.hpp"

namespace SubrosaDG {

template <typename SimulationControl, InitialConditionEnum InitialConditionType>
struct InitialConditionBase;

template <typename SimulationControl>
struct InitialConditionBase<SimulationControl, InitialConditionEnum::Function> {
  std::function<Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber>(
      const Eigen::Vector<Real, SimulationControl::kDimension>& coordinate)>
      function_;

  template <typename ElementTrait>
  void getVariableBasisFunctionCoefficient(
      const ElementMesh<ElementTrait>& element_mesh, const ThermalModel<SimulationControl>& thermal_model,
      Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kQuadratureNumber>,
                   Eigen::Dynamic, 1>& quadrature_node_conserved_variable) {
    Variable<SimulationControl> variable;
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto) \
    shared(Eigen::Dynamic, element_mesh, thermal_model, quadrature_node_conserved_variable) private(variable)
#endif  // SUBROSA_DG_DEVELOP
    for (Isize i = 0; i < element_mesh.number_; i++) {
      for (Isize j = 0; j < ElementTrait::kQuadratureNumber; j++) {
        variable.primitive_ = this->function_(element_mesh.element_(i).quadrature_node_coordinate_.col(j));
        variable.calculateConservedFromPrimitive(thermal_model);
        quadrature_node_conserved_variable(i).col(j) = variable.conserved_;
      }
    }
  }
};

template <typename SimulationControl>
struct InitialConditionBase<SimulationControl, InitialConditionEnum::SpecificFile> {
  std::filesystem::path raw_binary_path_;
  std::stringstream raw_binary_ss_;

  template <typename ElementTrait>
  void getVariableBasisFunctionCoefficient(
      const ElementMesh<ElementTrait>& element_mesh,
      [[maybe_unused]] const ThermalModel<SimulationControl>& thermal_model,
      Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kQuadratureNumber>,
                   Eigen::Dynamic, 1>& quadrature_node_conserved_variable) {
    constexpr int kBasisFunctionNumber{
        getElementBasisFunctionNumber<ElementTrait::kElementType, SimulationControl::kInitialPolynomialOrder>()};
    Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, kBasisFunctionNumber>, Eigen::Dynamic,
                 1>
        variable_basis_function_coefficient(element_mesh.number_);
    Eigen::Matrix<Real, ElementTrait::kQuadratureNumber, kBasisFunctionNumber, Eigen::RowMajor> basis_function_value;
    const auto& [local_coord, weights] = getElementQuadrature<ElementTrait>();
    std::vector<double> basis_functions =
        getElementBasisFunction<ElementTrait::kElementType, SimulationControl::kInitialPolynomialOrder>(local_coord);
    for (Isize i = 0; i < ElementTrait::kQuadratureNumber; i++) {
      for (Isize j = 0; j < kBasisFunctionNumber; j++) {
        basis_function_value(i, j) =
            static_cast<Real>(basis_functions[static_cast<Usize>(i * kBasisFunctionNumber + j)]);
      }
    }
    for (Isize i = 0; i < element_mesh.number_; i++) {
      this->raw_binary_ss_.read(reinterpret_cast<char*>(variable_basis_function_coefficient(i).data()),
                                SimulationControl::kConservedVariableNumber * kBasisFunctionNumber *
                                    static_cast<std::streamsize>(sizeof(Real)));
      if constexpr (SimulationControl::kEquationModel == EquationModelEnum::NavierStokes) {
        Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                      kBasisFunctionNumber>
            variable_gradient_basis_function_coefficient;
        this->raw_binary_ss_.read(reinterpret_cast<char*>(variable_gradient_basis_function_coefficient.data()),
                                  SimulationControl::kConservedVariableNumber * SimulationControl::kDimension *
                                      kBasisFunctionNumber * static_cast<std::streamsize>(sizeof(Real)));
      }
    }
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto)                                              \
    shared(Eigen::Dynamic, element_mesh, quadrature_node_conserved_variable, variable_basis_function_coefficient) \
    firstprivate(basis_function_value)
#endif  // SUBROSA_DG_DEVELOP
    for (Isize i = 0; i < element_mesh.number_; i++) {
      quadrature_node_conserved_variable(i) = variable_basis_function_coefficient(i) * basis_function_value.transpose();
    }
  }
};

template <typename SimulationControl>
struct InitialConditionBase<SimulationControl, InitialConditionEnum::LastStep> {
  std::filesystem::path raw_binary_path_;
  std::stringstream raw_binary_ss_;

  template <typename ComputationalElementTrait>
  void getVariableBasisFunctionCoefficient(const ElementMesh<ComputationalElementTrait>& element_mesh,
                                           [[maybe_unused]] const ThermalModel<SimulationControl>& thermal_model,
                                           Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber,
                                                                      ComputationalElementTrait::kQuadratureNumber>,
                                                        Eigen::Dynamic, 1>& quadrature_node_conserved_variable) {
    Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber,
                               ComputationalElementTrait::kBasisFunctionNumber>,
                 Eigen::Dynamic, 1>
        variable_basis_function_coefficient(element_mesh.number_);
    for (Isize i = 0; i < element_mesh.number_; i++) {
      this->raw_binary_ss_.read(reinterpret_cast<char*>(variable_basis_function_coefficient(i).data()),
                                SimulationControl::kConservedVariableNumber *
                                    ComputationalElementTrait::kBasisFunctionNumber *
                                    static_cast<std::streamsize>(sizeof(Real)));
      if constexpr (SimulationControl::kEquationModel == EquationModelEnum::NavierStokes) {
        Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                      ComputationalElementTrait::kBasisFunctionNumber>
            variable_gradient_basis_function_coefficient;
        this->raw_binary_ss_.read(reinterpret_cast<char*>(variable_gradient_basis_function_coefficient.data()),
                                  SimulationControl::kConservedVariableNumber * SimulationControl::kDimension *
                                      ComputationalElementTrait::kBasisFunctionNumber *
                                      static_cast<std::streamsize>(sizeof(Real)));
      }
    }
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto) \
    shared(Eigen::Dynamic, element_mesh, quadrature_node_conserved_variable, variable_basis_function_coefficient)
#endif  // SUBROSA_DG_DEVELOP
    for (Isize i = 0; i < element_mesh.number_; i++) {
      quadrature_node_conserved_variable(i) =
          variable_basis_function_coefficient(i) * element_mesh.basis_function_.value_.transpose();
    }
  }
};

template <typename SimulationControl>
struct InitialCondition : InitialConditionBase<SimulationControl, SimulationControl::kInitialCondition> {};

template <typename ElementTrait, typename SimulationControl>
inline void ElementSolverBase<ElementTrait, SimulationControl>::initializeElementSolver(
    const ElementMesh<ElementTrait>& element_mesh, const ThermalModel<SimulationControl>& thermal_model,
    InitialCondition<SimulationControl>& initial_condition) {
  this->number_ = element_mesh.number_;
  this->element_.resize(this->number_);
  Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kQuadratureNumber>,
               Eigen::Dynamic, 1>
      quadrature_node_conserved_variable(this->number_);
  initial_condition.getVariableBasisFunctionCoefficient(element_mesh, thermal_model,
                                                        quadrature_node_conserved_variable);
  Eigen::LLT<Eigen::Matrix<Real, ElementTrait::kBasisFunctionNumber, ElementTrait::kBasisFunctionNumber>>
      basis_function_value_llt(element_mesh.basis_function_.value_.transpose() * element_mesh.basis_function_.value_);
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto) \
    shared(Eigen::Dynamic, element_mesh, quadrature_node_conserved_variable) firstprivate(basis_function_value_llt)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < this->number_; i++) {
    this->element_(i).variable_basis_function_coefficient_(1).noalias() =
        basis_function_value_llt
            .solve((quadrature_node_conserved_variable(i) * element_mesh.basis_function_.value_).transpose())
            .transpose();
  }
}

template <typename ElementTrait, typename SimulationControl>
inline void
ElementSolver<ElementTrait, SimulationControl, EquationModelEnum::NavierStokes>::initializeElementGardientSolver() {
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto) shared(Eigen::Dynamic)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < this->number_; i++) {
    for (Isize j = 0; j < ElementTrait::kAdjacencyNumber; j++) {
      this->element_(i).variable_gradient_interface_adjacency_quadrature_(j).setZero();
    }
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::initializeSolver(
    const Mesh<SimulationControl>& mesh, const ThermalModel<SimulationControl>& thermal_model,
    std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition,
    InitialCondition<SimulationControl>& initial_condition) {
  for (auto& [name, variable] : boundary_condition) {
    variable->boundary_dummy_variable_.calculateConservedFromPrimitive(thermal_model);
    variable->boundary_dummy_variable_.calculateComputationalFromPrimitive(thermal_model);
  }
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.initializeElementSolver(mesh.line_, thermal_model, initial_condition);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.initializeElementSolver(mesh.triangle_, thermal_model, initial_condition);
      if constexpr (SimulationControl::kEquationModel == EquationModelEnum::NavierStokes) {
        this->triangle_.initializeElementGardientSolver();
      }
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.initializeElementSolver(mesh.quadrangle_, thermal_model, initial_condition);
      if constexpr (SimulationControl::kEquationModel == EquationModelEnum::NavierStokes) {
        this->quadrangle_.initializeElementGardientSolver();
      }
    }
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.initializeElementSolver(mesh.tetrahedron_, thermal_model, initial_condition);
      if constexpr (SimulationControl::kEquationModel == EquationModelEnum::NavierStokes) {
        this->tetrahedron_.initializeElementGardientSolver();
      }
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.initializeElementSolver(mesh.pyramid_, thermal_model, initial_condition);
      if constexpr (SimulationControl::kEquationModel == EquationModelEnum::NavierStokes) {
        this->pyramid_.initializeElementGardientSolver();
      }
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.initializeElementSolver(mesh.hexahedron_, thermal_model, initial_condition);
      if constexpr (SimulationControl::kEquationModel == EquationModelEnum::NavierStokes) {
        this->hexahedron_.initializeElementGardientSolver();
      }
    }
  }
  this->relative_error_.setZero();
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_INITIAL_CONDITION_HPP_
