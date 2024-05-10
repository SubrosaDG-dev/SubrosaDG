/**
 * @file VariableConvertor.hpp
 * @brief The head file of SubrosaDG variable convertor.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-06
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_VARIABLE_CONVERTOR_HPP_
#define SUBROSA_DG_VARIABLE_CONVERTOR_HPP_

#include <Eigen/Core>

#include "Mesh/ReadControl.hpp"
#include "Solver/SimulationControl.hpp"
#include "Solver/SolveControl.hpp"
#include "Solver/ThermalModel.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Enum.hpp"

namespace SubrosaDG {

template <int Dimension, ConservedVariableEnum ConservedVariableType>
inline consteval int getConservedVariableIndex() {
  if constexpr (ConservedVariableType == ConservedVariableEnum::Density) {
    return 0;
  }
  if constexpr (ConservedVariableType == ConservedVariableEnum::MomentumX) {
    return 1;
  }
  if constexpr (ConservedVariableType == ConservedVariableEnum::MomentumY) {
    return 2;
  }
  if constexpr (ConservedVariableType == ConservedVariableEnum::MomentumZ) {
    return 3;
  }
  if constexpr (ConservedVariableType == ConservedVariableEnum::DensityTotalEnergy) {
    return Dimension + 1;
  }
  return -1;
}

template <int Dimension, ComputationalVariableEnum ComputationalVariableType>
inline consteval int getComputationalVariableIndex() {
  if constexpr (ComputationalVariableType == ComputationalVariableEnum::Density) {
    return 0;
  }
  if constexpr (ComputationalVariableType == ComputationalVariableEnum::VelocityX) {
    return 1;
  }
  if constexpr (ComputationalVariableType == ComputationalVariableEnum::VelocityY) {
    return 2;
  }
  if constexpr (ComputationalVariableType == ComputationalVariableEnum::VelocityZ) {
    return 3;
  }
  if constexpr (ComputationalVariableType == ComputationalVariableEnum::InternalEnergy) {
    return Dimension + 1;
  }
  if constexpr (ComputationalVariableType == ComputationalVariableEnum::Pressure) {
    return Dimension + 2;
  }
  return -1;
}

template <int Dimension, PrimitiveVariableEnum PrimitiveVariableType>
inline consteval int getPrimitiveVariableIndex() {
  if constexpr (PrimitiveVariableType == PrimitiveVariableEnum::Density) {
    return 0;
  }
  if constexpr (PrimitiveVariableType == PrimitiveVariableEnum::VelocityX) {
    return 1;
  }
  if constexpr (PrimitiveVariableType == PrimitiveVariableEnum::VelocityY) {
    return 2;
  }
  if constexpr (PrimitiveVariableType == PrimitiveVariableEnum::VelocityZ) {
    return 3;
  }
  if constexpr (PrimitiveVariableType == PrimitiveVariableEnum::Temperature) {
    return Dimension + 1;
  }
  return -1;
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
    this->normal_variable_(getConservedVariableIndex<SimulationControl::kDimension, ConservedVariableType>()) = value;
  }

  template <ConservedVariableEnum ConservedVariableType>
  inline void setVector(const Eigen::Vector<Real, SimulationControl::kDimension>& value);

  template <>
  inline void setVector<ConservedVariableEnum::Momentum>(
      const Eigen::Vector<Real, SimulationControl::kDimension>& value) {
    this->normal_variable_(Eigen::seqN(Eigen::fix<1>(), Eigen::fix<SimulationControl::kDimension>())) = value;
  }
};

template <typename SimulationControl>
struct FluxVariable {
  Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kConservedVariableNumber> variable_;

  template <ConservedVariableEnum ConservedVariableType>
  inline void setVector(const Eigen::Vector<Real, SimulationControl::kDimension>& value) {
    this->variable_.col(getConservedVariableIndex<SimulationControl::kDimension, ConservedVariableType>()) = value;
  }

  template <ConservedVariableEnum ConservedVariableType>
  inline void setMatrix(const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>& value);

  template <>
  inline void setMatrix<ConservedVariableEnum::Momentum>(
      const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>& value) {
    this->variable_(Eigen::all, Eigen::seqN(Eigen::fix<1>(), Eigen::fix<SimulationControl::kDimension>())) = value;
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

template <typename SimulationControl>
struct Variable {
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> conserved_;
  Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber> computational_;
  Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber> primitive_;

  template <ConservedVariableEnum ConservedVariableType>
  [[nodiscard]] inline Real getScalar() const {
    return this->conserved_(getConservedVariableIndex<SimulationControl::kDimension, ConservedVariableType>());
  }

  template <ConservedVariableEnum ConservedVariableType>
  [[nodiscard]] inline Eigen::Vector<Real, SimulationControl::kDimension> getVector() const;

  template <>
  [[nodiscard]] inline Eigen::Vector<Real, SimulationControl::kDimension> getVector<ConservedVariableEnum::Momentum>()
      const {
    return this->conserved_(Eigen::seqN(Eigen::fix<1>(), Eigen::fix<SimulationControl::kDimension>()));
  }

  template <ComputationalVariableEnum ComputationalVariableType>
  [[nodiscard]] inline Real getScalar() const {
    return this->computational_(
        getComputationalVariableIndex<SimulationControl::kDimension, ComputationalVariableType>());
  }

  template <ComputationalVariableEnum ComputationalVariableType>
  [[nodiscard]] inline Eigen::Vector<Real, SimulationControl::kDimension> getVector() const;

  template <>
  [[nodiscard]] inline Eigen::Vector<Real, SimulationControl::kDimension>
  getVector<ComputationalVariableEnum::Velocity>() const {
    return this->computational_(Eigen::seqN(Eigen::fix<1>(), Eigen::fix<SimulationControl::kDimension>()));
  }

  template <>
  [[nodiscard]] inline Real getScalar<ComputationalVariableEnum::VelocitySquareSummation>() const {
    return this->getVector<ComputationalVariableEnum::Velocity>().squaredNorm();
  }

  template <PrimitiveVariableEnum PrimitiveVariableType>
  [[nodiscard]] inline Real getScalar() const {
    return this->primitive_(getPrimitiveVariableIndex<SimulationControl::kDimension, PrimitiveVariableType>());
  }

  template <PrimitiveVariableEnum PrimitiveVariableType>
  [[nodiscard]] inline Eigen::Vector<Real, SimulationControl::kDimension> getVector() const;

  template <>
  [[nodiscard]] inline Eigen::Vector<Real, SimulationControl::kDimension> getVector<PrimitiveVariableEnum::Velocity>()
      const {
    return this->primitive_(Eigen::seqN(Eigen::fix<1>(), Eigen::fix<SimulationControl::kDimension>()));
  }

  template <ConservedVariableEnum ConservedVariableType>
  inline void setScalar(const Real value) {
    this->conserved_(getConservedVariableIndex<SimulationControl::kDimension, ConservedVariableType>()) = value;
  }

  template <ConservedVariableEnum ConservedVariableType>
  inline void setVector(const Eigen::Vector<Real, SimulationControl::kDimension>&);

  template <>
  inline void setVector<ConservedVariableEnum::Momentum>(
      const Eigen::Vector<Real, SimulationControl::kDimension>& value) {
    this->conserved_(Eigen::seqN(Eigen::fix<1>(), Eigen::fix<SimulationControl::kDimension>())) = value;
  }

  template <ConservedVariableEnum ConservedVariableType>
  inline void setScalar(const Variable<SimulationControl>& variable) {
    this->conserved_(getConservedVariableIndex<SimulationControl::kDimension, ConservedVariableType>()) =
        variable.conserved_(getConservedVariableIndex<SimulationControl::kDimension, ConservedVariableType>());
  }

  template <ComputationalVariableEnum ComputationalVariableType>
  inline void setScalar(const Real value) {
    this->computational_(getComputationalVariableIndex<SimulationControl::kDimension, ComputationalVariableType>()) =
        value;
  }

  template <ComputationalVariableEnum ComputationalVariableType>
  inline void setVector(const Eigen::Vector<Real, SimulationControl::kDimension>&);

  template <>
  inline void setVector<ComputationalVariableEnum::Velocity>(
      const Eigen::Vector<Real, SimulationControl::kDimension>& value) {
    this->computational_(Eigen::seqN(Eigen::fix<1>(), Eigen::fix<SimulationControl::kDimension>())) = value;
  }

  template <ComputationalVariableEnum ComputationalVariableType>
  inline void setScalar(const Variable<SimulationControl>& variable) {
    this->computational_(getComputationalVariableIndex<SimulationControl::kDimension, ComputationalVariableType>()) =
        variable.computational_(
            getComputationalVariableIndex<SimulationControl::kDimension, ComputationalVariableType>());
  }

  template <PrimitiveVariableEnum PrimitiveVariableType>
  inline void setScalar(const Real value) {
    this->primitive_(getPrimitiveVariableIndex<SimulationControl::kDimension, PrimitiveVariableType>()) = value;
  }

  template <PrimitiveVariableEnum PrimitiveVariableType>
  inline void setVector(const Eigen::Vector<Real, SimulationControl::kDimension>&);

  template <>
  inline void setVector<PrimitiveVariableEnum::Velocity>(
      const Eigen::Vector<Real, SimulationControl::kDimension>& value) {
    this->primitive_(Eigen::seqN(Eigen::fix<1>(), Eigen::fix<SimulationControl::kDimension>())) = value;
  }

  template <PrimitiveVariableEnum PrimitiveVariableType>
  inline void setScalar(const Variable<SimulationControl>& variable) {
    this->primitive_(getPrimitiveVariableIndex<SimulationControl::kDimension, PrimitiveVariableType>()) =
        variable.primitive_(getPrimitiveVariableIndex<SimulationControl::kDimension, PrimitiveVariableType>());
  }

  inline void calculateConservedFromComputational() {
    const Real density = this->getScalar<ComputationalVariableEnum::Density>();
    this->setScalar<ConservedVariableEnum::Density>(density);
    this->setVector<ConservedVariableEnum::Momentum>(density * this->getVector<ComputationalVariableEnum::Velocity>());
    const Real total_energy = this->getScalar<ComputationalVariableEnum::InternalEnergy>() +
                              this->getScalar<ComputationalVariableEnum::VelocitySquareSummation>() / 2.0;
    this->setScalar<ConservedVariableEnum::DensityTotalEnergy>(density * total_energy);
  }

  inline void calculateComputationalFromConserved(const ThermalModel<SimulationControl>& thermal_model) {
    const Real density = this->getScalar<ConservedVariableEnum::Density>();
    this->setScalar<ComputationalVariableEnum::Density>(density);
    this->setVector<ComputationalVariableEnum::Velocity>(this->getVector<ConservedVariableEnum::Momentum>() / density);
    const Real internal_energy = this->getScalar<ConservedVariableEnum::DensityTotalEnergy>() / density -
                                 this->getScalar<ComputationalVariableEnum::VelocitySquareSummation>() / 2.0;
    this->setScalar<ComputationalVariableEnum::InternalEnergy>(internal_energy);
    this->setScalar<ComputationalVariableEnum::Pressure>(
        thermal_model.calculatePressureFormDensityInternalEnergy(density, internal_energy));
  }

  inline void calculateConservedFromPrimitive(const ThermalModel<SimulationControl>& thermal_model) {
    const Real density = this->getScalar<PrimitiveVariableEnum::Density>();
    this->setScalar<ConservedVariableEnum::Density>(density);
    this->setVector<ConservedVariableEnum::Momentum>(density * this->getVector<PrimitiveVariableEnum::Velocity>());
    this->setVector<ComputationalVariableEnum::Velocity>(this->getVector<PrimitiveVariableEnum::Velocity>());
    const Real total_energy =
        thermal_model.calculateInternalEnergyFromTemperature(this->getScalar<PrimitiveVariableEnum::Temperature>()) +
        this->getScalar<ComputationalVariableEnum::VelocitySquareSummation>() / 2.0;
    this->setScalar<ConservedVariableEnum::DensityTotalEnergy>(density * total_energy);
  }

  inline void calculateComputationalFromPrimitive(const ThermalModel<SimulationControl>& thermal_model) {
    this->setScalar<ComputationalVariableEnum::Density>(this->getScalar<PrimitiveVariableEnum::Density>());
    this->setVector<ComputationalVariableEnum::Velocity>(this->getVector<PrimitiveVariableEnum::Velocity>());
    this->setScalar<ComputationalVariableEnum::InternalEnergy>(
        thermal_model.calculateInternalEnergyFromTemperature(this->getScalar<PrimitiveVariableEnum::Temperature>()));
    this->setScalar<ComputationalVariableEnum::Pressure>(thermal_model.calculatePressureFormDensityInternalEnergy(
        this->getScalar<ComputationalVariableEnum::Density>(),
        this->getScalar<ComputationalVariableEnum::InternalEnergy>()));
  }

  template <typename ElementTrait>
  inline void getFromSelf(const ElementMesh<ElementTrait>& element_mesh,
                          const ElementSolverBase<ElementTrait, SimulationControl>& element_solver,
                          const Isize element_index, const Isize element_quadrature_node_sequence) {
    this->conserved_.noalias() = element_solver.element_(element_index).variable_basis_function_coefficient_(1) *
                                 element_mesh.basis_function_.value_.row(element_quadrature_node_sequence).transpose();
  }

  inline void getFromParent(const Mesh<SimulationControl>& mesh, const Solver<SimulationControl>& solver,
                            const Isize parent_gmsh_type_number, const Isize parent_index_each_type,
                            const Isize adjacency_quadrature_node_sequence_in_parent) {
    if constexpr (SimulationControl::kDimension == 1) {
      this->conserved_.noalias() =
          solver.line_.element_(parent_index_each_type).variable_basis_function_coefficient_(1) *
          mesh.line_.basis_function_.adjacency_value_.row(adjacency_quadrature_node_sequence_in_parent).transpose();
    } else if constexpr (SimulationControl::kDimension == 2) {
      if (parent_gmsh_type_number == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->conserved_.noalias() =
            solver.triangle_.element_(parent_index_each_type).variable_basis_function_coefficient_(1) *
            mesh.triangle_.basis_function_.adjacency_value_.row(adjacency_quadrature_node_sequence_in_parent)
                .transpose();
      } else if (parent_gmsh_type_number == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->conserved_.noalias() =
            solver.quadrangle_.element_(parent_index_each_type).variable_basis_function_coefficient_(1) *
            mesh.quadrangle_.basis_function_.adjacency_value_.row(adjacency_quadrature_node_sequence_in_parent)
                .transpose();
      }
    }
  }
};

template <typename SimulationControl>
struct VariableGradient {
  Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kConservedVariableNumber> conserved_;
  Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kPrimitiveVariableNumber> primitive_;

  template <ConservedVariableEnum ConservedVariableType>
  [[nodiscard]] inline Eigen::Vector<Real, SimulationControl::kDimension> getVector() const {
    return this->conserved_.col(getConservedVariableIndex<SimulationControl::kDimension, ConservedVariableType>());
  }

  template <ConservedVariableEnum ConservedVariableType>
  [[nodiscard]] inline Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension> getMatrix()
      const;

  template <>
  [[nodiscard]] inline Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>
  getMatrix<ConservedVariableEnum::Momentum>() const {
    return this->conserved_(Eigen::all, Eigen::seqN(Eigen::fix<1>(), Eigen::fix<SimulationControl::kDimension>()));
  }

  template <PrimitiveVariableEnum PrimitiveVariableType, VariableGradientEnum VariableGradientType>
  [[nodiscard]] inline Real getScalar() const {
    return this->primitive_(getVariableGradientIndex<VariableGradientType>(),
                            getPrimitiveVariableIndex<SimulationControl::kDimension, PrimitiveVariableType>());
  }

  template <PrimitiveVariableEnum PrimitiveVariableType>
  [[nodiscard]] inline Eigen::Vector<Real, SimulationControl::kDimension> getVector() const {
    return this->primitive_.col(getPrimitiveVariableIndex<SimulationControl::kDimension, PrimitiveVariableType>());
  }

  template <PrimitiveVariableEnum PrimitiveVariableType>
  [[nodiscard]] inline Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension> getMatrix()
      const;

  template <>
  [[nodiscard]] inline Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>
  getMatrix<PrimitiveVariableEnum::Velocity>() const {
    return this->primitive_(Eigen::all, Eigen::seqN(Eigen::fix<1>(), Eigen::fix<SimulationControl::kDimension>()));
  }

  template <ConservedVariableEnum ConservedVariableType>
  inline void setVector(const Eigen::Vector<Real, SimulationControl::kDimension>& value) {
    this->conserved_.col(getConservedVariableIndex<SimulationControl::kDimension, ConservedVariableType>()) = value;
  }

  template <ConservedVariableEnum ConservedVariableType>
  inline void setMatrix(const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>& value);

  template <>
  inline void setMatrix<ConservedVariableEnum::Momentum>(
      const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>& value) {
    this->conserved_(Eigen::all, Eigen::seqN(Eigen::fix<1>(), Eigen::fix<SimulationControl::kDimension>())) = value;
  }

  template <PrimitiveVariableEnum PrimitiveVariableType>
  inline void setVector(const Eigen::Vector<Real, SimulationControl::kDimension>& value) {
    this->primitive_.col(getPrimitiveVariableIndex<SimulationControl::kDimension, PrimitiveVariableType>()) = value;
  }

  template <PrimitiveVariableEnum PrimitiveVariableType>
  inline void setMatrix(const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>& value);

  template <>
  inline void setMatrix<PrimitiveVariableEnum::Velocity>(
      const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension>& value) {
    this->primitive_(Eigen::all, Eigen::seqN(Eigen::fix<1>(), Eigen::fix<SimulationControl::kDimension>())) = value;
  }

  inline void calculatePrimitiveFromConserved(const ThermalModel<SimulationControl>& thermal_model,
                                              const Variable<SimulationControl>& variable) {
    const Real density = variable.template getScalar<ComputationalVariableEnum::Density>();
    const Eigen::Vector<Real, SimulationControl::kDimension> density_gradient =
        this->getVector<ConservedVariableEnum::Density>();
    this->setVector<PrimitiveVariableEnum::Density>(density_gradient);
    const Eigen::Vector<Real, SimulationControl::kDimension> velocity =
        variable.template getVector<ComputationalVariableEnum::Velocity>();
    const Eigen::Matrix<Real, SimulationControl::kDimension, SimulationControl::kDimension> velocity_gradient =
        (this->getMatrix<ConservedVariableEnum::Momentum>() - density_gradient * velocity.transpose()) / density;
    this->setMatrix<PrimitiveVariableEnum::Velocity>(velocity_gradient);
    const Real total_energy = variable.template getScalar<ConservedVariableEnum::DensityTotalEnergy>() / density;
    const Eigen::Vector<Real, SimulationControl::kDimension> internal_energy_gradient =
        (this->getVector<ConservedVariableEnum::DensityTotalEnergy>() - density_gradient * total_energy) / density -
        velocity_gradient * velocity;
    Eigen::Vector<Real, SimulationControl::kDimension> temperature_gradient;
    for (Isize i = 0; i < SimulationControl::kDimension; i++) {
      temperature_gradient(i) = thermal_model.calculateTemperatureFromInternalEnergy(internal_energy_gradient(i));
    }
    this->setVector<PrimitiveVariableEnum::Temperature>(temperature_gradient);
  }

  template <typename ElementTrait>
  inline void getFromSelf(const ElementMesh<ElementTrait>& element_mesh,
                          const ElementSolverBase<ElementTrait, SimulationControl>& element_solver,
                          const Isize element_index, const Isize element_quadrature_node_sequence) {
    this->conserved_.noalias() =
        (element_solver.element_(element_index).variable_gradient_basis_function_coefficient_ *
         element_mesh.basis_function_.value_.row(element_quadrature_node_sequence).transpose())
            .reshaped(SimulationControl::kDimension, SimulationControl::kConservedVariableNumber);
  }

  template <ViscousFluxEnum ViscousFluxType>
  inline void getFromParent(const Mesh<SimulationControl>& mesh, const Solver<SimulationControl>& solver,
                            Isize parent_gmsh_type_number, Isize parent_index_each_type,
                            Isize adjacency_sequence_in_parent, Isize adjacency_quadrature_node_sequence_in_parent);

  template <>
  inline void getFromParent<ViscousFluxEnum::BR1>(const Mesh<SimulationControl>& mesh,
                                                  const Solver<SimulationControl>& solver,
                                                  const Isize parent_gmsh_type_number,
                                                  const Isize parent_index_each_type,
                                                  [[maybe_unused]] const Isize adjacency_sequence_in_parent,
                                                  const Isize adjacency_quadrature_node_sequence_in_parent) {
    if constexpr (SimulationControl::kDimension == 2) {
      if (parent_gmsh_type_number == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->conserved_.noalias() =
            (solver.triangle_.element_(parent_index_each_type).variable_gradient_basis_function_coefficient_ *
             mesh.triangle_.basis_function_.adjacency_value_.row(adjacency_quadrature_node_sequence_in_parent)
                 .transpose())
                .reshaped(SimulationControl::kDimension, SimulationControl::kConservedVariableNumber);
      } else if (parent_gmsh_type_number == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->conserved_.noalias() =
            (solver.quadrangle_.element_(parent_index_each_type).variable_gradient_basis_function_coefficient_ *
             mesh.quadrangle_.basis_function_.adjacency_value_.row(adjacency_quadrature_node_sequence_in_parent)
                 .transpose())
                .reshaped(SimulationControl::kDimension, SimulationControl::kConservedVariableNumber);
      }
    }
  }

  template <>
  inline void getFromParent<ViscousFluxEnum::BR2>(const Mesh<SimulationControl>& mesh,
                                                  const Solver<SimulationControl>& solver,
                                                  const Isize parent_gmsh_type_number,
                                                  const Isize parent_index_each_type,
                                                  const Isize adjacency_sequence_in_parent,
                                                  const Isize adjacency_quadrature_node_sequence_in_parent) {
    if constexpr (SimulationControl::kDimension == 2) {
      if (parent_gmsh_type_number == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->conserved_.noalias() =
            ((solver.triangle_.element_(parent_index_each_type).variable_gradient_volume_basis_function_coefficient_ +
              solver.triangle_.element_(parent_index_each_type)
                  .variable_gradient_interface_basis_function_coefficient_(adjacency_sequence_in_parent)) *
             mesh.triangle_.basis_function_.adjacency_value_.row(adjacency_quadrature_node_sequence_in_parent)
                 .transpose())
                .reshaped(SimulationControl::kDimension, SimulationControl::kConservedVariableNumber);
      } else if (parent_gmsh_type_number == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->conserved_.noalias() =
            ((solver.quadrangle_.element_(parent_index_each_type).variable_gradient_volume_basis_function_coefficient_ +
              solver.quadrangle_.element_(parent_index_each_type)
                  .variable_gradient_interface_basis_function_coefficient_(adjacency_sequence_in_parent)) *
             mesh.quadrangle_.basis_function_.adjacency_value_.row(adjacency_quadrature_node_sequence_in_parent)
                 .transpose())
                .reshaped(SimulationControl::kDimension, SimulationControl::kConservedVariableNumber);
      }
    }
  }
};

template <typename SimulationControl>
struct ViewVariable {
  Variable<SimulationControl> variable_;
  VariableGradient<SimulationControl> variable_gradient_;

  [[nodiscard]] inline Real getView(const ThermalModel<SimulationControl>& thermal_model,
                                    const ViewVariableEnum variable_type) const {
    switch (variable_type) {
    case ViewVariableEnum::Density:
      return this->variable_.template getScalar<ComputationalVariableEnum::Density>();
    case ViewVariableEnum::Velocity:
      return std::sqrt(this->variable_.template getScalar<ComputationalVariableEnum::VelocitySquareSummation>());
    case ViewVariableEnum::Temperature:
      return thermal_model.calculateTemperatureFromInternalEnergy(
          this->variable_.template getScalar<ComputationalVariableEnum::InternalEnergy>());
    case ViewVariableEnum::Pressure:
      return this->variable_.template getScalar<ComputationalVariableEnum::Pressure>();
    case ViewVariableEnum::SoundSpeed:
      return thermal_model.calculateSoundSpeedFromInternalEnergy(
          this->variable_.template getScalar<ComputationalVariableEnum::InternalEnergy>());
    case ViewVariableEnum::MachNumber:
      return std::sqrt(this->variable_.template getScalar<ComputationalVariableEnum::VelocitySquareSummation>()) /
             thermal_model.calculateSoundSpeedFromInternalEnergy(
                 this->variable_.template getScalar<ComputationalVariableEnum::InternalEnergy>());
    case ViewVariableEnum::Entropy:
      return thermal_model.calculateEntropyFromDensityPressure(
          this->variable_.template getScalar<ComputationalVariableEnum::Density>(),
          this->variable_.template getScalar<ComputationalVariableEnum::Pressure>());
    case ViewVariableEnum::Vorticity:
      if constexpr (SimulationControl::kDimension == 2) {
        return this->variable_gradient_
                   .template getScalar<PrimitiveVariableEnum::VelocityY, VariableGradientEnum::X>() -
               this->variable_gradient_.template getScalar<PrimitiveVariableEnum::VelocityX, VariableGradientEnum::Y>();
      }
      if constexpr (SimulationControl::kDimension == 3) {
        return std::sqrt(
            std::pow(this->variable_gradient_
                             .template getScalar<PrimitiveVariableEnum::VelocityZ, VariableGradientEnum::Y>() -
                         this->variable_gradient_
                             .template getScalar<PrimitiveVariableEnum::VelocityY, VariableGradientEnum::Z>(),
                     2) +
            std::pow(this->variable_gradient_
                             .template getScalar<PrimitiveVariableEnum::VelocityX, VariableGradientEnum::Z>() -
                         this->variable_gradient_
                             .template getScalar<PrimitiveVariableEnum::VelocityZ, VariableGradientEnum::X>(),
                     2) +
            std::pow(this->variable_gradient_
                             .template getScalar<PrimitiveVariableEnum::VelocityY, VariableGradientEnum::X>() -
                         this->variable_gradient_
                             .template getScalar<PrimitiveVariableEnum::VelocityX, VariableGradientEnum::Y>(),
                     2));
      }
    case ViewVariableEnum::MachNumberX:
      return this->variable_.template getScalar<ComputationalVariableEnum::VelocityX>() /
             thermal_model.calculateSoundSpeedFromInternalEnergy(
                 this->variable_.template getScalar<ComputationalVariableEnum::InternalEnergy>());
    case ViewVariableEnum::MachNumberY:
      return this->variable_.template getScalar<ComputationalVariableEnum::VelocityY>() /
             thermal_model.calculateSoundSpeedFromInternalEnergy(
                 this->variable_.template getScalar<ComputationalVariableEnum::InternalEnergy>());
    case ViewVariableEnum::MachNumberZ:
      return this->variable_.template getScalar<ComputationalVariableEnum::VelocityZ>() /
             thermal_model.calculateSoundSpeedFromInternalEnergy(
                 this->variable_.template getScalar<ComputationalVariableEnum::InternalEnergy>());
    case ViewVariableEnum::VelocityX:
      return this->variable_.template getScalar<ComputationalVariableEnum::VelocityX>();
    case ViewVariableEnum::VelocityY:
      return this->variable_.template getScalar<ComputationalVariableEnum::VelocityY>();
    case ViewVariableEnum::VelocityZ:
      return this->variable_.template getScalar<ComputationalVariableEnum::VelocityZ>();
    case ViewVariableEnum::VorticityX:
      return this->variable_gradient_.template getScalar<PrimitiveVariableEnum::VelocityZ, VariableGradientEnum::Y>() -
             this->variable_gradient_.template getScalar<PrimitiveVariableEnum::VelocityY, VariableGradientEnum::Z>();
    case ViewVariableEnum::VorticityY:
      return this->variable_gradient_.template getScalar<PrimitiveVariableEnum::VelocityX, VariableGradientEnum::Z>() -
             this->variable_gradient_.template getScalar<PrimitiveVariableEnum::VelocityZ, VariableGradientEnum::X>();
    case ViewVariableEnum::VorticityZ:
      return this->variable_gradient_.template getScalar<PrimitiveVariableEnum::VelocityY, VariableGradientEnum::X>() -
             this->variable_gradient_.template getScalar<PrimitiveVariableEnum::VelocityX, VariableGradientEnum::Y>();
    default:
      return 0.0;
    }
  }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_VARIABLE_CONVERTOR_HPP_
