/**
 * @file BasisFunction.cpp
 * @brief The header file of SubrosaDG BasisFunction.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_BASIS_FUNCTION_CPP_
#define SUBROSA_DG_BASIS_FUNCTION_CPP_

#include <gmsh.h>

#include <Eigen/Core>
#include <array>
#include <format>
#include <string>
#include <vector>

#include "Mesh/Quadrature.cpp"
#include "Solver/SimulationControl.cpp"
#include "Utils/BasicDataType.cpp"
#include "Utils/Enum.cpp"
#include "Utils/Transformation.cpp"

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
struct ElementBasisFunction {
  static std::vector<double> get(const std::vector<double>& local_coord, const bool is_gradient) {
    constexpr int kElementGmshTypeNumber{getElementGmshTypeNumber<ElementType, PolynomialOrder>()};
    int num_components;
    std::vector<double> basis_functions;
    int num_orientations;
    gmsh::model::mesh::getBasisFunctions(kElementGmshTypeNumber, local_coord,
                                         std::format("{}Lagrange{}", is_gradient ? "Grad" : "", PolynomialOrder),
                                         num_components, basis_functions, num_orientations);
    return basis_functions;
  }
};

template <typename AdjacencyElementTrait>
struct AdjacencyElementBasisFunction {
  Eigen::Array<
      Eigen::Matrix<Real, AdjacencyElementTrait::kQuadratureNumber, AdjacencyElementTrait::kBasisFunctionNumber>,
      AdjacencyElementTrait::kDimension, 1>
      nodal_gradient_basis_function_;

  AdjacencyElementBasisFunction() {
    const auto& [local_coord, weights] = ElementQuadrature<AdjacencyElementTrait>::get();
    std::vector<double> gradient_basis_functions{
        ElementBasisFunction<AdjacencyElementTrait::kElementType, AdjacencyElementTrait::kPolynomialOrder>::get(
            local_coord, true)};
    for (Isize i = 0; i < AdjacencyElementTrait::kQuadratureNumber; i++) {
      for (Isize j = 0; j < AdjacencyElementTrait::kBasisFunctionNumber; j++) {
        for (Isize k = 0; k < AdjacencyElementTrait::kDimension; k++) {
          this->nodal_gradient_basis_function_(k)(i, j) = static_cast<Real>(gradient_basis_functions[static_cast<Usize>(
              (i * AdjacencyElementTrait::kBasisFunctionNumber + j) * 3 + k)]);
        }
      }
    }
  }
};

template <typename VolumeElementTrait>
struct VolumeElementBasisFunction {
  Eigen::Matrix<Real, VolumeElementTrait::kQuadratureNumber, VolumeElementTrait::kBasisFunctionNumber>
      nodal_basis_function_;
  Eigen::Matrix<Real, VolumeElementTrait::kDimension * VolumeElementTrait::kQuadratureNumber,
                VolumeElementTrait::kBasisFunctionNumber>
      nodal_gradient_basis_function_;
  Eigen::Matrix<Real, VolumeElementTrait::kAllAdjacencyQuadratureNumber, VolumeElementTrait::kBasisFunctionNumber>
      nodal_adjacency_basis_function_;
  Eigen::Matrix<Real, VolumeElementTrait::kBasisFunctionNumber, VolumeElementTrait::kBasisFunctionNumber>
      nodal_least_squares_inverse_;

  template <typename AdjacencyElementTrait>
  std::vector<double> getVolumeElementPerAdjacencyBasisFunction(
      const Eigen::Matrix<Real, VolumeElementTrait::kDimension, AdjacencyElementTrait::kBasicNodeNumber>&
          adjacency_basic_node_coordinate) {
    const auto& [local_coord, weights] = ElementQuadrature<AdjacencyElementTrait>::get();
    std::vector<double> basis_functions{
        ElementBasisFunction<AdjacencyElementTrait::kElementType, 1>::get(local_coord, false)};
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
    adjacency_local_coord(Eigen::seqN(Eigen::fix<0>, Eigen::fix<VolumeElementTrait::kDimension>),
                          Eigen::placeholders::all) =
        adjacency_basic_node_coordinate * basis_function_value.transpose();
    Eigen::Matrix<double, 3, AdjacencyElementTrait::kQuadratureNumber> adjacency_local_coord_double =
        adjacency_local_coord.template cast<double>();
    return ElementBasisFunction<VolumeElementTrait::kElementType, VolumeElementTrait::kPolynomialOrder>::get(
        {adjacency_local_coord_double.data(),
         adjacency_local_coord_double.data() + adjacency_local_coord_double.size()},
        false);
  }

  template <int Index>
  void getVolumeElementAdjacencyBasisFunction(int node_column = 0, int quadrature_column = 0) {
    if constexpr (Index < VolumeElementTrait::kAdjacencyNumber) {
      constexpr std::array<ElementEnum, VolumeElementTrait::kAdjacencyNumber> kAdjacencyElementType{
          getVolumeElementPerAdjacencyType<VolumeElementTrait::kElementType>()};
      constexpr std::array<int, VolumeElementTrait::kAdjacencyNumber> kAdjacencyNodeNumber{
          getVolumeElementPerAdjacencyNodeNumber<VolumeElementTrait::kElementType>()};
      constexpr std::array<int, VolumeElementTrait::kAllAdjacencyNodeNumber> kAdjacencyNodeIndex{
          getVolumeElementPerAdjacencyNodeIndex<VolumeElementTrait::kElementType>()};
      constexpr std::array<int, getVolumeElementAdjacencyNumber<VolumeElementTrait::kElementType>()>
          kAdjacencyQuadratureNumber{
              getVolumeElementPerAdjacencyQuadratureNumber<VolumeElementTrait::kElementType,
                                                           VolumeElementTrait::kPolynomialOrder>()};
      const Eigen::Matrix<double, VolumeElementTrait::kDimension, VolumeElementTrait::kBasicNodeNumber>
          basic_node_coordinate_double{getElementNodeCoordinate<VolumeElementTrait::kElementType, 1>().data()};
      const Eigen::Matrix<Real, VolumeElementTrait::kDimension, VolumeElementTrait::kBasicNodeNumber>
          basic_node_coordinate = basic_node_coordinate_double.template cast<Real>();
      Eigen::Matrix<Real, VolumeElementTrait::kDimension, kAdjacencyNodeNumber[static_cast<Usize>(Index)]>
          adjacency_basic_node_coordinate;
      for (Isize j = 0; j < kAdjacencyNodeNumber[static_cast<Usize>(Index)]; j++) {
        adjacency_basic_node_coordinate.col(j) =
            basic_node_coordinate.col(kAdjacencyNodeIndex[static_cast<Usize>(node_column + j)]);
      }
      const std::vector<double> nodal_adjacency_basis_functions{
          this->getVolumeElementPerAdjacencyBasisFunction<AdjacencyElementTrait<
              kAdjacencyElementType[static_cast<Usize>(Index)], VolumeElementTrait::kPolynomialOrder>>(
              adjacency_basic_node_coordinate)};
      for (Isize j = 0; j < kAdjacencyQuadratureNumber[static_cast<Usize>(Index)]; j++) {
        for (Isize k = 0; k < VolumeElementTrait::kBasisFunctionNumber; k++) {
          this->nodal_adjacency_basis_function_(quadrature_column + j, k) = static_cast<Real>(
              nodal_adjacency_basis_functions[static_cast<Usize>(j * VolumeElementTrait::kBasisFunctionNumber + k)]);
        }
      }
      this->getVolumeElementAdjacencyBasisFunction<Index + 1>(
          node_column + kAdjacencyNodeNumber[static_cast<Usize>(Index)],
          quadrature_column + kAdjacencyQuadratureNumber[static_cast<Usize>(Index)]);
    } else {
      return;
    }
  }

  VolumeElementBasisFunction() {
    const auto& [local_coord, weights] = ElementQuadrature<VolumeElementTrait>::get();
    std::vector<double> nodal_basis_functions{
        ElementBasisFunction<VolumeElementTrait::kElementType, VolumeElementTrait::kPolynomialOrder>::get(local_coord,
                                                                                                          false)};
    for (Isize i = 0; i < VolumeElementTrait::kQuadratureNumber; i++) {
      for (Isize j = 0; j < VolumeElementTrait::kBasisFunctionNumber; j++) {
        this->nodal_basis_function_(i, j) = static_cast<Real>(
            nodal_basis_functions[static_cast<Usize>(i * VolumeElementTrait::kBasisFunctionNumber + j)]);
      }
    }
    this->nodal_least_squares_inverse_ =
        (this->nodal_basis_function_.transpose() * this->nodal_basis_function_).inverse();
    std::vector<double> nodal_gradient_basis_functions{
        ElementBasisFunction<VolumeElementTrait::kElementType, VolumeElementTrait::kPolynomialOrder>::get(local_coord,
                                                                                                          true)};
    for (Isize i = 0; i < VolumeElementTrait::kQuadratureNumber; i++) {
      for (Isize j = 0; j < VolumeElementTrait::kBasisFunctionNumber; j++) {
        for (Isize k = 0; k < VolumeElementTrait::kDimension; k++) {
          this->nodal_gradient_basis_function_(i * VolumeElementTrait::kDimension + k, j) =
              static_cast<Real>(nodal_gradient_basis_functions[static_cast<Usize>(
                  (i * VolumeElementTrait::kBasisFunctionNumber + j) * 3 + k)]);
        }
      }
    }
    this->getVolumeElementAdjacencyBasisFunction<0>();
  }
};

template <typename VolumeElementTrait>
struct VolumeElementBasisFunctionDevice {
  Device::Matrix<Real, VolumeElementTrait::kQuadratureNumber, VolumeElementTrait::kBasisFunctionNumber>
      nodal_basis_function_;
  Device::Matrix<Real, VolumeElementTrait::kDimension * VolumeElementTrait::kQuadratureNumber,
                 VolumeElementTrait::kBasisFunctionNumber>
      nodal_gradient_basis_function_;
  Device::Matrix<Real, VolumeElementTrait::kAllAdjacencyQuadratureNumber, VolumeElementTrait::kBasisFunctionNumber>
      nodal_adjacency_basis_function_;
  Device::Matrix<Real, VolumeElementTrait::kBasisFunctionNumber, VolumeElementTrait::kBasisFunctionNumber>
      nodal_least_squares_inverse_;

  void transferVolumeElementBasisFunctionToDevice(
      const VolumeElementBasisFunction<VolumeElementTrait>& volume_element_basis_function) {
    Utils::transferToDevice<Real, VolumeElementTrait::kQuadratureNumber, VolumeElementTrait::kBasisFunctionNumber>(
        volume_element_basis_function.nodal_basis_function_, this->nodal_basis_function_);
    Utils::transferToDevice<Real, VolumeElementTrait::kDimension * VolumeElementTrait::kQuadratureNumber,
                            VolumeElementTrait::kBasisFunctionNumber>(
        volume_element_basis_function.nodal_gradient_basis_function_, this->nodal_gradient_basis_function_);
    Utils::transferToDevice<Real, VolumeElementTrait::kAllAdjacencyQuadratureNumber,
                            VolumeElementTrait::kBasisFunctionNumber>(
        volume_element_basis_function.nodal_adjacency_basis_function_, this->nodal_adjacency_basis_function_);
    Utils::transferToDevice<Real, VolumeElementTrait::kBasisFunctionNumber, VolumeElementTrait::kBasisFunctionNumber>(
        volume_element_basis_function.nodal_least_squares_inverse_, this->nodal_least_squares_inverse_);
  }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_BASIS_FUNCTION_CPP_
