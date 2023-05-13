/**
 * @file get_integration.cpp
 * @brief The source file to get element integration.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-12
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

// clang-format off

#include "mesh/get_integration.h"

#include <fmt/core.h>             // for format
#include <gmsh.h>                 // for getBasisFunctions, getIntegrationPoints, getJacobian
#include <Eigen/Core>             // for Block, Matrix, Vector, DenseCoeffsBase, CommaInitializer, DenseBase, DenseB...
#include <cstddef>                // for size_t
#include <vector>                 // for vector
#include <utility>                // for pair

#include "mesh/mesh_structure.h"  // for ElementIntegral, ElementGradIntegral, ElementMesh

// clang-format on

namespace SubrosaDG::Internal {

void getElementJacobian(ElementMesh& element_mesh) {
  element_mesh.elements_jacobian_.resize(element_mesh.elements_num_);
  std::vector<double> local_coord{0.0, 0.0, 0.0};
  std::vector<double> jacobians;
  std::vector<double> determinants;
  std::vector<double> coord;
  for (Isize i = 0; i < element_mesh.elements_num_; i++) {
    gmsh::model::mesh::getJacobian(static_cast<std::size_t>(i + element_mesh.elements_range_.first), local_coord,
                                   jacobians, determinants, coord);
    element_mesh.elements_jacobian_(i) = static_cast<Real>(determinants[0]);
  }
}

std::vector<double> getElementIntegral(Isize polynomial_order, Isize gauss_integral_accuracy,
                                       ElementIntegral& element_integral) {
  std::vector<double> local_coords;
  std::vector<double> weights;
  gmsh::model::mesh::getIntegrationPoints(element_integral.element_type_,
                                          fmt::format("Gauss{}", gauss_integral_accuracy), local_coords, weights);
  element_integral.integral_nodes_num_ = static_cast<Isize>(weights.size());
  element_integral.integral_nodes_.resize(3, element_integral.integral_nodes_num_);
  element_integral.weights_.resize(element_integral.integral_nodes_num_);
  for (Usize i = 0; i < weights.size(); i++) {
    element_integral.integral_nodes_.col(static_cast<Isize>(i)) << static_cast<Real>(local_coords[3 * i]),
        static_cast<Real>(local_coords[3 * i + 1]), static_cast<Real>(local_coords[3 * i + 2]);
    element_integral.weights_(static_cast<Isize>(i)) = static_cast<Real>(weights[i]);
  }
  int num_components;
  int num_orientations;
  std::vector<double> basis_functions;
  gmsh::model::mesh::getBasisFunctions(element_integral.element_type_, local_coords,
                                       fmt::format("Lagrange{}", polynomial_order), num_components, basis_functions,
                                       num_orientations);
  element_integral.basis_function_num_ = static_cast<Isize>(basis_functions.size() / weights.size());
  element_integral.basis_functions_.resize(element_integral.basis_function_num_, element_integral.integral_nodes_num_);
  for (Usize i = 0; i < static_cast<Usize>(element_integral.integral_nodes_num_); i++) {
    for (Usize j = 0; j < static_cast<Usize>(element_integral.basis_function_num_); j++) {
      element_integral.basis_functions_(static_cast<Isize>(j), static_cast<Isize>(i)) =
          static_cast<Real>(basis_functions[i * static_cast<Usize>(element_integral.basis_function_num_) + j]);
    }
  }
  return local_coords;
}

void getElementGradIntegral(Isize polynomial_order, Isize gauss_integral_accuracy,
                            ElementGradIntegral& element_grad_integral) {
  std::vector<double> local_coords =
      getElementIntegral(polynomial_order, gauss_integral_accuracy, element_grad_integral);
  int num_components;
  int num_orientations;
  std::vector<double> basis_functions;
  gmsh::model::mesh::getBasisFunctions(element_grad_integral.element_type_, local_coords,
                                       fmt::format("GradLagrange{}", polynomial_order), num_components, basis_functions,
                                       num_orientations);
  element_grad_integral.grad_basis_functions_.resize(element_grad_integral.basis_function_num_ * 3,
                                                     element_grad_integral.integral_nodes_num_);
  for (Usize i = 0; i < static_cast<Usize>(element_grad_integral.integral_nodes_num_); i++) {
    for (Usize j = 0; j < static_cast<Usize>(element_grad_integral.basis_function_num_ * 3); j++) {
      element_grad_integral.grad_basis_functions_(static_cast<Isize>(j), static_cast<Isize>(i)) =
          static_cast<Real>(basis_functions[i * static_cast<Usize>(element_grad_integral.basis_function_num_ * 3) + j]);
    }
  }
}

}  // namespace SubrosaDG::Internal
