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
inline std::vector<double> getElementNodalBasisFunction(const bool gradient, const std::vector<double>& local_coord) {
  constexpr int kElementGmshTypeNumber{getElementGmshTypeNumber<ElementType, PolynomialOrder>()};
  int num_components;
  std::vector<double> basis_functions;
  int num_orientations;
  gmsh::model::mesh::getBasisFunctions(kElementGmshTypeNumber, local_coord,
                                       std::format("{}Lagrange{}", gradient ? "Grad" : "", PolynomialOrder),
                                       num_components, basis_functions, num_orientations);
  return basis_functions;
}

template <ElementEnum ElementType, int PolynomialOrder>
inline std::vector<double> getElementModalBasisFunction(const bool gradient, const std::vector<double>& local_coord) {
  constexpr int kElementGmshTypeNumber{getElementGmshTypeNumber<ElementType, PolynomialOrder>()};
  int num_components;
  std::vector<double> basis_functions;
  int num_orientations{0};
  // NOTE: Gmsh now does not support the basis functions of the pyramid element.
  if constexpr (ElementType == ElementEnum::Pyramid) {
    gmsh::model::mesh::getBasisFunctions(kElementGmshTypeNumber, local_coord,
                                         std::format("{}Lagrange{}", gradient ? "Grad" : "", PolynomialOrder),
                                         num_components, basis_functions, num_orientations);
  } else {
    gmsh::model::mesh::getBasisFunctions(kElementGmshTypeNumber, local_coord,
                                         std::format("{}H1Legendre{}", gradient ? "Grad" : "", PolynomialOrder),
                                         num_components, basis_functions, num_orientations);
  }
  return basis_functions;
}

template <typename ElementTrait, typename AdjacencyElementTrait>
inline std::vector<double> getElementPerAdjacencyBasisFunction(
    const BasisFunctionEnum basis_function_type,
    const Eigen::Matrix<Real, ElementTrait::kDimension, AdjacencyElementTrait::kBasicNodeNumber>&
        adjacency_basic_node_coordinate) {
  const auto& [local_coord, weights] = getElementQuadrature<AdjacencyElementTrait>();
  std::vector<double> basis_functions{
      getElementNodalBasisFunction<AdjacencyElementTrait::kElementType, 1>(false, local_coord)};
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
  adjacency_local_coord(Eigen::seqN(Eigen::fix<0>, Eigen::fix<ElementTrait::kDimension>), Eigen::placeholders::all) =
      adjacency_basic_node_coordinate * basis_function_value.transpose();
  Eigen::Matrix<double, 3, AdjacencyElementTrait::kQuadratureNumber> adjacency_local_coord_double =
      adjacency_local_coord.template cast<double>();
  if (basis_function_type == BasisFunctionEnum::Nodal) {
    return getElementNodalBasisFunction<ElementTrait::kElementType, 1>(
        false, {adjacency_local_coord_double.data(),
                adjacency_local_coord_double.data() + adjacency_local_coord_double.size()});
  }
  if (basis_function_type == BasisFunctionEnum::Modal) {
    return getElementModalBasisFunction<ElementTrait::kElementType, ElementTrait::kPolynomialOrder>(
        false, {adjacency_local_coord_double.data(),
                adjacency_local_coord_double.data() + adjacency_local_coord_double.size()});
  }
  return {};
}

template <typename AdjacencyElementTrait>
struct AdjacencyElementBasisFunction {
  Eigen::Array<
      Eigen::Matrix<Real, AdjacencyElementTrait::kQuadratureNumber, AdjacencyElementTrait::kBasisFunctionNumber>,
      AdjacencyElementTrait::kDimension, 1>
      nodal_gradient_value_;

  inline AdjacencyElementBasisFunction() {
    const auto& [local_coord, weights] = getElementQuadrature<AdjacencyElementTrait>();
    std::vector<double> gradient_basis_functions{
        getElementNodalBasisFunction<AdjacencyElementTrait::kElementType, AdjacencyElementTrait::kPolynomialOrder>(
            true, local_coord)};
    for (Isize i = 0; i < AdjacencyElementTrait::kQuadratureNumber; i++) {
      for (Isize j = 0; j < AdjacencyElementTrait::kBasisFunctionNumber; j++) {
        for (Isize k = 0; k < AdjacencyElementTrait::kDimension; k++) {
          this->nodal_gradient_value_(k)(i, j) = static_cast<Real>(gradient_basis_functions[static_cast<Usize>(
              (i * AdjacencyElementTrait::kBasisFunctionNumber + j) * 3 + k)]);
        }
      }
    }
  }
};

template <typename ElementTrait>
struct ElementBasisFunction {
  Eigen::Matrix<Real, ElementTrait::kQuadratureNumber, ElementTrait::kBasicNodeNumber, Eigen::RowMajor> nodal_value_;
  Eigen::Matrix<Real, ElementTrait::kAllAdjacencyQuadratureNumber, ElementTrait::kBasicNodeNumber, Eigen::RowMajor>
      nodal_adjacency_value_;
  Eigen::Matrix<Real, ElementTrait::kQuadratureNumber, ElementTrait::kBasisFunctionNumber, Eigen::RowMajor>
      modal_value_;
  Eigen::Matrix<Real, ElementTrait::kQuadratureNumber * ElementTrait::kDimension, ElementTrait::kBasisFunctionNumber>
      modal_gradient_value_;
  Eigen::Matrix<Real, ElementTrait::kAllAdjacencyQuadratureNumber, ElementTrait::kBasisFunctionNumber, Eigen::RowMajor>
      modal_adjacency_value_;
  Eigen::Matrix<Real, ElementTrait::kBasisFunctionNumber, ElementTrait::kBasisFunctionNumber>
      modal_least_squares_inverse_;

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
      const Eigen::Matrix<double, ElementTrait::kDimension, ElementTrait::kBasicNodeNumber>
          basic_node_coordinate_double{getElementNodeCoordinate<ElementTrait::kElementType, 1>().data()};
      const Eigen::Matrix<Real, ElementTrait::kDimension, ElementTrait::kBasicNodeNumber> basic_node_coordinate =
          basic_node_coordinate_double.template cast<Real>();
      Eigen::Matrix<Real, ElementTrait::kDimension, kElementPerAdjacencyNodeNumber[static_cast<Usize>(I)]>
          adjacency_basic_node_coordinate;
      for (Isize j = 0; j < kElementPerAdjacencyNodeNumber[static_cast<Usize>(I)]; j++) {
        adjacency_basic_node_coordinate.col(j) =
            basic_node_coordinate.col(kElementAdjacencyNodeIndex[static_cast<Usize>(node_column + j)]);
      }
      const std::vector<double> nodal_adjacency_basis_functions{getElementPerAdjacencyBasisFunction<
          ElementTrait,
          AdjacencyElementTrait<kAdjacencyElementType[static_cast<Usize>(I)], ElementTrait::kPolynomialOrder>>(
          BasisFunctionEnum::Nodal, adjacency_basic_node_coordinate)};
      for (Isize j = 0; j < kElementPerAdjacencyQuadratureNumber[static_cast<Usize>(I)]; j++) {
        for (Isize k = 0; k < ElementTrait::kBasicNodeNumber; k++) {
          this->nodal_adjacency_value_(quadrature_column + j, k) = static_cast<Real>(
              nodal_adjacency_basis_functions[static_cast<Usize>(j * ElementTrait::kBasicNodeNumber + k)]);
        }
      }
      const std::vector<double> modal_adjacency_basis_functions{getElementPerAdjacencyBasisFunction<
          ElementTrait,
          AdjacencyElementTrait<kAdjacencyElementType[static_cast<Usize>(I)], ElementTrait::kPolynomialOrder>>(
          BasisFunctionEnum::Modal, adjacency_basic_node_coordinate)};
      for (Isize j = 0; j < kElementPerAdjacencyQuadratureNumber[static_cast<Usize>(I)]; j++) {
        for (Isize k = 0; k < ElementTrait::kBasisFunctionNumber; k++) {
          this->modal_adjacency_value_(quadrature_column + j, k) = static_cast<Real>(
              modal_adjacency_basis_functions[static_cast<Usize>(j * ElementTrait::kBasisFunctionNumber + k)]);
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
    std::vector<double> nodal_basis_functions{
        getElementNodalBasisFunction<ElementTrait::kElementType, 1>(false, local_coord)};
    for (Isize i = 0; i < ElementTrait::kQuadratureNumber; i++) {
      for (Isize j = 0; j < ElementTrait::kBasicNodeNumber; j++) {
        this->nodal_value_(i, j) =
            static_cast<Real>(nodal_basis_functions[static_cast<Usize>(i * ElementTrait::kBasicNodeNumber + j)]);
      }
    }
    std::vector<double> modal_basis_functions{
        getElementModalBasisFunction<ElementTrait::kElementType, ElementTrait::kPolynomialOrder>(false, local_coord)};
    for (Isize i = 0; i < ElementTrait::kQuadratureNumber; i++) {
      for (Isize j = 0; j < ElementTrait::kBasisFunctionNumber; j++) {
        this->modal_value_(i, j) =
            static_cast<Real>(modal_basis_functions[static_cast<Usize>(i * ElementTrait::kBasisFunctionNumber + j)]);
      }
    }
    this->modal_least_squares_inverse_ = (this->modal_value_.transpose() * this->modal_value_).inverse();
    std::vector<double> modal_gradient_basis_functions{
        getElementModalBasisFunction<ElementTrait::kElementType, ElementTrait::kPolynomialOrder>(true, local_coord)};
    for (Isize i = 0; i < ElementTrait::kQuadratureNumber; i++) {
      for (Isize j = 0; j < ElementTrait::kBasisFunctionNumber; j++) {
        for (Isize k = 0; k < ElementTrait::kDimension; k++) {
          this->modal_gradient_value_(i * ElementTrait::kDimension + k, j) = static_cast<Real>(
              modal_gradient_basis_functions[static_cast<Usize>((i * ElementTrait::kBasisFunctionNumber + j) * 3 + k)]);
        }
      }
    }
    this->template getElementAdjacencyBasisFunction<0>();
  }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_BASIS_FUNCTION_HPP_
