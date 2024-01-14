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

template <typename SimulationControl, EquationModelEnum EquationModelType>
struct FluxData;

template <typename SimulationControl>
struct FluxData<SimulationControl, EquationModelEnum::Euler> {
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> left_convective_n_;
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> right_convective_n_;
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> convective_n_;
};

template <typename SimulationControl>
struct FluxData<SimulationControl, EquationModelEnum::NS> {
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> left_convective_n_;
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> right_convective_n_;
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> convective_n_;
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> left_viscous_n_;
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> right_viscous_n_;
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> viscous_n_;
};

template <typename SimulationControl>
struct Flux : FluxData<SimulationControl, SimulationControl::kEquationModel> {};

template <typename SimulationControl>
struct Variable {
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> conserved_;
  Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber> computational_;
  Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber> primitive_;

  template <ConservedVariableEnum ConservedVariableType>
  [[nodiscard]] inline Real get() const {
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
  [[nodiscard]] inline Real get() const {
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
  [[nodiscard]] inline Real get<ComputationalVariableEnum::VelocitySquareSummation>() const {
    return this->getVector<ComputationalVariableEnum::Velocity>().squaredNorm();
  }

  template <PrimitiveVariableEnum PrimitiveVariableType>
  [[nodiscard]] inline Real get() const {
    return this->primitive_(getPrimitiveVariableIndex<SimulationControl::kDimension, PrimitiveVariableType>());
  }

  template <PrimitiveVariableEnum PrimitiveVariableType>
  [[nodiscard]] inline Eigen::Vector<Real, SimulationControl::kDimension> getVector() const;

  template <>
  [[nodiscard]] inline Eigen::Vector<Real, SimulationControl::kDimension> getVector<PrimitiveVariableEnum::Velocity>()
      const {
    return this->primitive_(Eigen::seqN(Eigen::fix<1>(), Eigen::fix<SimulationControl::kDimension>()));
  }

  [[nodiscard]] inline Real get(const ThermalModel<SimulationControl>& thermal_model,
                                const ViewVariableEnum& view_variable) const {
    if (view_variable == ViewVariableEnum::Density) {
      return this->get<ComputationalVariableEnum::Density>();
    }
    if (view_variable == ViewVariableEnum::Velocity) {
      return std::sqrt(this->template get<ComputationalVariableEnum::VelocitySquareSummation>());
    }
    if (view_variable == ViewVariableEnum::VelocityX) {
      return this->get<ComputationalVariableEnum::VelocityX>();
    }
    if (view_variable == ViewVariableEnum::VelocityY) {
      return this->get<ComputationalVariableEnum::VelocityY>();
    }
    if (view_variable == ViewVariableEnum::VelocityZ) {
      return this->get<ComputationalVariableEnum::VelocityZ>();
    }
    if (view_variable == ViewVariableEnum::Temperature) {
      return thermal_model.calculateTemperatureFromInternalEnergy(
          this->get<ComputationalVariableEnum::InternalEnergy>());
    }
    if (view_variable == ViewVariableEnum::Pressure) {
      return this->get<ComputationalVariableEnum::Pressure>();
    }
    if (view_variable == ViewVariableEnum::SoundSpeed) {
      return thermal_model.calculateSoundSpeedFromInternalEnergy(
          this->get<ComputationalVariableEnum::InternalEnergy>());
    }
    if (view_variable == ViewVariableEnum::MachNumber) {
      return std::sqrt(this->template get<ComputationalVariableEnum::VelocitySquareSummation>()) /
             thermal_model.calculateSoundSpeedFromInternalEnergy(
                 this->get<ComputationalVariableEnum::InternalEnergy>());
    }
    if (view_variable == ViewVariableEnum::MachNumberX) {
      return this->get<ComputationalVariableEnum::VelocityX>() /
             thermal_model.calculateSoundSpeedFromInternalEnergy(
                 this->get<ComputationalVariableEnum::InternalEnergy>());
    }
    if (view_variable == ViewVariableEnum::MachNumberY) {
      return this->get<ComputationalVariableEnum::VelocityY>() /
             thermal_model.calculateSoundSpeedFromInternalEnergy(
                 this->get<ComputationalVariableEnum::InternalEnergy>());
    }
    if (view_variable == ViewVariableEnum::MachNumberZ) {
      return this->get<ComputationalVariableEnum::VelocityZ>() /
             thermal_model.calculateSoundSpeedFromInternalEnergy(
                 this->get<ComputationalVariableEnum::InternalEnergy>());
    }
    if (view_variable == ViewVariableEnum::Entropy) {
      return thermal_model.calculateEntropyFromDensityPressure(this->get<ComputationalVariableEnum::Density>(),
                                                               this->get<ComputationalVariableEnum::Pressure>());
    }
    return -1.0;
  }

  template <ConservedVariableEnum ConservedVariableType>
  inline void set(const Real value) {
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
  inline void set(const Variable<SimulationControl>& variable) {
    this->conserved_(getConservedVariableIndex<SimulationControl::kDimension, ConservedVariableType>()) =
        variable.conserved_(getConservedVariableIndex<SimulationControl::kDimension, ConservedVariableType>());
  }

  template <ComputationalVariableEnum ComputationalVariableType>
  inline void set(const Real value) {
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
  inline void set(const Variable<SimulationControl>& variable) {
    this->computational_(getComputationalVariableIndex<SimulationControl::kDimension, ComputationalVariableType>()) =
        variable.computational_(
            getComputationalVariableIndex<SimulationControl::kDimension, ComputationalVariableType>());
  }

  template <PrimitiveVariableEnum PrimitiveVariableType>
  inline void set(const Real value) {
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
  inline void set(const Variable<SimulationControl>& variable) {
    this->primitive_(getPrimitiveVariableIndex<SimulationControl::kDimension, PrimitiveVariableType>()) =
        variable.primitive_(getPrimitiveVariableIndex<SimulationControl::kDimension, PrimitiveVariableType>());
  }

  inline void calculateConservedFromComputational() {
    const Real density = this->get<ComputationalVariableEnum::Density>();
    this->set<ConservedVariableEnum::Density>(density);
    this->setVector<ConservedVariableEnum::Momentum>(density * this->getVector<ComputationalVariableEnum::Velocity>());
    const Real total_energy = this->get<ComputationalVariableEnum::InternalEnergy>() +
                              this->get<ComputationalVariableEnum::VelocitySquareSummation>() / 2.0;
    this->set<ConservedVariableEnum::DensityTotalEnergy>(density * total_energy);
  }

  inline void calculateComputationalFromConserved(const ThermalModel<SimulationControl>& thermal_model) {
    const Real density = this->get<ConservedVariableEnum::Density>();
    this->set<ComputationalVariableEnum::Density>(density);
    this->setVector<ComputationalVariableEnum::Velocity>(this->getVector<ConservedVariableEnum::Momentum>() / density);
    const Real internal_energy = this->get<ConservedVariableEnum::DensityTotalEnergy>() / density -
                                 this->get<ComputationalVariableEnum::VelocitySquareSummation>() / 2.0;
    this->set<ComputationalVariableEnum::InternalEnergy>(internal_energy);
    this->set<ComputationalVariableEnum::Pressure>(
        thermal_model.calculatePressureFormDensityInternalEnergy(density, internal_energy));
  }

  inline void calculateConservedFromPrimitive(const ThermalModel<SimulationControl>& thermal_model) {
    const Real density = this->get<PrimitiveVariableEnum::Density>();
    this->set<ConservedVariableEnum::Density>(density);
    this->setVector<ConservedVariableEnum::Momentum>(density * this->getVector<PrimitiveVariableEnum::Velocity>());
    this->setVector<ComputationalVariableEnum::Velocity>(this->getVector<PrimitiveVariableEnum::Velocity>());
    const Real total_energy =
        thermal_model.calculateInternalEnergyFromTemperature(this->get<PrimitiveVariableEnum::Temperature>()) +
        this->get<ComputationalVariableEnum::VelocitySquareSummation>() / 2.0;
    this->set<ConservedVariableEnum::DensityTotalEnergy>(density * total_energy);
  }

  inline void calculateComputationalFromPrimitive(const ThermalModel<SimulationControl>& thermal_model) {
    this->set<ComputationalVariableEnum::Density>(this->get<PrimitiveVariableEnum::Density>());
    this->setVector<ComputationalVariableEnum::Velocity>(this->getVector<PrimitiveVariableEnum::Velocity>());
    this->set<ComputationalVariableEnum::InternalEnergy>(
        thermal_model.calculateInternalEnergyFromTemperature(this->get<PrimitiveVariableEnum::Temperature>()));
    this->set<ComputationalVariableEnum::Pressure>(thermal_model.calculatePressureFormDensityInternalEnergy(
        this->get<ComputationalVariableEnum::Density>(), this->get<ComputationalVariableEnum::InternalEnergy>()));
  }

  template <typename ElementTrait>
  inline void getFromSelf(
      const Isize element_index, const Isize element_gaussian_quadrature_node_sequence,
      const ElementMesh<ElementTrait>& element_mesh, const ThermalModel<SimulationControl>& thermal_model,
      const ElementSolver<ElementTrait, SimulationControl, SimulationControl::kEquationModel>& element_solver) {
    this->conserved_.noalias() =
        element_solver.element_(element_index).conserved_variable_basis_function_coefficient_(1) *
        element_mesh.basis_function_.value_.row(element_gaussian_quadrature_node_sequence).transpose();
    this->calculateComputationalFromConserved(thermal_model);
  }

  inline void getFromParent(const Isize parent_gmsh_type_number, const Isize parent_index,
                            const Isize adjacency_gaussian_quadrature_node_sequence_in_parent,
                            const Mesh<SimulationControl>& mesh, const ThermalModel<SimulationControl>& thermal_model,
                            const Solver<SimulationControl>& solver) {
    if constexpr (SimulationControl::kDimension == 1) {
      this->conserved_.noalias() =
          solver.line_.element_(parent_index).conserved_variable_basis_function_coefficient_(1) *
          mesh.line_.basis_function_.adjacency_value_.row(adjacency_gaussian_quadrature_node_sequence_in_parent)
              .transpose();
    } else if constexpr (SimulationControl::kDimension == 2) {
      if (parent_gmsh_type_number == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->conserved_.noalias() =
            solver.triangle_.element_(parent_index).conserved_variable_basis_function_coefficient_(1) *
            mesh.triangle_.basis_function_.adjacency_value_.row(adjacency_gaussian_quadrature_node_sequence_in_parent)
                .transpose();
      } else if (parent_gmsh_type_number == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
        this->conserved_.noalias() =
            solver.quadrangle_.element_(parent_index).conserved_variable_basis_function_coefficient_(1) *
            mesh.quadrangle_.basis_function_.adjacency_value_.row(adjacency_gaussian_quadrature_node_sequence_in_parent)
                .transpose();
      }
    }
    this->calculateComputationalFromConserved(thermal_model);
  }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_VARIABLE_CONVERTOR_HPP_
