/**
 * @file SolveControl.hpp
 * @brief The header file of SubrosaDG solve control.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-09
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_SOLVE_CONTROL_HPP_
#define SUBROSA_DG_SOLVE_CONTROL_HPP_

#include <Eigen/Core>
#include <filesystem>
#include <fstream>
#include <future>
#include <memory>
#include <sstream>
#include <unordered_map>

#include "Mesh/ReadControl.hpp"
#include "Solver/SimulationControl.hpp"
#include "Solver/ThermalModel.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Enum.hpp"

namespace SubrosaDG {

template <typename AdjacencyElementTrait, typename SimulationControl>
struct AdjacencyElementVariable;
template <typename SimulationControl>
struct SourceTerm;
template <typename SimulationControl>
struct BoundaryConditionBase;
template <typename SimulationControl>
struct InitialCondition;
template <typename SimulationControl>
struct TimeIntegration;
template <typename SimulationControl, int Dimension>
struct SolverData;
template <typename SimulationControl>
struct Solver;
template <typename ElementTrait, typename SimulationControl, SourceTermEnum SourceTermType>
struct PerElementSolverSource;

template <typename ElementTrait, typename SimulationControl>
struct PerElementSolverSource<ElementTrait, SimulationControl, SourceTermEnum::None> {};

template <typename ElementTrait, typename SimulationControl>
struct PerElementSolverSource<ElementTrait, SimulationControl, SourceTermEnum::Gravity> {
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kQuadratureNumber>
      variable_source_quadrature_;
};

template <typename ElementTrait, typename SimulationControl, ViscousFluxEnum ViscousFluxType>
struct PerElementSolverGradientInterface;

template <typename ElementTrait, typename SimulationControl>
struct PerElementSolverGradientInterface<ElementTrait, SimulationControl, ViscousFluxEnum::BR1> {
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                ElementTrait::kBasisFunctionNumber>
      variable_gradient_interface_basis_function_coefficient_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                ElementTrait::kAllAdjacencyQuadratureNumber>
      variable_gradient_interface_adjacency_quadrature_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                ElementTrait::kBasisFunctionNumber>
      variable_gradient_interface_residual_;
};

template <typename ElementTrait, typename SimulationControl>
struct PerElementSolverGradientInterface<ElementTrait, SimulationControl, ViscousFluxEnum::BR2> {
  Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                             ElementTrait::kBasisFunctionNumber>,
               ElementTrait::kAdjacencyNumber, 1>
      variable_gradient_interface_basis_function_coefficient_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                ElementTrait::kAllAdjacencyQuadratureNumber>
      variable_gradient_interface_adjacency_quadrature_;
  Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                             ElementTrait::kBasisFunctionNumber>,
               ElementTrait::kAdjacencyNumber, 1>
      variable_gradient_interface_residual_;
};

template <typename ElementTrait, typename SimulationControl, EquationModelEnum EquationModelType>
struct PerElementSolver;

template <typename ElementTrait, typename SimulationControl>
struct PerElementSolver<ElementTrait, SimulationControl, EquationModelEnum::Euler>
    : PerElementSolverSource<ElementTrait, SimulationControl, SimulationControl::kSourceTerm> {
  Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kBasisFunctionNumber>, 2,
               1>
      variable_basis_function_coefficient_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber,
                ElementTrait::kQuadratureNumber * SimulationControl::kDimension>
      variable_quadrature_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kAllAdjacencyQuadratureNumber>
      variable_adjacency_quadrature_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kBasisFunctionNumber>
      variable_residual_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                ElementTrait::kBasisFunctionNumber>
      variable_gradient_volume_basis_function_coefficient_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                ElementTrait::kQuadratureNumber * SimulationControl::kDimension>
      variable_gradient_volume_quadrature_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                ElementTrait::kAllAdjacencyQuadratureNumber>
      variable_gradient_volume_adjacency_quadrature_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                ElementTrait::kBasisFunctionNumber>
      variable_gradient_volume_residual_;
  Eigen::Vector<Real, ElementTrait::kBasicNodeNumber> variable_artificial_viscosity_;
};

template <typename ElementTrait, typename SimulationControl>
struct PerElementSolver<ElementTrait, SimulationControl, EquationModelEnum::NavierStokes>
    : PerElementSolverSource<ElementTrait, SimulationControl, SimulationControl::kSourceTerm>,
      PerElementSolverGradientInterface<ElementTrait, SimulationControl, SimulationControl::kViscousFlux> {
  Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kBasisFunctionNumber>, 2,
               1>
      variable_basis_function_coefficient_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber,
                ElementTrait::kQuadratureNumber * SimulationControl::kDimension>
      variable_quadrature_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kAllAdjacencyQuadratureNumber>
      variable_adjacency_quadrature_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kBasisFunctionNumber>
      variable_residual_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                ElementTrait::kBasisFunctionNumber>
      variable_gradient_basis_function_coefficient_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                ElementTrait::kBasisFunctionNumber>
      variable_gradient_volume_basis_function_coefficient_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                ElementTrait::kQuadratureNumber * SimulationControl::kDimension>
      variable_gradient_volume_quadrature_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                ElementTrait::kAllAdjacencyQuadratureNumber>
      variable_gradient_volume_adjacency_quadrature_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                ElementTrait::kBasisFunctionNumber>
      variable_gradient_volume_residual_;
  Eigen::Vector<Real, ElementTrait::kBasicNodeNumber> variable_artificial_viscosity_;
};

template <typename ElementTrait, typename SimulationControl>
struct ElementSolverBase {
  Isize number_{0};
  Eigen::Array<PerElementSolver<ElementTrait, SimulationControl, SimulationControl::kEquationModel>, Eigen::Dynamic, 1>
      element_;

  inline void initializeElementSolver(const ElementMesh<ElementTrait>& element_mesh,
                                      const ThermalModel<SimulationControl>& thermal_model,
                                      InitialCondition<SimulationControl>& initial_condition);

  inline void copyElementBasisFunctionCoefficient();

  inline void calculateElementArtificialViscosity(const ElementMesh<ElementTrait>& element_mesh,
                                                  Real empirical_tolerance, Real artificial_viscosity_factor);

  inline void maxElementArtificialViscosity(const ElementMesh<ElementTrait>& element_mesh,
                                            Eigen::Vector<Real, Eigen::Dynamic>& node_artificial_viscosity);

  inline void storeElementArtificialViscosity(const ElementMesh<ElementTrait>& element_mesh,
                                              const Eigen::Vector<Real, Eigen::Dynamic>& node_artificial_viscosity);

  inline void calculateElementGardientQuadrature(const ElementMesh<ElementTrait>& element_mesh);

  inline Real calculateElementDeltaTime(const ElementMesh<ElementTrait>& element_mesh,
                                        const ThermalModel<SimulationControl>& thermal_model,
                                        Real courant_friedrichs_lewy_number);

  inline void calculateElementResidual(const ElementMesh<ElementTrait>& element_mesh);

  inline void calculateElementGardientResidual(const ElementMesh<ElementTrait>& element_mesh);

  inline void updateElementBasisFunctionCoefficient(int step, const ElementMesh<ElementTrait>& element_mesh,
                                                    const TimeIntegration<SimulationControl>& time_integration);

  inline void updateElementGardientBasisFunctionCoefficient(const ElementMesh<ElementTrait>& element_mesh);

  inline void calculateElementRelativeError(
      const ElementMesh<ElementTrait>& element_mesh,
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& relative_error);
};

template <typename ElementTrait, typename SimulationControl, EquationModelEnum EquationModelType>
struct ElementSolver;

template <typename ElementTrait, typename SimulationControl>
struct ElementSolver<ElementTrait, SimulationControl, EquationModelEnum::Euler>
    : ElementSolverBase<ElementTrait, SimulationControl> {
  inline void calculateElementQuadrature(const ElementMesh<ElementTrait>& element_mesh,
                                         [[maybe_unused]] const SourceTerm<SimulationControl>& source_term,
                                         const ThermalModel<SimulationControl>& thermal_model);

  inline void writeElementRawBinary(std::stringstream& raw_binary_ss) const;
};

template <typename ElementTrait, typename SimulationControl>
struct ElementSolver<ElementTrait, SimulationControl, EquationModelEnum::NavierStokes>
    : ElementSolverBase<ElementTrait, SimulationControl> {
  inline void calculateElementQuadrature(const ElementMesh<ElementTrait>& element_mesh,
                                         [[maybe_unused]] const SourceTerm<SimulationControl>& source_term,
                                         const ThermalModel<SimulationControl>& thermal_model);

  inline void writeElementRawBinary(std::stringstream& raw_binary_ss) const;
};

template <typename AdjacencyElementTrait, typename SimulationControl>
struct AdjacencyElementSolverBase {
  Isize interior_number_{0};
  Isize boundary_number_{0};
  Eigen::Array<AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl>, Eigen::Dynamic, 1>
      boundary_dummy_variable_;

  inline void initializeAdjacencyElementSolver(
      const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
      const ThermalModel<SimulationControl>& thermal_model,
      const std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition);

  [[nodiscard]] inline Isize getAdjacencyParentElementAccumulateAdjacencyQuadratureNumber(
      [[maybe_unused]] Isize parent_gmsh_type_number, Isize adjacency_sequence_in_parent);

  inline void calculateAdjacencyElementArtificialViscosity(
      const Mesh<SimulationControl>& mesh, const Solver<SimulationControl>& solver,
      Eigen::Vector<Real, AdjacencyElementTrait::kQuadratureNumber>& quadrature_node_artificial_viscosity,
      Isize parent_gmsh_type_number, Isize parent_index_each_type, Isize adjacency_sequence_in_parent);

  inline void calculateInteriorAdjacencyElementGardientQuadrature(const Mesh<SimulationControl>& mesh,
                                                                  Solver<SimulationControl>& solver);

  inline void calculateBoundaryAdjacencyElementGardientQuadrature(
      const Mesh<SimulationControl>& mesh, const ThermalModel<SimulationControl>& thermal_model,
      const std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition,
      Solver<SimulationControl>& solver);

  inline void storeAdjacencyElementNodeQuadrature(
      Isize parent_gmsh_type_number, Isize parent_index, Isize quadrature_node_sequence_in_parent,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& quadrature_node_temporary_variable,
      Solver<SimulationControl>& solver);

  inline void storeAdjacencyElementNodeVolumeGardientQuadrature(
      Isize parent_gmsh_type_number, Isize parent_index, Isize quadrature_node_sequence_in_parent,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>&
          quadrature_node_temporary_variable,
      Solver<SimulationControl>& solver);

  inline void storeAdjacencyElementNodeInterfaceGardientQuadrature(
      Isize parent_gmsh_type_number, Isize parent_index, Isize quadrature_node_sequence_in_parent,
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>&
          quadrature_node_temporary_variable,
      Solver<SimulationControl>& solver);
};

template <typename AdjacencyElementTrait, typename SimulationControl, EquationModelEnum EquationModelType>
struct AdjacencyElementSolver;

template <typename AdjacencyElementTrait, typename SimulationControl>
struct AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl, EquationModelEnum::Euler>
    : AdjacencyElementSolverBase<AdjacencyElementTrait, SimulationControl> {
  inline void calculateInteriorAdjacencyElementQuadrature(const Mesh<SimulationControl>& mesh,
                                                          const ThermalModel<SimulationControl>& thermal_model,
                                                          Solver<SimulationControl>& solver);

  inline void calculateBoundaryAdjacencyElementQuadrature(
      const Mesh<SimulationControl>& mesh, const ThermalModel<SimulationControl>& thermal_model,
      const std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition,
      Solver<SimulationControl>& solver);

  inline void writeBoundaryAdjacencyElementRawBinary(
      const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
      const Solver<SimulationControl>& solver, std::stringstream& raw_binary_ss) const;
};

template <typename AdjacencyElementTrait, typename SimulationControl>
struct AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl, EquationModelEnum::NavierStokes>
    : AdjacencyElementSolverBase<AdjacencyElementTrait, SimulationControl> {
  inline void calculateInteriorAdjacencyElementQuadrature(const Mesh<SimulationControl>& mesh,
                                                          const ThermalModel<SimulationControl>& thermal_model,
                                                          Solver<SimulationControl>& solver);

  inline void calculateBoundaryAdjacencyElementQuadrature(
      const Mesh<SimulationControl>& mesh, const ThermalModel<SimulationControl>& thermal_model,
      const std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition,
      Solver<SimulationControl>& solver);

  inline void writeBoundaryAdjacencyElementRawBinary(
      const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
      const Solver<SimulationControl>& solver, std::stringstream& raw_binary_ss) const;
};

template <typename SimulationControl>
struct SolverBase {
  Real empirical_tolerance_{0.0};
  Real artificial_viscosity_factor_{1.0};

  std::stringstream raw_binary_ss_;
  std::fstream error_finout_;
  std::future<void> write_raw_binary_future_;

  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> relative_error_{
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>::Zero()};
  Eigen::Vector<Real, Eigen::Dynamic> node_artificial_viscosity_;
};

template <typename SimulationControl>
struct SolverData<SimulationControl, 1> : SolverBase<SimulationControl> {
  AdjacencyElementSolver<AdjacencyPointTrait<SimulationControl::kPolynomialOrder>, SimulationControl,
                         SimulationControl::kEquationModel>
      point_;
  ElementSolver<LineTrait<SimulationControl::kPolynomialOrder>, SimulationControl, SimulationControl::kEquationModel>
      line_;
};

template <typename SimulationControl>
struct SolverData<SimulationControl, 2> : SolverBase<SimulationControl> {
  AdjacencyElementSolver<AdjacencyLineTrait<SimulationControl::kPolynomialOrder>, SimulationControl,
                         SimulationControl::kEquationModel>
      line_;
  ElementSolver<TriangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl,
                SimulationControl::kEquationModel>
      triangle_;
  ElementSolver<QuadrangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl,
                SimulationControl::kEquationModel>
      quadrangle_;
};

template <typename SimulationControl>
struct SolverData<SimulationControl, 3> : SolverBase<SimulationControl> {
  AdjacencyElementSolver<AdjacencyTriangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl,
                         SimulationControl::kEquationModel>
      triangle_;
  AdjacencyElementSolver<AdjacencyQuadrangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl,
                         SimulationControl::kEquationModel>
      quadrangle_;
  ElementSolver<TetrahedronTrait<SimulationControl::kPolynomialOrder>, SimulationControl,
                SimulationControl::kEquationModel>
      tetrahedron_;
  ElementSolver<PyramidTrait<SimulationControl::kPolynomialOrder>, SimulationControl, SimulationControl::kEquationModel>
      pyramid_;
  ElementSolver<HexahedronTrait<SimulationControl::kPolynomialOrder>, SimulationControl,
                SimulationControl::kEquationModel>
      hexahedron_;
};

template <typename SimulationControl>
struct Solver : SolverData<SimulationControl, SimulationControl::kDimension> {
  template <typename ElementTrait>
  inline static ElementSolver<ElementTrait, SimulationControl, SimulationControl::kEquationModel> Solver::*
  getElement() {
    if constexpr (SimulationControl::kDimension == 1) {
      if constexpr (ElementTrait::kElementType == ElementEnum::Line) {
        return &Solver<SimulationControl>::line_;
      }
    } else if constexpr (SimulationControl::kDimension == 2) {
      if constexpr (ElementTrait::kElementType == ElementEnum::Triangle) {
        return &Solver<SimulationControl>::triangle_;
      }
      if constexpr (ElementTrait::kElementType == ElementEnum::Quadrangle) {
        return &Solver<SimulationControl>::quadrangle_;
      }
    } else if constexpr (SimulationControl::kDimension == 3) {
      if constexpr (ElementTrait::kElementType == ElementEnum::Tetrahedron) {
        return &Solver<SimulationControl>::tetrahedron_;
      }
      if constexpr (ElementTrait::kElementType == ElementEnum::Pyramid) {
        return &Solver<SimulationControl>::pyramid_;
      }
      if constexpr (ElementTrait::kElementType == ElementEnum::Hexahedron) {
        return &Solver<SimulationControl>::hexahedron_;
      }
    }
    return nullptr;
  }

  template <typename AdjacencyElementTrait>
  inline static AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl, SimulationControl::kEquationModel>
      Solver::*getAdjacencyElement() {
    if constexpr (SimulationControl::kDimension == 1) {
      if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
        return &Solver<SimulationControl>::point_;
      }
    } else if constexpr (SimulationControl::kDimension == 2) {
      if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
        return &Solver<SimulationControl>::line_;
      }
    } else if constexpr (SimulationControl::kDimension == 3) {
      if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
        return &Solver<SimulationControl>::triangle_;
      }
      if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
        return &Solver<SimulationControl>::quadrangle_;
      }
    }
    return nullptr;
  }

  inline void initializeSolver(
      const Mesh<SimulationControl>& mesh, const ThermalModel<SimulationControl>& thermal_model,
      const std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition,
      InitialCondition<SimulationControl>& initial_condition);

  inline void copyBasisFunctionCoefficient();

  inline void calculateDeltaTime(const Mesh<SimulationControl>& mesh,
                                 const ThermalModel<SimulationControl>& thermal_model,
                                 TimeIntegration<SimulationControl>& time_integration);

  inline void calculateArtificialViscosity(const Mesh<SimulationControl>& mesh);

  inline void calculateQuadrature(const Mesh<SimulationControl>& mesh,
                                  [[maybe_unused]] const SourceTerm<SimulationControl>& source_term,
                                  const ThermalModel<SimulationControl>& thermal_model);

  inline void calculateGardientQuadrature(const Mesh<SimulationControl>& mesh);

  inline void calculateAdjacencyQuadrature(
      const Mesh<SimulationControl>& mesh, const ThermalModel<SimulationControl>& thermal_model,
      const std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition);

  inline void calculateAdjacencyGardientQuadrature(
      const Mesh<SimulationControl>& mesh, const ThermalModel<SimulationControl>& thermal_model,
      const std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition);

  inline void calculateBoundaryAdjacencyForce(const Mesh<SimulationControl>& mesh,
                                              const ThermalModel<SimulationControl>& thermal_model);

  inline void calculateResidual(const Mesh<SimulationControl>& mesh);

  inline void calculateGardientResidual(const Mesh<SimulationControl>& mesh);

  inline void updateBasisFunctionCoefficient(int step, const Mesh<SimulationControl>& mesh,
                                             const TimeIntegration<SimulationControl>& time_integration);

  inline void updateGardientBasisFunctionCoefficient(const Mesh<SimulationControl>& mesh);

  inline void stepSolver(
      const Mesh<SimulationControl>& mesh, [[maybe_unused]] const SourceTerm<SimulationControl>& source_term,
      const ThermalModel<SimulationControl>& thermal_model,
      const std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition,
      const TimeIntegration<SimulationControl>& time_integration);

  inline void calculateRelativeError(const Mesh<SimulationControl>& mesh);

  inline void writeRawBinary(const Mesh<SimulationControl>& mesh, const std::filesystem::path& raw_binary_path);
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_SOLVE_CONTROL_HPP_
