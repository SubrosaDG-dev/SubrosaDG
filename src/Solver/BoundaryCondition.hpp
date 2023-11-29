/**
 * @file BoundaryCondition.hpp
 * @brief The header file of BoundaryCondition.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_BOUNDARY_CONDITION_HPP_
#define SUBROSA_DG_BOUNDARY_CONDITION_HPP_

#include <Eigen/Core>
#include <cmath>
#include <compare>

#include "Solver/ConvectiveFlux.hpp"
#include "Solver/SolveControl.hpp"
#include "Solver/ThermalModel.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Enum.hpp"

namespace SubrosaDG {

template <typename SimulationControl>
struct BoundaryConditionBase {
  Variable<SimulationControl, SimulationControl::kDimension> variable_;

  virtual inline void calculateBoundaryConvectiveFlux(
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl, SimulationControl::kDimension>& left_quadrature_node_variable,
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& boundary_flux) const = 0;

  virtual ~BoundaryConditionBase() = default;
};

template <typename SimulationControl, BoundaryCondition BoundaryConditionType>
struct BoundaryConditionData : BoundaryConditionBase<SimulationControl> {};

template <typename SimulationControl>
struct BoundaryConditionData<SimulationControl, BoundaryCondition::NormalFarfield>
    : BoundaryConditionBase<SimulationControl> {
  inline void calculateBoundaryConvectiveFlux(
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl, SimulationControl::kDimension>& left_quadrature_node_variable,
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& boundary_flux) const override {
    calculateConvectiveFlux<SimulationControl>(thermal_model, normal_vector, left_quadrature_node_variable,
                                               this->variable_, boundary_flux);
  }
};

template <typename SimulationControl>
struct BoundaryConditionData<SimulationControl, BoundaryCondition::CharacteristicFarfield>
    : BoundaryConditionBase<SimulationControl> {
  inline void calculateBoundaryConvectiveFlux(
      [[maybe_unused]] const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl, SimulationControl::kDimension>& left_quadrature_node_variable,
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& boundary_flux) const override {
    if constexpr (SimulationControl::kDimension == 2) {
      Variable<SimulationControl, SimulationControl::kDimension> wall_variable;
      Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, SimulationControl::kDimension>
          boundary_convective_variable;
      const Real velocity_normal = left_quadrature_node_variable.primitive_(1) * normal_vector(0) +
                                   left_quadrature_node_variable.primitive_(2) * normal_vector(1);
      const Real mach_number_normal = velocity_normal / thermal_model.calculateSoundSpeedFromInternalEnergy(
                                                            left_quadrature_node_variable.primitive_(4));
      const bool is_supersonic = (std::fabs(mach_number_normal) <=> 1.0) == std::partial_ordering::greater;
      const bool is_positive = (mach_number_normal <=> 0.0) == std::partial_ordering::greater;
      if (is_supersonic) {
        if (is_positive) {
          calculateConvectiveVariable(left_quadrature_node_variable, boundary_convective_variable);
        } else {
          calculateConvectiveVariable(this->variable_, boundary_convective_variable);
        }
        boundary_flux.noalias() = boundary_convective_variable * normal_vector;
      } else {
        if (is_positive) {
          wall_variable.primitive_(0) = left_quadrature_node_variable.primitive_(0);
          wall_variable.primitive_(1) = left_quadrature_node_variable.primitive_(1);
          wall_variable.primitive_(2) = left_quadrature_node_variable.primitive_(2);
          wall_variable.primitive_(3) = thermal_model.calculatePressureFormDensityInternalEnergy(
              left_quadrature_node_variable.primitive_(0), this->variable_.primitive_(4));
          wall_variable.primitive_(4) = this->variable_.primitive_(4);
          calculateConvectiveVariable(wall_variable, boundary_convective_variable);
          boundary_flux.noalias() = boundary_convective_variable * normal_vector;
        } else {
          calculateConvectiveFlux<SimulationControl>(thermal_model, normal_vector, left_quadrature_node_variable,
                                                     this->variable_, boundary_flux);
        }
      }
    }
  }
};

template <typename SimulationControl>
struct BoundaryConditionData<SimulationControl, BoundaryCondition::NoSlipWall>
    : BoundaryConditionBase<SimulationControl> {
  inline void calculateBoundaryConvectiveFlux(
      [[maybe_unused]] const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl, SimulationControl::kDimension>& left_quadrature_node_variable,
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& boundary_flux) const override {
    Variable<SimulationControl, SimulationControl::kDimension> wall_variable;
    if constexpr (SimulationControl::kDimension == 2) {
      wall_variable.primitive_(0) = left_quadrature_node_variable.primitive_(0);
      wall_variable.primitive_(1) = 0.0;
      wall_variable.primitive_(2) = 0.0;
      wall_variable.primitive_(3) = left_quadrature_node_variable.primitive_(3);
      wall_variable.primitive_(4) = left_quadrature_node_variable.primitive_(4);
      Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, SimulationControl::kDimension>
          boundary_convective_variable;
      calculateConvectiveVariable(wall_variable, boundary_convective_variable);
      boundary_flux.noalias() = boundary_convective_variable * normal_vector;
    }
  }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_BOUNDARY_CONDITION_HPP_
