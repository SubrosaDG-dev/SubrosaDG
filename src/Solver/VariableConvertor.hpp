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
    if constexpr (ConservedVariableType == ConservedVariable::Density) {
      return this->conserved_(0);
    } else if constexpr (ConservedVariableType == ConservedVariable::MomentumX) {
      return this->conserved_(1);
    } else if constexpr (ConservedVariableType == ConservedVariable::MomentumY) {
      return this->conserved_(2);
    } else if constexpr (ConservedVariableType == ConservedVariable::MomentumZ) {
      return this->conserved_(3);
    } else if constexpr (ConservedVariableType == ConservedVariable::DensityTotalEnergy) {
      return this->conserved_(SimulationControl::kDimension + 1);
    }
  }

  template <ComputationalVariable ComputationalVariableType>
  [[nodiscard]] inline Real get() const {
    if constexpr (ComputationalVariableType == ComputationalVariable::Density) {
      return this->computational_(0);
    } else if constexpr (ComputationalVariableType == ComputationalVariable::VelocityX) {
      return this->computational_(1);
    } else if constexpr (ComputationalVariableType == ComputationalVariable::VelocityY) {
      return this->computational_(2);
    } else if constexpr (ComputationalVariableType == ComputationalVariable::VelocityZ) {
      return this->computational_(3);
    } else if constexpr (ComputationalVariableType == ComputationalVariable::InternalEnergy) {
      return this->computational_(SimulationControl::kDimension + 1);
    } else if constexpr (ComputationalVariableType == ComputationalVariable::Pressure) {
      return this->computational_(SimulationControl::kDimension + 2);
    }
  }

  template <PrimitiveVariable PrimitiveVariableType>
  [[nodiscard]] inline Real get() const {
    if constexpr (PrimitiveVariableType == PrimitiveVariable::Density) {
      return this->primitive_(0);
    } else if constexpr (PrimitiveVariableType == PrimitiveVariable::VelocityX) {
      return this->primitive_(1);
    } else if constexpr (PrimitiveVariableType == PrimitiveVariable::VelocityY) {
      return this->primitive_(2);
    } else if constexpr (PrimitiveVariableType == PrimitiveVariable::VelocityZ) {
      return this->primitive_(3);
    } else if constexpr (PrimitiveVariableType == PrimitiveVariable::Temperature) {
      return this->primitive_(SimulationControl::kDimension + 1);
    } else if constexpr (PrimitiveVariableType == PrimitiveVariable::Pressure) {
      return this->primitive_(SimulationControl::kDimension + 2);
    }
  }

  [[nodiscard]] inline Eigen::Vector<Real, SimulationControl::kDimension> getVelocity() const {
    return this->computational_(Eigen::seqN(Eigen::fix<1>(), Eigen::fix<SimulationControl::kDimension>()));
  }

  [[nodiscard]] inline Real getVelocitySquareSummation() const { return this->getVelocity().array().square().sum(); }

  template <ConservedVariable ConservedVariableType>
  inline void set(const Real value) {
    if constexpr (ConservedVariableType == ConservedVariable::Density) {
      this->conserved_(0) = value;
    } else if constexpr (ConservedVariableType == ConservedVariable::MomentumX) {
      this->conserved_(1) = value;
    } else if constexpr (ConservedVariableType == ConservedVariable::MomentumY) {
      this->conserved_(2) = value;
    } else if constexpr (ConservedVariableType == ConservedVariable::MomentumZ) {
      this->conserved_(3) = value;
    } else if constexpr (ConservedVariableType == ConservedVariable::DensityTotalEnergy) {
      this->conserved_(SimulationControl::kDimension + 1) = value;
    }
  }

  template <ConservedVariable ConservedVariableType>
  inline void set(const Variable<SimulationControl>& variable) {
    if constexpr (ConservedVariableType == ConservedVariable::Density) {
      this->conserved_(0) = variable.conserved_(0);
    } else if constexpr (ConservedVariableType == ConservedVariable::MomentumX) {
      this->conserved_(1) = variable.conserved_(1);
    } else if constexpr (ConservedVariableType == ConservedVariable::MomentumY) {
      this->conserved_(2) = variable.conserved_(2);
    } else if constexpr (ConservedVariableType == ConservedVariable::MomentumZ) {
      this->conserved_(3) = variable.conserved_(3);
    } else if constexpr (ConservedVariableType == ConservedVariable::DensityTotalEnergy) {
      this->conserved_(SimulationControl::kDimension + 1) = variable.conserved_(SimulationControl::kDimension + 1);
    }
  }

  template <ComputationalVariable ComputationalVariableType>
  inline void set(const Real value) {
    if constexpr (ComputationalVariableType == ComputationalVariable::Density) {
      this->computational_(0) = value;
    } else if constexpr (ComputationalVariableType == ComputationalVariable::VelocityX) {
      this->computational_(1) = value;
    } else if constexpr (ComputationalVariableType == ComputationalVariable::VelocityY) {
      this->computational_(2) = value;
    } else if constexpr (ComputationalVariableType == ComputationalVariable::VelocityZ) {
      this->computational_(3) = value;
    } else if constexpr (ComputationalVariableType == ComputationalVariable::InternalEnergy) {
      this->computational_(SimulationControl::kDimension + 1) = value;
    } else if constexpr (ComputationalVariableType == ComputationalVariable::Pressure) {
      this->computational_(SimulationControl::kDimension + 2) = value;
    }
  }

  template <ComputationalVariable ComputationalVariableType>
  inline void set(const Variable<SimulationControl>& variable) {
    if constexpr (ComputationalVariableType == ComputationalVariable::Density) {
      this->computational_(0) = variable.computational_(0);
    } else if constexpr (ComputationalVariableType == ComputationalVariable::VelocityX) {
      this->computational_(1) = variable.computational_(1);
    } else if constexpr (ComputationalVariableType == ComputationalVariable::VelocityY) {
      this->computational_(2) = variable.computational_(2);
    } else if constexpr (ComputationalVariableType == ComputationalVariable::VelocityZ) {
      this->computational_(3) = variable.computational_(3);
    } else if constexpr (ComputationalVariableType == ComputationalVariable::InternalEnergy) {
      this->computational_(SimulationControl::kDimension + 1) =
          variable.computational_(SimulationControl::kDimension + 1);
    } else if constexpr (ComputationalVariableType == ComputationalVariable::Pressure) {
      this->computational_(SimulationControl::kDimension + 2) =
          variable.computational_(SimulationControl::kDimension + 2);
    }
  }

  template <PrimitiveVariable PrimitiveVariableType>
  inline void set(const Real value) {
    if constexpr (PrimitiveVariableType == PrimitiveVariable::Density) {
      this->primitive_(0) = value;
    } else if constexpr (PrimitiveVariableType == PrimitiveVariable::VelocityX) {
      this->primitive_(1) = value;
    } else if constexpr (PrimitiveVariableType == PrimitiveVariable::VelocityY) {
      this->primitive_(2) = value;
    } else if constexpr (PrimitiveVariableType == PrimitiveVariable::VelocityZ) {
      this->primitive_(3) = value;
    } else if constexpr (PrimitiveVariableType == PrimitiveVariable::Temperature) {
      this->primitive_(SimulationControl::kDimension + 1) = value;
    } else if constexpr (PrimitiveVariableType == PrimitiveVariable::Pressure) {
      this->primitive_(SimulationControl::kDimension + 2) = value;
    }
  }

  template <PrimitiveVariable PrimitiveVariableType>
  inline void set(const Variable<SimulationControl>& variable) {
    if constexpr (PrimitiveVariableType == PrimitiveVariable::Density) {
      this->primitive_(0) = variable.primitive_(0);
    } else if constexpr (PrimitiveVariableType == PrimitiveVariable::VelocityX) {
      this->primitive_(1) = variable.primitive_(1);
    } else if constexpr (PrimitiveVariableType == PrimitiveVariable::VelocityY) {
      this->primitive_(2) = variable.primitive_(2);
    } else if constexpr (PrimitiveVariableType == PrimitiveVariable::VelocityZ) {
      this->primitive_(3) = variable.primitive_(3);
    } else if constexpr (PrimitiveVariableType == PrimitiveVariable::Temperature) {
      this->primitive_(SimulationControl::kDimension + 1) = variable.primitive_(SimulationControl::kDimension + 1);
    } else if constexpr (PrimitiveVariableType == PrimitiveVariable::Pressure) {
      this->primitive_(SimulationControl::kDimension + 2) = variable.primitive_(SimulationControl::kDimension + 2);
    }
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
        this->get<ComputationalVariable::InternalEnergy>() + 0.5 * this->getVelocitySquareSummation();
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
        this->get<ConservedVariable::DensityTotalEnergy>() / density - 0.5 * this->getVelocitySquareSummation();
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
        0.5 * this->getVelocitySquareSummation();
    this->set<ConservedVariable::DensityTotalEnergy>(density * total_energy);
  }

  inline void calculateComputationalFromPrimitive(
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model) {
    const Real density = this->get<PrimitiveVariable::Density>();
    this->set<ComputationalVariable::Density>(density);
    this->set<ComputationalVariable::VelocityX>(this->get<PrimitiveVariable::VelocityX>());
    this->set<ComputationalVariable::VelocityY>(this->get<PrimitiveVariable::VelocityY>());
    if constexpr (SimulationControl::kDimension == 3) {
      this->set<ComputationalVariable::VelocityZ>(this->get<PrimitiveVariable::VelocityZ>());
    }
    this->set<ComputationalVariable::InternalEnergy>(
        thermal_model.calculateInternalEnergyFromTemperature(this->get<PrimitiveVariable::Temperature>()));
    this->set<ComputationalVariable::Pressure>(this->get<PrimitiveVariable::Pressure>());
  }

  inline void calculatePrimitiveFromConserved(
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model) {
    const Real density = this->get<ConservedVariable::Density>();
    this->set<PrimitiveVariable::Density>(density);
    this->set<PrimitiveVariable::VelocityX>(this->get<ConservedVariable::MomentumX>() / density);
    this->set<ComputationalVariable::VelocityX>(this->get<PrimitiveVariable::VelocityX>());
    this->set<PrimitiveVariable::VelocityY>(this->get<ConservedVariable::MomentumY>() / density);
    this->set<ComputationalVariable::VelocityY>(this->get<PrimitiveVariable::VelocityY>());
    if constexpr (SimulationControl::kDimension == 3) {
      this->set<PrimitiveVariable::VelocityZ>(this->get<ConservedVariable::MomentumZ>() / density);
      this->set<ComputationalVariable::VelocityZ>(this->get<PrimitiveVariable::VelocityZ>());
    }
    const Real internal_energy =
        this->get<ConservedVariable::DensityTotalEnergy>() / density - 0.5 * this->getVelocitySquareSummation();
    this->set<PrimitiveVariable::Temperature>(thermal_model.calculateTemperatureFromInternalEnergy(internal_energy));
    this->set<PrimitiveVariable::Pressure>(
        thermal_model.calculatePressureFormDensityInternalEnergy(density, internal_energy));
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
