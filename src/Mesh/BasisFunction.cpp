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
#include <vector>

#include "Mesh/Polynomial.cpp"
#include "Mesh/Quadrature.cpp"
#include "Solver/SimulationControl.cpp"
#include "Utils/BasicDataType.cpp"
#include "Utils/Enum.cpp"
#include "Utils/Transformation.cpp"

namespace SubrosaDG {

template <ElementEnum ElementType, int PolynomialOrder>
std::vector<double> getLagrangeBasisFunction(const std::vector<double>& local_coordinate, const bool is_gradient) {
  constexpr int kElementGmshTypeNumber{getElementGmshTypeNumber<ElementType, PolynomialOrder>()};
  int num_components;
  std::vector<double> basis_functions;
  int num_orientations;
  gmsh::model::mesh::getBasisFunctions(kElementGmshTypeNumber, local_coordinate,
                                       std::format("{}Lagrange{}", is_gradient ? "Grad" : "", PolynomialOrder),
                                       num_components, basis_functions, num_orientations);
  return basis_functions;
}

template <typename ElementTrait>
struct ElementBasisFunction {
  Eigen::Matrix<Real, ElementTrait::kDimension, ElementTrait::kBasicNodeNumber> lagrange_basic_node_coordinate_;
  Eigen::Matrix<Real, ElementTrait::kDimension, ElementTrait::kAllNodeNumber> lagrange_node_coordinate_;
  Eigen::Matrix<Real, ElementTrait::kDimension, ElementTrait::kP1BasisFunctionNumber> lagrange_p1_monomial_;
  Eigen::Matrix<Isize, ElementTrait::kDimension, ElementTrait::kAllBasisFunctionNumber> lagrange_monomial_;
  Eigen::Matrix<Real, ElementTrait::kP1BasisFunctionNumber, ElementTrait::kP1BasisFunctionNumber>
      lagrange_p1_coefficient_matrix_;
  Eigen::Matrix<Real, ElementTrait::kAllBasisFunctionNumber, ElementTrait::kAllBasisFunctionNumber>
      lagrange_coefficient_matrix_;

  void computeLagrangeP1Value(const Eigen::Vector<Real, ElementTrait::kDimension>& local_coordinate,
                              Eigen::Vector<Real, ElementTrait::kP1BasisFunctionNumber>& basis_function_value) const {
    Eigen::Vector<Real, ElementTrait::kP1BasisFunctionNumber> monomial_value;
    for (Isize i = 0; i < ElementTrait::kP1BasisFunctionNumber; i++) {
      Real product = 1.0_r;
      for (Isize j = 0; j < ElementTrait::kDimension; j++) {
        product *= std::pow(local_coordinate(j), this->lagrange_p1_monomial_(j, i));
      }
      if constexpr (ElementTrait::kElementType == ElementEnum::Pyramid) {
        const Real one_minus_z = 1.0_r - local_coordinate(2);
        product *=
            std::pow(one_minus_z, std::max(this->lagrange_p1_monomial_(0, i), this->lagrange_p1_monomial_(1, i)) -
                                      this->lagrange_p1_monomial_(0, i) - this->lagrange_p1_monomial_(1, i));
      }
      monomial_value(i) = product;
    }
    basis_function_value = this->lagrange_p1_coefficient_matrix_.transpose() * monomial_value;
  }

  void computeLagrangeValue(const Eigen::Vector<Real, ElementTrait::kDimension>& local_coordinate,
                            Eigen::Vector<Real, ElementTrait::kAllBasisFunctionNumber>& basis_function_value) const {
    Eigen::Vector<Real, ElementTrait::kAllBasisFunctionNumber> monomial_value;
    for (Isize i = 0; i < ElementTrait::kAllBasisFunctionNumber; i++) {
      Real product = 1.0_r;
      for (Isize j = 0; j < ElementTrait::kDimension; j++) {
        product *= std::pow(local_coordinate(j), this->lagrange_monomial_(j, i));
      }
      if constexpr (ElementTrait::kElementType == ElementEnum::Pyramid) {
        const Real one_minus_z = 1.0_r - local_coordinate(2);
        product *= std::pow(one_minus_z, std::max(this->lagrange_monomial_(0, i), this->lagrange_monomial_(1, i)) -
                                             this->lagrange_monomial_(0, i) - this->lagrange_monomial_(1, i));
      }
      monomial_value(i) = product;
    }
    basis_function_value = this->lagrange_coefficient_matrix_.transpose() * monomial_value;
  }

  // NOTE: The gradient value is not implemented for the pyramid element since it is not used in the current
  // implementation. The pyramid element use Bergot basis function which is x^i * y^j * z^k * (1 - z)^(max(i, j) - i -
  // j) and the gradient value is not easy to compute.
  void computeLagrangeGradientValue(
      const Eigen::Vector<Real, ElementTrait::kDimension>& local_coordinate,
      Eigen::Matrix<Real, ElementTrait::kDimension, ElementTrait::kAllBasisFunctionNumber>&
          basis_function_gradient_value) const {
    Eigen::Matrix<Real, ElementTrait::kAllBasisFunctionNumber, ElementTrait::kDimension> monomial_gradient_value;
    for (Isize i = 0; i < ElementTrait::kAllBasisFunctionNumber; i++) {
      for (Isize j = 0; j < ElementTrait::kDimension; j++) {
        if (lagrange_monomial_(j, i) == 0) {
          monomial_gradient_value(i, j) = 0.0_r;
          continue;
        }
        Real product = 1.0_r;
        for (Isize k = 0; k < ElementTrait::kDimension; k++) {
          if (k == j) {
            product *= lagrange_monomial_(j, i) * std::pow(local_coordinate(k), lagrange_monomial_(k, i) - 1);
          } else {
            product *= std::pow(local_coordinate(k), lagrange_monomial_(k, i));
          }
        }
        monomial_gradient_value(i, j) = product;
      }
    }
    basis_function_gradient_value = monomial_gradient_value.transpose() * this->lagrange_coefficient_matrix_;
  }

  ElementBasisFunction() {
    std::vector<Real> points{Lagrange<ElementTrait::kElementType, ElementTrait::kPolynomialOrder>::points()};
    for (Isize i = 0; i < ElementTrait::kBasicNodeNumber; i++) {
      for (Isize j = 0; j < ElementTrait::kDimension; j++) {
        this->lagrange_basic_node_coordinate_(j, i) =
            static_cast<Real>(points[static_cast<Usize>(i * ElementTrait::kDimension + j)]);
      }
    }
    for (Isize i = 0; i < ElementTrait::kAllNodeNumber; i++) {
      for (Isize j = 0; j < ElementTrait::kDimension; j++) {
        this->lagrange_node_coordinate_(j, i) =
            static_cast<Real>(points[static_cast<Usize>(i * ElementTrait::kDimension + j)]);
      }
    }
    std::vector<Isize> p1_monomials{Lagrange<ElementTrait::kElementType, 1>::monomials()};
    for (Isize i = 0; i < ElementTrait::kP1BasisFunctionNumber; i++) {
      for (Isize j = 0; j < ElementTrait::kDimension; j++) {
        this->lagrange_p1_monomial_(j, i) = p1_monomials[static_cast<Usize>(i * ElementTrait::kDimension + j)];
      }
    }
    std::vector<Isize> monomials{Lagrange<ElementTrait::kElementType, ElementTrait::kPolynomialOrder>::monomials()};
    for (Isize i = 0; i < ElementTrait::kAllBasisFunctionNumber; i++) {
      for (Isize j = 0; j < ElementTrait::kDimension; j++) {
        this->lagrange_monomial_(j, i) = monomials[static_cast<Usize>(i * ElementTrait::kDimension + j)];
      }
    }
    Eigen::Matrix<Real, ElementTrait::kP1BasisFunctionNumber, ElementTrait::kP1BasisFunctionNumber>
        p1_vandermonde_matrix;
    for (Isize i = 0; i < ElementTrait::kP1BasisFunctionNumber; i++) {
      for (Isize j = 0; j < ElementTrait::kP1BasisFunctionNumber; j++) {
        Real product = 1.0_r;
        for (Isize k = 0; k < ElementTrait::kDimension; k++) {
          product *= std::pow(this->lagrange_basic_node_coordinate_(k, i), this->lagrange_p1_monomial_(k, j));
        }
        if constexpr (ElementTrait::kElementType == ElementEnum::Pyramid) {
          const Real one_minus_z = std::max(kRealEpsilon, 1.0_r - this->lagrange_basic_node_coordinate_(2, i));
          product *=
              std::pow(one_minus_z, std::max(this->lagrange_p1_monomial_(0, j), this->lagrange_p1_monomial_(1, j)) -
                                        this->lagrange_p1_monomial_(0, j) - this->lagrange_p1_monomial_(1, j));
        }
        p1_vandermonde_matrix(i, j) = product;
      }
    }
    this->lagrange_p1_coefficient_matrix_ = p1_vandermonde_matrix.inverse();
    Eigen::Matrix<Real, ElementTrait::kAllBasisFunctionNumber, ElementTrait::kAllBasisFunctionNumber>
        vandermonde_matrix;
    for (Isize i = 0; i < ElementTrait::kAllBasisFunctionNumber; i++) {
      for (Isize j = 0; j < ElementTrait::kAllBasisFunctionNumber; j++) {
        Real product = 1.0_r;
        for (Isize k = 0; k < ElementTrait::kDimension; k++) {
          product *= std::pow(this->lagrange_node_coordinate_(k, i), this->lagrange_monomial_(k, j));
        }
        if constexpr (ElementTrait::kElementType == ElementEnum::Pyramid) {
          const Real one_minus_z = std::max(kRealEpsilon, 1.0_r - this->lagrange_node_coordinate_(2, i));
          product *= std::pow(one_minus_z, std::max(this->lagrange_monomial_(0, j), this->lagrange_monomial_(1, j)) -
                                               this->lagrange_monomial_(0, j) - this->lagrange_monomial_(1, j));
        }
        vandermonde_matrix(i, j) = product;
      }
    }
    this->lagrange_coefficient_matrix_ = vandermonde_matrix.inverse();
  }
};

template <typename ElementTrait>
struct ElementBasisFunctionDevice {
  Device::Matrix<Real, ElementTrait::kDimension, ElementTrait::kBasicNodeNumber> lagrange_basic_node_coordinate_;
  Device::Matrix<Real, ElementTrait::kDimension, ElementTrait::kAllNodeNumber> lagrange_node_coordinate_;
  Device::Matrix<Real, ElementTrait::kDimension, ElementTrait::kP1BasisFunctionNumber> lagrange_p1_monomial_;
  Device::Matrix<Isize, ElementTrait::kDimension, ElementTrait::kAllBasisFunctionNumber> lagrange_monomial_;
  Device::Matrix<Real, ElementTrait::kP1BasisFunctionNumber, ElementTrait::kP1BasisFunctionNumber>
      lagrange_p1_coefficient_matrix_;
  Device::Matrix<Real, ElementTrait::kAllBasisFunctionNumber, ElementTrait::kAllBasisFunctionNumber>
      lagrange_coefficient_matrix_;

  void computeLagrangeP1Value(
      const Device::StaticVector<Real, ElementTrait::kDimension>& local_coordinate,
      Device::StaticVector<Real, ElementTrait::kP1BasisFunctionNumber>& basis_function_value) const {
    Device::StaticVector<Real, ElementTrait::kP1BasisFunctionNumber> monomial_value;
    for (Isize i = 0; i < ElementTrait::kP1BasisFunctionNumber; i++) {
      Real product = 1.0_r;
      for (Isize j = 0; j < ElementTrait::kDimension; j++) {
        product *= sycl::pow(local_coordinate(j), this->lagrange_p1_monomial_(j, i));
      }
      if constexpr (ElementTrait::kElementType == ElementEnum::Pyramid) {
        const Real one_minus_z = 1.0_r - local_coordinate(2);
        product *=
            sycl::pow(one_minus_z, sycl::max(this->lagrange_p1_monomial_(0, i), this->lagrange_p1_monomial_(1, i)) -
                                       this->lagrange_p1_monomial_(0, i) - this->lagrange_p1_monomial_(1, i));
      }
      monomial_value(i) = product;
    }
    for (Isize m = 0; m < ElementTrait::kP1BasisFunctionNumber; m++) {
      Real sum = 0.0_r;
      for (Isize n = 0; n < ElementTrait::kP1BasisFunctionNumber; n++) {
        sum += this->lagrange_p1_coefficient_matrix_(n, m) * monomial_value(n);
      }
      basis_function_value(m) = sum;
    }
  }

  void computeLagrangeValue(
      const Device::StaticVector<Real, ElementTrait::kDimension>& local_coordinate,
      Device::StaticVector<Real, ElementTrait::kAllBasisFunctionNumber>& basis_function_value) const {
    Device::StaticVector<Real, ElementTrait::kAllBasisFunctionNumber> monomial_value;
    for (Isize i = 0; i < ElementTrait::kAllBasisFunctionNumber; i++) {
      Real product = 1.0_r;
      for (Isize j = 0; j < ElementTrait::kDimension; j++) {
        product *= sycl::pow(local_coordinate(j), this->lagrange_monomial_(j, i));
      }
      if constexpr (ElementTrait::kElementType == ElementEnum::Pyramid) {
        const Real one_minus_z = 1.0_r - local_coordinate(2);
        product *= sycl::pow(one_minus_z, sycl::max(this->lagrange_monomial_(0, i), this->lagrange_monomial_(1, i)) -
                                              this->lagrange_monomial_(0, i) - this->lagrange_monomial_(1, i));
      }
      monomial_value(i) = product;
    }
    for (Isize m = 0; m < ElementTrait::kAllBasisFunctionNumber; m++) {
      Real sum = 0.0_r;
      for (Isize n = 0; n < ElementTrait::kAllBasisFunctionNumber; n++) {
        sum += this->lagrange_coefficient_matrix_(n, m) * monomial_value(n);
      }
      basis_function_value(m) = sum;
    }
  }

  void computeLagrangeGradientValue(
      const Device::StaticVector<Real, ElementTrait::kDimension>& local_coordinate,
      Device::StaticMatrix<Real, ElementTrait::kDimension, ElementTrait::kAllBasisFunctionNumber>&
          basis_function_gradient_value) const {
    Device::StaticMatrix<Real, ElementTrait::kAllBasisFunctionNumber, ElementTrait::kDimension> monomial_gradient_value;
    for (Isize i = 0; i < ElementTrait::kAllBasisFunctionNumber; i++) {
      for (Isize j = 0; j < ElementTrait::kDimension; j++) {
        if (lagrange_monomial_(j, i) == 0) {
          monomial_gradient_value(i, j) = 0.0_r;
          continue;
        }
        Real product = 1.0_r;
        for (Isize k = 0; k < ElementTrait::kDimension; k++) {
          if (k == j) {
            product *= lagrange_monomial_(j, i) * sycl::pow(local_coordinate(k), lagrange_monomial_(k, i) - 1);
          } else {
            product *= sycl::pow(local_coordinate(k), lagrange_monomial_(k, i));
          }
        }
        monomial_gradient_value(i, j) = product;
      }
    }
    for (Isize m = 0; m < ElementTrait::kDimension; m++) {
      for (Isize n = 0; n < ElementTrait::kAllBasisFunctionNumber; n++) {
        Real sum = 0.0_r;
        for (Isize k = 0; k < ElementTrait::kAllBasisFunctionNumber; k++) {
          sum += monomial_gradient_value(k, m) * this->lagrange_coefficient_matrix_(k, n);
        }
        basis_function_gradient_value(m, n) = sum;
      }
    }
  }

  void transferVolumeElementBasisFunctionToDevice(const ElementBasisFunction<ElementTrait>& element_basis_function) {
    Utils::transferToDevice<Real, ElementTrait::kDimension, ElementTrait::kBasicNodeNumber>(
        element_basis_function.lagrange_basic_node_coordinate_, this->lagrange_basic_node_coordinate_);
    Utils::transferToDevice<Real, ElementTrait::kDimension, ElementTrait::kAllNodeNumber>(
        element_basis_function.lagrange_node_coordinate_, this->lagrange_node_coordinate_);
    Utils::transferToDevice<Real, ElementTrait::kDimension, ElementTrait::kP1BasisFunctionNumber>(
        element_basis_function.lagrange_p1_monomial_, this->lagrange_p1_monomial_);
    Utils::transferToDevice<Isize, ElementTrait::kDimension, ElementTrait::kAllBasisFunctionNumber>(
        element_basis_function.lagrange_monomial_, this->lagrange_monomial_);
    Utils::transferToDevice<Real, ElementTrait::kP1BasisFunctionNumber, ElementTrait::kP1BasisFunctionNumber>(
        element_basis_function.lagrange_p1_coefficient_matrix_, this->lagrange_p1_coefficient_matrix_);
    Utils::transferToDevice<Real, ElementTrait::kAllBasisFunctionNumber, ElementTrait::kAllBasisFunctionNumber>(
        element_basis_function.lagrange_coefficient_matrix_, this->lagrange_coefficient_matrix_);
  }
};

template <typename VolumeElementTrait>
struct VolumeElementBasisFunction : ElementBasisFunction<VolumeElementTrait> {
  Eigen::Matrix<Real, VolumeElementTrait::kQuadratureNumber, VolumeElementTrait::kAllBasisFunctionNumber>
      nodal_basis_function_;
  Eigen::Matrix<Real, VolumeElementTrait::kDimension * VolumeElementTrait::kQuadratureNumber,
                VolumeElementTrait::kAllBasisFunctionNumber>
      nodal_gradient_basis_function_;
  Eigen::Matrix<Real, VolumeElementTrait::kAllAdjacencyQuadratureNumber, VolumeElementTrait::kAllBasisFunctionNumber>
      nodal_adjacency_basis_function_;
  Eigen::Matrix<Real, VolumeElementTrait::kAllBasisFunctionNumber, VolumeElementTrait::kAllBasisFunctionNumber>
      nodal_least_squares_inverse_;

  template <typename AdjacencyElementTrait>
  std::vector<double> getVolumeElementPerAdjacencyBasisFunction(
      const Eigen::Matrix<Real, VolumeElementTrait::kDimension, AdjacencyElementTrait::kBasicNodeNumber>&
          adjacency_basic_node_coordinate) {
    const auto& [local_coordinate, weights] =
        getGaussQuadrature<AdjacencyElementTrait::kElementType, AdjacencyElementTrait::kQuadratureOrder>();
    std::vector<double> basis_functions{
        getLagrangeBasisFunction<AdjacencyElementTrait::kElementType, 1>(local_coordinate, false)};
    constexpr int kAdjacencyElementP1BasisFunctionNumber =
        getElementBasisFunctionNumber<AdjacencyElementTrait::kElementType, 1>();
    Eigen::Matrix<Real, AdjacencyElementTrait::kQuadratureNumber, kAdjacencyElementP1BasisFunctionNumber>
        adjacency_basis_function_value;
    for (Isize i = 0; i < AdjacencyElementTrait::kQuadratureNumber; i++) {
      for (Isize j = 0; j < kAdjacencyElementP1BasisFunctionNumber; j++) {
        adjacency_basis_function_value(i, j) =
            static_cast<Real>(basis_functions[static_cast<Usize>(i * kAdjacencyElementP1BasisFunctionNumber + j)]);
      }
    }
    Eigen::Matrix<Real, 3, AdjacencyElementTrait::kQuadratureNumber> volume_local_coordinate{
        Eigen::Matrix<Real, 3, AdjacencyElementTrait::kQuadratureNumber>::Zero()};
    volume_local_coordinate(Eigen::seqN(Eigen::fix<0>, Eigen::fix<VolumeElementTrait::kDimension>),
                            Eigen::placeholders::all) =
        adjacency_basic_node_coordinate * adjacency_basis_function_value.transpose();
    Eigen::Matrix<double, 3, AdjacencyElementTrait::kQuadratureNumber> adjacency_local_coord_double =
        volume_local_coordinate.template cast<double>();
    return getLagrangeBasisFunction<VolumeElementTrait::kElementType, VolumeElementTrait::kPolynomialOrder>(
        {adjacency_local_coord_double.data(),
         adjacency_local_coord_double.data() + adjacency_local_coord_double.size()},
        false);
  }

  template <int I>
  void getVolumeElementAdjacencyBasisFunction(
      const Eigen::Matrix<Real, VolumeElementTrait::kDimension, VolumeElementTrait::kBasicNodeNumber>
          volume_basic_node_coordinate,
      const int node_column = 0, const int quadrature_column = 0) {
    if constexpr (I < VolumeElementTrait::kAdjacencyNumber) {
      constexpr std::array<ElementEnum, VolumeElementTrait::kAdjacencyNumber> kVolumeElementPerAdjacencyElementType{
          getVolumeElementPerAdjacencyType<VolumeElementTrait::kElementType>()};
      constexpr std::array<int, VolumeElementTrait::kAdjacencyNumber> kVolumeElementPerAdjacencyNodeNumber{
          getVolumeElementPerAdjacencyNodeNumber<VolumeElementTrait::kElementType>()};
      constexpr std::array<int, VolumeElementTrait::kAllAdjacencyNodeNumber> kVolumeElementPerAdjacencyNodeIndex{
          getVolumeElementPerAdjacencyNodeIndex<VolumeElementTrait::kElementType>()};
      constexpr std::array<int, VolumeElementTrait::kAdjacencyNumber> kVolumeElementPerAdjacencyQuadratureNumber{
          getVolumeElementPerAdjacencyQuadratureNumber<VolumeElementTrait::kElementType,
                                                       VolumeElementTrait::kPolynomialOrder>()};
      Eigen::Matrix<Real, VolumeElementTrait::kDimension, kVolumeElementPerAdjacencyNodeNumber[static_cast<Usize>(I)]>
          adjacency_basic_node_coordinate;
      for (Isize j = 0; j < kVolumeElementPerAdjacencyNodeNumber[static_cast<Usize>(I)]; j++) {
        adjacency_basic_node_coordinate.col(j) =
            volume_basic_node_coordinate.col(kVolumeElementPerAdjacencyNodeIndex[static_cast<Usize>(node_column + j)]);
      }
      const std::vector<double> nodal_adjacency_basis_functions{
          this->getVolumeElementPerAdjacencyBasisFunction<AdjacencyElementTrait<
              kVolumeElementPerAdjacencyElementType[static_cast<Usize>(I)], VolumeElementTrait::kPolynomialOrder>>(
              adjacency_basic_node_coordinate)};
      for (Isize j = 0; j < kVolumeElementPerAdjacencyQuadratureNumber[static_cast<Usize>(I)]; j++) {
        for (Isize k = 0; k < VolumeElementTrait::kAllBasisFunctionNumber; k++) {
          this->nodal_adjacency_basis_function_(quadrature_column + j, k) = static_cast<Real>(
              nodal_adjacency_basis_functions[static_cast<Usize>(j * VolumeElementTrait::kAllBasisFunctionNumber + k)]);
        }
      }
      this->getVolumeElementAdjacencyBasisFunction<I + 1>(
          volume_basic_node_coordinate, node_column + kVolumeElementPerAdjacencyNodeNumber[static_cast<Usize>(I)],
          quadrature_column + kVolumeElementPerAdjacencyQuadratureNumber[static_cast<Usize>(I)]);
    } else {
      return;
    }
  }

  VolumeElementBasisFunction() {
    const auto& [local_coordinate, weights] =
        getGaussQuadrature<VolumeElementTrait::kElementType, VolumeElementTrait::kQuadratureOrder>();
    std::vector<double> nodal_basis_functions{
        getLagrangeBasisFunction<VolumeElementTrait::kElementType, VolumeElementTrait::kPolynomialOrder>(
            local_coordinate, false)};
    for (Isize i = 0; i < VolumeElementTrait::kQuadratureNumber; i++) {
      for (Isize j = 0; j < VolumeElementTrait::kAllBasisFunctionNumber; j++) {
        this->nodal_basis_function_(i, j) = static_cast<Real>(
            nodal_basis_functions[static_cast<Usize>(i * VolumeElementTrait::kAllBasisFunctionNumber + j)]);
      }
    }
    this->nodal_least_squares_inverse_ =
        (this->nodal_basis_function_.transpose() * this->nodal_basis_function_).inverse();
    std::vector<double> nodal_gradient_basis_functions{
        getLagrangeBasisFunction<VolumeElementTrait::kElementType, VolumeElementTrait::kPolynomialOrder>(
            local_coordinate, true)};
    for (Isize i = 0; i < VolumeElementTrait::kQuadratureNumber; i++) {
      for (Isize j = 0; j < VolumeElementTrait::kAllBasisFunctionNumber; j++) {
        for (Isize k = 0; k < VolumeElementTrait::kDimension; k++) {
          this->nodal_gradient_basis_function_(i * VolumeElementTrait::kDimension + k, j) =
              static_cast<Real>(nodal_gradient_basis_functions[static_cast<Usize>(
                  (i * VolumeElementTrait::kAllBasisFunctionNumber + j) * 3 + k)]);
        }
      }
    }
    this->getVolumeElementAdjacencyBasisFunction<0>(this->lagrange_basic_node_coordinate_);
  }
};

template <typename VolumeElementTrait>
struct VolumeElementBasisFunctionDevice : ElementBasisFunctionDevice<VolumeElementTrait> {
  Device::Matrix<Real, VolumeElementTrait::kQuadratureNumber, VolumeElementTrait::kAllBasisFunctionNumber>
      nodal_basis_function_;
  Device::Matrix<Real, VolumeElementTrait::kDimension * VolumeElementTrait::kQuadratureNumber,
                 VolumeElementTrait::kAllBasisFunctionNumber>
      nodal_gradient_basis_function_;
  Device::Matrix<Real, VolumeElementTrait::kAllAdjacencyQuadratureNumber, VolumeElementTrait::kAllBasisFunctionNumber>
      nodal_adjacency_basis_function_;
  Device::Matrix<Real, VolumeElementTrait::kAllBasisFunctionNumber, VolumeElementTrait::kAllBasisFunctionNumber>
      nodal_least_squares_inverse_;

  void transferVolumeElementBasisFunctionToDevice(
      const VolumeElementBasisFunction<VolumeElementTrait>& volume_element_basis_function) {
    this->ElementBasisFunctionDevice<VolumeElementTrait>::transferVolumeElementBasisFunctionToDevice(
        static_cast<const ElementBasisFunction<VolumeElementTrait>&>(volume_element_basis_function));
    Utils::transferToDevice<Real, VolumeElementTrait::kQuadratureNumber, VolumeElementTrait::kAllBasisFunctionNumber>(
        volume_element_basis_function.nodal_basis_function_, this->nodal_basis_function_);
    Utils::transferToDevice<Real, VolumeElementTrait::kDimension * VolumeElementTrait::kQuadratureNumber,
                            VolumeElementTrait::kAllBasisFunctionNumber>(
        volume_element_basis_function.nodal_gradient_basis_function_, this->nodal_gradient_basis_function_);
    Utils::transferToDevice<Real, VolumeElementTrait::kAllAdjacencyQuadratureNumber,
                            VolumeElementTrait::kAllBasisFunctionNumber>(
        volume_element_basis_function.nodal_adjacency_basis_function_, this->nodal_adjacency_basis_function_);
    Utils::transferToDevice<Real, VolumeElementTrait::kAllBasisFunctionNumber,
                            VolumeElementTrait::kAllBasisFunctionNumber>(
        volume_element_basis_function.nodal_least_squares_inverse_, this->nodal_least_squares_inverse_);
  }
};

template <typename AdjacencyElementTrait>
struct AdjacencyElementBasisFunction : ElementBasisFunction<AdjacencyElementTrait> {
  Eigen::Matrix<Real, AdjacencyElementTrait::kQuadratureNumber, AdjacencyElementTrait::kAllBasisFunctionNumber>
      nodal_basis_function_;
  Eigen::Matrix<Real, AdjacencyElementTrait::kDimension * AdjacencyElementTrait::kQuadratureNumber,
                AdjacencyElementTrait::kAllBasisFunctionNumber>
      nodal_gradient_basis_function_;

  template <typename VolumeElementTrait>
  void getAdjacencyElementParentVolumeLocalCoordinate(
      const Eigen::Matrix<Real, VolumeElementTrait::kDimension, VolumeElementTrait::kBasicNodeNumber>&
          volume_basic_node_coordinate,
      const Eigen::Vector<Real, AdjacencyElementTrait::kDimension>& adjacency_local_coordinate,
      Eigen::Vector<Real, VolumeElementTrait::kDimension>& volume_local_coordinate,
      const Isize adjacency_sequence) const {
    constexpr std::array<int, VolumeElementTrait::kAdjacencyNumber> kVolumeElementPerAdjacencyNodeNumber{
        getVolumeElementPerAdjacencyNodeNumber<VolumeElementTrait::kElementType>()};
    constexpr std::array<int, VolumeElementTrait::kAllAdjacencyNodeNumber> kVolumeElementPerAdjacencyNodeIndex{
        getVolumeElementPerAdjacencyNodeIndex<VolumeElementTrait::kElementType>()};
    Eigen::Vector<Real, AdjacencyElementTrait::kP1BasisFunctionNumber> adjacency_p1_basis_function_value;
    this->computeLagrangeP1Value(adjacency_local_coordinate, adjacency_p1_basis_function_value);
    Eigen::Matrix<Real, VolumeElementTrait::kDimension, AdjacencyElementTrait::kBasicNodeNumber>
        adjacency_basic_node_coordinate;
    Isize node_column = 0;
    for (Isize i = 0; i < adjacency_sequence; i++) {
      node_column += kVolumeElementPerAdjacencyNodeNumber[static_cast<Usize>(i)];
    }
    for (Isize i = 0; i < AdjacencyElementTrait::kBasicNodeNumber; i++) {
      adjacency_basic_node_coordinate.col(i) =
          volume_basic_node_coordinate.col(kVolumeElementPerAdjacencyNodeIndex[static_cast<Usize>(node_column + i)]);
    }
    volume_local_coordinate = adjacency_basic_node_coordinate * adjacency_p1_basis_function_value;
  }

  AdjacencyElementBasisFunction() {
    const auto& [local_coordinate, weights] =
        getGaussQuadrature<AdjacencyElementTrait::kElementType, AdjacencyElementTrait::kQuadratureOrder>();
    std::vector<double> nodal_basis_functions{
        getLagrangeBasisFunction<AdjacencyElementTrait::kElementType, AdjacencyElementTrait::kPolynomialOrder>(
            local_coordinate, false)};
    for (Isize i = 0; i < AdjacencyElementTrait::kQuadratureNumber; i++) {
      for (Isize j = 0; j < AdjacencyElementTrait::kAllBasisFunctionNumber; j++) {
        this->nodal_basis_function_(i, j) = static_cast<Real>(
            nodal_basis_functions[static_cast<Usize>(i * AdjacencyElementTrait::kAllBasisFunctionNumber + j)]);
      }
    }
    std::vector<double> gradient_basis_functions{
        getLagrangeBasisFunction<AdjacencyElementTrait::kElementType, AdjacencyElementTrait::kPolynomialOrder>(
            local_coordinate, true)};
    for (Isize i = 0; i < AdjacencyElementTrait::kQuadratureNumber; i++) {
      for (Isize j = 0; j < AdjacencyElementTrait::kAllBasisFunctionNumber; j++) {
        for (Isize k = 0; k < AdjacencyElementTrait::kDimension; k++) {
          this->nodal_gradient_basis_function_(i * AdjacencyElementTrait::kDimension + k, j) =
              static_cast<Real>(gradient_basis_functions[static_cast<Usize>(
                  (i * AdjacencyElementTrait::kAllBasisFunctionNumber + j) * 3 + k)]);
        }
      }
    }
  }
};

template <typename AdjacencyElementTrait>
struct AdjacencyElementBasisFunctionDevice : ElementBasisFunctionDevice<AdjacencyElementTrait> {
  Device::Matrix<Real, AdjacencyElementTrait::kQuadratureNumber, AdjacencyElementTrait::kAllBasisFunctionNumber>
      nodal_basis_function_;
  Device::Matrix<Real, AdjacencyElementTrait::kDimension * AdjacencyElementTrait::kQuadratureNumber,
                 AdjacencyElementTrait::kAllBasisFunctionNumber>
      nodal_gradient_basis_function_;

  template <typename VolumeElementTrait>
  void getAdjacencyElementParentVolumeLocalCoordinate(
      const Device::Matrix<Real, VolumeElementTrait::kDimension, VolumeElementTrait::kBasicNodeNumber>&
          volume_basic_node_coordinate,
      const Device::StaticVector<Real, AdjacencyElementTrait::kDimension>& adjacency_local_coordinate,
      Device::StaticVector<Real, VolumeElementTrait::kDimension>& volume_local_coordinate,
      const Isize adjacency_sequence) const {
    constexpr std::array<int, VolumeElementTrait::kAdjacencyNumber> kVolumeElementPerAdjacencyNodeNumber{
        getVolumeElementPerAdjacencyNodeNumber<VolumeElementTrait::kElementType>()};
    constexpr std::array<int, VolumeElementTrait::kAllAdjacencyNodeNumber> kVolumeElementPerAdjacencyNodeIndex{
        getVolumeElementPerAdjacencyNodeIndex<VolumeElementTrait::kElementType>()};
    Device::StaticVector<Real, AdjacencyElementTrait::kP1BasisFunctionNumber> adjacency_p1_basis_function_value;
    this->computeLagrangeP1Value(adjacency_local_coordinate, adjacency_p1_basis_function_value);
    Device::StaticMatrix<Real, VolumeElementTrait::kDimension, AdjacencyElementTrait::kBasicNodeNumber>
        adjacency_basic_node_coordinate;
    Isize node_column = 0;
    for (Isize i = 0; i < adjacency_sequence; i++) {
      node_column += kVolumeElementPerAdjacencyNodeNumber[static_cast<Usize>(i)];
    }
    for (Isize i = 0; i < AdjacencyElementTrait::kBasicNodeNumber; i++) {
      for (Isize m = 0; m < VolumeElementTrait::kDimension; m++) {
        adjacency_basic_node_coordinate(m, i) =
            volume_basic_node_coordinate(m, kVolumeElementPerAdjacencyNodeIndex[static_cast<Usize>(node_column + i)]);
      }
    }
    for (Isize m = 0; m < VolumeElementTrait::kDimension; m++) {
      Real sum = 0.0_r;
      for (Isize n = 0; n < AdjacencyElementTrait::kBasicNodeNumber; n++) {
        sum += adjacency_basic_node_coordinate(m, n) * adjacency_p1_basis_function_value(n);
      }
      volume_local_coordinate(m) = sum;
    }
  }

  void transferAdjacencyElementBasisFunctionToDevice(
      const AdjacencyElementBasisFunction<AdjacencyElementTrait>& adjacency_element_basis_function) {
    this->ElementBasisFunctionDevice<AdjacencyElementTrait>::transferVolumeElementBasisFunctionToDevice(
        static_cast<const ElementBasisFunction<AdjacencyElementTrait>&>(adjacency_element_basis_function));
    Utils::transferToDevice<Real, AdjacencyElementTrait::kQuadratureNumber,
                            AdjacencyElementTrait::kAllBasisFunctionNumber>(
        adjacency_element_basis_function.nodal_basis_function_, this->nodal_basis_function_);
    Utils::transferToDevice<Real, AdjacencyElementTrait::kDimension * AdjacencyElementTrait::kQuadratureNumber,
                            AdjacencyElementTrait::kAllBasisFunctionNumber>(
        adjacency_element_basis_function.nodal_gradient_basis_function_, this->nodal_gradient_basis_function_);
  }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_BASIS_FUNCTION_CPP_
