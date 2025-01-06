/**
 * @file SolveControl.cpp
 * @brief The header file of SubrosaDG solve control.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-09
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2025 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_SOLVE_CONTROL_CPP_
#define SUBROSA_DG_SOLVE_CONTROL_CPP_

#include <Eigen/Core>
#include <filesystem>
#include <fstream>
#include <future>
#include <memory>
#include <sstream>
#include <unordered_map>

#include "Mesh/ReadControl.cpp"
#include "Solver/PhysicalModel.cpp"
#include "Solver/SimulationControl.cpp"
#include "Utils/BasicDataType.cpp"
#include "Utils/Enum.cpp"

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

template <typename ElementTrait, typename SimulationControl>
struct PerElementBaseSolver {
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kBasisFunctionNumber>
      variable_basis_function_coefficient_last_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kBasisFunctionNumber>
      variable_basis_function_coefficient_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber,
                ElementTrait::kQuadratureNumber * SimulationControl::kDimension>
      variable_quadrature_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kAllAdjacencyQuadratureNumber>
      variable_adjacency_quadrature_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kBasisFunctionNumber>
      variable_residual_;
};

template <typename ElementTrait, typename SimulationControl>
struct PerElementVolumeGradientSolver {
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                ElementTrait::kBasisFunctionNumber>
      variable_volume_gradient_basis_function_coefficient_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                ElementTrait::kQuadratureNumber * SimulationControl::kDimension>
      variable_volume_gradient_quadrature_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                ElementTrait::kAllAdjacencyQuadratureNumber>
      variable_volume_gradient_adjacency_quadrature_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                ElementTrait::kBasisFunctionNumber>
      variable_volume_gradient_residual_;
};

template <typename ElementTrait, typename SimulationControl, ViscousFluxEnum ViscousFluxType>
struct PerElementInterfaceGradientSolver;

template <typename ElementTrait, typename SimulationControl>
struct PerElementInterfaceGradientSolver<ElementTrait, SimulationControl, ViscousFluxEnum::BR1> {
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                ElementTrait::kBasisFunctionNumber>
      variable_interface_gradient_basis_function_coefficient_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                ElementTrait::kAllAdjacencyQuadratureNumber>
      variable_interface_gradient_adjacency_quadrature_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                ElementTrait::kBasisFunctionNumber>
      variable_interface_gradient_residual_;
};

template <typename ElementTrait, typename SimulationControl>
struct PerElementInterfaceGradientSolver<ElementTrait, SimulationControl, ViscousFluxEnum::BR2> {
  Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                             ElementTrait::kBasisFunctionNumber>,
               ElementTrait::kAdjacencyNumber, 1>
      variable_interface_gradient_basis_function_coefficient_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                ElementTrait::kAllAdjacencyQuadratureNumber>
      variable_interface_gradient_adjacency_quadrature_;
  Eigen::Array<Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                             ElementTrait::kBasisFunctionNumber>,
               ElementTrait::kAdjacencyNumber, 1>
      variable_interface_gradient_residual_;
};

template <typename ElementTrait, typename SimulationControl, SourceTermEnum SourceTermType>
struct PerElementSourceSolver;

template <typename ElementTrait, typename SimulationControl>
struct PerElementSourceSolver<ElementTrait, SimulationControl, SourceTermEnum::None> {};

template <typename ElementTrait, typename SimulationControl>
struct PerElementSourceSolver<ElementTrait, SimulationControl, SourceTermEnum::Boussinesq> {
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kQuadratureNumber>
      variable_source_quadrature_;
};

template <typename ElementTrait, typename SimulationControl, ShockCapturingEnum ShockCapturingType>
struct PerElementShockCapturingSolver;

template <typename ElementTrait, typename SimulationControl>
struct PerElementShockCapturingSolver<ElementTrait, SimulationControl, ShockCapturingEnum::None> {};

template <typename ElementTrait, typename SimulationControl>
struct PerElementShockCapturingSolver<ElementTrait, SimulationControl, ShockCapturingEnum::ArtificialViscosity> {
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber,
                ElementTrait::kQuadratureNumber * SimulationControl::kDimension>
      variable_artificial_viscosity_quadrature_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, ElementTrait::kAllAdjacencyQuadratureNumber>
      variable_artificial_viscosity_adjacency_quadrature_;
  Eigen::Vector<Real, ElementTrait::kBasicNodeNumber> variable_artificial_viscosity_;
};

template <typename ElementTrait, typename SimulationControl, EquationModelEnum EquationModelType>
struct PerElementSolver;

template <typename ElementTrait, typename SimulationControl>
struct PerElementSolver<ElementTrait, SimulationControl, EquationModelEnum::CompresibleEuler>
    : PerElementBaseSolver<ElementTrait, SimulationControl>,
      PerElementVolumeGradientSolver<ElementTrait, SimulationControl>,
      PerElementSourceSolver<ElementTrait, SimulationControl, SimulationControl::kSourceTerm>,
      PerElementShockCapturingSolver<ElementTrait, SimulationControl, SimulationControl::kShockCapturing> {};

template <typename ElementTrait, typename SimulationControl>
struct PerElementSolver<ElementTrait, SimulationControl, EquationModelEnum::CompresibleNS>
    : PerElementBaseSolver<ElementTrait, SimulationControl>,
      PerElementVolumeGradientSolver<ElementTrait, SimulationControl>,
      PerElementInterfaceGradientSolver<ElementTrait, SimulationControl, SimulationControl::kViscousFlux>,
      PerElementSourceSolver<ElementTrait, SimulationControl, SimulationControl::kSourceTerm>,
      PerElementShockCapturingSolver<ElementTrait, SimulationControl, SimulationControl::kShockCapturing> {
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                ElementTrait::kBasisFunctionNumber>
      variable_gradient_basis_function_coefficient_;
};

template <typename ElementTrait, typename SimulationControl>
struct PerElementSolver<ElementTrait, SimulationControl, EquationModelEnum::IncompresibleEuler>
    : PerElementBaseSolver<ElementTrait, SimulationControl>,
      PerElementVolumeGradientSolver<ElementTrait, SimulationControl>,
      PerElementSourceSolver<ElementTrait, SimulationControl, SimulationControl::kSourceTerm>,
      PerElementShockCapturingSolver<ElementTrait, SimulationControl, SimulationControl::kShockCapturing> {};

template <typename ElementTrait, typename SimulationControl>
struct PerElementSolver<ElementTrait, SimulationControl, EquationModelEnum::IncompresibleNS>
    : PerElementBaseSolver<ElementTrait, SimulationControl>,
      PerElementVolumeGradientSolver<ElementTrait, SimulationControl>,
      PerElementInterfaceGradientSolver<ElementTrait, SimulationControl, SimulationControl::kViscousFlux>,
      PerElementSourceSolver<ElementTrait, SimulationControl, SimulationControl::kSourceTerm>,
      PerElementShockCapturingSolver<ElementTrait, SimulationControl, SimulationControl::kShockCapturing> {
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                ElementTrait::kBasisFunctionNumber>
      variable_gradient_basis_function_coefficient_;
};

template <typename ElementTrait, typename SimulationControl>
struct ElementSolver {
  Isize number_{0};
  Eigen::Array<PerElementSolver<ElementTrait, SimulationControl, SimulationControl::kEquationModel>, Eigen::Dynamic, 1>
      element_;

  inline void initializeElementSolver(const ElementMesh<ElementTrait>& element_mesh,
                                      const PhysicalModel<SimulationControl>& physical_model,
                                      InitialCondition<SimulationControl>& initial_condition);

  inline void copyElementBasisFunctionCoefficient();

  inline void calculateElementArtificialViscosity(const ElementMesh<ElementTrait>& element_mesh,
                                                  Real empirical_tolerance, Real artificial_viscosity_factor);

  inline void maxElementArtificialViscosity(const ElementMesh<ElementTrait>& element_mesh,
                                            Eigen::Vector<Real, Eigen::Dynamic>& node_artificial_viscosity);

  inline void storeElementArtificialViscosity(const ElementMesh<ElementTrait>& element_mesh,
                                              const Eigen::Vector<Real, Eigen::Dynamic>& node_artificial_viscosity);

  inline void calculateElementQuadrature(const ElementMesh<ElementTrait>& element_mesh,
                                         [[maybe_unused]] const SourceTerm<SimulationControl>& source_term,
                                         const PhysicalModel<SimulationControl>& physical_model);

  inline void calculateElementGardientQuadrature(const ElementMesh<ElementTrait>& element_mesh);

  inline Real calculateElementDeltaTime(const ElementMesh<ElementTrait>& element_mesh,
                                        const PhysicalModel<SimulationControl>& physical_model,
                                        Real courant_friedrichs_lewy_number);

  inline void calculateElementResidual(const ElementMesh<ElementTrait>& element_mesh);

  inline void calculateElementGardientResidual(const ElementMesh<ElementTrait>& element_mesh);

  inline void updateElementBasisFunctionCoefficient(int rk_step, const ElementMesh<ElementTrait>& element_mesh,
                                                    const TimeIntegration<SimulationControl>& time_integration);

  inline void updateElementGardientBasisFunctionCoefficient(const ElementMesh<ElementTrait>& element_mesh);

  inline void calculateElementRelativeError(
      const ElementMesh<ElementTrait>& element_mesh,
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& relative_error);

  inline void writeElementRawBinary(std::stringstream& raw_binary_ss) const;
};

template <typename AdjacencyElementTrait, typename SimulationControl>
struct AdjacencyElementSolver {
  Isize interior_number_{0};
  Isize boundary_number_{0};
  Eigen::Array<AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl>, Eigen::Dynamic, 1>
      boundary_dummy_variable_;

  inline void initializeAdjacencyElementSolver(
      const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
      const PhysicalModel<SimulationControl>& physical_model,
      const std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition);

  inline void updateAdjacencyElementBoundaryVariable(
      const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
      const PhysicalModel<SimulationControl>& physical_model,
      const std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition,
      const TimeIntegration<SimulationControl>& time_integration);

  [[nodiscard]] inline Isize getAdjacencyParentElementAccumulateAdjacencyQuadratureNumber(
      [[maybe_unused]] Isize parent_gmsh_type_number, Isize adjacency_sequence_in_parent);

  inline void calculateAdjacencyElementArtificialViscosity(
      const Mesh<SimulationControl>& mesh, const Solver<SimulationControl>& solver,
      Eigen::Vector<Real, AdjacencyElementTrait::kQuadratureNumber>& quadrature_node_artificial_viscosity,
      Isize parent_gmsh_type_number, Isize parent_index_each_type, Isize adjacency_sequence_in_parent);

  inline void calculateInteriorAdjacencyElementQuadrature(const Mesh<SimulationControl>& mesh,
                                                          const PhysicalModel<SimulationControl>& physical_model,
                                                          Solver<SimulationControl>& solver);

  inline void calculateBoundaryAdjacencyElementQuadrature(
      const Mesh<SimulationControl>& mesh, const PhysicalModel<SimulationControl>& physical_model,
      const std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition,
      Solver<SimulationControl>& solver);

  inline void calculateInteriorAdjacencyElementGardientQuadrature(const Mesh<SimulationControl>& mesh,
                                                                  Solver<SimulationControl>& solver);

  inline void calculateBoundaryAdjacencyElementGardientQuadrature(
      const Mesh<SimulationControl>& mesh, const PhysicalModel<SimulationControl>& physical_model,
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

  template <typename ElementTrait>
  inline void writeBoundaryAdjacencyPerElementRawBinary(
      const ElementSolver<ElementTrait, SimulationControl>& element_solver, std::stringstream& raw_binary_ss,
      Isize parent_index_each_type, [[maybe_unused]] Isize adjacency_sequence_in_parent) const;

  inline void writeBoundaryAdjacencyElementRawBinary(
      const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh,
      const Solver<SimulationControl>& solver, std::stringstream& raw_binary_ss) const;
};

template <typename SimulationControl>
struct SolverBase {
  Real empirical_tolerance_{0.0_r};
  Real artificial_viscosity_factor_{1.0_r};

  std::stringstream raw_binary_ss_;
  std::fstream error_finout_;
  std::future<void> write_raw_binary_future_;

  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> relative_error_{
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>::Zero()};
  Eigen::Vector<Real, Eigen::Dynamic> node_artificial_viscosity_;
};

template <typename SimulationControl>
struct SolverData<SimulationControl, 1> : SolverBase<SimulationControl> {
  AdjacencyElementSolver<AdjacencyPointTrait<SimulationControl::kPolynomialOrder>, SimulationControl> point_;
  ElementSolver<LineTrait<SimulationControl::kPolynomialOrder>, SimulationControl> line_;
};

template <typename SimulationControl>
struct SolverData<SimulationControl, 2> : SolverBase<SimulationControl> {
  AdjacencyElementSolver<AdjacencyLineTrait<SimulationControl::kPolynomialOrder>, SimulationControl> line_;
  ElementSolver<TriangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl> triangle_;
  ElementSolver<QuadrangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl> quadrangle_;
};

template <typename SimulationControl>
struct SolverData<SimulationControl, 3> : SolverBase<SimulationControl> {
  AdjacencyElementSolver<AdjacencyTriangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl> triangle_;
  AdjacencyElementSolver<AdjacencyQuadrangleTrait<SimulationControl::kPolynomialOrder>, SimulationControl> quadrangle_;
  ElementSolver<TetrahedronTrait<SimulationControl::kPolynomialOrder>, SimulationControl> tetrahedron_;
  ElementSolver<PyramidTrait<SimulationControl::kPolynomialOrder>, SimulationControl> pyramid_;
  ElementSolver<HexahedronTrait<SimulationControl::kPolynomialOrder>, SimulationControl> hexahedron_;
};

template <typename SimulationControl>
struct Solver : SolverData<SimulationControl, SimulationControl::kDimension> {
  template <typename ElementTrait>
  inline static ElementSolver<ElementTrait, SimulationControl> Solver::* getElement() {
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
  inline static AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl> Solver::* getAdjacencyElement() {
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
      const Mesh<SimulationControl>& mesh, const PhysicalModel<SimulationControl>& physical_model,
      const std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition,
      InitialCondition<SimulationControl>& initial_condition);

  inline void updateBoundaryVariable(
      const Mesh<SimulationControl>& mesh, const PhysicalModel<SimulationControl>& physical_model,
      const std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition,
      const TimeIntegration<SimulationControl>& time_integration);

  inline void copyBasisFunctionCoefficient();

  inline void calculateDeltaTime(const Mesh<SimulationControl>& mesh,
                                 const PhysicalModel<SimulationControl>& physical_model,
                                 TimeIntegration<SimulationControl>& time_integration);

  inline void calculateVariable(const Mesh<SimulationControl>& mesh,
                                const PhysicalModel<SimulationControl>& physical_model);

  inline void calculateVariableGardient(const Mesh<SimulationControl>& mesh,
                                        const PhysicalModel<SimulationControl>& physical_model);

  inline void calculateArtificialViscosity(const Mesh<SimulationControl>& mesh);

  inline void calculateQuadrature(const Mesh<SimulationControl>& mesh,
                                  [[maybe_unused]] const SourceTerm<SimulationControl>& source_term,
                                  const PhysicalModel<SimulationControl>& physical_model);

  inline void calculateGardientQuadrature(const Mesh<SimulationControl>& mesh);

  inline void calculateAdjacencyQuadrature(
      const Mesh<SimulationControl>& mesh, const PhysicalModel<SimulationControl>& physical_model,
      const std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition);

  inline void calculateAdjacencyGardientQuadrature(
      const Mesh<SimulationControl>& mesh, const PhysicalModel<SimulationControl>& physical_model,
      const std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition);

  inline void calculateBoundaryAdjacencyForce(const Mesh<SimulationControl>& mesh,
                                              const PhysicalModel<SimulationControl>& physical_model);

  inline void calculateResidual(const Mesh<SimulationControl>& mesh);

  inline void calculateGardientResidual(const Mesh<SimulationControl>& mesh);

  inline void updateBasisFunctionCoefficient(int rk_step, const Mesh<SimulationControl>& mesh,
                                             const TimeIntegration<SimulationControl>& time_integration);

  inline void updateGardientBasisFunctionCoefficient(const Mesh<SimulationControl>& mesh);

  inline void stepSolver(
      const Mesh<SimulationControl>& mesh, [[maybe_unused]] const SourceTerm<SimulationControl>& source_term,
      const PhysicalModel<SimulationControl>& physical_model,
      const std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition,
      const TimeIntegration<SimulationControl>& time_integration);

  inline void calculateRelativeError(const Mesh<SimulationControl>& mesh);

  inline void writeRawBinary(const Mesh<SimulationControl>& mesh, const std::filesystem::path& raw_binary_path);
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_SOLVE_CONTROL_CPP_
