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
#include <Utils/Enum.hpp>

#include "Solver/ConvectiveFlux.hpp"
#include "Solver/ThermalModel.hpp"
#include "Solver/VariableConvertor.hpp"
#include "Utils/BasicDataType.hpp"

namespace SubrosaDG {

template <typename SimulationControl>
struct BoundaryConditionBase {
  Variable<SimulationControl, SimulationControl::kDimension> variable_;

  virtual inline void calculateBoundaryConvectiveFlux(
      const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl, SimulationControl::kDimension>& left_quadrature_node_variable,
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& boundary_flux) = 0;

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
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& boundary_flux) override {
    calculateConvectiveFlux<SimulationControl>(thermal_model, normal_vector, left_quadrature_node_variable,
                                               this->variable_, boundary_flux);
  }
};

template <typename SimulationControl>
struct BoundaryConditionData<SimulationControl, BoundaryCondition::RiemannFarfield>
    : BoundaryConditionBase<SimulationControl> {};

template <typename SimulationControl>
struct BoundaryConditionData<SimulationControl, BoundaryCondition::NoSlipWall>
    : BoundaryConditionBase<SimulationControl> {
  inline void calculateBoundaryConvectiveFlux(
      [[maybe_unused]] const ThermalModel<SimulationControl, SimulationControl::kEquationModel>& thermal_model,
      const Eigen::Vector<Real, SimulationControl::kDimension>& normal_vector,
      const Variable<SimulationControl, SimulationControl::kDimension>& left_quadrature_node_variable,
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& boundary_flux) override {
    Variable<SimulationControl, SimulationControl::kDimension> wall_variable;
    if constexpr (SimulationControl::kDimension == 2) {
      wall_variable.primitive_(0) = left_quadrature_node_variable.primitive_(0);
      wall_variable.primitive_(1) = 0.0;
      wall_variable.primitive_(2) = 0.0;
      wall_variable.primitive_(3) = left_quadrature_node_variable.primitive_(3);
      wall_variable.primitive_(4) =
          left_quadrature_node_variable.primitive_(4) -
          0.5 * (left_quadrature_node_variable.primitive_(1) * left_quadrature_node_variable.primitive_(1) +
                 left_quadrature_node_variable.primitive_(2) * left_quadrature_node_variable.primitive_(2));
      Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, SimulationControl::kDimension>
          boundary_convective_variable;
      calculateConvectiveVariable(wall_variable, boundary_convective_variable);
      boundary_flux.noalias() = boundary_convective_variable * normal_vector;
    }
  }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_BOUNDARY_CONDITION_HPP_
