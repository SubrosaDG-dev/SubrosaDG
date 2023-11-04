/**
 * @file get_elem_integral.hpp
 * @brief The head file to get element integral information.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-07-14
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_GET_ELEM_INTEGRAL_HPP_
#define SUBROSA_DG_GET_ELEM_INTEGRAL_HPP_

#include <fmt/core.h>
#include <gmsh.h>

#include <vector>

#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "integral/get_gauss_quad.hpp"
#include "integral/get_integral_num.hpp"
#include "integral/integral_structure.hpp"
#include "mesh/get_elem_info.hpp"

namespace SubrosaDG {

template <PolyOrder P, ElemType ElemT>
inline void getElemIntegral(ElemIntegral<P, ElemT>& elem_integral) {
  using T = ElemIntegral<P, ElemT>;
  std::vector<double> local_coords = getElemGaussQuad<T::kIntegralNum, P>(getElemIntegralOrder(P), elem_integral);
  int num_components;
  int num_orientations;
  std::vector<double> basis_functions;
  gmsh::model::mesh::getBasisFunctions(getTopology<ElemT>(P), local_coords,
                                       fmt::format("Lagrange{}", static_cast<int>(P)), num_components, basis_functions,
                                       num_orientations);
  for (Isize i = 0; i < T::kIntegralNum; i++) {
    for (Isize j = 0; j < T::kBasisFunNum; j++) {
      elem_integral.basis_fun_(i, j) = static_cast<Real>(basis_functions[static_cast<Usize>(i * T::kBasisFunNum + j)]);
    }
  }
  std::vector<double> grad_basis_functions;
  gmsh::model::mesh::getBasisFunctions(getTopology<ElemT>(P), local_coords,
                                       fmt::format("GradLagrange{}", static_cast<int>(P)), num_components,
                                       grad_basis_functions, num_orientations);
  for (Isize i = 0; i < T::kIntegralNum; i++) {
    for (Isize j = 0; j < T::kBasisFunNum; j++) {
      for (Isize k = 0; k < getDim<ElemT>(); k++) {
        elem_integral.grad_basis_fun_(i * getDim<ElemT>() + k, j) =
            static_cast<Real>(grad_basis_functions[static_cast<Usize>((i * T::kBasisFunNum + j) * 3 + k)]);
      }
    }
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_ELEM_INTEGRAL_HPP_
