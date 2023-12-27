/**
 * @file BasisFunction.hpp
 * @brief The header file of SubrosaDG BasisFunction.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_BASIS_FUNCTION_HPP_
#define SUBROSA_DG_BASIS_FUNCTION_HPP_

#include <gmsh.h>

#include <Eigen/Core>
#include <format>
#include <vector>

#include "Mesh/GaussianQuadrature.hpp"
#include "Solver/SimulationControl.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Enum.hpp"

namespace SubrosaDG {

template <typename ElementTrait>
struct ElementBasisFunction {
  Eigen::Matrix<Real, ElementTrait::kQuadratureNumber, ElementTrait::kBasisFunctionNumber, Eigen::RowMajor> value_;
  Eigen::Matrix<Real, ElementTrait::kQuadratureNumber * ElementTrait::kDimension, ElementTrait::kBasisFunctionNumber>
      gradient_value_;
  Eigen::Matrix<Real, ElementTrait::kAdjacencyQuadratureNumber, ElementTrait::kBasisFunctionNumber, Eigen::RowMajor>
      adjacency_value_;

  template <typename AdjacencyElementTrait>
  inline void getElementAdjacencyBasisFunction();

  inline ElementBasisFunction();
};

template <typename ElementTrait>
inline std::vector<double> getElementBasisFunction(const std::vector<double>& local_coord) {
  int num_components;
  std::vector<double> basis_functions;
  int num_orientations;
  gmsh::model::mesh::getBasisFunctions(ElementTrait::kGmshTypeNumber, local_coord,
                                       std::format("Lagrange{}", static_cast<int>(ElementTrait::kPolynomialOrder)),
                                       num_components, basis_functions, num_orientations);
  return basis_functions;
}

template <typename ElementTrait>
template <typename AdjacencyElementTrait>
inline void ElementBasisFunction<ElementTrait>::getElementAdjacencyBasisFunction() {
  const auto [local_coord, weights] = getElementGaussianQuadrature<AdjacencyElementTrait>();
  int num_components;
  std::vector<double> basis_functions;
  int num_orientations;
  gmsh::model::mesh::getBasisFunctions(AdjacencyElementTrait::kGmshTypeNumber, local_coord, "Lagrange1", num_components,
                                       basis_functions, num_orientations);
  Eigen::Matrix<Real, 3, ElementTrait::kAdjacencyQuadratureNumber> adjacency_local_coord;
  adjacency_local_coord.setZero();
  Eigen::Matrix<Real, ElementTrait::kDimension, ElementTrait::kBasicNodeNumber> basic_node_coord{
      getElementNodeCoordinate<ElementTrait::kElementType, PolynomialOrderEnum::P1>().data()};
  for (Isize i = 0; i < ElementTrait::kAdjacencyNumber; i++) {
    for (Isize j = 0; j < AdjacencyElementTrait::kQuadratureNumber; j++) {
      adjacency_local_coord(Eigen::seqN(Eigen::fix<0>, Eigen::fix<ElementTrait::kDimension>),
                            i * AdjacencyElementTrait::kQuadratureNumber + j) =
          (basic_node_coord.col((i + 1) % ElementTrait::kAdjacencyNumber) *
               basis_functions[static_cast<Usize>(j * AdjacencyElementTrait::kAdjacencyNumber + 1)] +
           basic_node_coord.col(i) * basis_functions[static_cast<Usize>(j * AdjacencyElementTrait::kAdjacencyNumber)])
              .transpose();
    }
  }
  Eigen::Matrix<double, 3, ElementTrait::kAdjacencyQuadratureNumber> adjacency_local_coord_double =
      adjacency_local_coord.template cast<double>();
  std::vector<double> adjacency_basis_functions = getElementBasisFunction<ElementTrait>(
      {adjacency_local_coord_double.data(), adjacency_local_coord_double.data() + adjacency_local_coord_double.size()});
  for (Isize i = 0; i < ElementTrait::kAdjacencyQuadratureNumber; i++) {
    for (Isize j = 0; j < ElementTrait::kBasisFunctionNumber; j++) {
      this->adjacency_value_(i, j) =
          static_cast<Real>(adjacency_basis_functions[static_cast<Usize>(i * ElementTrait::kBasisFunctionNumber + j)]);
    }
  }
}

template <typename ElementTrait>
inline ElementBasisFunction<ElementTrait>::ElementBasisFunction() {
  const auto [local_coord, weights] = getElementGaussianQuadrature<ElementTrait>();
  std::vector<double> basis_functions = getElementBasisFunction<ElementTrait>(local_coord);
  for (Isize i = 0; i < ElementTrait::kQuadratureNumber; i++) {
    for (Isize j = 0; j < ElementTrait::kBasisFunctionNumber; j++) {
      this->value_(i, j) =
          static_cast<Real>(basis_functions[static_cast<Usize>(i * ElementTrait::kBasisFunctionNumber + j)]);
    }
  }
  int num_components;
  std::vector<double> gradient_basis_functions;
  int num_orientations;
  gmsh::model::mesh::getBasisFunctions(ElementTrait::kGmshTypeNumber, local_coord,
                                       std::format("GradLagrange{}", static_cast<int>(ElementTrait::kPolynomialOrder)),
                                       num_components, gradient_basis_functions, num_orientations);
  for (Isize i = 0; i < ElementTrait::kQuadratureNumber; i++) {
    for (Isize j = 0; j < ElementTrait::kBasisFunctionNumber; j++) {
      for (Isize k = 0; k < ElementTrait::kDimension; k++) {
        this->gradient_value_(i * ElementTrait::kDimension + k, j) = static_cast<Real>(
            gradient_basis_functions[static_cast<Usize>((i * ElementTrait::kBasisFunctionNumber + j) * 3 + k)]);
      }
    }
  }
  if constexpr (ElementTrait::kElementType == ElementEnum::Line) {
    this->getElementAdjacencyBasisFunction<AdjacencyPointTrait<ElementTrait::kPolynomialOrder>>();
  } else if constexpr (ElementTrait::kElementType == ElementEnum::Triangle ||
                       ElementTrait::kElementType == ElementEnum::Quadrangle) {
    this->getElementAdjacencyBasisFunction<AdjacencyLineTrait<ElementTrait::kPolynomialOrder>>();
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_BASIS_FUNCTION_HPP_
