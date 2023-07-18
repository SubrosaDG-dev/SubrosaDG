/**
 * @file cal_mass_mat.hpp
 * @brief The header file to calculate the mass matrix inverse.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-07-15
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CAL_MASS_MAT_HPP_
#define SUBROSA_DG_CAL_MASS_MAT_HPP_

#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "integral/integral_structure.hpp"
#include "mesh/mesh_structure.hpp"

namespace SubrosaDG {

template <int Dim, PolyOrder P, ElemType ElemT>
inline void calElemLocalMassMatInv(const ElemIntegral<P, ElemT>& elem_integral, ElemMesh<Dim, P, ElemT>& elem_mesh) {
  for (Isize i = 0; i < elem_mesh.num_; i++) {
    elem_mesh.elem_(i).local_mass_mat_inv_.noalias() =
        (elem_integral.basis_fun_.transpose() *
         (elem_integral.basis_fun_.array().colwise() *
          (elem_integral.weight_.array() * elem_mesh.elem_(i).jacobian_det_.array()))
             .matrix())
            .inverse();
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CAL_MASS_MAT_HPP_
