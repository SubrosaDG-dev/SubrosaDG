/**
 * @file SpatialDiscrete.hpp
 * @brief The header file of SubrosaDG spatial discrete.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-16
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_SPATIAL_DISCRETE_HPP_
#define SUBROSA_DG_SPATIAL_DISCRETE_HPP_

#include <Eigen/Core>
#include <array>
#include <cmath>
#include <memory>
#include <type_traits>
#include <unordered_map>

#include "Mesh/ReadControl.hpp"
#include "Solver/BoundaryCondition.hpp"
#include "Solver/ConvectiveFlux.hpp"
#include "Solver/PhysicalModel.hpp"
#include "Solver/SimulationControl.hpp"
#include "Solver/SolveControl.hpp"
#include "Solver/SourceTerm.hpp"
#include "Solver/VariableConvertor.hpp"
#include "Solver/ViscousFlux.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Concept.hpp"
#include "Utils/Constant.hpp"
#include "Utils/Enum.hpp"

namespace SubrosaDG {

template <typename ElementTrait, typename SimulationControl>
inline void ElementSolverBase<ElementTrait, SimulationControl>::calculateElementArtificialViscosity(
    const ElementMesh<ElementTrait>& element_mesh, const Real empirical_tolerance,
    const Real artificial_viscosity_factor) {
  [[maybe_unused]] constexpr int kBasisFunctionNumber{
      getElementBasisFunctionNumber<ElementTrait::kElementType, SimulationControl::kPolynomialOrder - 1>()};
  constexpr Real kPolynomialOrderArtificialViscosityTolerance{
      getPolynomialOrderArtificialViscosityTolerance<SimulationControl::kPolynomialOrder>()};
  Eigen::Vector<Real, ElementTrait::kQuadratureNumber> variable_density_high_order;
  Eigen::Vector<Real, ElementTrait::kQuadratureNumber> variable_density_all_order;
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto)                                   \
    shared(Eigen::all, Eigen::Dynamic, element_mesh, empirical_tolerance, artificial_viscosity_factor, \
               kBasisFunctionNumber,                                                                   \
               kPolynomialOrderArtificialViscosityTolerance) private(variable_density_high_order,      \
                                                                         variable_density_all_order)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < element_mesh.number_; i++) {
    variable_density_all_order.noalias() = element_mesh.basis_function_.modal_value_ *
                                           this->element_(i).variable_basis_function_coefficient_(1).row(0).transpose();
    if constexpr (SimulationControl::kPolynomialOrder == 1) {
      variable_density_high_order.noalias() = variable_density_all_order;
    } else {
      variable_density_high_order.noalias() =
          element_mesh.basis_function_.modal_value_(
              Eigen::all, Eigen::seq(kBasisFunctionNumber, ElementTrait::kBasisFunctionNumber - 1)) *
          this->element_(i)
              .variable_basis_function_coefficient_(1)(
                  0, Eigen::seq(kBasisFunctionNumber, ElementTrait::kBasisFunctionNumber - 1))
              .transpose();
    }
    // NOTE: http://persson.berkeley.edu/pub/persson13transient_shocks.pdf
    const Real shock_scale =
        std::log10((variable_density_high_order.transpose() *
                    (variable_density_high_order.array() * element_mesh.element_(i).jacobian_determinant_.array() *
                     element_mesh.quadrature_.weight_.array())
                        .matrix())
                       .sum() /
                   (variable_density_all_order.transpose() *
                    (variable_density_all_order.array() * element_mesh.element_(i).jacobian_determinant_.array() *
                     element_mesh.quadrature_.weight_.array())
                        .matrix())
                       .sum());
    if (shock_scale < kPolynomialOrderArtificialViscosityTolerance - empirical_tolerance) [[likely]] {
      this->element_(i).variable_artificial_viscosity_.fill(0.0_r);
    } else if (shock_scale > kPolynomialOrderArtificialViscosityTolerance + empirical_tolerance) {
      this->element_(i).variable_artificial_viscosity_.fill(
          artificial_viscosity_factor * (element_mesh.element_(i).inner_radius_ / SimulationControl::kPolynomialOrder));
    } else {
      this->element_(i).variable_artificial_viscosity_.fill(
          artificial_viscosity_factor * (element_mesh.element_(i).inner_radius_ / SimulationControl::kPolynomialOrder) *
          (1.0_r + std::sin(kPi * (shock_scale - kPolynomialOrderArtificialViscosityTolerance) /
                            (2.0_r * empirical_tolerance))) /
          2.0_r);
    }
  }
}

template <typename ElementTrait, typename SimulationControl>
inline void ElementSolverBase<ElementTrait, SimulationControl>::maxElementArtificialViscosity(
    const ElementMesh<ElementTrait>& element_mesh, Eigen::Vector<Real, Eigen::Dynamic>& node_artificial_viscosity) {
  Eigen::Vector<Real, Eigen::Dynamic> local_node_artificial_viscosity(node_artificial_viscosity.size());
  local_node_artificial_viscosity.setZero();
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel default(none) shared(element_mesh, node_artificial_viscosity) \
    firstprivate(local_node_artificial_viscosity)
#endif  // SUBROSA_DG_DEVELOP
  {
#ifndef SUBROSA_DG_DEVELOP
#pragma omp for schedule(nonmonotonic : auto)
#endif  // SUBROSA_DG_DEVELOP
    for (Isize i = 0; i < element_mesh.number_; i++) {
      for (Isize j = 0; j < ElementTrait::kBasicNodeNumber; j++) {
        local_node_artificial_viscosity(element_mesh.element_(i).node_tag_(j) - 1) =
            std::max(local_node_artificial_viscosity(element_mesh.element_(i).node_tag_(j) - 1),
                     this->element_(i).variable_artificial_viscosity_(j));
      }
    }
#ifndef SUBROSA_DG_DEVELOP
#pragma omp critical
#endif  // SUBROSA_DG_DEVELOP
    { node_artificial_viscosity = node_artificial_viscosity.cwiseMax(local_node_artificial_viscosity); }
  }
}

template <typename ElementTrait, typename SimulationControl>
inline void ElementSolverBase<ElementTrait, SimulationControl>::storeElementArtificialViscosity(
    const ElementMesh<ElementTrait>& element_mesh,
    const Eigen::Vector<Real, Eigen::Dynamic>& node_artificial_viscosity) {
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto) \
    shared(Eigen::all, element_mesh, node_artificial_viscosity)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < element_mesh.number_; i++) {
    for (Isize j = 0; j < ElementTrait::kBasicNodeNumber; j++) {
      this->element_(i).variable_artificial_viscosity_(j) =
          node_artificial_viscosity(element_mesh.element_(i).node_tag_(j) - 1);
    }
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::calculateArtificialViscosity(const Mesh<SimulationControl>& mesh) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.calculateElementArtificialViscosity(mesh.line_, this->empirical_tolerance_,
                                                    this->artificial_viscosity_factor_);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.calculateElementArtificialViscosity(mesh.triangle_, this->empirical_tolerance_,
                                                          this->artificial_viscosity_factor_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.calculateElementArtificialViscosity(mesh.quadrangle_, this->empirical_tolerance_,
                                                            this->artificial_viscosity_factor_);
    }
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.calculateElementArtificialViscosity(mesh.tetrahedron_, this->empirical_tolerance_,
                                                             this->artificial_viscosity_factor_);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.calculateElementArtificialViscosity(mesh.pyramid_, this->empirical_tolerance_,
                                                         this->artificial_viscosity_factor_);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.calculateElementArtificialViscosity(mesh.hexahedron_, this->empirical_tolerance_,
                                                            this->artificial_viscosity_factor_);
    }
  }
  this->node_artificial_viscosity_.setZero();
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.maxElementArtificialViscosity(mesh.line_, this->node_artificial_viscosity_);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.maxElementArtificialViscosity(mesh.triangle_, this->node_artificial_viscosity_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.maxElementArtificialViscosity(mesh.quadrangle_, this->node_artificial_viscosity_);
    }
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.maxElementArtificialViscosity(mesh.tetrahedron_, this->node_artificial_viscosity_);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.maxElementArtificialViscosity(mesh.pyramid_, this->node_artificial_viscosity_);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.maxElementArtificialViscosity(mesh.hexahedron_, this->node_artificial_viscosity_);
    }
  }
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.storeElementArtificialViscosity(mesh.line_, this->node_artificial_viscosity_);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.storeElementArtificialViscosity(mesh.triangle_, this->node_artificial_viscosity_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.storeElementArtificialViscosity(mesh.quadrangle_, this->node_artificial_viscosity_);
    }
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.storeElementArtificialViscosity(mesh.tetrahedron_, this->node_artificial_viscosity_);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.storeElementArtificialViscosity(mesh.pyramid_, this->node_artificial_viscosity_);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.storeElementArtificialViscosity(mesh.hexahedron_, this->node_artificial_viscosity_);
    }
  }
}

template <typename ElementTrait, typename SimulationControl>
inline void ElementSolver<ElementTrait, SimulationControl, EquationModelEnum::Euler>::calculateElementQuadrature(
    const ElementMesh<ElementTrait>& element_mesh, [[maybe_unused]] const SourceTerm<SimulationControl>& source_term,
    const PhysicalModel<SimulationControl>& physical_model) {
  ElementVariable<ElementTrait, SimulationControl> quadrature_node_variable;
  ElementVariableGradient<ElementTrait, SimulationControl> quadrature_node_variable_volumn_gradient;
  FluxVariable<SimulationControl> convective_raw_flux;
  FluxVariable<SimulationControl> artificial_viscous_raw_flux;
  [[maybe_unused]] FluxNormalVariable<SimulationControl> source_flux;
  Eigen::Vector<Real, ElementTrait::kQuadratureNumber> quadrature_node_artificial_viscosity;
  Eigen::Matrix<Real, ElementTrait::kDimension, ElementTrait::kDimension> quadrature_node_jacobian_transpose_inverse;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, SimulationControl::kDimension>
      quadrature_node_temporary_variable;
  [[maybe_unused]] Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>
      quadrature_node_source_temporary_variable;
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto) shared(                              \
        Eigen::all, Eigen::fix<SimulationControl::kDimension>, Eigen::Dynamic, element_mesh, source_term, \
            physical_model) private(quadrature_node_variable, quadrature_node_variable_volumn_gradient,   \
                                        convective_raw_flux, artificial_viscous_raw_flux, source_flux,    \
                                        quadrature_node_artificial_viscosity,                             \
                                        quadrature_node_jacobian_transpose_inverse,                       \
                                        quadrature_node_temporary_variable, quadrature_node_source_temporary_variable)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < element_mesh.number_; i++) {
    quadrature_node_variable.get(element_mesh, *this, i);
    quadrature_node_variable.calculateComputationalFromConserved(physical_model);
    quadrature_node_variable_volumn_gradient.template get<ViscousFluxEnum::None>(element_mesh, *this, i);
    if constexpr (SimulationControl::kShockCapturing == ShockCapturingEnum::ArtificialViscosity) {
      quadrature_node_artificial_viscosity.noalias() =
          element_mesh.basis_function_.nodal_value_ * this->element_(i).variable_artificial_viscosity_;
    }
    for (Isize j = 0; j < ElementTrait::kQuadratureNumber; j++) {
      calculateConvectiveRawFlux(quadrature_node_variable, convective_raw_flux, j);
      if constexpr (SimulationControl::kShockCapturing == ShockCapturingEnum::ArtificialViscosity) {
        calculateArtificialViscousRawFlux(quadrature_node_artificial_viscosity(j),
                                          quadrature_node_variable_volumn_gradient, artificial_viscous_raw_flux, j);
      }
      quadrature_node_jacobian_transpose_inverse.noalias() =
          element_mesh.element_(i).jacobian_transpose_inverse_.col(j).reshaped(ElementTrait::kDimension,
                                                                               ElementTrait::kDimension);
      if constexpr (SimulationControl::kShockCapturing == ShockCapturingEnum::ArtificialViscosity) {
        quadrature_node_temporary_variable.noalias() =
            (convective_raw_flux.variable_.transpose() - artificial_viscous_raw_flux.variable_.transpose()) *
            quadrature_node_jacobian_transpose_inverse * element_mesh.element_(i).jacobian_determinant_(j) *
            element_mesh.quadrature_.weight_(j);
      } else {
        quadrature_node_temporary_variable.noalias() =
            convective_raw_flux.variable_.transpose() * quadrature_node_jacobian_transpose_inverse *
            element_mesh.element_(i).jacobian_determinant_(j) * element_mesh.quadrature_.weight_(j);
      }

      this->element_(i).variable_quadrature_(
          Eigen::all, Eigen::seqN(j * SimulationControl::kDimension, Eigen::fix<SimulationControl::kDimension>)) =
          quadrature_node_temporary_variable;
      if constexpr (SimulationControl::kSourceTerm != SourceTermEnum::None) {
        source_term.calculateSourceTerm(quadrature_node_variable, source_flux, j);
        quadrature_node_source_temporary_variable.noalias() = source_flux.normal_variable_ *
                                                              element_mesh.element_(i).jacobian_determinant_(j) *
                                                              element_mesh.quadrature_.weight_(j);
        this->element_(i).variable_source_quadrature_.col(j) = quadrature_node_source_temporary_variable;
      }
    }
  }
}

template <typename ElementTrait, typename SimulationControl>
inline void ElementSolver<ElementTrait, SimulationControl, EquationModelEnum::NavierStokes>::calculateElementQuadrature(
    const ElementMesh<ElementTrait>& element_mesh, [[maybe_unused]] const SourceTerm<SimulationControl>& source_term,
    const PhysicalModel<SimulationControl>& physical_model) {
  ElementVariable<ElementTrait, SimulationControl> quadrature_node_variable;
  ElementVariableGradient<ElementTrait, SimulationControl> quadrature_node_variable_gradient;
  ElementVariableGradient<ElementTrait, SimulationControl> quadrature_node_variable_volumn_gradient;
  FluxVariable<SimulationControl> convective_raw_flux;
  FluxVariable<SimulationControl> viscous_raw_flux;
  FluxVariable<SimulationControl> artificial_viscous_raw_flux;
  [[maybe_unused]] FluxNormalVariable<SimulationControl> source_flux;
  Eigen::Vector<Real, ElementTrait::kQuadratureNumber> quadrature_node_artificial_viscosity;
  Eigen::Matrix<Real, ElementTrait::kDimension, ElementTrait::kDimension> quadrature_node_jacobian_transpose_inverse;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber, SimulationControl::kDimension>
      quadrature_node_temporary_variable;
  [[maybe_unused]] Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>
      quadrature_node_source_temporary_variable;
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto) shared(                              \
        Eigen::all, Eigen::fix<SimulationControl::kDimension>, Eigen::Dynamic, element_mesh, source_term, \
            physical_model) private(quadrature_node_variable, quadrature_node_variable_gradient,          \
                                        quadrature_node_variable_volumn_gradient, convective_raw_flux,    \
                                        viscous_raw_flux, artificial_viscous_raw_flux, source_flux,       \
                                        quadrature_node_artificial_viscosity,                             \
                                        quadrature_node_jacobian_transpose_inverse,                       \
                                        quadrature_node_temporary_variable, quadrature_node_source_temporary_variable)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < element_mesh.number_; i++) {
    quadrature_node_variable.get(element_mesh, *this, i);
    quadrature_node_variable.calculateComputationalFromConserved(physical_model);
    quadrature_node_variable_gradient.template get<SimulationControl::kViscousFlux>(element_mesh, *this, i);
    quadrature_node_variable_gradient.calculatePrimitiveFromConserved(physical_model, quadrature_node_variable);
    quadrature_node_variable_volumn_gradient.template get<ViscousFluxEnum::None>(element_mesh, *this, i);
    if constexpr (SimulationControl::kShockCapturing == ShockCapturingEnum::ArtificialViscosity) {
      quadrature_node_artificial_viscosity.noalias() =
          element_mesh.basis_function_.nodal_value_ * this->element_(i).variable_artificial_viscosity_;
    }
    for (Isize j = 0; j < ElementTrait::kQuadratureNumber; j++) {
      calculateConvectiveRawFlux(quadrature_node_variable, convective_raw_flux, j);
      calculateViscousRawFlux(physical_model, quadrature_node_variable, quadrature_node_variable_gradient,
                              viscous_raw_flux, j);
      if constexpr (SimulationControl::kShockCapturing == ShockCapturingEnum::ArtificialViscosity) {
        calculateArtificialViscousRawFlux(quadrature_node_artificial_viscosity(j), quadrature_node_variable_gradient,
                                          artificial_viscous_raw_flux, j);
      }
      quadrature_node_jacobian_transpose_inverse.noalias() =
          element_mesh.element_(i).jacobian_transpose_inverse_.col(j).reshaped(ElementTrait::kDimension,
                                                                               ElementTrait::kDimension);
      if constexpr (SimulationControl::kShockCapturing == ShockCapturingEnum::ArtificialViscosity) {
        quadrature_node_temporary_variable.noalias() =
            (convective_raw_flux.variable_.transpose() - viscous_raw_flux.variable_.transpose() -
             artificial_viscous_raw_flux.variable_.transpose()) *
            quadrature_node_jacobian_transpose_inverse * element_mesh.element_(i).jacobian_determinant_(j) *
            element_mesh.quadrature_.weight_(j);
      } else {
        quadrature_node_temporary_variable.noalias() =
            (convective_raw_flux.variable_.transpose() - viscous_raw_flux.variable_.transpose()) *
            quadrature_node_jacobian_transpose_inverse * element_mesh.element_(i).jacobian_determinant_(j) *
            element_mesh.quadrature_.weight_(j);
      }
      this->element_(i).variable_quadrature_(
          Eigen::all, Eigen::seqN(j * SimulationControl::kDimension, Eigen::fix<SimulationControl::kDimension>)) =
          quadrature_node_temporary_variable;
      if constexpr (SimulationControl::kSourceTerm != SourceTermEnum::None) {
        source_term.calculateSourceTerm(quadrature_node_variable, source_flux, j);
        quadrature_node_source_temporary_variable.noalias() = source_flux.normal_variable_ *
                                                              element_mesh.element_(i).jacobian_determinant_(j) *
                                                              element_mesh.quadrature_.weight_(j);
        this->element_(i).variable_source_quadrature_.col(j) = quadrature_node_source_temporary_variable;
      }
    }
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::calculateQuadrature(
    const Mesh<SimulationControl>& mesh, [[maybe_unused]] const SourceTerm<SimulationControl>& source_term,
    const PhysicalModel<SimulationControl>& physical_model) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.calculateElementQuadrature(mesh.line_, source_term, physical_model);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.calculateElementQuadrature(mesh.triangle_, source_term, physical_model);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.calculateElementQuadrature(mesh.quadrangle_, source_term, physical_model);
    }
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.calculateElementQuadrature(mesh.tetrahedron_, source_term, physical_model);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.calculateElementQuadrature(mesh.pyramid_, source_term, physical_model);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.calculateElementQuadrature(mesh.hexahedron_, source_term, physical_model);
    }
  }
}

template <typename ElementTrait, typename SimulationControl>
inline void ElementSolverBase<ElementTrait, SimulationControl>::calculateElementGardientQuadrature(
    const ElementMesh<ElementTrait>& element_mesh) {
  ElementVariable<ElementTrait, SimulationControl> quadrature_node_variable;
  Eigen::Matrix<Real, ElementTrait::kDimension, ElementTrait::kDimension> quadrature_node_jacobian_transpose_inverse;
  Eigen::Matrix<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension,
                SimulationControl::kDimension>
      quadrature_node_temporary_variable;
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto)                                     \
    shared(Eigen::all, Eigen::fix<SimulationControl::kDimension>, Eigen::Dynamic, element_mesh) private( \
            quadrature_node_variable, quadrature_node_jacobian_transpose_inverse, quadrature_node_temporary_variable)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < element_mesh.number_; i++) {
    quadrature_node_variable.get(element_mesh, *this, i);
    for (Isize j = 0; j < ElementTrait::kQuadratureNumber; j++) {
      quadrature_node_jacobian_transpose_inverse.noalias() =
          element_mesh.element_(i).jacobian_transpose_inverse_.col(j).reshaped(ElementTrait::kDimension,
                                                                               ElementTrait::kDimension);
      for (Isize k = 0; k < SimulationControl::kConservedVariableNumber; k++) {
        quadrature_node_temporary_variable(
            Eigen::seqN(k * SimulationControl::kDimension, Eigen::fix<SimulationControl::kDimension>), Eigen::all) =
            quadrature_node_variable.conserved_(k, j) * quadrature_node_jacobian_transpose_inverse *
            element_mesh.element_(i).jacobian_determinant_(j) * element_mesh.quadrature_.weight_(j);
      }
      this->element_(i).variable_gradient_volume_quadrature_(
          Eigen::all, Eigen::seqN(j * SimulationControl::kDimension, Eigen::fix<SimulationControl::kDimension>)) =
          quadrature_node_temporary_variable;
    }
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::calculateGardientQuadrature(const Mesh<SimulationControl>& mesh) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.calculateElementGardientQuadrature(mesh.line_);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.calculateElementGardientQuadrature(mesh.triangle_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.calculateElementGardientQuadrature(mesh.quadrangle_);
    }
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.calculateElementGardientQuadrature(mesh.tetrahedron_);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.calculateElementGardientQuadrature(mesh.pyramid_);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.calculateElementGardientQuadrature(mesh.hexahedron_);
    }
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
[[nodiscard]] inline Isize AdjacencyElementSolverBase<AdjacencyElementTrait, SimulationControl>::
    getAdjacencyParentElementAccumulateAdjacencyQuadratureNumber([[maybe_unused]] const Isize parent_gmsh_type_number,
                                                                 const Isize adjacency_sequence_in_parent) {
  if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
    constexpr std::array<int, LineTrait<SimulationControl::kPolynomialOrder>::kAdjacencyNumber + 1>
        kElementAccumulateAdjacencyQuadratureNumber{
            getElementAccumulateAdjacencyQuadratureNumber<ElementEnum::Line, SimulationControl::kPolynomialOrder>()};
    return kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent)];
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
    if (parent_gmsh_type_number == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      constexpr std::array<int, TriangleTrait<SimulationControl::kPolynomialOrder>::kAdjacencyNumber + 1>
          kElementAccumulateAdjacencyQuadratureNumber{
              getElementAccumulateAdjacencyQuadratureNumber<ElementEnum::Triangle,
                                                            SimulationControl::kPolynomialOrder>()};
      return kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent)];
    }
    if (parent_gmsh_type_number == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      constexpr std::array<int, QuadrangleTrait<SimulationControl::kPolynomialOrder>::kAdjacencyNumber + 1>
          kElementAccumulateAdjacencyQuadratureNumber{
              getElementAccumulateAdjacencyQuadratureNumber<ElementEnum::Quadrangle,
                                                            SimulationControl::kPolynomialOrder>()};
      return kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent)];
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
    if (parent_gmsh_type_number == TetrahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      constexpr std::array<int, TetrahedronTrait<SimulationControl::kPolynomialOrder>::kAdjacencyNumber + 1>
          kElementAccumulateAdjacencyQuadratureNumber{
              getElementAccumulateAdjacencyQuadratureNumber<ElementEnum::Tetrahedron,
                                                            SimulationControl::kPolynomialOrder>()};
      return kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent)];
    }
    if (parent_gmsh_type_number == PyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      constexpr std::array<int, PyramidTrait<SimulationControl::kPolynomialOrder>::kAdjacencyNumber + 1>
          kElementAccumulateAdjacencyQuadratureNumber{
              getElementAccumulateAdjacencyQuadratureNumber<ElementEnum::Pyramid,
                                                            SimulationControl::kPolynomialOrder>()};
      return kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent)];
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
    if (parent_gmsh_type_number == PyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      constexpr std::array<int, PyramidTrait<SimulationControl::kPolynomialOrder>::kAdjacencyNumber + 1>
          kElementAccumulateAdjacencyQuadratureNumber{
              getElementAccumulateAdjacencyQuadratureNumber<ElementEnum::Pyramid,
                                                            SimulationControl::kPolynomialOrder>()};
      return kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent)];
    }
    if (parent_gmsh_type_number == HexahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      constexpr std::array<int, HexahedronTrait<SimulationControl::kPolynomialOrder>::kAdjacencyNumber + 1>
          kElementAccumulateAdjacencyQuadratureNumber{
              getElementAccumulateAdjacencyQuadratureNumber<ElementEnum::Hexahedron,
                                                            SimulationControl::kPolynomialOrder>()};
      return kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent)];
    }
  }
  return -1;
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementSolverBase<AdjacencyElementTrait, SimulationControl>::storeAdjacencyElementNodeQuadrature(
    const Isize parent_gmsh_type_number, const Isize parent_index, const Isize quadrature_node_sequence_in_parent,
    const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& quadrature_node_temporary_variable,
    Solver<SimulationControl>& solver) {
  if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
    solver.line_.element_(parent_index).variable_adjacency_quadrature_.col(quadrature_node_sequence_in_parent) =
        quadrature_node_temporary_variable;
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
    if (parent_gmsh_type_number == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.triangle_.element_(parent_index).variable_adjacency_quadrature_.col(quadrature_node_sequence_in_parent) =
          quadrature_node_temporary_variable;
    } else if (parent_gmsh_type_number == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.quadrangle_.element_(parent_index).variable_adjacency_quadrature_.col(quadrature_node_sequence_in_parent) =
          quadrature_node_temporary_variable;
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
    if (parent_gmsh_type_number == TetrahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.tetrahedron_.element_(parent_index)
          .variable_adjacency_quadrature_.col(quadrature_node_sequence_in_parent) = quadrature_node_temporary_variable;
    } else if (parent_gmsh_type_number == PyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.pyramid_.element_(parent_index).variable_adjacency_quadrature_.col(quadrature_node_sequence_in_parent) =
          quadrature_node_temporary_variable;
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
    if (parent_gmsh_type_number == PyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.pyramid_.element_(parent_index).variable_adjacency_quadrature_.col(quadrature_node_sequence_in_parent) =
          quadrature_node_temporary_variable;
    } else if (parent_gmsh_type_number == HexahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.hexahedron_.element_(parent_index).variable_adjacency_quadrature_.col(quadrature_node_sequence_in_parent) =
          quadrature_node_temporary_variable;
    }
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void
AdjacencyElementSolverBase<AdjacencyElementTrait, SimulationControl>::storeAdjacencyElementNodeVolumeGardientQuadrature(
    const Isize parent_gmsh_type_number, const Isize parent_index, const Isize quadrature_node_sequence_in_parent,
    const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>&
        quadrature_node_temporary_variable,
    Solver<SimulationControl>& solver) {
  if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
    solver.line_.element_(parent_index)
        .variable_gradient_volume_adjacency_quadrature_.col(quadrature_node_sequence_in_parent) =
        quadrature_node_temporary_variable;
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
    if (parent_gmsh_type_number == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.triangle_.element_(parent_index)
          .variable_gradient_volume_adjacency_quadrature_.col(quadrature_node_sequence_in_parent) =
          quadrature_node_temporary_variable;
    } else if (parent_gmsh_type_number == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.quadrangle_.element_(parent_index)
          .variable_gradient_volume_adjacency_quadrature_.col(quadrature_node_sequence_in_parent) =
          quadrature_node_temporary_variable;
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
    if (parent_gmsh_type_number == TetrahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.tetrahedron_.element_(parent_index)
          .variable_gradient_volume_adjacency_quadrature_.col(quadrature_node_sequence_in_parent) =
          quadrature_node_temporary_variable;
    } else if (parent_gmsh_type_number == PyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.pyramid_.element_(parent_index)
          .variable_gradient_volume_adjacency_quadrature_.col(quadrature_node_sequence_in_parent) =
          quadrature_node_temporary_variable;
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
    if (parent_gmsh_type_number == PyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.pyramid_.element_(parent_index)
          .variable_gradient_volume_adjacency_quadrature_.col(quadrature_node_sequence_in_parent) =
          quadrature_node_temporary_variable;
    } else if (parent_gmsh_type_number == HexahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.hexahedron_.element_(parent_index)
          .variable_gradient_volume_adjacency_quadrature_.col(quadrature_node_sequence_in_parent) =
          quadrature_node_temporary_variable;
    }
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementSolverBase<AdjacencyElementTrait, SimulationControl>::
    storeAdjacencyElementNodeInterfaceGardientQuadrature(
        const Isize parent_gmsh_type_number, const Isize parent_index, const Isize quadrature_node_sequence_in_parent,
        const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>&
            quadrature_node_temporary_variable,
        Solver<SimulationControl>& solver) {
  if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
    solver.line_.element_(parent_index)
        .variable_gradient_interface_adjacency_quadrature_.col(quadrature_node_sequence_in_parent) =
        quadrature_node_temporary_variable;
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
    if (parent_gmsh_type_number == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.triangle_.element_(parent_index)
          .variable_gradient_interface_adjacency_quadrature_.col(quadrature_node_sequence_in_parent) =
          quadrature_node_temporary_variable;
    } else if (parent_gmsh_type_number == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.quadrangle_.element_(parent_index)
          .variable_gradient_interface_adjacency_quadrature_.col(quadrature_node_sequence_in_parent) =
          quadrature_node_temporary_variable;
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
    if (parent_gmsh_type_number == TetrahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.tetrahedron_.element_(parent_index)
          .variable_gradient_interface_adjacency_quadrature_.col(quadrature_node_sequence_in_parent) =
          quadrature_node_temporary_variable;
    } else if (parent_gmsh_type_number == PyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.pyramid_.element_(parent_index)
          .variable_gradient_interface_adjacency_quadrature_.col(quadrature_node_sequence_in_parent) =
          quadrature_node_temporary_variable;
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
    if (parent_gmsh_type_number == PyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.pyramid_.element_(parent_index)
          .variable_gradient_interface_adjacency_quadrature_.col(quadrature_node_sequence_in_parent) =
          quadrature_node_temporary_variable;
    } else if (parent_gmsh_type_number == HexahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      solver.hexahedron_.element_(parent_index)
          .variable_gradient_interface_adjacency_quadrature_.col(quadrature_node_sequence_in_parent) =
          quadrature_node_temporary_variable;
    }
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void
AdjacencyElementSolverBase<AdjacencyElementTrait, SimulationControl>::calculateAdjacencyElementArtificialViscosity(
    const Mesh<SimulationControl>& mesh, const Solver<SimulationControl>& solver,
    Eigen::Vector<Real, AdjacencyElementTrait::kQuadratureNumber>& quadrature_node_artificial_viscosity,
    const Isize parent_gmsh_type_number, const Isize parent_index_each_type, const Isize adjacency_sequence_in_parent) {
  if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Point) {
    constexpr std::array<int, LineTrait<SimulationControl::kPolynomialOrder>::kAdjacencyNumber + 1>
        kElementAccumulateAdjacencyQuadratureNumber{
            getElementAccumulateAdjacencyQuadratureNumber<ElementEnum::Line, SimulationControl::kPolynomialOrder>()};
    quadrature_node_artificial_viscosity.noalias() =
        mesh.line_.basis_function_.nodal_adjacency_value_(
            Eigen::seq(
                kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent)],
                kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent) + 1] - 1),
            Eigen::all) *
        solver.line_.element_(parent_index_each_type).variable_artificial_viscosity_;
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Line) {
    if (parent_gmsh_type_number == TriangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      constexpr std::array<int, TriangleTrait<SimulationControl::kPolynomialOrder>::kAdjacencyNumber + 1>
          kElementAccumulateAdjacencyQuadratureNumber{
              getElementAccumulateAdjacencyQuadratureNumber<ElementEnum::Triangle,
                                                            SimulationControl::kPolynomialOrder>()};
      quadrature_node_artificial_viscosity.noalias() =
          mesh.triangle_.basis_function_.nodal_adjacency_value_(
              Eigen::seq(
                  kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent)],
                  kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent) + 1] -
                      1),
              Eigen::all) *
          solver.triangle_.element_(parent_index_each_type).variable_artificial_viscosity_;
    } else if (parent_gmsh_type_number == QuadrangleTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      constexpr std::array<int, QuadrangleTrait<SimulationControl::kPolynomialOrder>::kAdjacencyNumber + 1>
          kElementAccumulateAdjacencyQuadratureNumber{
              getElementAccumulateAdjacencyQuadratureNumber<ElementEnum::Quadrangle,
                                                            SimulationControl::kPolynomialOrder>()};
      quadrature_node_artificial_viscosity.noalias() =
          mesh.quadrangle_.basis_function_.nodal_adjacency_value_(
              Eigen::seq(
                  kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent)],
                  kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent) + 1] -
                      1),
              Eigen::all) *
          solver.quadrangle_.element_(parent_index_each_type).variable_artificial_viscosity_;
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Triangle) {
    if (parent_gmsh_type_number == TetrahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      constexpr std::array<int, TetrahedronTrait<SimulationControl::kPolynomialOrder>::kAdjacencyNumber + 1>
          kElementAccumulateAdjacencyQuadratureNumber{
              getElementAccumulateAdjacencyQuadratureNumber<ElementEnum::Tetrahedron,
                                                            SimulationControl::kPolynomialOrder>()};
      quadrature_node_artificial_viscosity.noalias() =
          mesh.tetrahedron_.basis_function_.nodal_adjacency_value_(
              Eigen::seq(
                  kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent)],
                  kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent) + 1] -
                      1),
              Eigen::all) *
          solver.tetrahedron_.element_(parent_index_each_type).variable_artificial_viscosity_;
    } else if (parent_gmsh_type_number == PyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      constexpr std::array<int, PyramidTrait<SimulationControl::kPolynomialOrder>::kAdjacencyNumber + 1>
          kElementAccumulateAdjacencyQuadratureNumber{
              getElementAccumulateAdjacencyQuadratureNumber<ElementEnum::Pyramid,
                                                            SimulationControl::kPolynomialOrder>()};
      quadrature_node_artificial_viscosity.noalias() =
          mesh.pyramid_.basis_function_.nodal_adjacency_value_(
              Eigen::seq(
                  kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent)],
                  kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent) + 1] -
                      1),
              Eigen::all) *
          solver.pyramid_.element_(parent_index_each_type).variable_artificial_viscosity_;
    }
  } else if constexpr (AdjacencyElementTrait::kElementType == ElementEnum::Quadrangle) {
    if (parent_gmsh_type_number == PyramidTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      constexpr std::array<int, PyramidTrait<SimulationControl::kPolynomialOrder>::kAdjacencyNumber + 1>
          kElementAccumulateAdjacencyQuadratureNumber{
              getElementAccumulateAdjacencyQuadratureNumber<ElementEnum::Pyramid,
                                                            SimulationControl::kPolynomialOrder>()};
      quadrature_node_artificial_viscosity.noalias() =
          mesh.pyramid_.basis_function_.nodal_adjacency_value_(
              Eigen::seq(
                  kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent)],
                  kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent) + 1] -
                      1),
              Eigen::all) *
          solver.pyramid_.element_(parent_index_each_type).variable_artificial_viscosity_;
    } else if (parent_gmsh_type_number == HexahedronTrait<SimulationControl::kPolynomialOrder>::kGmshTypeNumber) {
      constexpr std::array<int, HexahedronTrait<SimulationControl::kPolynomialOrder>::kAdjacencyNumber + 1>
          kElementAccumulateAdjacencyQuadratureNumber{
              getElementAccumulateAdjacencyQuadratureNumber<ElementEnum::Hexahedron,
                                                            SimulationControl::kPolynomialOrder>()};
      quadrature_node_artificial_viscosity.noalias() =
          mesh.hexahedron_.basis_function_.nodal_adjacency_value_(
              Eigen::seq(
                  kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent)],
                  kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(adjacency_sequence_in_parent) + 1] -
                      1),
              Eigen::all) *
          solver.hexahedron_.element_(parent_index_each_type).variable_artificial_viscosity_;
    }
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl, EquationModelEnum::Euler>::
    calculateInteriorAdjacencyElementQuadrature(const Mesh<SimulationControl>& mesh,
                                                const PhysicalModel<SimulationControl>& physical_model,
                                                Solver<SimulationControl>& solver) {
  const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh =
      mesh.*(std::remove_reference<decltype(mesh)>::type::template getAdjacencyElement<AdjacencyElementTrait>());
  AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl> left_quadrature_node_variable;
  AdjacencyElementVariableGradient<AdjacencyElementTrait, SimulationControl>
      left_quadrature_node_variable_volumn_gradient;
  AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl> right_quadrature_node_variable;
  AdjacencyElementVariableGradient<AdjacencyElementTrait, SimulationControl>
      right_quadrature_node_variable_volumn_gradient;
  Flux<SimulationControl> convective_flux;
  Flux<SimulationControl> artificial_viscous_flux;
  Eigen::Vector<Real, AdjacencyElementTrait::kQuadratureNumber> left_quadrature_node_artificial_viscosity;
  Eigen::Vector<Real, AdjacencyElementTrait::kQuadratureNumber> right_quadrature_node_artificial_viscosity;
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> quadrature_node_temporary_variable;
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto)                                             \
    shared(Eigen::Dynamic, mesh, adjacency_element_mesh, physical_model, solver) private(                        \
            left_quadrature_node_variable, left_quadrature_node_variable_volumn_gradient,                        \
                right_quadrature_node_variable, right_quadrature_node_variable_volumn_gradient, convective_flux, \
                artificial_viscous_flux, left_quadrature_node_artificial_viscosity,                              \
                right_quadrature_node_artificial_viscosity, quadrature_node_temporary_variable)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < adjacency_element_mesh.interior_number_; i++) {
    const Isize adjacency_right_rotation = adjacency_element_mesh.element_(i).adjacency_right_rotation_;
    const std::array<int, AdjacencyElementTrait::kQuadratureNumber> adjacency_element_quadrature_sequence{
        getAdjacencyElementQuadratureSequence<AdjacencyElementTrait::kElementType, SimulationControl::kPolynomialOrder>(
            static_cast<int>(adjacency_right_rotation))};
    const Eigen::Vector<Isize, 2>& parent_index_each_type = adjacency_element_mesh.element_(i).parent_index_each_type_;
    const Eigen::Vector<Isize, 2>& adjacency_sequence_in_parent =
        adjacency_element_mesh.element_(i).adjacency_sequence_in_parent_;
    const Eigen::Vector<Isize, 2>& parent_gmsh_type_number =
        adjacency_element_mesh.element_(i).parent_gmsh_type_number_;
    const Isize left_adjacency_accumulate_quadrature_number =
        this->getAdjacencyParentElementAccumulateAdjacencyQuadratureNumber(parent_gmsh_type_number(0),
                                                                           adjacency_sequence_in_parent(0));
    const Isize right_adjacency_accumulate_quadrature_number =
        this->getAdjacencyParentElementAccumulateAdjacencyQuadratureNumber(parent_gmsh_type_number(1),
                                                                           adjacency_sequence_in_parent(1));
    left_quadrature_node_variable.get(mesh, solver, parent_gmsh_type_number(0), parent_index_each_type(0),
                                      adjacency_sequence_in_parent(0));
    right_quadrature_node_variable.get(mesh, solver, parent_gmsh_type_number(1), parent_index_each_type(1),
                                       adjacency_sequence_in_parent(1));
    left_quadrature_node_variable.calculateComputationalFromConserved(physical_model);
    right_quadrature_node_variable.calculateComputationalFromConserved(physical_model);
    left_quadrature_node_variable_volumn_gradient.template get<ViscousFluxEnum::None>(
        mesh, solver, parent_gmsh_type_number(0), parent_index_each_type(0), adjacency_sequence_in_parent(0));
    right_quadrature_node_variable_volumn_gradient.template get<ViscousFluxEnum::None>(
        mesh, solver, parent_gmsh_type_number(1), parent_index_each_type(1), adjacency_sequence_in_parent(1));
    if constexpr (SimulationControl::kShockCapturing == ShockCapturingEnum::ArtificialViscosity) {
      this->calculateAdjacencyElementArtificialViscosity(mesh, solver, left_quadrature_node_artificial_viscosity,
                                                         parent_gmsh_type_number(0), parent_index_each_type(0),
                                                         adjacency_sequence_in_parent(0));
      this->calculateAdjacencyElementArtificialViscosity(mesh, solver, right_quadrature_node_artificial_viscosity,
                                                         parent_gmsh_type_number(1), parent_index_each_type(1),
                                                         adjacency_sequence_in_parent(1));
    }
    for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
      calculateConvectiveFlux(physical_model, adjacency_element_mesh.element_(i).normal_vector_.col(j),
                              left_quadrature_node_variable, right_quadrature_node_variable, convective_flux, j,
                              adjacency_element_quadrature_sequence[static_cast<Usize>(j)]);
      if constexpr (SimulationControl::kShockCapturing == ShockCapturingEnum::ArtificialViscosity) {
        calculateArtificialViscousFlux(
            adjacency_element_mesh.element_(i).normal_vector_.col(j), left_quadrature_node_artificial_viscosity(j),
            left_quadrature_node_variable_volumn_gradient, right_quadrature_node_artificial_viscosity(j),
            right_quadrature_node_variable_volumn_gradient, artificial_viscous_flux, j,
            adjacency_element_quadrature_sequence[static_cast<Usize>(j)]);
      }
      if constexpr (SimulationControl::kShockCapturing == ShockCapturingEnum::ArtificialViscosity) {
        quadrature_node_temporary_variable.noalias() =
            (convective_flux.result_.normal_variable_ - artificial_viscous_flux.result_.normal_variable_) *
            adjacency_element_mesh.element_(i).jacobian_determinant_(j) * adjacency_element_mesh.quadrature_.weight_(j);
      } else {
        quadrature_node_temporary_variable.noalias() = convective_flux.result_.normal_variable_ *
                                                       adjacency_element_mesh.element_(i).jacobian_determinant_(j) *
                                                       adjacency_element_mesh.quadrature_.weight_(j);
      }
      this->storeAdjacencyElementNodeQuadrature(parent_gmsh_type_number(0), parent_index_each_type(0),
                                                left_adjacency_accumulate_quadrature_number + j,
                                                quadrature_node_temporary_variable, solver);
      this->storeAdjacencyElementNodeQuadrature(
          parent_gmsh_type_number(1), parent_index_each_type(1),
          right_adjacency_accumulate_quadrature_number + adjacency_element_quadrature_sequence[static_cast<Usize>(j)],
          -quadrature_node_temporary_variable, solver);
    }
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl, EquationModelEnum::NavierStokes>::
    calculateInteriorAdjacencyElementQuadrature(const Mesh<SimulationControl>& mesh,
                                                const PhysicalModel<SimulationControl>& physical_model,
                                                Solver<SimulationControl>& solver) {
  const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh =
      mesh.*(std::remove_reference<decltype(mesh)>::type::template getAdjacencyElement<AdjacencyElementTrait>());
  AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl> left_quadrature_node_variable;
  AdjacencyElementVariableGradient<AdjacencyElementTrait, SimulationControl> left_quadrature_node_variable_gradient;
  AdjacencyElementVariableGradient<AdjacencyElementTrait, SimulationControl>
      left_quadrature_node_variable_volumn_gradient;
  AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl> right_quadrature_node_variable;
  AdjacencyElementVariableGradient<AdjacencyElementTrait, SimulationControl> right_quadrature_node_variable_gradient;
  AdjacencyElementVariableGradient<AdjacencyElementTrait, SimulationControl>
      right_quadrature_node_variable_volumn_gradient;
  Flux<SimulationControl> convective_flux;
  Flux<SimulationControl> viscous_flux;
  Flux<SimulationControl> artificial_viscous_flux;
  Eigen::Vector<Real, AdjacencyElementTrait::kQuadratureNumber> left_quadrature_node_artificial_viscosity;
  Eigen::Vector<Real, AdjacencyElementTrait::kQuadratureNumber> right_quadrature_node_artificial_viscosity;
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> quadrature_node_temporary_variable;
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto)                                               \
    shared(Eigen::Dynamic, mesh, adjacency_element_mesh, physical_model, solver) private(                          \
            left_quadrature_node_variable, left_quadrature_node_variable_gradient,                                 \
                left_quadrature_node_variable_volumn_gradient, right_quadrature_node_variable,                     \
                right_quadrature_node_variable_gradient, right_quadrature_node_variable_volumn_gradient,           \
                convective_flux, viscous_flux, artificial_viscous_flux, left_quadrature_node_artificial_viscosity, \
                right_quadrature_node_artificial_viscosity, quadrature_node_temporary_variable)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < adjacency_element_mesh.interior_number_; i++) {
    const Isize adjacency_right_rotation = adjacency_element_mesh.element_(i).adjacency_right_rotation_;
    const std::array<int, AdjacencyElementTrait::kQuadratureNumber> adjacency_element_quadrature_sequence{
        getAdjacencyElementQuadratureSequence<AdjacencyElementTrait::kElementType, SimulationControl::kPolynomialOrder>(
            static_cast<int>(adjacency_right_rotation))};
    const Eigen::Vector<Isize, 2>& parent_index_each_type = adjacency_element_mesh.element_(i).parent_index_each_type_;
    const Eigen::Vector<Isize, 2>& adjacency_sequence_in_parent =
        adjacency_element_mesh.element_(i).adjacency_sequence_in_parent_;
    const Eigen::Vector<Isize, 2>& parent_gmsh_type_number =
        adjacency_element_mesh.element_(i).parent_gmsh_type_number_;
    const Isize left_adjacency_accumulate_quadrature_number =
        this->getAdjacencyParentElementAccumulateAdjacencyQuadratureNumber(parent_gmsh_type_number(0),
                                                                           adjacency_sequence_in_parent(0));
    const Isize right_adjacency_accumulate_quadrature_number =
        this->getAdjacencyParentElementAccumulateAdjacencyQuadratureNumber(parent_gmsh_type_number(1),
                                                                           adjacency_sequence_in_parent(1));
    left_quadrature_node_variable.get(mesh, solver, parent_gmsh_type_number(0), parent_index_each_type(0),
                                      adjacency_sequence_in_parent(0));
    right_quadrature_node_variable.get(mesh, solver, parent_gmsh_type_number(1), parent_index_each_type(1),
                                       adjacency_sequence_in_parent(1));
    left_quadrature_node_variable.calculateComputationalFromConserved(physical_model);
    right_quadrature_node_variable.calculateComputationalFromConserved(physical_model);
    left_quadrature_node_variable_gradient.template get<SimulationControl::kViscousFlux>(
        mesh, solver, parent_gmsh_type_number(0), parent_index_each_type(0), adjacency_sequence_in_parent(0));
    right_quadrature_node_variable_gradient.template get<SimulationControl::kViscousFlux>(
        mesh, solver, parent_gmsh_type_number(1), parent_index_each_type(1), adjacency_sequence_in_parent(1));
    left_quadrature_node_variable_gradient.calculatePrimitiveFromConserved(physical_model,
                                                                           left_quadrature_node_variable);
    right_quadrature_node_variable_gradient.calculatePrimitiveFromConserved(physical_model,
                                                                            right_quadrature_node_variable);
    left_quadrature_node_variable_volumn_gradient.template get<ViscousFluxEnum::None>(
        mesh, solver, parent_gmsh_type_number(0), parent_index_each_type(0), adjacency_sequence_in_parent(0));
    right_quadrature_node_variable_volumn_gradient.template get<ViscousFluxEnum::None>(
        mesh, solver, parent_gmsh_type_number(1), parent_index_each_type(1), adjacency_sequence_in_parent(1));
    if constexpr (SimulationControl::kShockCapturing == ShockCapturingEnum::ArtificialViscosity) {
      this->calculateAdjacencyElementArtificialViscosity(mesh, solver, left_quadrature_node_artificial_viscosity,
                                                         parent_gmsh_type_number(0), parent_index_each_type(0),
                                                         adjacency_sequence_in_parent(0));
      this->calculateAdjacencyElementArtificialViscosity(mesh, solver, right_quadrature_node_artificial_viscosity,
                                                         parent_gmsh_type_number(1), parent_index_each_type(1),
                                                         adjacency_sequence_in_parent(1));
    }
    for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
      calculateConvectiveFlux(physical_model, adjacency_element_mesh.element_(i).normal_vector_.col(j),
                              left_quadrature_node_variable, right_quadrature_node_variable, convective_flux, j,
                              adjacency_element_quadrature_sequence[static_cast<Usize>(j)]);
      calculateViscousFlux(physical_model, adjacency_element_mesh.element_(i).normal_vector_.col(j),
                           left_quadrature_node_variable, left_quadrature_node_variable_gradient,
                           right_quadrature_node_variable, right_quadrature_node_variable_gradient, viscous_flux, j,
                           adjacency_element_quadrature_sequence[static_cast<Usize>(j)]);
      if constexpr (SimulationControl::kShockCapturing == ShockCapturingEnum::ArtificialViscosity) {
        calculateArtificialViscousFlux(
            adjacency_element_mesh.element_(i).normal_vector_.col(j), left_quadrature_node_artificial_viscosity(j),
            left_quadrature_node_variable_volumn_gradient, right_quadrature_node_artificial_viscosity(j),
            right_quadrature_node_variable_volumn_gradient, artificial_viscous_flux, j,
            adjacency_element_quadrature_sequence[static_cast<Usize>(j)]);
      }
      if constexpr (SimulationControl::kShockCapturing == ShockCapturingEnum::ArtificialViscosity) {
        quadrature_node_temporary_variable.noalias() =
            (convective_flux.result_.normal_variable_ - viscous_flux.result_.normal_variable_ -
             artificial_viscous_flux.result_.normal_variable_) *
            adjacency_element_mesh.element_(i).jacobian_determinant_(j) * adjacency_element_mesh.quadrature_.weight_(j);
      } else {
        quadrature_node_temporary_variable.noalias() =
            (convective_flux.result_.normal_variable_ - viscous_flux.result_.normal_variable_) *
            adjacency_element_mesh.element_(i).jacobian_determinant_(j) * adjacency_element_mesh.quadrature_.weight_(j);
      }
      this->storeAdjacencyElementNodeQuadrature(parent_gmsh_type_number(0), parent_index_each_type(0),
                                                left_adjacency_accumulate_quadrature_number + j,
                                                quadrature_node_temporary_variable, solver);
      this->storeAdjacencyElementNodeQuadrature(
          parent_gmsh_type_number(1), parent_index_each_type(1),
          right_adjacency_accumulate_quadrature_number + adjacency_element_quadrature_sequence[static_cast<Usize>(j)],
          -quadrature_node_temporary_variable, solver);
    }
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementSolverBase<AdjacencyElementTrait, SimulationControl>::
    calculateInteriorAdjacencyElementGardientQuadrature(const Mesh<SimulationControl>& mesh,
                                                        Solver<SimulationControl>& solver) {
  const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh =
      mesh.*(std::remove_reference<decltype(mesh)>::type::template getAdjacencyElement<AdjacencyElementTrait>());
  AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl> left_quadrature_node_variable;
  AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl> right_quadrature_node_variable;
  FluxVariable<SimulationControl> gardient_flux;
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>
      quadrature_node_temporary_variable;
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto)                      \
    shared(Eigen::Dynamic, mesh, adjacency_element_mesh, solver) private(                 \
            left_quadrature_node_variable, right_quadrature_node_variable, gardient_flux, \
                quadrature_node_temporary_variable)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < adjacency_element_mesh.interior_number_; i++) {
    const Isize adjacency_right_rotation = adjacency_element_mesh.element_(i).adjacency_right_rotation_;
    const std::array<int, AdjacencyElementTrait::kQuadratureNumber> adjacency_element_quadrature_sequence{
        getAdjacencyElementQuadratureSequence<AdjacencyElementTrait::kElementType, SimulationControl::kPolynomialOrder>(
            static_cast<int>(adjacency_right_rotation))};
    const Eigen::Vector<Isize, 2>& parent_index_each_type = adjacency_element_mesh.element_(i).parent_index_each_type_;
    const Eigen::Vector<Isize, 2>& adjacency_sequence_in_parent =
        adjacency_element_mesh.element_(i).adjacency_sequence_in_parent_;
    const Eigen::Vector<Isize, 2>& parent_gmsh_type_number =
        adjacency_element_mesh.element_(i).parent_gmsh_type_number_;
    const Isize left_adjacency_accumulate_quadrature_number =
        this->getAdjacencyParentElementAccumulateAdjacencyQuadratureNumber(parent_gmsh_type_number(0),
                                                                           adjacency_sequence_in_parent(0));
    const Isize right_adjacency_accumulate_quadrature_number =
        this->getAdjacencyParentElementAccumulateAdjacencyQuadratureNumber(parent_gmsh_type_number(1),
                                                                           adjacency_sequence_in_parent(1));
    left_quadrature_node_variable.get(mesh, solver, parent_gmsh_type_number(0), parent_index_each_type(0),
                                      adjacency_sequence_in_parent(0));
    right_quadrature_node_variable.get(mesh, solver, parent_gmsh_type_number(1), parent_index_each_type(1),
                                       adjacency_sequence_in_parent(1));
    for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
      calculateVolumeGardientFlux(adjacency_element_mesh.element_(i).normal_vector_.col(j),
                                  left_quadrature_node_variable, right_quadrature_node_variable, gardient_flux, j,
                                  adjacency_element_quadrature_sequence[static_cast<Usize>(j)]);
      quadrature_node_temporary_variable.noalias() =
          (gardient_flux.variable_ * adjacency_element_mesh.element_(i).jacobian_determinant_(j) *
           adjacency_element_mesh.quadrature_.weight_(j))
              .reshaped();
      this->storeAdjacencyElementNodeVolumeGardientQuadrature(parent_gmsh_type_number(0), parent_index_each_type(0),
                                                              left_adjacency_accumulate_quadrature_number + j,
                                                              quadrature_node_temporary_variable, solver);
      this->storeAdjacencyElementNodeVolumeGardientQuadrature(
          parent_gmsh_type_number(1), parent_index_each_type(1),
          right_adjacency_accumulate_quadrature_number + adjacency_element_quadrature_sequence[static_cast<Usize>(j)],
          -quadrature_node_temporary_variable, solver);
      if constexpr (SimulationControl::kEquationModel == EquationModelEnum::NavierStokes) {
        calculateInterfaceGardientFlux(adjacency_element_mesh.element_(i).normal_vector_.col(j),
                                       left_quadrature_node_variable, right_quadrature_node_variable, gardient_flux, j,
                                       adjacency_element_quadrature_sequence[static_cast<Usize>(j)]);
        quadrature_node_temporary_variable.noalias() =
            (gardient_flux.variable_ * adjacency_element_mesh.element_(i).jacobian_determinant_(j) *
             adjacency_element_mesh.quadrature_.weight_(j))
                .reshaped();
        this->storeAdjacencyElementNodeInterfaceGardientQuadrature(
            parent_gmsh_type_number(0), parent_index_each_type(0), left_adjacency_accumulate_quadrature_number + j,
            quadrature_node_temporary_variable, solver);
        this->storeAdjacencyElementNodeInterfaceGardientQuadrature(
            parent_gmsh_type_number(1), parent_index_each_type(1),
            right_adjacency_accumulate_quadrature_number + adjacency_element_quadrature_sequence[static_cast<Usize>(j)],
            quadrature_node_temporary_variable, solver);
      }
    }
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl, EquationModelEnum::Euler>::
    calculateBoundaryAdjacencyElementQuadrature(
        const Mesh<SimulationControl>& mesh, const PhysicalModel<SimulationControl>& physical_model,
        const std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition,
        Solver<SimulationControl>& solver) {
  const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh =
      mesh.*(std::remove_reference<decltype(mesh)>::type::template getAdjacencyElement<AdjacencyElementTrait>());
  AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl> left_quadrature_node_variable;
  AdjacencyElementVariableGradient<AdjacencyElementTrait, SimulationControl>
      left_quadrature_node_variable_volumn_gradient;
  Variable<SimulationControl> boundary_quadrature_node_variable;
  Flux<SimulationControl> convective_flux;
  FluxNormalVariable<SimulationControl> artificial_viscous_normal_flux;
  Eigen::Vector<Real, AdjacencyElementTrait::kQuadratureNumber> left_quadrature_node_artificial_viscosity;
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> quadrature_node_temporary_variable;
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto)                                          \
    shared(Eigen::Dynamic, mesh, adjacency_element_mesh, physical_model, boundary_condition, solver) private( \
            left_quadrature_node_variable, left_quadrature_node_variable_volumn_gradient,                     \
                boundary_quadrature_node_variable, convective_flux, artificial_viscous_normal_flux,           \
                left_quadrature_node_artificial_viscosity, quadrature_node_temporary_variable)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = adjacency_element_mesh.interior_number_;
       i < adjacency_element_mesh.boundary_number_ + adjacency_element_mesh.interior_number_; i++) {
    const Isize parent_index_each_type = adjacency_element_mesh.element_(i).parent_index_each_type_(0);
    const Isize adjacency_sequence_in_parent = adjacency_element_mesh.element_(i).adjacency_sequence_in_parent_(0);
    const Isize parent_gmsh_type_number = adjacency_element_mesh.element_(i).parent_gmsh_type_number_(0);
    const Isize left_adjacency_accumulate_quadrature_number =
        this->getAdjacencyParentElementAccumulateAdjacencyQuadratureNumber(parent_gmsh_type_number,
                                                                           adjacency_sequence_in_parent);
    left_quadrature_node_variable.get(mesh, solver, parent_gmsh_type_number, parent_index_each_type,
                                      adjacency_sequence_in_parent);
    left_quadrature_node_variable.calculateComputationalFromConserved(physical_model);
    left_quadrature_node_variable_volumn_gradient.template get<ViscousFluxEnum::None>(
        mesh, solver, parent_gmsh_type_number, parent_index_each_type, adjacency_sequence_in_parent);
    if constexpr (SimulationControl::kShockCapturing == ShockCapturingEnum::ArtificialViscosity) {
      this->calculateAdjacencyElementArtificialViscosity(mesh, solver, left_quadrature_node_artificial_viscosity,
                                                         parent_gmsh_type_number, parent_index_each_type,
                                                         adjacency_sequence_in_parent);
    }
    for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
      boundary_condition.at(adjacency_element_mesh.element_(i).gmsh_physical_index_)
          ->calculateBoundaryVariable(physical_model, adjacency_element_mesh.element_(i).normal_vector_.col(j),
                                      left_quadrature_node_variable,
                                      this->boundary_dummy_variable_(i - adjacency_element_mesh.interior_number_),
                                      boundary_quadrature_node_variable, j);
      calculateConvectiveNormalFlux(adjacency_element_mesh.element_(i).normal_vector_.col(j),
                                    boundary_quadrature_node_variable, convective_flux.result_, 0);
      if constexpr (SimulationControl::kShockCapturing == ShockCapturingEnum::ArtificialViscosity) {
        calculateArtificialViscousNormalFlux(
            adjacency_element_mesh.element_(i).normal_vector_.col(j), left_quadrature_node_artificial_viscosity(j),
            left_quadrature_node_variable_volumn_gradient, artificial_viscous_normal_flux, j);
      }
      if constexpr (SimulationControl::kShockCapturing == ShockCapturingEnum::ArtificialViscosity) {
        quadrature_node_temporary_variable.noalias() =
            (convective_flux.result_.normal_variable_ - artificial_viscous_normal_flux.normal_variable_) *
            adjacency_element_mesh.element_(i).jacobian_determinant_(j) * adjacency_element_mesh.quadrature_.weight_(j);
      } else {
        quadrature_node_temporary_variable.noalias() = convective_flux.result_.normal_variable_ *
                                                       adjacency_element_mesh.element_(i).jacobian_determinant_(j) *
                                                       adjacency_element_mesh.quadrature_.weight_(j);
      }
      this->storeAdjacencyElementNodeQuadrature(parent_gmsh_type_number, parent_index_each_type,
                                                left_adjacency_accumulate_quadrature_number + j,
                                                quadrature_node_temporary_variable, solver);
    }
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementSolver<AdjacencyElementTrait, SimulationControl, EquationModelEnum::NavierStokes>::
    calculateBoundaryAdjacencyElementQuadrature(
        const Mesh<SimulationControl>& mesh, const PhysicalModel<SimulationControl>& physical_model,
        const std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition,
        Solver<SimulationControl>& solver) {
  const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh =
      mesh.*(std::remove_reference<decltype(mesh)>::type::template getAdjacencyElement<AdjacencyElementTrait>());
  AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl> left_quadrature_node_variable;
  AdjacencyElementVariableGradient<AdjacencyElementTrait, SimulationControl> left_quadrature_node_variable_gradient;
  AdjacencyElementVariableGradient<AdjacencyElementTrait, SimulationControl>
      left_quadrature_node_variable_volumn_gradient;
  Variable<SimulationControl> boundary_quadrature_node_variable;
  VariableGradient<SimulationControl> boundary_quadrature_node_variable_gradient;
  Flux<SimulationControl> convective_flux;
  Flux<SimulationControl> viscous_flux;
  FluxNormalVariable<SimulationControl> artificial_viscous_normal_flux;
  Eigen::Vector<Real, AdjacencyElementTrait::kQuadratureNumber> left_quadrature_node_artificial_viscosity;
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> quadrature_node_temporary_variable;
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto)                                          \
    shared(Eigen::Dynamic, mesh, adjacency_element_mesh, physical_model, boundary_condition, solver) private( \
            left_quadrature_node_variable, left_quadrature_node_variable_gradient,                            \
                left_quadrature_node_variable_volumn_gradient, boundary_quadrature_node_variable,             \
                boundary_quadrature_node_variable_gradient, convective_flux, viscous_flux,                    \
                artificial_viscous_normal_flux, left_quadrature_node_artificial_viscosity,                    \
                quadrature_node_temporary_variable)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = adjacency_element_mesh.interior_number_;
       i < adjacency_element_mesh.boundary_number_ + adjacency_element_mesh.interior_number_; i++) {
    const Isize parent_index_each_type = adjacency_element_mesh.element_(i).parent_index_each_type_(0);
    const Isize adjacency_sequence_in_parent = adjacency_element_mesh.element_(i).adjacency_sequence_in_parent_(0);
    const Isize parent_gmsh_type_number = adjacency_element_mesh.element_(i).parent_gmsh_type_number_(0);
    const Isize left_adjacency_accumulate_quadrature_number =
        this->getAdjacencyParentElementAccumulateAdjacencyQuadratureNumber(parent_gmsh_type_number,
                                                                           adjacency_sequence_in_parent);
    left_quadrature_node_variable.get(mesh, solver, parent_gmsh_type_number, parent_index_each_type,
                                      adjacency_sequence_in_parent);
    left_quadrature_node_variable.calculateComputationalFromConserved(physical_model);
    left_quadrature_node_variable_gradient.template get<SimulationControl::kViscousFlux>(
        mesh, solver, parent_gmsh_type_number, parent_index_each_type, adjacency_sequence_in_parent);
    left_quadrature_node_variable_gradient.calculatePrimitiveFromConserved(physical_model,
                                                                           left_quadrature_node_variable);
    left_quadrature_node_variable_volumn_gradient.template get<ViscousFluxEnum::None>(
        mesh, solver, parent_gmsh_type_number, parent_index_each_type, adjacency_sequence_in_parent);
    if constexpr (SimulationControl::kShockCapturing == ShockCapturingEnum::ArtificialViscosity) {
      this->calculateAdjacencyElementArtificialViscosity(mesh, solver, left_quadrature_node_artificial_viscosity,
                                                         parent_gmsh_type_number, parent_index_each_type,
                                                         adjacency_sequence_in_parent);
    }
    for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
      boundary_condition.at(adjacency_element_mesh.element_(i).gmsh_physical_index_)
          ->calculateBoundaryVariable(physical_model, adjacency_element_mesh.element_(i).normal_vector_.col(j),
                                      left_quadrature_node_variable,
                                      this->boundary_dummy_variable_(i - adjacency_element_mesh.interior_number_),
                                      boundary_quadrature_node_variable, j);
      calculateConvectiveNormalFlux(adjacency_element_mesh.element_(i).normal_vector_.col(j),
                                    boundary_quadrature_node_variable, convective_flux.result_, 0);
      boundary_condition.at(adjacency_element_mesh.element_(i).gmsh_physical_index_)
          ->calculateBoundaryVariableGradient(left_quadrature_node_variable, left_quadrature_node_variable_gradient,
                                              boundary_quadrature_node_variable,
                                              boundary_quadrature_node_variable_gradient, j);
      calculateViscousFlux(physical_model, adjacency_element_mesh.element_(i).normal_vector_.col(j),
                           left_quadrature_node_variable, left_quadrature_node_variable_gradient,
                           boundary_quadrature_node_variable, boundary_quadrature_node_variable_gradient, viscous_flux,
                           j, 0);
      if constexpr (SimulationControl::kShockCapturing == ShockCapturingEnum::ArtificialViscosity) {
        calculateArtificialViscousNormalFlux(
            adjacency_element_mesh.element_(i).normal_vector_.col(j), left_quadrature_node_artificial_viscosity(j),
            left_quadrature_node_variable_volumn_gradient, artificial_viscous_normal_flux, j);
      }
      if constexpr (SimulationControl::kShockCapturing == ShockCapturingEnum::ArtificialViscosity) {
        quadrature_node_temporary_variable.noalias() =
            (convective_flux.result_.normal_variable_ - viscous_flux.result_.normal_variable_ -
             artificial_viscous_normal_flux.normal_variable_) *
            adjacency_element_mesh.element_(i).jacobian_determinant_(j) * adjacency_element_mesh.quadrature_.weight_(j);
      } else {
        quadrature_node_temporary_variable.noalias() =
            (convective_flux.result_.normal_variable_ - viscous_flux.result_.normal_variable_) *
            adjacency_element_mesh.element_(i).jacobian_determinant_(j) * adjacency_element_mesh.quadrature_.weight_(j);
      }
      this->storeAdjacencyElementNodeQuadrature(parent_gmsh_type_number, parent_index_each_type,
                                                left_adjacency_accumulate_quadrature_number + j,
                                                quadrature_node_temporary_variable, solver);
    }
  }
}

template <typename AdjacencyElementTrait, typename SimulationControl>
inline void AdjacencyElementSolverBase<AdjacencyElementTrait, SimulationControl>::
    calculateBoundaryAdjacencyElementGardientQuadrature(
        const Mesh<SimulationControl>& mesh, const PhysicalModel<SimulationControl>& physical_model,
        const std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition,
        Solver<SimulationControl>& solver) {
  const AdjacencyElementMesh<AdjacencyElementTrait>& adjacency_element_mesh =
      mesh.*(std::remove_reference<decltype(mesh)>::type::template getAdjacencyElement<AdjacencyElementTrait>());
  AdjacencyElementVariable<AdjacencyElementTrait, SimulationControl> left_quadrature_node_variable;
  Variable<SimulationControl> boundary_quadrature_node_volume_gradient_variable;
  Variable<SimulationControl> boundary_quadrature_node_interface_gradient_variable;
  FluxVariable<SimulationControl> gardient_flux;
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber * SimulationControl::kDimension>
      quadrature_node_temporary_variable;
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto)                                          \
    shared(Eigen::Dynamic, mesh, physical_model, boundary_condition, adjacency_element_mesh, solver) private( \
            left_quadrature_node_variable, boundary_quadrature_node_volume_gradient_variable,                 \
                boundary_quadrature_node_interface_gradient_variable, gardient_flux,                          \
                quadrature_node_temporary_variable)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = adjacency_element_mesh.interior_number_;
       i < adjacency_element_mesh.boundary_number_ + adjacency_element_mesh.interior_number_; i++) {
    const Isize parent_index_each_type = adjacency_element_mesh.element_(i).parent_index_each_type_(0);
    const Isize adjacency_sequence_in_parent = adjacency_element_mesh.element_(i).adjacency_sequence_in_parent_(0);
    const Isize parent_gmsh_type_number = adjacency_element_mesh.element_(i).parent_gmsh_type_number_(0);
    const Isize left_adjacency_accumulate_quadrature_number =
        this->getAdjacencyParentElementAccumulateAdjacencyQuadratureNumber(parent_gmsh_type_number,
                                                                           adjacency_sequence_in_parent);
    left_quadrature_node_variable.get(mesh, solver, parent_gmsh_type_number, parent_index_each_type,
                                      adjacency_sequence_in_parent);
    left_quadrature_node_variable.calculateComputationalFromConserved(physical_model);
    for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
      boundary_condition.at(adjacency_element_mesh.element_(i).gmsh_physical_index_)
          ->calculateBoundaryGradientVariable(
              physical_model, adjacency_element_mesh.element_(i).normal_vector_.col(j), left_quadrature_node_variable,
              this->boundary_dummy_variable_(i - adjacency_element_mesh.interior_number_),
              boundary_quadrature_node_volume_gradient_variable, boundary_quadrature_node_interface_gradient_variable,
              j);
      calculateGardientRawFlux(adjacency_element_mesh.element_(i).normal_vector_.col(j),
                               boundary_quadrature_node_volume_gradient_variable, gardient_flux, 0);
      quadrature_node_temporary_variable.noalias() =
          (gardient_flux.variable_ * adjacency_element_mesh.element_(i).jacobian_determinant_(j) *
           adjacency_element_mesh.quadrature_.weight_(j))
              .reshaped();
      this->storeAdjacencyElementNodeVolumeGardientQuadrature(parent_gmsh_type_number, parent_index_each_type,
                                                              left_adjacency_accumulate_quadrature_number + j,
                                                              quadrature_node_temporary_variable, solver);
      if constexpr (SimulationControl::kEquationModel == EquationModelEnum::NavierStokes) {
        calculateGardientRawFlux(adjacency_element_mesh.element_(i).normal_vector_.col(j),
                                 boundary_quadrature_node_interface_gradient_variable, gardient_flux, 0);
        quadrature_node_temporary_variable.noalias() =
            (gardient_flux.variable_ * adjacency_element_mesh.element_(i).jacobian_determinant_(j) *
             adjacency_element_mesh.quadrature_.weight_(j))
                .reshaped();
        this->storeAdjacencyElementNodeInterfaceGardientQuadrature(parent_gmsh_type_number, parent_index_each_type,
                                                                   left_adjacency_accumulate_quadrature_number + j,
                                                                   quadrature_node_temporary_variable, solver);
      }
    }
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::calculateAdjacencyQuadrature(
    const Mesh<SimulationControl>& mesh, const PhysicalModel<SimulationControl>& physical_model,
    const std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->point_.calculateInteriorAdjacencyElementQuadrature(mesh, physical_model, *this);
    this->point_.calculateBoundaryAdjacencyElementQuadrature(mesh, physical_model, boundary_condition, *this);
  } else if constexpr (SimulationControl::kDimension == 2) {
    this->line_.calculateInteriorAdjacencyElementQuadrature(mesh, physical_model, *this);
    this->line_.calculateBoundaryAdjacencyElementQuadrature(mesh, physical_model, boundary_condition, *this);
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasAdjacencyTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.calculateInteriorAdjacencyElementQuadrature(mesh, physical_model, *this);
      this->triangle_.calculateBoundaryAdjacencyElementQuadrature(mesh, physical_model, boundary_condition, *this);
    }
    if constexpr (HasAdjacencyQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.calculateInteriorAdjacencyElementQuadrature(mesh, physical_model, *this);
      this->quadrangle_.calculateBoundaryAdjacencyElementQuadrature(mesh, physical_model, boundary_condition, *this);
    }
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::calculateAdjacencyGardientQuadrature(
    const Mesh<SimulationControl>& mesh, const PhysicalModel<SimulationControl>& physical_model,
    const std::unordered_map<Isize, std::unique_ptr<BoundaryConditionBase<SimulationControl>>>& boundary_condition) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->point_.calculateInteriorAdjacencyElementGardientQuadrature(mesh, *this);
    this->point_.calculateBoundaryAdjacencyElementGardientQuadrature(mesh, physical_model, boundary_condition, *this);
  } else if constexpr (SimulationControl::kDimension == 2) {
    this->line_.calculateInteriorAdjacencyElementGardientQuadrature(mesh, *this);
    this->line_.calculateBoundaryAdjacencyElementGardientQuadrature(mesh, physical_model, boundary_condition, *this);
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasAdjacencyTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.calculateInteriorAdjacencyElementGardientQuadrature(mesh, *this);
      this->triangle_.calculateBoundaryAdjacencyElementGardientQuadrature(mesh, physical_model, boundary_condition,
                                                                          *this);
    }
    if constexpr (HasAdjacencyQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.calculateInteriorAdjacencyElementGardientQuadrature(mesh, *this);
      this->quadrangle_.calculateBoundaryAdjacencyElementGardientQuadrature(mesh, physical_model, boundary_condition,
                                                                            *this);
    }
  }
}

template <typename ElementTrait, typename SimulationControl>
inline void ElementSolverBase<ElementTrait, SimulationControl>::calculateElementResidual(
    const ElementMesh<ElementTrait>& element_mesh) {
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto) shared(Eigen::Dynamic, element_mesh)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < this->number_; i++) {
    // NOTE: Here we split the calculation to trigger eigen's noalias to avoid intermediate variables.
    this->element_(i).variable_residual_.noalias() =
        this->element_(i).variable_quadrature_ * element_mesh.basis_function_.modal_gradient_value_;
    this->element_(i).variable_residual_.noalias() -=
        this->element_(i).variable_adjacency_quadrature_ * element_mesh.basis_function_.modal_adjacency_value_;
    if constexpr (SimulationControl::kSourceTerm != SourceTermEnum::None) {
      this->element_(i).variable_residual_.noalias() +=
          this->element_(i).variable_source_quadrature_ * element_mesh.basis_function_.modal_value_;
    }
  }
}

template <typename ElementTrait, typename SimulationControl>
inline void ElementSolverBase<ElementTrait, SimulationControl>::calculateElementGardientResidual(
    const ElementMesh<ElementTrait>& element_mesh) {
  [[maybe_unused]] constexpr std::array<int,
                                        ElementTrait::kAdjacencyNumber + 1> kElementAccumulateAdjacencyQuadratureNumber{
      getElementAccumulateAdjacencyQuadratureNumber<ElementTrait::kElementType, SimulationControl::kPolynomialOrder>()};
#ifndef SUBROSA_DG_DEVELOP
#pragma omp parallel for default(none) schedule(nonmonotonic : auto) \
    shared(Eigen::Dynamic, Eigen::all, element_mesh, kElementAccumulateAdjacencyQuadratureNumber)
#endif  // SUBROSA_DG_DEVELOP
  for (Isize i = 0; i < this->number_; i++) {
    this->element_(i).variable_gradient_volume_residual_.noalias() =
        this->element_(i).variable_gradient_volume_adjacency_quadrature_ *
        element_mesh.basis_function_.modal_adjacency_value_;
    this->element_(i).variable_gradient_volume_residual_.noalias() -=
        this->element_(i).variable_gradient_volume_quadrature_ * element_mesh.basis_function_.modal_gradient_value_;
    if constexpr (SimulationControl::kEquationModel == EquationModelEnum::NavierStokes) {
      if constexpr (SimulationControl::kViscousFlux == ViscousFluxEnum::BR1) {
        this->element_(i).variable_gradient_interface_residual_.noalias() =
            this->element_(i).variable_gradient_interface_adjacency_quadrature_ *
            element_mesh.basis_function_.modal_adjacency_value_;
      } else if constexpr (SimulationControl::kViscousFlux == ViscousFluxEnum::BR2) {
        for (Isize j = 0; j < ElementTrait::kAdjacencyNumber; j++) {
          this->element_(i).variable_gradient_interface_residual_(j).noalias() =
              this->element_(i).variable_gradient_interface_adjacency_quadrature_(
                  Eigen::all, Eigen::seq(kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(j)],
                                         kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(j) + 1] - 1)) *
              element_mesh.basis_function_.modal_adjacency_value_(
                  Eigen::seq(kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(j)],
                             kElementAccumulateAdjacencyQuadratureNumber[static_cast<Usize>(j) + 1] - 1),
                  Eigen::all);
        }
      }
    }
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::calculateResidual(const Mesh<SimulationControl>& mesh) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.calculateElementResidual(mesh.line_);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.calculateElementResidual(mesh.triangle_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.calculateElementResidual(mesh.quadrangle_);
    }
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.calculateElementResidual(mesh.tetrahedron_);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.calculateElementResidual(mesh.pyramid_);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.calculateElementResidual(mesh.hexahedron_);
    }
  }
}

template <typename SimulationControl>
inline void Solver<SimulationControl>::calculateGardientResidual(const Mesh<SimulationControl>& mesh) {
  if constexpr (SimulationControl::kDimension == 1) {
    this->line_.calculateElementGardientResidual(mesh.line_);
  } else if constexpr (SimulationControl::kDimension == 2) {
    if constexpr (HasTriangle<SimulationControl::kMeshModel>) {
      this->triangle_.calculateElementGardientResidual(mesh.triangle_);
    }
    if constexpr (HasQuadrangle<SimulationControl::kMeshModel>) {
      this->quadrangle_.calculateElementGardientResidual(mesh.quadrangle_);
    }
  } else if constexpr (SimulationControl::kDimension == 3) {
    if constexpr (HasTetrahedron<SimulationControl::kMeshModel>) {
      this->tetrahedron_.calculateElementGardientResidual(mesh.tetrahedron_);
    }
    if constexpr (HasPyramid<SimulationControl::kMeshModel>) {
      this->pyramid_.calculateElementGardientResidual(mesh.pyramid_);
    }
    if constexpr (HasHexahedron<SimulationControl::kMeshModel>) {
      this->hexahedron_.calculateElementGardientResidual(mesh.hexahedron_);
    }
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_SPATIAL_DISCRETE_HPP_
