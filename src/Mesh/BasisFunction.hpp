/**
 * @file BasisFunction.hpp
 * @brief The header file of SubrosaDG BasisFunction.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_BASIS_FUNCTION_HPP_
#define SUBROSA_DG_BASIS_FUNCTION_HPP_

#include <gmsh.h>

#include <Eigen/Core>
#include <array>
#include <format>
#include <string>
#include <vector>

#include "Mesh/Quadrature.hpp"
#include "Solver/SimulationControl.hpp"
#include "Utils/BasicDataType.hpp"
#include "Utils/Enum.hpp"

namespace SubrosaDG {

template <ElementEnum ElementType, int PolynomialOrder>
inline std::vector<double> getElementNodeCoordinate() {
  constexpr int kElementGmshTypeNumber{getElementGmshTypeNumber<ElementType, PolynomialOrder>()};
  std::string element_name;
  int dim;
  int order;
  int num_nodes;
  std::vector<double> local_node_coord;
  int num_primary_nodes;
  gmsh::model::mesh::getElementProperties(kElementGmshTypeNumber, element_name, dim, order, num_nodes, local_node_coord,
                                          num_primary_nodes);
  return local_node_coord;
}

template <ElementEnum ElementType, int PolynomialOrder>
inline std::vector<double> getElementBasisFunction(const std::vector<double>& local_coord) {
  constexpr int kElementGmshTypeNumber{getElementGmshTypeNumber<ElementType, PolynomialOrder>()};
  int num_components;
  std::vector<double> basis_functions;
  int num_orientations;
  gmsh::model::mesh::getBasisFunctions(kElementGmshTypeNumber, local_coord, std::format("Lagrange{}", PolynomialOrder),
                                       num_components, basis_functions, num_orientations);
  return basis_functions;
}

template <typename ElementTrait, typename AdjacencyElementTrait>
inline std::vector<double> getElementPerAdjacencyBasisFunction(
    const Eigen::Matrix<Real, ElementTrait::kDimension, AdjacencyElementTrait::kBasicNodeNumber>&
        adjacency_basic_node_coordinate) {
  const auto& [local_coord, weights] = getElementQuadrature<AdjacencyElementTrait>();
  int num_components;
  std::vector<double> basis_functions;
  int num_orientations;
  gmsh::model::mesh::getBasisFunctions(AdjacencyElementTrait::kGmshTypeNumber, local_coord, "Lagrange1", num_components,
                                       basis_functions, num_orientations);
  constexpr int kAdjacencyElementP1BasisFunctionNumber =
      getElementBasisFunctionNumber<AdjacencyElementTrait::kElementType, 1>();
  Eigen::Matrix<Real, AdjacencyElementTrait::kQuadratureNumber, kAdjacencyElementP1BasisFunctionNumber>
      basis_function_value;
  for (Isize i = 0; i < AdjacencyElementTrait::kQuadratureNumber; i++) {
    for (Isize j = 0; j < kAdjacencyElementP1BasisFunctionNumber; j++) {
      basis_function_value(i, j) =
          static_cast<Real>(basis_functions[static_cast<Usize>(i * kAdjacencyElementP1BasisFunctionNumber + j)]);
    }
  }
  Eigen::Matrix<Real, 3, AdjacencyElementTrait::kQuadratureNumber> adjacency_local_coord =
      Eigen::Matrix<Real, 3, AdjacencyElementTrait::kQuadratureNumber>::Zero();
  adjacency_local_coord(Eigen::seqN(Eigen::fix<0>, Eigen::fix<ElementTrait::kDimension>), Eigen::all) =
      adjacency_basic_node_coordinate * basis_function_value.transpose();
  Eigen::Matrix<double, 3, AdjacencyElementTrait::kQuadratureNumber> adjacency_local_coord_double =
      adjacency_local_coord.template cast<double>();
  return (getElementBasisFunction<ElementTrait::kElementType, ElementTrait::kPolynomialOrder>(
      {adjacency_local_coord_double.data(),
       adjacency_local_coord_double.data() + adjacency_local_coord_double.size()}));
}

template <typename AdjacencyElementTrait>
struct AdjacencyElementBasisFunction {
  Eigen::Array<
      Eigen::Matrix<Real, AdjacencyElementTrait::kQuadratureNumber, AdjacencyElementTrait::kBasisFunctionNumber>,
      AdjacencyElementTrait::kDimension, 1>
      gradient_value_;

  inline AdjacencyElementBasisFunction() {
    const auto& [local_coord, weights] = getElementQuadrature<AdjacencyElementTrait>();
    int num_components;
    std::vector<double> gradient_basis_functions;
    int num_orientations;
    gmsh::model::mesh::getBasisFunctions(AdjacencyElementTrait::kGmshTypeNumber, local_coord,
                                         std::format("GradLagrange{}", AdjacencyElementTrait::kPolynomialOrder),
                                         num_components, gradient_basis_functions, num_orientations);
    for (Isize i = 0; i < AdjacencyElementTrait::kQuadratureNumber; i++) {
      for (Isize j = 0; j < AdjacencyElementTrait::kBasisFunctionNumber; j++) {
        for (Isize k = 0; k < AdjacencyElementTrait::kDimension; k++) {
          this->gradient_value_(k)(i, j) = static_cast<Real>(gradient_basis_functions[static_cast<Usize>(
              (i * AdjacencyElementTrait::kBasisFunctionNumber + j) * 3 + k)]);
        }
      }
    }
  }
};

template <typename ElementTrait>
struct ElementBasisFunction {
  Eigen::Matrix<Real, ElementTrait::kQuadratureNumber, ElementTrait::kBasisFunctionNumber, Eigen::RowMajor> value_;
  Eigen::Matrix<Real, ElementTrait::kQuadratureNumber * ElementTrait::kDimension, ElementTrait::kBasisFunctionNumber>
      gradient_value_;
  Eigen::Matrix<Real, ElementTrait::kAllAdjacencyQuadratureNumber, ElementTrait::kBasisFunctionNumber, Eigen::RowMajor>
      adjacency_value_;

  template <int I>
  inline void getElementAdjacencyBasisFunction(int node_column = 0, int quadrature_column = 0) {
    if constexpr (I < ElementTrait::kAdjacencyNumber) {
      constexpr std::array<ElementEnum, ElementTrait::kAdjacencyNumber> kAdjacencyElementType{
          getElementPerAdjacencyType<ElementTrait::kElementType>()};
      constexpr std::array<int, ElementTrait::kAdjacencyNumber> kElementPerAdjacencyNodeNumber{
          getElementPerAdjacencyNodeNumber<ElementTrait::kElementType>()};
      constexpr std::array<int, ElementTrait::kAllAdjacencyNodeNumber> kElementAdjacencyNodeIndex{
          getElementPerAdjacencyNodeIndex<ElementTrait::kElementType>()};
      constexpr std::array<int, getElementAdjacencyNumber<ElementTrait::kElementType>()>
          kElementPerAdjacencyQuadratureNumber{
              getElementPerAdjacencyQuadratureNumber<ElementTrait::kElementType, ElementTrait::kPolynomialOrder>()};
      const Eigen::Matrix<Real, ElementTrait::kDimension, ElementTrait::kBasicNodeNumber> basic_node_coordinate{
          getElementNodeCoordinate<ElementTrait::kElementType, 1>().data()};
      Eigen::Matrix<Real, ElementTrait::kDimension, kElementPerAdjacencyNodeNumber[static_cast<Usize>(I)]>
          adjacency_basic_node_coordinate;
      for (Isize j = 0; j < kElementPerAdjacencyNodeNumber[static_cast<Usize>(I)]; j++) {
        adjacency_basic_node_coordinate.col(j) =
            basic_node_coordinate.col(kElementAdjacencyNodeIndex[static_cast<Usize>(node_column + j)]);
      }
      const std::vector<double> adjacency_basis_functions = getElementPerAdjacencyBasisFunction<
          ElementTrait,
          AdjacencyElementTrait<kAdjacencyElementType[static_cast<Usize>(I)], ElementTrait::kPolynomialOrder>>(
          adjacency_basic_node_coordinate);
      for (Isize j = 0; j < kElementPerAdjacencyQuadratureNumber[static_cast<Usize>(I)]; j++) {
        for (Isize k = 0; k < ElementTrait::kBasisFunctionNumber; k++) {
          this->adjacency_value_(quadrature_column + j, k) = static_cast<Real>(
              adjacency_basis_functions[static_cast<Usize>(j * ElementTrait::kBasisFunctionNumber + k)]);
        }
      }
      this->template getElementAdjacencyBasisFunction<I + 1>(
          node_column + kElementPerAdjacencyNodeNumber[static_cast<Usize>(I)],
          quadrature_column + kElementPerAdjacencyQuadratureNumber[static_cast<Usize>(I)]);
    } else {
      return;
    }
  }

  inline ElementBasisFunction() {
    const auto& [local_coord, weights] = getElementQuadrature<ElementTrait>();
    std::vector<double> basis_functions{
        getElementBasisFunction<ElementTrait::kElementType, ElementTrait::kPolynomialOrder>(local_coord)};
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
                                         std::format("GradLagrange{}", ElementTrait::kPolynomialOrder), num_components,
                                         gradient_basis_functions, num_orientations);
    for (Isize i = 0; i < ElementTrait::kQuadratureNumber; i++) {
      for (Isize j = 0; j < ElementTrait::kBasisFunctionNumber; j++) {
        for (Isize k = 0; k < ElementTrait::kDimension; k++) {
          this->gradient_value_(i * ElementTrait::kDimension + k, j) = static_cast<Real>(
              gradient_basis_functions[static_cast<Usize>((i * ElementTrait::kBasisFunctionNumber + j) * 3 + k)]);
        }
      }
    }
    this->template getElementAdjacencyBasisFunction<0>();
  }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_BASIS_FUNCTION_HPP_
