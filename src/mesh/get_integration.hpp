/**
 * @file get_integration.hpp
 * @brief The header file to get element integration.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-12
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_GET_INTEGRATION_HPP_
#define SUBROSA_DG_GET_INTEGRATION_HPP_

// clang-format off

#include <fmt/core.h>               // for format
#include <gmsh.h>                   // for getBasisFunctions, getIntegrationPoints, getJacobian
#include <vector>                   // for vector
#include <algorithm>                // for copy
#include <cstddef>                  // for size_t

#include "basic/data_types.hpp"     // for Isize, Usize, Real
#include "mesh/mesh_structure.hpp"  // for ElementGradIntegral, ElementIntegral, ElementMeshNoIndex (ptr only)
#include "mesh/element_types.hpp"   // for ElementType

// clang-format on

namespace SubrosaDG {

template <ElementType Type>
inline void getElementJacobian(ElementMeshNoIndex<Type>& element_mesh_no_index) {
  element_mesh_no_index.elements_jacobian_.resize(element_mesh_no_index.elements_num_);
  const std::vector<double> local_coord{0.0, 0.0, 0.0};
  std::vector<double> jacobians;
  std::vector<double> determinants;
  std::vector<double> coord;
  for (Isize i = 0; i < element_mesh_no_index.elements_num_; i++) {
    gmsh::model::mesh::getJacobian(static_cast<std::size_t>(i + element_mesh_no_index.elements_range_.first),
                                   local_coord, jacobians, determinants, coord);
    element_mesh_no_index.elements_jacobian_(i) = static_cast<Real>(determinants[0]);
  }
}

template <ElementType Type, Isize PolynomialOrder>
inline void getIntegrationPoints(std::vector<double>& local_coords, std::vector<double>& weights) {
  gmsh::model::mesh::getIntegrationPoints(Type.kElementTag, fmt::format("Gauss{}", 2 * PolynomialOrder + 1),
                                          local_coords, weights);
}

template <ElementType Type, Isize PolynomialOrder>
inline void getBasisFunctions(const std::vector<double>& local_coords, std::vector<double>& basis_functions) {
  int num_components;
  int num_orientations;
  gmsh::model::mesh::getBasisFunctions(Type.kElementTag, local_coords, fmt::format("Lagrange{}", PolynomialOrder),
                                       num_components, basis_functions, num_orientations);
}

template <ElementType Type, Isize PolynomialOrder>
inline void getBasisGradFunctions(const std::vector<double>& local_coords, std::vector<double>& grad_basis_functions) {
  int num_components;
  int num_orientations;
  gmsh::model::mesh::getBasisFunctions(Type.kElementTag, local_coords, fmt::format("GradLagrange{}", PolynomialOrder),
                                       num_components, grad_basis_functions, num_orientations);
}

template <ElementType Type, Isize PolynomialOrder>
inline void getElementIntegral() {
  std::vector<double> local_coords;
  std::vector<double> weights;
  getIntegrationPoints<Type, PolynomialOrder>(local_coords, weights);
  ElementIntegral<Type, PolynomialOrder>::integral_nodes_num = static_cast<Isize>(weights.size());
  ElementIntegral<Type, PolynomialOrder>::integral_nodes.resize(
      3, ElementIntegral<Type, PolynomialOrder>::integral_nodes_num);
  ElementIntegral<Type, PolynomialOrder>::weights.resize(ElementIntegral<Type, PolynomialOrder>::integral_nodes_num);
  for (Usize i = 0; i < weights.size(); i++) {
    ElementIntegral<Type, PolynomialOrder>::integral_nodes.col(static_cast<Isize>(i))
        << static_cast<Real>(local_coords[3 * i]),
        static_cast<Real>(local_coords[3 * i + 1]), static_cast<Real>(local_coords[3 * i + 2]);
    ElementIntegral<Type, PolynomialOrder>::weights(static_cast<Isize>(i)) = static_cast<Real>(weights[i]);
  }
  std::vector<double> basis_functions;
  getBasisFunctions<Type, PolynomialOrder>(local_coords, basis_functions);
  ElementIntegral<Type, PolynomialOrder>::basis_function_num =
      static_cast<Isize>(basis_functions.size() / weights.size());
  ElementIntegral<Type, PolynomialOrder>::basis_functions.resize(
      ElementIntegral<Type, PolynomialOrder>::basis_function_num,
      ElementIntegral<Type, PolynomialOrder>::integral_nodes_num);
  for (Usize i = 0; i < static_cast<Usize>(ElementIntegral<Type, PolynomialOrder>::integral_nodes_num); i++) {
    for (Usize j = 0; j < static_cast<Usize>(ElementIntegral<Type, PolynomialOrder>::basis_function_num); j++) {
      ElementIntegral<Type, PolynomialOrder>::basis_functions(static_cast<Isize>(j), static_cast<Isize>(i)) =
          static_cast<Real>(
              basis_functions[i * static_cast<Usize>(ElementIntegral<Type, PolynomialOrder>::basis_function_num) + j]);
    }
  }
}

template <ElementType Type, Isize PolynomialOrder>
inline void getElementGradIntegral() {
  std::vector<double> local_coords;
  std::vector<double> weights;
  getIntegrationPoints<Type, PolynomialOrder>(local_coords, weights);
  ElementGradIntegral<Type, PolynomialOrder>::integral_nodes_num = static_cast<Isize>(weights.size());
  ElementGradIntegral<Type, PolynomialOrder>::integral_nodes.resize(
      3, ElementGradIntegral<Type, PolynomialOrder>::integral_nodes_num);
  ElementGradIntegral<Type, PolynomialOrder>::weights.resize(
      ElementGradIntegral<Type, PolynomialOrder>::integral_nodes_num);
  for (Usize i = 0; i < weights.size(); i++) {
    ElementGradIntegral<Type, PolynomialOrder>::integral_nodes.col(static_cast<Isize>(i))
        << static_cast<Real>(local_coords[3 * i]),
        static_cast<Real>(local_coords[3 * i + 1]), static_cast<Real>(local_coords[3 * i + 2]);
    ElementGradIntegral<Type, PolynomialOrder>::weights(static_cast<Isize>(i)) = static_cast<Real>(weights[i]);
  }
  std::vector<double> basis_functions;
  getBasisFunctions<Type, PolynomialOrder>(local_coords, basis_functions);
  ElementGradIntegral<Type, PolynomialOrder>::basis_function_num =
      static_cast<Isize>(basis_functions.size() / weights.size());
  ElementGradIntegral<Type, PolynomialOrder>::basis_functions.resize(
      ElementGradIntegral<Type, PolynomialOrder>::basis_function_num,
      ElementGradIntegral<Type, PolynomialOrder>::integral_nodes_num);
  for (Usize i = 0; i < static_cast<Usize>(ElementGradIntegral<Type, PolynomialOrder>::integral_nodes_num); i++) {
    for (Usize j = 0; j < static_cast<Usize>(ElementGradIntegral<Type, PolynomialOrder>::basis_function_num); j++) {
      ElementGradIntegral<Type, PolynomialOrder>::basis_functions(static_cast<Isize>(j), static_cast<Isize>(i)) =
          static_cast<Real>(
              basis_functions[i * static_cast<Usize>(ElementGradIntegral<Type, PolynomialOrder>::basis_function_num) +
                              j]);
    }
  }
  std::vector<double> grad_basis_functions;
  getBasisGradFunctions<Type, PolynomialOrder>(local_coords, grad_basis_functions);
  ElementGradIntegral<Type, PolynomialOrder>::grad_basis_functions.resize(
      ElementGradIntegral<Type, PolynomialOrder>::basis_function_num * 3,
      ElementGradIntegral<Type, PolynomialOrder>::integral_nodes_num);
  for (Usize i = 0; i < static_cast<Usize>(ElementGradIntegral<Type, PolynomialOrder>::integral_nodes_num); i++) {
    for (Usize j = 0; j < static_cast<Usize>(ElementGradIntegral<Type, PolynomialOrder>::basis_function_num * 3); j++) {
      ElementGradIntegral<Type, PolynomialOrder>::grad_basis_functions(static_cast<Isize>(j), static_cast<Isize>(i)) =
          static_cast<Real>(
              grad_basis_functions
                  [i * static_cast<Usize>(ElementGradIntegral<Type, PolynomialOrder>::basis_function_num * 3) + j]);
    }
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_INTEGRATION_HPP_
