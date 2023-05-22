/**
 * @file get_element_integral.hpp
 * @brief The header file to get element integral.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-12
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_GET_ELEMENT_INTEGRAL_HPP_
#define SUBROSA_DG_GET_ELEMENT_INTEGRAL_HPP_

// clang-format off

#include <fmt/core.h>              // for format
#include <gmsh.h>                  // for getBasisFunctions, getIntegrationPoints
#include <vector>                  // for vector

#include "basic/data_types.hpp"    // for Isize, Usize, Real
#include "mesh/element_types.hpp"  // for ElementType, BasisFunction

// clang-format on

namespace SubrosaDG {

template <ElementType Type, Isize PolynomialOrder>
struct ElementGradIntegral;
template <ElementType Type, Isize PolynomialOrder>
struct ElementIntegral;

template <ElementType Type, Isize PolynomialOrder>
inline void getIntegrationPoints(std::vector<double>& local_coords, std::vector<double>& weights) {
  gmsh::model::mesh::getIntegrationPoints(Type.kElementTag, fmt::format("Gauss{}", 2 * PolynomialOrder + 1),
                                          local_coords, weights);
}

template <ElementType Type, Isize PolynomialOrder, bool NeedGrad>
inline void getAllBasisFunctions(const std::vector<double>& local_coords, std::vector<double>& basis_functions) {
  int num_components;
  int num_orientations;
  if constexpr (NeedGrad) {
    gmsh::model::mesh::getBasisFunctions(Type.kElementTag, local_coords, fmt::format("GradLagrange{}", PolynomialOrder),
                                         num_components, basis_functions, num_orientations);
  } else {
    gmsh::model::mesh::getBasisFunctions(Type.kElementTag, local_coords, fmt::format("Lagrange{}", PolynomialOrder),
                                         num_components, basis_functions, num_orientations);
  }
}

template <typename T, ElementType Type, Isize PolynomialOrder>
inline std::vector<double> getElementGaussianQuadrature() {
  std::vector<double> local_coords;
  std::vector<double> weights;
  getIntegrationPoints<Type, PolynomialOrder>(local_coords, weights);
  T::integral_nodes_num = static_cast<Isize>(weights.size());
  T::integral_nodes.resize(Type.kDimension, T::integral_nodes_num);
  T::weights.resize(T::integral_nodes_num);
  for (Usize i = 0; i < weights.size(); i++) {
    for (Usize j = 0; j < static_cast<Usize>(Type.kDimension); j++) {
      T::integral_nodes(static_cast<Isize>(j), static_cast<Isize>(i)) = static_cast<Real>(local_coords[3 * i + j]);
    }
    T::weights(static_cast<Isize>(i)) = static_cast<Real>(weights[i]);
  }
  return local_coords;
}

template <typename T, ElementType Type, Isize PolynomialOrder>
inline void getElementIntegral(const std::vector<double>& local_coords) {
  std::vector<double> basis_functions;
  getAllBasisFunctions<Type, PolynomialOrder, false>(local_coords, basis_functions);
  T::basis_functions.resize(BasisFunction<Type, PolynomialOrder>::kNum, T::integral_nodes_num);
  for (Usize i = 0; i < static_cast<Usize>(T::integral_nodes_num); i++) {
    for (Usize j = 0; j < static_cast<Usize>(BasisFunction<Type, PolynomialOrder>::kNum); j++) {
      T::basis_functions(static_cast<Isize>(j), static_cast<Isize>(i)) =
          static_cast<Real>(basis_functions[i * static_cast<Usize>(BasisFunction<Type, PolynomialOrder>::kNum) + j]);
    }
  }
}

template <typename T, ElementType Type, Isize PolynomialOrder>
inline void getElementGradIntegral(const std::vector<double>& local_coords) {
  T::local_mass_matrix_inverse.noalias() =
      ((T::basis_functions.array().rowwise() * T::weights.transpose().array()).matrix() *
       T::basis_functions.transpose())
          .inverse();
  std::vector<double> grad_basis_functions;
  getAllBasisFunctions<Type, PolynomialOrder, true>(local_coords, grad_basis_functions);
  T::grad_basis_functions.resize(Type.kDimension * BasisFunction<Type, PolynomialOrder>::kNum, T::integral_nodes_num);
  for (Usize i = 0; i < static_cast<Usize>(T::integral_nodes_num); i++) {
    for (Usize j = 0; j < static_cast<Usize>(BasisFunction<Type, PolynomialOrder>::kNum); j++) {
      for (Usize k = 0; k < static_cast<Usize>(Type.kDimension); k++) {
        T::grad_basis_functions(static_cast<Isize>(j * Type.kDimension + k), static_cast<Isize>(i)) = static_cast<Real>(
            grad_basis_functions[i * static_cast<Usize>(BasisFunction<Type, PolynomialOrder>::kNum * 3) + j * 3 + k]);
      }
    }
  }
}

template <bool IsAdjacencyElement, ElementType Type, Isize PolynomialOrder>
struct FElementIntegral {};

template <ElementType Type, Isize PolynomialOrder>
struct FElementIntegral<true, Type, PolynomialOrder> {
  inline static void get() {
    std::vector<double> local_coords =
        getElementGaussianQuadrature<ElementIntegral<Type, PolynomialOrder>, Type, PolynomialOrder>();
    getElementIntegral<ElementIntegral<Type, PolynomialOrder>, Type, PolynomialOrder>(local_coords);
  };
};

template <ElementType Type, Isize PolynomialOrder>
struct FElementIntegral<false, Type, PolynomialOrder> {
  inline static void get() {
    std::vector<double> local_coords =
        getElementGaussianQuadrature<ElementIntegral<Type, PolynomialOrder>, Type, PolynomialOrder>();
    getElementIntegral<ElementIntegral<Type, PolynomialOrder>, Type, PolynomialOrder>(local_coords);
    getElementGradIntegral<ElementGradIntegral<Type, PolynomialOrder>, Type, PolynomialOrder>(local_coords);
  };
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_ELEMENT_INTEGRAL_HPP_
