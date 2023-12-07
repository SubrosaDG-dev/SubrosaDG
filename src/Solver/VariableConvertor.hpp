/**
 * @file VariableConvertor.hpp
 * @brief The head file of SubroseDG variable convertor.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-06
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
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

template <typename SimulationControl, ConservedVariable ConservedVariableType>
inline static consteval int getConservedVariableIndex() {
  if constexpr (ConservedVariableType == ConservedVariable::Density) {
    return 0;
  } else if constexpr (ConservedVariableType == ConservedVariable::MomentumX) {
    return 1;
  } else if constexpr (ConservedVariableType == ConservedVariable::MomentumY) {
    return 2;
  } else if constexpr (ConservedVariableType == ConservedVariable::MomentumZ) {
    return 3;
  } else if constexpr (ConservedVariableType == ConservedVariable::DensityTotalEnergy) {
    return SimulationControl::kDimension + 1;
  }
}

template <typename SimulationControl, ComputationalVariable ComputationalVariableType>
inline static consteval int getComputationalVariableIndex() {
  if constexpr (ComputationalVariableType == ComputationalVariable::Density) {
    return 0;
  } else if constexpr (ComputationalVariableType == ComputationalVariable::VelocityX) {
    return 1;
  } else if constexpr (ComputationalVariableType == ComputationalVariable::VelocityY) {
    return 2;
  } else if constexpr (ComputationalVariableType == ComputationalVariable::VelocityZ) {
    return 3;
  } else if constexpr (ComputationalVariableType == ComputationalVariable::InternalEnergy) {
    return SimulationControl::kDimension + 1;
  } else if constexpr (ComputationalVariableType == ComputationalVariable::Pressure) {
    return SimulationControl::kDimension + 2;
  }
}

template <typename SimulationControl, PrimitiveVariable PrimitiveVariableType>
inline static consteval int getPrimitiveVariableIndex() {
  if constexpr (PrimitiveVariableType == PrimitiveVariable::Density) {
    return 0;
  } else if constexpr (PrimitiveVariableType == PrimitiveVariable::VelocityX) {
    return 1;
  } else if constexpr (PrimitiveVariableType == PrimitiveVariable::VelocityY) {
    return 2;
  } else if constexpr (PrimitiveVariableType == PrimitiveVariable::VelocityZ) {
    return 3;
  } else if constexpr (PrimitiveVariableType == PrimitiveVariable::Temperature) {
    return SimulationControl::kDimension + 1;
  }
}

template <typename SimulationControl, EquationModel EquationModelType>
struct Flux;

template <typename SimulationControl>
struct Flux<SimulationControl, EquationModel::Euler> {
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> convective_n_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, SimulationControl::kDimension> left_convective_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, SimulationControl::kDimension> right_convective_;
};

template <typename SimulationControl>
struct Flux<SimulationControl, EquationModel::NS> {
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> convective_n_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, SimulationControl::kDimension> left_convective_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, SimulationControl::kDimension> right_convective_;
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> viscous_n_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, SimulationControl::kDimension> left_viscous_;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, SimulationControl::kDimension> right_viscous_;
};

template <typename SimulationControl>
struct Variable {
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> conserved_;
  Eigen::Vector<Real, SimulationControl::kComputationalVariableNumber> computational_;
  Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber> primitive_;

  template <ConservedVariable ConservedVariableType>
  [[nodiscard]] inline Real get() const {
    return this->conserved_(getConservedVariableIndex<SimulationControl, ConservedVariableType>());
  }

  template <ComputationalVariable ComputationalVariableType>
  [[nodiscard]] inline Real get() const {
    return this->computational_(getComputationalVariableIndex<SimulationControl, ComputationalVariableType>());
  }

  template <PrimitiveVariable PrimitiveVariableType>
  [[nodiscard]] inline Real get() const {
    return this->primitive_(getPrimitiveVariableIndex<SimulationControl, PrimitiveVariableType>());
  }

  [[nodiscard]] inline Eigen::Vector<Real, SimulationControl::kDimension> getVelocity() const {
    return this->computational_(Eigen::seqN(Eigen::fix<1>(), Eigen::fix<SimulationControl::kDimension>()));
  }

  [[nodiscard]] inline Real getVelocitySquareSummation() const { return this->getVelocity().array().square().sum(); }

  [[nodiscard]] inline Real get(const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
                                const ViewElementVariable& view_element_variable) const {
    if (view_element_variable == ViewElementVariable::Density) {
      return this->get<ComputationalVariable::Density>();
    }
    if (view_element_variable == ViewElementVariable::Velocity) {
      return std::sqrt(this->getVelocitySquareSummation());
    }
    if (view_element_variable == ViewElementVariable::VelocityX) {
      return this->get<ComputationalVariable::VelocityX>();
    }
    if (view_element_variable == ViewElementVariable::VelocityY) {
      return this->get<ComputationalVariable::VelocityY>();
    }
    if (view_element_variable == ViewElementVariable::VelocityZ) {
      return this->get<ComputationalVariable::VelocityZ>();
    }
    if (view_element_variable == ViewElementVariable::Temperature) {
      return thermal_model.calculateTemperatureFromInternalEnergy(this->get<ComputationalVariable::InternalEnergy>());
    }
    if (view_element_variable == ViewElementVariable::Pressure) {
      return this->get<ComputationalVariable::Pressure>();
    }
    if (view_element_variable == ViewElementVariable::SoundSpeed) {
      return thermal_model.calculateSoundSpeedFromInternalEnergy(this->get<ComputationalVariable::InternalEnergy>());
    }
    if (view_element_variable == ViewElementVariable::MachNumber) {
      return std::sqrt(this->getVelocitySquareSummation()) /
             thermal_model.calculateSoundSpeedFromInternalEnergy(this->get<ComputationalVariable::InternalEnergy>());
    }
    if (view_element_variable == ViewElementVariable::MachNumberX) {
      return this->get<ComputationalVariable::VelocityX>() /
             thermal_model.calculateSoundSpeedFromInternalEnergy(this->get<ComputationalVariable::InternalEnergy>());
    }
    if (view_element_variable == ViewElementVariable::MachNumberY) {
      return this->get<ComputationalVariable::VelocityY>() /
             thermal_model.calculateSoundSpeedFromInternalEnergy(this->get<ComputationalVariable::InternalEnergy>());
    }
    if (view_element_variable == ViewElementVariable::MachNumberZ) {
      return this->get<ComputationalVariable::VelocityZ>() /
             thermal_model.calculateSoundSpeedFromInternalEnergy(this->get<ComputationalVariable::InternalEnergy>());
    }
    return 0.0;
  }

  template <ConservedVariable ConservedVariableType>
  inline void set(const Real value) {
    this->conserved_(getConservedVariableIndex<SimulationControl, ConservedVariableType>()) = value;
  }

  template <ConservedVariable ConservedVariableType>
  inline void set(const Variable<SimulationControl>& variable) {
    this->conserved_(getConservedVariableIndex<SimulationControl, ConservedVariableType>()) =
        variable.conserved_(getConservedVariableIndex<SimulationControl, ConservedVariableType>());
  }

  template <ComputationalVariable ComputationalVariableType>
  inline void set(const Real value) {
    this->computational_(getComputationalVariableIndex<SimulationControl, ComputationalVariableType>()) = value;
  }

  template <ComputationalVariable ComputationalVariableType>
  inline void set(const Variable<SimulationControl>& variable) {
    this->computational_(getComputationalVariableIndex<SimulationControl, ComputationalVariableType>()) =
        variable.computational_(getComputationalVariableIndex<SimulationControl, ComputationalVariableType>());
  }

  template <PrimitiveVariable PrimitiveVariableType>
  inline void set(const Real value) {
    this->primitive_(getPrimitiveVariableIndex<SimulationControl, PrimitiveVariableType>()) = value;
  }

  template <PrimitiveVariable PrimitiveVariableType>
  inline void set(const Variable<SimulationControl>& variable) {
    this->primitive_(getPrimitiveVariableIndex<SimulationControl, PrimitiveVariableType>()) =
        variable.primitive_(getPrimitiveVariableIndex<SimulationControl, PrimitiveVariableType>());
  }

  inline void calculateConservedFromComputational() {
    const Real density = this->get<ComputationalVariable::Density>();
    this->set<ConservedVariable::Density>(density);
    this->set<ConservedVariable::MomentumX>(density * this->get<ComputationalVariable::VelocityX>());
    this->set<ConservedVariable::MomentumY>(density * this->get<ComputationalVariable::VelocityY>());
    if constexpr (SimulationControl::kDimension == 3) {
      this->set<ConservedVariable::MomentumZ>(density * this->get<ComputationalVariable::VelocityZ>());
    }
    const Real total_energy =
        this->get<ComputationalVariable::InternalEnergy>() + this->getVelocitySquareSummation() / 2.0;
    this->set<ConservedVariable::DensityTotalEnergy>(density * total_energy);
  }

  inline void calculateComputationalFromConserved(
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model) {
    const Real density = this->get<ConservedVariable::Density>();
    this->set<ComputationalVariable::Density>(density);
    this->set<ComputationalVariable::VelocityX>(this->get<ConservedVariable::MomentumX>() / density);
    this->set<ComputationalVariable::VelocityY>(this->get<ConservedVariable::MomentumY>() / density);
    if constexpr (SimulationControl::kDimension == 3) {
      this->set<ComputationalVariable::VelocityZ>(this->get<ConservedVariable::MomentumZ>() / density);
    }
    const Real internal_energy =
        this->get<ConservedVariable::DensityTotalEnergy>() / density - this->getVelocitySquareSummation() / 2.0;
    this->set<ComputationalVariable::InternalEnergy>(internal_energy);
    this->set<ComputationalVariable::Pressure>(
        thermal_model.calculatePressureFormDensityInternalEnergy(density, internal_energy));
  }

  inline void calculateConservedFromPrimitive(
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model) {
    const Real density = this->get<PrimitiveVariable::Density>();
    this->set<ConservedVariable::Density>(density);
    this->set<ConservedVariable::MomentumX>(density * this->get<PrimitiveVariable::VelocityX>());
    this->set<ComputationalVariable::VelocityX>(this->get<PrimitiveVariable::VelocityX>());
    this->set<ConservedVariable::MomentumY>(density * this->get<PrimitiveVariable::VelocityY>());
    this->set<ComputationalVariable::VelocityY>(this->get<PrimitiveVariable::VelocityY>());
    if constexpr (SimulationControl::kDimension == 3) {
      this->set<ConservedVariable::MomentumZ>(density * this->get<PrimitiveVariable::VelocityZ>());
      this->set<ComputationalVariable::VelocityZ>(this->get<PrimitiveVariable::VelocityZ>());
    }
    const Real total_energy =
        thermal_model.calculateInternalEnergyFromTemperature(this->get<PrimitiveVariable::Temperature>()) +
        this->getVelocitySquareSummation() / 2.0;
    this->set<ConservedVariable::DensityTotalEnergy>(density * total_energy);
  }

  inline void calculateComputationalFromPrimitive(
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model) {
    this->set<ComputationalVariable::Density>(this->get<PrimitiveVariable::Density>());
    this->set<ComputationalVariable::VelocityX>(this->get<PrimitiveVariable::VelocityX>());
    this->set<ComputationalVariable::VelocityY>(this->get<PrimitiveVariable::VelocityY>());
    if constexpr (SimulationControl::kDimension == 3) {
      this->set<ComputationalVariable::VelocityZ>(this->get<PrimitiveVariable::VelocityZ>());
    }
    this->set<ComputationalVariable::InternalEnergy>(
        thermal_model.calculateInternalEnergyFromTemperature(this->get<PrimitiveVariable::Temperature>()));
    this->set<ComputationalVariable::Pressure>(thermal_model.calculatePressureFormDensityInternalEnergy(
        this->get<ComputationalVariable::Density>(), this->get<ComputationalVariable::InternalEnergy>()));
  }

  template <typename ElementTrait>
  inline void getFromSelf(
      const Isize element_index, const Isize element_gaussian_quadrature_node_sequence,
      const ElementMesh<ElementTrait>& element_mesh,
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
      const ElementSolver<ElementTrait, SimulationControl, SimulationControl::kEquationModel>& element_solver) {
    this->conserved_.noalias() =
        element_solver.element_(element_index).conserved_variable_basis_function_coefficient_(1) *
        element_mesh.basis_function_.value_.row(element_gaussian_quadrature_node_sequence).transpose();
    this->calculateComputationalFromConserved(thermal_model);
  }

  inline void getFromParent(const Isize parent_gmsh_type_number, const Isize parent_index,
                            const Isize adjacency_gaussian_quadrature_node_sequence_in_parent,
                            const Mesh<SimulationControl, SimulationControl::kDimension>& mesh,
                            const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
                            const Solver<SimulationControl, SimulationControl::kDimension>& solver) {
    if constexpr (SimulationControl::kDimension == 2) {
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
