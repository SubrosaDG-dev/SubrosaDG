/**
 * @file VariableConvertor.cpp
 * @brief The head file of SubrosaDG variable convertor.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-06
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2025 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_VARIABLE_CONVERTOR_CPP_
#define SUBROSA_DG_VARIABLE_CONVERTOR_CPP_

#include <Eigen/Core>
#include <array>

#include "Mesh/ReadControl.cpp"
#include "Solver/PhysicalModel.cpp"
#include "Solver/SimulationControl.cpp"
#include "Solver/SolveControl.cpp"
#include "Utils/BasicDataType.cpp"
#include "Utils/Enum.cpp"

namespace SubrosaDG {

template <typename SimulationControl, ConservedVariableEnum ConservedVariableType>
inline consteval int getConservedVariableIndex() {
  if constexpr (IsCompresible<SimulationControl::kEquationModel>) {
    if constexpr (ConservedVariableType == ConservedVariableEnum::Density) {
      return 0;
    }
    if constexpr (ConservedVariableType == ConservedVariableEnum::MomentumX ||
                  ConservedVariableType == ConservedVariableEnum::Momentum) {
      return 1;
    }
    if constexpr (ConservedVariableType == ConservedVariableEnum::MomentumY) {
      return 2;
    }
    if constexpr (ConservedVariableType == ConservedVariableEnum::MomentumZ) {
      return 3;
    }
    if constexpr (ConservedVariableType == ConservedVariableEnum::DensityTotalEnergy) {
      return SimulationControl::kDimension + 1;
    }
    return -1;
  }
  if constexpr (IsIncompresible<SimulationControl::kEquationModel>) {
    if constexpr (ConservedVariableType == ConservedVariableEnum::Density) {
      return 0;
    }
    if constexpr (ConservedVariableType == ConservedVariableEnum::MomentumX ||
                  ConservedVariableType == ConservedVariableEnum::Momentum) {
      return 1;
    }
    if constexpr (ConservedVariableType == ConservedVariableEnum::MomentumY) {
      return 2;
    }
    if constexpr (ConservedVariableType == ConservedVariableEnum::MomentumZ) {
      return 3;
    }
    if constexpr (ConservedVariableType == ConservedVariableEnum::DensityInternalEnergy) {
      return SimulationControl::kDimension + 1;
    }
    return -1;
  }
}

template <typename SimulationControl, ComputationalVariableEnum ComputationalVariableType>
inline consteval int getComputationalVariableIndex() {
  if constexpr (IsCompresible<SimulationControl::kEquationModel>) {
    if constexpr (ComputationalVariableType == ComputationalVariableEnum::Density) {
      return 0;
    }
    if constexpr (ComputationalVariableType == ComputationalVariableEnum::VelocityX ||
                  ComputationalVariableType == ComputationalVariableEnum::Velocity) {
      return 1;
    }
    if constexpr (ComputationalVariableType == ComputationalVariableEnum::VelocityY) {
      return 2;
    }
    if constexpr (ComputationalVariableType == ComputationalVariableEnum::VelocityZ) {
      return 3;
    }
    if constexpr (ComputationalVariableType == ComputationalVariableEnum::InternalEnergy) {
      return SimulationControl::kDimension + 1;
    }
    if constexpr (ComputationalVariableType == ComputationalVariableEnum::Pressure) {
      return SimulationControl::kDimension + 2;
    }
    return -1;
  }
  if constexpr (IsIncompresible<SimulationControl::kEquationModel>) {
    if constexpr (ComputationalVariableType == ComputationalVariableEnum::Density) {
      return 0;
    }
    if constexpr (ComputationalVariableType == ComputationalVariableEnum::VelocityX ||
                  ComputationalVariableType == ComputationalVariableEnum::Velocity) {
      return 1;
    }
    if constexpr (ComputationalVariableType == ComputationalVariableEnum::VelocityY) {
      return 2;
    }
    if constexpr (ComputationalVariableType == ComputationalVariableEnum::VelocityZ) {
      return 3;
    }
    if constexpr (ComputationalVariableType == ComputationalVariableEnum::InternalEnergy) {
      return SimulationControl::kDimension + 1;
    }
    if constexpr (ComputationalVariableType == ComputationalVariableEnum::Pressure) {
      return SimulationControl::kDimension + 2;
    }
    return -1;
  }
}

template <typename SimulationControl, PrimitiveVariableEnum PrimitiveVariableType>
inline consteval int getPrimitiveVariableIndex() {
  if constexpr (IsEuler<SimulationControl::kEquationModel> || IsNS<SimulationControl::kEquationModel>) {
    if constexpr (PrimitiveVariableType == PrimitiveVariableEnum::Density) {
      return 0;
    }
    if constexpr (PrimitiveVariableType == PrimitiveVariableEnum::VelocityX ||
                  PrimitiveVariableType == PrimitiveVariableEnum::Velocity) {
      return 1;
    }
    if constexpr (PrimitiveVariableType == PrimitiveVariableEnum::VelocityY) {
      return 2;
    }
    if constexpr (PrimitiveVariableType == PrimitiveVariableEnum::VelocityZ) {
      return 3;
    }
    if constexpr (PrimitiveVariableType == PrimitiveVariableEnum::Temperature) {
      return SimulationControl::kDimension + 1;
    }
    return -1;
  }
}

template <VariableGradientEnum VariableGradientType>
inline consteval int getVariableGradientIndex() {
  if constexpr (VariableGradientType == VariableGradientEnum::X) {
    return 0;
  }
  if constexpr (VariableGradientType == VariableGradientEnum::Y) {
    return 1;
  }
  if constexpr (VariableGradientType == VariableGradientEnum::Z) {
    return 2;
  }
  return -1;
}

template <typename SimulationControl>
struct FluxNormalVariable {
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> normal_variable_;

  template <ConservedVariableEnum ConservedVariableType>
  inline void setScalar(const Real value) {
    this->normal_variable_(getConservedVariableIndex<SimulationControl, ConservedVariableType>()) = value;
  }

  template <ConservedVariableEnum ConservedVariableType>
  inline void setVector(const Eigen::Vector<Real, SimulationControl::kDimension>& value) {
    this->normal_variable_(
        Eigen::seqN(Eigen::fix<getConservedVariableIndex<SimulationControl, ConservedVariableType>()>,
                    Eigen::fix<SimulationControl::kDimension>)) = value;
  }
};

template <typename SimulationControl>
struct FluxVariable {
  Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kConservedVariableNumber> variable_;

  template <ConservedVariableEnum ConservedVariableType>
  inline void setVector(const Eigen::Vector<Real, SimulationControl::kDimension>& value) {
    this->variable_.col(getConservedVariableIndex<SimulationControl, ConservedVariableType>()) = value;
  }

  template <ConservedVariableEnum ConservedVariableType>
  inline void setMatrix(
      const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>& value) {
    this->variable_(Eigen::placeholders::all,
                    Eigen::seqN(Eigen::fix<getConservedVariableIndex<SimulationControl, ConservedVariableType>()>,
                                Eigen::fix<SimulationControl::kDimension>)) = value;
  }
};

template <typename SimulationControl>
struct Flux {
  FluxNormalVariable<SimulationControl> left_;
  FluxNormalVariable<SimulationControl> right_;
  FluxNormalVariable<SimulationControl> result_;
};

template <typename SimulationControl>
struct FluxGradient {
  FluxVariable<SimulationControl> left_;
  FluxVariable<SimulationControl> right_;
  FluxVariable<SimulationControl> result_;
};

template <typename SimulationControl, int N>
struct Variable {
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, N> conserved_;
  Eigen::Matrix<Real, SimulationControl::kComputationalVariableNumber, N> computational_;
  Eigen::Matrix<Real, SimulationControl::kPrimitiveVariableNumber, N> primitive_;

  template <ConservedVariableEnum ConservedVariableType>
  [[nodiscard]] inline Real getScalar(const Isize column) const {
    return this->conserved_(getConservedVariableIndex<SimulationControl, ConservedVariableType>(), column);
  }

  template <ConservedVariableEnum ConservedVariableType>
  [[nodiscard]] inline Eigen::Vector<Real, SimulationControl::kDimension> getVector(const Isize column) const {
    return this->conserved_(
        Eigen::seqN(Eigen::fix<getConservedVariableIndex<SimulationControl, ConservedVariableType>()>,
                    Eigen::fix<SimulationControl::kDimension>),
        column);
  }

  template <ComputationalVariableEnum ComputationalVariableType>
  [[nodiscard]] inline Real getScalar(const Isize column) const {
    return this->computational_(getComputationalVariableIndex<SimulationControl, ComputationalVariableType>(), column);
  }

  template <>
  [[nodiscard]] inline Real getScalar<ComputationalVariableEnum::VelocitySquaredNorm>(const Isize column) const {
    return this->getVector<ComputationalVariableEnum::Velocity>(column).squaredNorm();
  }

  template <ComputationalVariableEnum ComputationalVariableType>
  [[nodiscard]] inline Eigen::Vector<Real, SimulationControl::kDimension> getVector(const Isize column) const {
    return this->computational_(
        Eigen::seqN(Eigen::fix<getComputationalVariableIndex<SimulationControl, ComputationalVariableType>()>,
                    Eigen::fix<SimulationControl::kDimension>),
        column);
  }

  template <PrimitiveVariableEnum PrimitiveVariableType>
  [[nodiscard]] inline Real getScalar(const Isize column) const {
    return this->primitive_(getPrimitiveVariableIndex<SimulationControl, PrimitiveVariableType>(), column);
  }

  template <PrimitiveVariableEnum PrimitiveVariableType>
  [[nodiscard]] inline Eigen::Vector<Real, SimulationControl::kDimension> getVector(const Isize column) const {
    return this->primitive_(
        Eigen::seqN(Eigen::fix<getPrimitiveVariableIndex<SimulationControl, PrimitiveVariableType>()>,
                    Eigen::fix<SimulationControl::kDimension>),
        column);
  }

  template <ConservedVariableEnum ConservedVariableType>
  inline void setScalar(const Real value, const Isize column) {
    this->conserved_(getConservedVariableIndex<SimulationControl, ConservedVariableType>(), column) = value;
  }

  template <ConservedVariableEnum ConservedVariableType>
  inline void setVector(const Eigen::Vector<Real, SimulationControl::kDimension>& value, const Isize column) {
    this->conserved_(Eigen::seqN(Eigen::fix<getConservedVariableIndex<SimulationControl, ConservedVariableType>()>,
                                 Eigen::fix<SimulationControl::kDimension>),
                     column) = value;
  }

  template <ComputationalVariableEnum ComputationalVariableType>
  inline void setScalar(const Real value, const Isize column) {
    this->computational_(getComputationalVariableIndex<SimulationControl, ComputationalVariableType>(), column) = value;
  }

  template <ComputationalVariableEnum ComputationalVariableType>
  inline void setVector(const Eigen::Vector<Real, SimulationControl::kDimension>& value, const Isize column) {
    this->computational_(
        Eigen::seqN(Eigen::fix<getComputationalVariableIndex<SimulationControl, ComputationalVariableType>()>,
                    Eigen::fix<SimulationControl::kDimension>),
        column) = value;
  }

  template <PrimitiveVariableEnum PrimitiveVariableType>
  inline void setScalar(const Real value, const Isize column) {
    this->primitive_(getPrimitiveVariableIndex<SimulationControl, PrimitiveVariableType>(), column) = value;
  }

  template <PrimitiveVariableEnum PrimitiveVariableType>
  inline void setVector(const Eigen::Vector<Real, SimulationControl::kDimension>& value, const Isize column) {
    this->primitive_(Eigen::seqN(Eigen::fix<getPrimitiveVariableIndex<SimulationControl, PrimitiveVariableType>()>,
                                 Eigen::fix<SimulationControl::kDimension>),
                     column) = value;
  }

  inline void calculateConservedFromComputational() {
    if constexpr (IsCompresible<SimulationControl::kEquationModel>) {
      for (Isize i = 0; i < N; i++) {
        const Real density = this->getScalar<ComputationalVariableEnum::Density>(i);
        this->setScalar<ConservedVariableEnum::Density>(density, i);
        this->setVector<ConservedVariableEnum::Momentum>(
            density * this->getVector<ComputationalVariableEnum::Velocity>(i), i);
        const Real total_energy = this->getScalar<ComputationalVariableEnum::InternalEnergy>(i) +
                                  this->getScalar<ComputationalVariableEnum::VelocitySquaredNorm>(i) / 2.0_r;
        this->setScalar<ConservedVariableEnum::DensityTotalEnergy>(density * total_energy, i);
      }
    }
    if constexpr (IsIncompresible<SimulationControl::kEquationModel>) {
      for (Isize i = 0; i < N; i++) {
        const Real density = this->getScalar<ComputationalVariableEnum::Density>(i);
        this->setScalar<ConservedVariableEnum::Density>(density, i);
        this->setVector<ConservedVariableEnum::Momentum>(
            density * this->getVector<ComputationalVariableEnum::Velocity>(i), i);
        this->setScalar<ConservedVariableEnum::DensityInternalEnergy>(
            density * this->getScalar<ComputationalVariableEnum::InternalEnergy>(i), i);
      }
    }
  }

  inline void calculateComputationalFromConserved(const PhysicalModel<SimulationControl>& physical_model) {
    for (Isize i = 0; i < N; i++) {
      if constexpr (IsCompresible<SimulationControl::kEquationModel>) {
        const Real density = this->getScalar<ConservedVariableEnum::Density>(i);
        this->setScalar<ComputationalVariableEnum::Density>(density, i);
        this->setVector<ComputationalVariableEnum::Velocity>(
            this->getVector<ConservedVariableEnum::Momentum>(i) / density, i);
        const Real internal_energy = this->getScalar<ConservedVariableEnum::DensityTotalEnergy>(i) / density -
                                     this->getScalar<ComputationalVariableEnum::VelocitySquaredNorm>(i) / 2.0_r;
        this->setScalar<ComputationalVariableEnum::InternalEnergy>(internal_energy, i);
        this->setScalar<ComputationalVariableEnum::Pressure>(
            physical_model.calculatePressureFormDensityInternalEnergy(density, internal_energy), i);
      }
      if constexpr (IsIncompresible<SimulationControl::kEquationModel>) {
        const Real density = this->getScalar<ConservedVariableEnum::Density>(i);
        this->setScalar<ComputationalVariableEnum::Density>(density, i);
        this->setVector<ComputationalVariableEnum::Velocity>(
            this->getVector<ConservedVariableEnum::Momentum>(i) / density, i);
        const Real internal_energy = this->getScalar<ConservedVariableEnum::DensityInternalEnergy>(i) / density;
        this->setScalar<ComputationalVariableEnum::InternalEnergy>(internal_energy, i);
        this->setScalar<ComputationalVariableEnum::Pressure>(
            physical_model.calculatePressureFormDensityInternalEnergy(density, internal_energy), i);
      }
    }
  }

  inline void calculateConservedFromPrimitive(const PhysicalModel<SimulationControl>& physical_model) {
    for (Isize i = 0; i < N; i++) {
      if constexpr (IsCompresible<SimulationControl::kEquationModel>) {
        const Real density = this->getScalar<PrimitiveVariableEnum::Density>(i);
        this->setScalar<ConservedVariableEnum::Density>(density, i);
        this->setVector<ConservedVariableEnum::Momentum>(density * this->getVector<PrimitiveVariableEnum::Velocity>(i),
                                                         i);
        this->setVector<ComputationalVariableEnum::Velocity>(this->getVector<PrimitiveVariableEnum::Velocity>(i), i);
        const Real total_energy = physical_model.calculateInternalEnergyFromTemperature(
                                      this->getScalar<PrimitiveVariableEnum::Temperature>(i)) +
                                  this->getScalar<ComputationalVariableEnum::VelocitySquaredNorm>(i) / 2.0_r;
        this->setScalar<ConservedVariableEnum::DensityTotalEnergy>(density * total_energy, i);
      }
      if constexpr (IsIncompresible<SimulationControl::kEquationModel>) {
        const Real density = this->getScalar<PrimitiveVariableEnum::Density>(i);
        this->setScalar<ConservedVariableEnum::Density>(density, i);
        this->setVector<ConservedVariableEnum::Momentum>(density * this->getVector<PrimitiveVariableEnum::Velocity>(i),
                                                         i);
        this->setVector<ComputationalVariableEnum::Velocity>(this->getVector<PrimitiveVariableEnum::Velocity>(i), i);
        this->setScalar<ConservedVariableEnum::DensityInternalEnergy>(
            density * physical_model.calculateInternalEnergyFromTemperature(
                          this->getScalar<PrimitiveVariableEnum::Temperature>(i)),
            i);
      }
    }
  }

  inline void calculateComputationalFromPrimitive(const PhysicalModel<SimulationControl>& physical_model) {
    for (Isize i = 0; i < N; i++) {
      if constexpr (IsEuler<SimulationControl::kEquationModel> || IsNS<SimulationControl::kEquationModel>) {
        const Real density = this->getScalar<PrimitiveVariableEnum::Density>(i);
        this->setScalar<ComputationalVariableEnum::Density>(density, i);
        this->setVector<ComputationalVariableEnum::Velocity>(this->getVector<PrimitiveVariableEnum::Velocity>(i), i);
        const Real internal_energy = physical_model.calculateInternalEnergyFromTemperature(
            this->getScalar<PrimitiveVariableEnum::Temperature>(i));
        this->setScalar<ComputationalVariableEnum::InternalEnergy>(internal_energy, i);
        this->setScalar<ComputationalVariableEnum::Pressure>(
            physical_model.calculatePressureFormDensityInternalEnergy(density, internal_energy), i);
      }
    }
  }

  inline void calculatePrimitiveFromConserved(const PhysicalModel<SimulationControl>& physical_model) {
    for (Isize i = 0; i < N; i++) {
      if constexpr (IsCompresible<SimulationControl::kEquationModel>) {
        const Real density = this->getScalar<ConservedVariableEnum::Density>(i);
        this->setScalar<PrimitiveVariableEnum::Density>(density, i);
        const Eigen::Vector<Real, SimulationControl::kDimension> velocity =
            this->getVector<ConservedVariableEnum::Momentum>(i) / density;
        this->setVector<PrimitiveVariableEnum::Velocity>(velocity, i);
        const Real internal_energy =
            this->getScalar<ConservedVariableEnum::DensityTotalEnergy>(i) / density - velocity.squaredNorm() / 2.0_r;
        this->setScalar<PrimitiveVariableEnum::Temperature>(
            physical_model.calculateTemperatureFromInternalEnergy(internal_energy), i);
      }
      if constexpr (IsIncompresible<SimulationControl::kEquationModel>) {
        const Real density = this->getScalar<ConservedVariableEnum::Density>(i);
        this->setScalar<PrimitiveVariableEnum::Density>(density, i);
        const Eigen::Vector<Real, SimulationControl::kDimension> velocity =
            this->getVector<ConservedVariableEnum::Momentum>(i) / density;
        this->setVector<PrimitiveVariableEnum::Velocity>(velocity, i);
        const Real internal_energy = this->getScalar<ConservedVariableEnum::DensityInternalEnergy>(i) / density;
        this->setScalar<PrimitiveVariableEnum::Temperature>(
            physical_model.calculateTemperatureFromInternalEnergy(internal_energy), i);
      }
    }
  }

  inline void calculatePrimitiveFromComputational(const PhysicalModel<SimulationControl>& physical_model) {
    for (Isize i = 0; i < N; i++) {
      if constexpr (IsEuler<SimulationControl::kEquationModel> || IsNS<SimulationControl::kEquationModel>) {
        this->setScalar<PrimitiveVariableEnum::Density>(this->getScalar<ComputationalVariableEnum::Density>(i), i);
        this->setVector<PrimitiveVariableEnum::Velocity>(this->getVector<ComputationalVariableEnum::Velocity>(i), i);
        this->setScalar<PrimitiveVariableEnum::Temperature>(
            physical_model.calculateTemperatureFromInternalEnergy(
                this->getScalar<ComputationalVariableEnum::InternalEnergy>(i)),
            i);
      }
    }
  }
};

template <typename ElementTrait, typename SimulationControl>
struct ElementVariable : Variable<SimulationControl, ElementTrait::kQuadratureNumber> {
  inline void get(const ElementMesh<ElementTrait>& element_mesh,
                  const ElementSolver<ElementTrait, SimulationControl>& element_solver, const Isize element_index) {
    this->conserved_.noalias() = element_solver.element_(element_index).variable_basis_function_coefficient_ *
                                 element_mesh.basis_function_.modal_value_.transpose();
  }
};

template <typename AdjacencyElementTrait, typename SimulationControl>
struct AdjacencyElementVariable : Variable<SimulationControl, AdjacencyElementTrait::kQuadratureNumber> {
  template <typename ElementTrait>
  void compute(const ElementMesh<ElementTrait>& element_mesh,
               const ElementSolver<ElementTrait, SimulationControl>& element_solver, const Isize parent_index_each_type,
               const Isize adjacency_sequence_in_parent) {
    constexpr std::array<int, ElementTrait::kAdjacencyNumber + 1> kElementAccumulateAdjacencyQuadratureNumber{
        getElementAccumulateAdjacencyQuadratureNumber<ElementTrait::kElementType,
                                                      SimulationControl::kPolynomialOrder>()};
    this->conserved_.noalias() =
        element_solver.element_(parent_index_each_type).variable_basis_function_coefficient_ *
        element_mesh.basis_function_
            .modal_adjacency_value_(
                Eigen::seq(
                    kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent)],
                    kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent) + 1] -
                        1),
                Eigen::placeholders::all)
            .transpose();
  }

  inline void get(const Mesh<SimulationControl>& mesh, const Solver<SimulationControl>& solver,
                  const Isize parent_gmsh_type_number, const Isize parent_index_each_type,
                  const Isize adjacency_sequence_in_parent) {
    if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
      this->compute<LineTrait<SimulationControl::kPolynomialOrder>>(mesh.line_, solver.line_, parent_index_each_type,
                                                                    adjacency_sequence_in_parent);
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
      if (parent_gmsh_type_number == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->compute<TriangleTrait<SimulationControl::kPolynomialOrder>>(
            mesh.triangle_, solver.triangle_, parent_index_each_type, adjacency_sequence_in_parent);
      } else if (parent_gmsh_type_number == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->compute<QuadrangleTrait<SimulationControl::kPolynomialOrder>>(
            mesh.quadrangle_, solver.quadrangle_, parent_index_each_type, adjacency_sequence_in_parent);
      }
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
      if (parent_gmsh_type_number == TetrahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->compute<TetrahedronTrait<SimulationControl::kPolynomialOrder>>(
            mesh.tetrahedron_, solver.tetrahedron_, parent_index_each_type, adjacency_sequence_in_parent);
      } else if (parent_gmsh_type_number == PyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->compute<PyramidTrait<SimulationControl::kPolynomialOrder>>(
            mesh.pyramid_, solver.pyramid_, parent_index_each_type, adjacency_sequence_in_parent);
      }
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
      if (parent_gmsh_type_number == PyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->compute<PyramidTrait<SimulationControl::kPolynomialOrder>>(
            mesh.pyramid_, solver.pyramid_, parent_index_each_type, adjacency_sequence_in_parent);
      } else if (parent_gmsh_type_number == HexahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->compute<HexahedronTrait<SimulationControl::kPolynomialOrder>>(
            mesh.hexahedron_, solver.hexahedron_, parent_index_each_type, adjacency_sequence_in_parent);
      }
    }
  }
};

template <typename SimulationControl, int N>
struct VariableGradient {
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension, N> conserved_;
  Eigen::Matrix<Real, SimulationControl::kPrimitiveVariableNumber * SimulationControl::kDimension, N> primitive_;

  template <ConservedVariableEnum ConservedVariableType>
  [[nodiscard]] inline Eigen::Vector<Real, SimulationControl::kDimension> getVector(const Isize column) const {
    return this->conserved_(
        Eigen::seqN(Eigen::fix<getConservedVariableIndex<SimulationControl, ConservedVariableType>() *
                               SimulationControl::kDimension>,
                    Eigen::fix<SimulationControl::kDimension>),
        column);
  }

  template <ConservedVariableEnum ConservedVariableType>
  [[nodiscard]] inline Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension> getMatrix(
      const Isize column) const {
    return this
        ->conserved_(Eigen::seqN(Eigen::fix<getConservedVariableIndex<SimulationControl, ConservedVariableType>() *
                                            SimulationControl::kDimension>,
                                 Eigen::fix<SimulationControl::kDimension * SimulationControl::kDimension>),
                     column)
        .reshaped(SimulationControl::kDimension, SimulationControl::kDimension);
  }

  template <PrimitiveVariableEnum PrimitiveVariableType, VariableGradientEnum VariableGradientType>
  [[nodiscard]] inline Real getScalar(const Isize column) const {
    return this->primitive_(
        getPrimitiveVariableIndex<SimulationControl, PrimitiveVariableType>() * SimulationControl::kDimension +
            getVariableGradientIndex<VariableGradientType>(),
        column);
  }

  template <PrimitiveVariableEnum PrimitiveVariableType>
  [[nodiscard]] inline Eigen::Vector<Real, SimulationControl::kDimension> getVector(const Isize column) const {
    return this->primitive_(
        Eigen::seqN(Eigen::fix<getPrimitiveVariableIndex<SimulationControl, PrimitiveVariableType>() *
                               SimulationControl::kDimension>,
                    Eigen::fix<SimulationControl::kDimension>),
        column);
  }

  template <PrimitiveVariableEnum PrimitiveVariableType>
  [[nodiscard]] inline Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension> getMatrix(
      const Isize column) const {
    return this
        ->primitive_(Eigen::seqN(Eigen::fix<getPrimitiveVariableIndex<SimulationControl, PrimitiveVariableType>() *
                                            SimulationControl::kDimension>,
                                 Eigen::fix<SimulationControl::kDimension * SimulationControl::kDimension>),
                     column)
        .reshaped(SimulationControl::kDimension, SimulationControl::kDimension);
  }

  template <ConservedVariableEnum ConservedVariableType>
  inline void setVector(const Eigen::Vector<Real, SimulationControl::kDimension>& value, const Isize column) {
    this->conserved_(Eigen::seqN(Eigen::fix<getConservedVariableIndex<SimulationControl, ConservedVariableType>() *
                                            SimulationControl::kDimension>,
                                 Eigen::fix<SimulationControl::kDimension>),
                     column) = value;
  }

  template <ConservedVariableEnum ConservedVariableType>
  inline void setMatrix(const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>& value,
                        const Isize column) {
    this->conserved_(Eigen::seqN(Eigen::fix<getConservedVariableIndex<SimulationControl, ConservedVariableType>() *
                                            SimulationControl::kDimension>,
                                 Eigen::fix<SimulationControl::kDimension * SimulationControl::kDimension>()),
                     column) = value.reshaped();
  }

  template <PrimitiveVariableEnum PrimitiveVariableType>
  inline void setVector(const Eigen::Vector<Real, SimulationControl::kDimension>& value, const Isize column) {
    this->primitive_(Eigen::seqN(Eigen::fix<getPrimitiveVariableIndex<SimulationControl, PrimitiveVariableType>() *
                                            SimulationControl::kDimension>,
                                 Eigen::fix<SimulationControl::kDimension>),
                     column) = value;
  }

  template <PrimitiveVariableEnum PrimitiveVariableType>
  inline void setMatrix(const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>& value,
                        const Isize column) {
    this->primitive_(Eigen::seqN(Eigen::fix<getPrimitiveVariableIndex<SimulationControl, PrimitiveVariableType>() *
                                            SimulationControl::kDimension>,
                                 Eigen::fix<SimulationControl::kDimension * SimulationControl::kDimension>),
                     column) = value.reshaped();
  }

  inline void calculatePrimitiveFromConserved(const PhysicalModel<SimulationControl>& physical_model,
                                              const Variable<SimulationControl, N>& variable) {
    for (Isize i = 0; i < N; i++) {
      if constexpr (IsCompresible<SimulationControl::kEquationModel>) {
        const Real density = variable.template getScalar<ComputationalVariableEnum::Density>(i);
        const Eigen::Vector<Real, SimulationControl::kDimension> density_gradient =
            this->getVector<ConservedVariableEnum::Density>(i);
        this->setVector<PrimitiveVariableEnum::Density>(density_gradient, i);
        const Eigen::Vector<Real, SimulationControl::kDimension> velocity =
            variable.template getVector<ComputationalVariableEnum::Velocity>(i);
        const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension> velocity_gradient =
            (this->getMatrix<ConservedVariableEnum::Momentum>(i) - density_gradient * velocity.transpose()) / density;
        this->setMatrix<PrimitiveVariableEnum::Velocity>(velocity_gradient, i);
        const Real total_energy = variable.template getScalar<ConservedVariableEnum::DensityTotalEnergy>(i) / density;
        const Eigen::Vector<Real, SimulationControl::kDimension> internal_energy_gradient =
            (this->getVector<ConservedVariableEnum::DensityTotalEnergy>(i) - density_gradient * total_energy) /
                density -
            velocity_gradient * velocity;
        Eigen::Vector<Real, SimulationControl::kDimension> temperature_gradient;
        for (Isize j = 0; j < SimulationControl::kDimension; j++) {
          temperature_gradient(j) = physical_model.calculateTemperatureFromInternalEnergy(internal_energy_gradient(j));
        }
        this->setVector<PrimitiveVariableEnum::Temperature>(temperature_gradient, i);
      }
      if constexpr (IsIncompresible<SimulationControl::kEquationModel>) {
        const Real density = variable.template getScalar<ComputationalVariableEnum::Density>(i);
        const Eigen::Vector<Real, SimulationControl::kDimension> density_gradient =
            this->getVector<ConservedVariableEnum::Density>(i);
        this->setVector<PrimitiveVariableEnum::Density>(density_gradient, i);
        const Eigen::Vector<Real, SimulationControl::kDimension> velocity =
            variable.template getVector<ComputationalVariableEnum::Velocity>(i);
        const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension> velocity_gradient =
            (this->getMatrix<ConservedVariableEnum::Momentum>(i) - density_gradient * velocity.transpose()) / density;
        this->setMatrix<PrimitiveVariableEnum::Velocity>(velocity_gradient, i);
        const Real internal_energy =
            variable.template getScalar<ConservedVariableEnum::DensityInternalEnergy>(i) / density;
        const Eigen::Vector<Real, SimulationControl::kDimension> internal_energy_gradient =
            (this->getVector<ConservedVariableEnum::DensityInternalEnergy>(i) - density_gradient * internal_energy) /
            density;
        Eigen::Vector<Real, SimulationControl::kDimension> temperature_gradient;
        for (Isize j = 0; j < SimulationControl::kDimension; j++) {
          temperature_gradient(j) = physical_model.calculateTemperatureFromInternalEnergy(internal_energy_gradient(j));
        }
        this->setVector<PrimitiveVariableEnum::Temperature>(temperature_gradient, i);
      }
    }
  }
};

template <typename ElementTrait, typename SimulationControl>
struct ElementVariableGradient : VariableGradient<SimulationControl, ElementTrait::kQuadratureNumber> {
  template <ViscousFluxEnum ViscousFluxType>
  inline void get(const ElementMesh<ElementTrait>& element_mesh,
                  const ElementSolver<ElementTrait, SimulationControl>& element_solver, Isize element_index) {
    if constexpr (ViscousFluxType == ViscousFluxEnum::None) {
      this->conserved_.noalias() =
          element_solver.element_(element_index).variable_volume_gradient_basis_function_coefficient_ *
          element_mesh.basis_function_.modal_value_.transpose();
    } else {
      this->conserved_.noalias() =
          element_solver.element_(element_index).variable_gradient_basis_function_coefficient_ *
          element_mesh.basis_function_.modal_value_.transpose();
    }
  }
};

template <typename AdjacencyElementTrait, typename SimulationControl>
struct AdjacencyElementVariableGradient
    : VariableGradient<SimulationControl, AdjacencyElementTrait::kQuadratureNumber> {
  template <typename ElementTrait, ViscousFluxEnum ViscousFluxType>
  void compute(const ElementMesh<ElementTrait>& element_mesh,
               const ElementSolver<ElementTrait, SimulationControl>& element_solver, const Isize parent_index_each_type,
               const Isize adjacency_sequence_in_parent) {
    constexpr std::array<int, ElementTrait::kAdjacencyNumber + 1> kElementAccumulateAdjacencyQuadratureNumber{
        getElementAccumulateAdjacencyQuadratureNumber<ElementTrait::kElementType,
                                                      SimulationControl::kPolynomialOrder>()};
    if constexpr (ViscousFluxType == ViscousFluxEnum::None) {
      this->conserved_.noalias() =
          element_solver.element_(parent_index_each_type).variable_volume_gradient_basis_function_coefficient_ *
          element_mesh.basis_function_
              .modal_adjacency_value_(
                  Eigen::seq(
                      kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent)],
                      kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent) +
                                                                  1] -
                          1),
                  Eigen::placeholders::all)
              .transpose();
    } else if constexpr (ViscousFluxType == ViscousFluxEnum::BR1) {
      this->conserved_.noalias() =
          element_solver.element_(parent_index_each_type).variable_gradient_basis_function_coefficient_ *
          element_mesh.basis_function_
              .modal_adjacency_value_(
                  Eigen::seq(
                      kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent)],
                      kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent) +
                                                                  1] -
                          1),
                  Eigen::placeholders::all)
              .transpose();
    } else if constexpr (ViscousFluxType == ViscousFluxEnum::BR2) {
      this->conserved_.noalias() =
          (element_solver.element_(parent_index_each_type).variable_volume_gradient_basis_function_coefficient_ +
           element_solver.element_(parent_index_each_type)
               .variable_interface_gradient_basis_function_coefficient_(adjacency_sequence_in_parent)) *
          element_mesh.basis_function_
              .modal_adjacency_value_(
                  Eigen::seq(
                      kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent)],
                      kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent) +
                                                                  1] -
                          1),
                  Eigen::placeholders::all)
              .transpose();
    }
  }

  template <ViscousFluxEnum ViscousFluxType>
  inline void get(const Mesh<SimulationControl>& mesh, const Solver<SimulationControl>& solver,
                  Isize parent_gmsh_type_number, Isize parent_index_each_type, Isize adjacency_sequence_in_parent) {
    if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
      this->compute<LineTrait<SimulationControl::kPolynomialOrder>, ViscousFluxType>(
          mesh.line_, solver.line_, parent_index_each_type, adjacency_sequence_in_parent);
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
      if (parent_gmsh_type_number == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->compute<TriangleTrait<SimulationControl::kPolynomialOrder>, ViscousFluxType>(
            mesh.triangle_, solver.triangle_, parent_index_each_type, adjacency_sequence_in_parent);
      } else if (parent_gmsh_type_number == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->compute<QuadrangleTrait<SimulationControl::kPolynomialOrder>, ViscousFluxType>(
            mesh.quadrangle_, solver.quadrangle_, parent_index_each_type, adjacency_sequence_in_parent);
      }
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
      if (parent_gmsh_type_number == TetrahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->compute<TetrahedronTrait<SimulationControl::kPolynomialOrder>, ViscousFluxType>(
            mesh.tetrahedron_, solver.tetrahedron_, parent_index_each_type, adjacency_sequence_in_parent);
      } else if (parent_gmsh_type_number == PyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->compute<PyramidTrait<SimulationControl::kPolynomialOrder>, ViscousFluxType>(
            mesh.pyramid_, solver.pyramid_, parent_index_each_type, adjacency_sequence_in_parent);
      }
    } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
      if (parent_gmsh_type_number == PyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->compute<PyramidTrait<SimulationControl::kPolynomialOrder>, ViscousFluxType>(
            mesh.pyramid_, solver.pyramid_, parent_index_each_type, adjacency_sequence_in_parent);
      } else if (parent_gmsh_type_number == HexahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->compute<HexahedronTrait<SimulationControl::kPolynomialOrder>, ViscousFluxType>(
            mesh.hexahedron_, solver.hexahedron_, parent_index_each_type, adjacency_sequence_in_parent);
      }
    }
  }
};

template <typename ElementTrait, typename SimulationControl, EquationModelEnum EquationModelType>
struct ViewVariableData;

template <typename ElementTrait, typename SimulationControl>
struct ViewVariableData<ElementTrait, SimulationControl, EquationModelEnum::CompresibleEuler> {
  Variable<SimulationControl, ElementTrait::kBasisFunctionNumber> variable_;
  Eigen::Vector<Real, ElementTrait::kBasisFunctionNumber> artificial_viscosity_;
};

template <typename ElementTrait, typename SimulationControl>
struct ViewVariableData<ElementTrait, SimulationControl, EquationModelEnum::CompresibleNS> {
  Variable<SimulationControl, ElementTrait::kBasisFunctionNumber> variable_;
  VariableGradient<SimulationControl, ElementTrait::kBasisFunctionNumber> variable_gradient_;
  Eigen::Vector<Real, ElementTrait::kBasisFunctionNumber> artificial_viscosity_;
};

template <typename ElementTrait, typename SimulationControl>
struct ViewVariableData<ElementTrait, SimulationControl, EquationModelEnum::IncompresibleEuler> {
  Variable<SimulationControl, ElementTrait::kBasisFunctionNumber> variable_;
  Eigen::Vector<Real, ElementTrait::kBasisFunctionNumber> artificial_viscosity_;
};

template <typename ElementTrait, typename SimulationControl>
struct ViewVariableData<ElementTrait, SimulationControl, EquationModelEnum::IncompresibleNS> {
  Variable<SimulationControl, ElementTrait::kBasisFunctionNumber> variable_;
  VariableGradient<SimulationControl, ElementTrait::kBasisFunctionNumber> variable_gradient_;
  Eigen::Vector<Real, ElementTrait::kBasisFunctionNumber> artificial_viscosity_;
};

template <typename ElementTrait, typename SimulationControl>
struct ViewVariable : ViewVariableData<ElementTrait, SimulationControl, SimulationControl::kEquationModel> {
  [[nodiscard]] inline Real get(const PhysicalModel<SimulationControl>& physical_model,
                                const ViewVariableEnum variable_type, const Isize column) const {
    switch (variable_type) {
    case ViewVariableEnum::Density:
      return this->variable_.template getScalar<ComputationalVariableEnum::Density>(column);
    case ViewVariableEnum::Velocity:
      return std::sqrt(this->variable_.template getScalar<ComputationalVariableEnum::VelocitySquaredNorm>(column));
    case ViewVariableEnum::Temperature:
      return physical_model.calculateTemperatureFromInternalEnergy(
          this->variable_.template getScalar<ComputationalVariableEnum::InternalEnergy>(column));
    case ViewVariableEnum::Pressure:
      return this->variable_.template getScalar<ComputationalVariableEnum::Pressure>(column);
    case ViewVariableEnum::SoundSpeed:
      return physical_model.calculateSoundSpeedFromDensityPressure(
          this->variable_.template getScalar<ComputationalVariableEnum::Density>(column),
          this->variable_.template getScalar<ComputationalVariableEnum::Pressure>(column));
    case ViewVariableEnum::MachNumber:
      return std::sqrt(this->variable_.template getScalar<ComputationalVariableEnum::VelocitySquaredNorm>(column)) /
             physical_model.calculateSoundSpeedFromDensityPressure(
                 this->variable_.template getScalar<ComputationalVariableEnum::Density>(column),
                 this->variable_.template getScalar<ComputationalVariableEnum::Pressure>(column));
    case ViewVariableEnum::Entropy:
      if constexpr (IsCompresible<SimulationControl::kEquationModel>) {
        return physical_model.calculateEntropyFromDensityPressure(
            this->variable_.template getScalar<ComputationalVariableEnum::Density>(column),
            this->variable_.template getScalar<ComputationalVariableEnum::Pressure>(column));
      }
    case ViewVariableEnum::Vorticity:
      if constexpr (IsNS<SimulationControl::kEquationModel>) {
        if constexpr (SimulationControl::kDimension == 2) {
          return this->variable_gradient_.template getScalar<PrimitiveVariableEnum::VelocityY, VariableGradientEnum::X>(
                     column) -
                 this->variable_gradient_.template getScalar<PrimitiveVariableEnum::VelocityX, VariableGradientEnum::Y>(
                     column);
        }
        if constexpr (SimulationControl::kDimension == 3) {
          return std::sqrt(
              (this->variable_gradient_.template getScalar<PrimitiveVariableEnum::VelocityZ, VariableGradientEnum::Y>(
                   column) -
               this->variable_gradient_.template getScalar<PrimitiveVariableEnum::VelocityY, VariableGradientEnum::Z>(
                   column)) *
                  (this->variable_gradient_
                       .template getScalar<PrimitiveVariableEnum::VelocityZ, VariableGradientEnum::Y>(column) -
                   this->variable_gradient_
                       .template getScalar<PrimitiveVariableEnum::VelocityY, VariableGradientEnum::Z>(column)) +
              (this->variable_gradient_.template getScalar<PrimitiveVariableEnum::VelocityX, VariableGradientEnum::Z>(
                   column) -
               this->variable_gradient_.template getScalar<PrimitiveVariableEnum::VelocityZ, VariableGradientEnum::X>(
                   column)) *
                  (this->variable_gradient_
                       .template getScalar<PrimitiveVariableEnum::VelocityX, VariableGradientEnum::Z>(column) -
                   this->variable_gradient_
                       .template getScalar<PrimitiveVariableEnum::VelocityZ, VariableGradientEnum::X>(column)) +
              (this->variable_gradient_.template getScalar<PrimitiveVariableEnum::VelocityY, VariableGradientEnum::X>(
                   column) -
               this->variable_gradient_.template getScalar<PrimitiveVariableEnum::VelocityX, VariableGradientEnum::Y>(
                   column)) *
                  (this->variable_gradient_
                       .template getScalar<PrimitiveVariableEnum::VelocityY, VariableGradientEnum::X>(column) -
                   this->variable_gradient_
                       .template getScalar<PrimitiveVariableEnum::VelocityX, VariableGradientEnum::Y>(column)));
        }
      }
    case ViewVariableEnum::ArtificialViscosity:
      return this->artificial_viscosity_(column);
    case ViewVariableEnum::VelocityX:
      return this->variable_.template getScalar<ComputationalVariableEnum::VelocityX>(column);
    case ViewVariableEnum::VelocityY:
      return this->variable_.template getScalar<ComputationalVariableEnum::VelocityY>(column);
    case ViewVariableEnum::VelocityZ:
      return this->variable_.template getScalar<ComputationalVariableEnum::VelocityZ>(column);
    case ViewVariableEnum::MachNumberX:
      return this->variable_.template getScalar<ComputationalVariableEnum::VelocityX>(column) /
             physical_model.calculateSoundSpeedFromDensityPressure(
                 this->variable_.template getScalar<ComputationalVariableEnum::Density>(column),
                 this->variable_.template getScalar<ComputationalVariableEnum::Pressure>(column));
    case ViewVariableEnum::MachNumberY:
      return this->variable_.template getScalar<ComputationalVariableEnum::VelocityY>(column) /
             physical_model.calculateSoundSpeedFromDensityPressure(
                 this->variable_.template getScalar<ComputationalVariableEnum::Density>(column),
                 this->variable_.template getScalar<ComputationalVariableEnum::Pressure>(column));
    case ViewVariableEnum::MachNumberZ:
      return this->variable_.template getScalar<ComputationalVariableEnum::VelocityZ>(column) /
             physical_model.calculateSoundSpeedFromDensityPressure(
                 this->variable_.template getScalar<ComputationalVariableEnum::Density>(column),
                 this->variable_.template getScalar<ComputationalVariableEnum::Pressure>(column));
    case ViewVariableEnum::VorticityX:
      if constexpr (IsNS<SimulationControl::kEquationModel>) {
        return this->variable_gradient_.template getScalar<PrimitiveVariableEnum::VelocityZ, VariableGradientEnum::Y>(
                   column) -
               this->variable_gradient_.template getScalar<PrimitiveVariableEnum::VelocityY, VariableGradientEnum::Z>(
                   column);
      }
    case ViewVariableEnum::VorticityY:
      if constexpr (IsNS<SimulationControl::kEquationModel>) {
        return this->variable_gradient_.template getScalar<PrimitiveVariableEnum::VelocityX, VariableGradientEnum::Z>(
                   column) -
               this->variable_gradient_.template getScalar<PrimitiveVariableEnum::VelocityZ, VariableGradientEnum::X>(
                   column);
      }
    case ViewVariableEnum::VorticityZ:
      if constexpr (IsNS<SimulationControl::kEquationModel>) {
        return this->variable_gradient_.template getScalar<PrimitiveVariableEnum::VelocityY, VariableGradientEnum::X>(
                   column) -
               this->variable_gradient_.template getScalar<PrimitiveVariableEnum::VelocityX, VariableGradientEnum::Y>(
                   column);
      }
    case ViewVariableEnum::HeatFluxX:
      if constexpr (IsNS<SimulationControl::kEquationModel>) {
        return this->variable_gradient_.template getScalar<PrimitiveVariableEnum::Temperature, VariableGradientEnum::X>(
            column);
      }
    case ViewVariableEnum::HeatFluxY:
      if constexpr (IsNS<SimulationControl::kEquationModel>) {
        return this->variable_gradient_.template getScalar<PrimitiveVariableEnum::Temperature, VariableGradientEnum::Y>(
            column);
      }
    case ViewVariableEnum::HeatFluxZ:
      if constexpr (IsNS<SimulationControl::kEquationModel>) {
        return this->variable_gradient_.template getScalar<PrimitiveVariableEnum::Temperature, VariableGradientEnum::Z>(
            column);
      }
    default:
      return 0.0_r;
    }
  }

  inline Eigen::Vector<Real, SimulationControl::kDimension> getForce(
      [[maybe_unused]] const PhysicalModel<SimulationControl>& physical_model,
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector, const Isize column) const {
    if constexpr (IsEuler<SimulationControl::kEquationModel>) {
      return this->variable_.template getScalar<ComputationalVariableEnum::Pressure>(column) * normal_vector;
    }
    if constexpr (IsNS<SimulationControl::kEquationModel>) {
      const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>& velocity_gradient =
          this->variable_gradient_.template getMatrix<PrimitiveVariableEnum::Velocity>(column);
      const Real tempurature = physical_model.calculateTemperatureFromInternalEnergy(
          this->variable_.template getScalar<ComputationalVariableEnum::InternalEnergy>(column));
      const Real dynamic_viscosity = physical_model.calculateDynamicViscosity(tempurature);
      const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension> viscous_stress =
          dynamic_viscosity * (velocity_gradient + velocity_gradient.transpose()) -
          2.0_r / 3.0_r * dynamic_viscosity * velocity_gradient.trace() *
              Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>::Identity();
      return (this->variable_.template getScalar<ComputationalVariableEnum::Pressure>(column) *
                  Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>::Identity() -
              viscous_stress) *
             normal_vector;
    }
  }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_VARIABLE_CONVERTOR_CPP_
