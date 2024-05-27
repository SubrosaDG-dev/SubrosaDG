/**
 * @file SourceTerm.hpp
 * @brief
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2024-05-13
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_SOURCE_TERM_HPP_
#define SUBROSA_DG_SOURCE_TERM_HPP_

#include "Solver/VariableConvertor.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Enum.hpp"

namespace SubrosaDG {

template <typename SimulationControl, SourceTermEnum SourceTermType>
struct SourceTermBase;

template <typename SimulationControl>
struct SourceTermBase<SimulationControl, SourceTermEnum::None> {};

template <typename SimulationControl>
struct SourceTermBase<SimulationControl, SourceTermEnum::Gravity> {
  inline static Real reference_density;
  inline static Real gravity;

  inline void calculateSourceTerm(const Variable<SimulationControl>& quadrature_node_variable,
                                  FluxNormalVariable<SimulationControl>& source_flux, const Isize column) const {
    source_flux.normal_variable_.setZero();
    source_flux.template setScalar<ConservedVariableEnum::MomentumY>(
        -(quadrature_node_variable.template getScalar<ConservedVariableEnum::Density>(column) -
          this->reference_density) *
        this->gravity);
  }
};

template <typename SimulationControl>
struct SourceTerm : SourceTermBase<SimulationControl, SimulationControl::kSourceTerm> {};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_SOURCE_TERM_HPP_
