/**
 * @file MassMatrix.hpp
 * @brief The header file of SubrosaDG MassMatrix.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-14
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_MASS_MATRIX_HPP_
#define SUBROSA_DG_MASS_MATRIX_HPP_

#include "Mesh/ReadControl.hpp"
#include "Utils/BasicDataType.hpp"

namespace SubrosaDG {

template <typename ElementTrait>
inline void ElementMesh<ElementTrait>::calculateElementLocalMassMatrixInverse() {
  for (Isize i = 0; i < this->number_; i++) {
    this->element_(i).local_mass_matrix_inverse_.noalias() =
        (this->basis_function_.value_.transpose() *
         (this->basis_function_.value_.array().colwise() *
          (this->gaussian_quadrature_.weight_.array() * this->element_(i).jacobian_determinant_.array()))
             .matrix())
            .inverse();
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_MASS_MATRIX_HPP_
