/**
 * @file cal_elem_integral.hpp
 * @brief The calculate element integral header file.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-06-05
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CAL_ELEM_INTEGRAL_HPP_
#define SUBROSA_DG_CAL_ELEM_INTEGRAL_HPP_

#include <Eigen/Core>

#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "solver/variable/cal_convective_var.hpp"
#include "solver/variable/cal_primitive_var.hpp"

namespace SubrosaDG {

template <EquModel EquModelT>
struct ThermoModel;
template <int Dim, ElemType ElemT>
struct ElemMesh;
template <int PolyOrder, ElemType ElemT>
struct ElemIntegral;
template <int Dim, int PolyOrder, ElemType ElemT, EquModel EquModelT>
struct PerElemSolver;

template <int Dim, int PolyOrder, ElemType ElemT, EquModel EquModelT>
inline void calElemIntegral(
    const ElemMesh<Dim, ElemT>& elem_mesh, const ElemIntegral<PolyOrder, ElemT>& elem_integral,
    const ThermoModel<EquModel::Euler>& thermo_model,
    Eigen::Vector<PerElemSolver<Dim, PolyOrder, ElemT, EquModelT>, Eigen::Dynamic>& elem_solver) {
  Eigen::Vector<Real, Dim + 2> conserved_var;
  Eigen::Vector<Real, Dim + 3> primitive_var;
  Eigen::Matrix<Real, Dim + 2, Dim> convective_var;
  Eigen::Matrix<Real, Dim + 2, Dim> elem_solver_integral;
  for (Isize i = 0; i < elem_mesh.num_; i++) {
    for (Isize j = 0; j < elem_integral.kIntegralNum; j++) {
      conserved_var.noalias() = elem_solver(i).basis_fun_coeff_(1) * elem_integral.basis_fun_.row(j).transpose();
      calPrimitiveVar(thermo_model, conserved_var, primitive_var);
      calConvectiveVar(primitive_var, convective_var);
      elem_solver_integral.noalias() = convective_var * elem_mesh.elem_(i).jacobian_ * elem_integral.weight_(j);
      elem_solver(i).elem_integral_(Eigen::all, Eigen::seqN(j * Dim, Eigen::fix<Dim>)) = elem_solver_integral;
    }
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CAL_ELEM_INTEGRAL_HPP_
