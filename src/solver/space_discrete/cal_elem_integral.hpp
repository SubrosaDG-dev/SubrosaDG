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

#include "basic/concept.hpp"
#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "config/thermo_model.hpp"
#include "integral/integral_structure.hpp"
#include "mesh/mesh_structure.hpp"
#include "solver/solver_structure.hpp"
#include "solver/variable/cal_convective_var.hpp"
#include "solver/variable/cal_primitive_var.hpp"
#include "solver/variable/get_var_num.hpp"

namespace SubrosaDG {

template <int Dim, PolyOrder P, ElemType ElemT, EquModel EquModelT>
inline void calElemIntegral(const ElemIntegral<P, ElemT>& elem_integral, const ElemMesh<Dim, P, ElemT>& elem_mesh,
                            const ThermoModel<EquModelT>& thermo_model,
                            ElemSolver<Dim, P, ElemT, EquModelT>& elem_solver) {
  Eigen::Vector<Real, getConservedVarNum<EquModelT>(Dim)> conserved_var;
  Eigen::Vector<Real, getPrimitiveVarNum<EquModelT>(Dim)> primitive_var;
  Eigen::Matrix<Real, getConservedVarNum<EquModelT>(Dim), Dim> convective_var;
  Eigen::Matrix<Real, getConservedVarNum<EquModelT>(Dim), Dim> elem_solver_integral;
#ifdef SUBROSA_DG_WITH_OPENMP
#pragma omp parallel for default(none) schedule(auto)                                                  \
    shared(Eigen::all, Eigen::fix <Dim>, elem_mesh, elem_integral, thermo_model, elem_solver) private( \
        conserved_var, primitive_var, convective_var, elem_solver_integral)
#endif
  for (Isize i = 0; i < elem_mesh.num_; i++) {
    for (Isize j = 0; j < elem_integral.kIntegralNum; j++) {
      conserved_var.noalias() = elem_solver.elem_(i).basis_fun_coeff_(1) * elem_integral.basis_fun_.row(j).transpose();
      calPrimitiveVar(thermo_model, conserved_var, primitive_var);
      calConvectiveVar<EquModelT>(primitive_var, convective_var);
      elem_solver_integral.noalias() =
          convective_var * elem_mesh.elem_(i).jacobian_trans_inv_(Eigen::all, Eigen::seqN(j * Dim, Eigen::fix<Dim>)) *
          elem_mesh.elem_(i).jacobian_det_(j) * elem_integral.weight_(j);
      elem_solver.elem_(i).elem_integral_(Eigen::all, Eigen::seqN(j * Dim, Eigen::fix<Dim>)) = elem_solver_integral;
    }
  }
}

template <PolyOrder P, MeshType MeshT, EquModel EquModelT, TimeDiscrete TimeDiscreteT>
inline void calElemIntegral(const Integral<2, P, MeshT>& integral, const Mesh<2, P, MeshT>& mesh,
                            const SolverSupplemental<2, EquModelT, TimeDiscreteT>& solver_supplemental,
                            Solver<2, P, MeshT, EquModelT>& solver) {
  if constexpr (HasTri<MeshT>) {
    calElemIntegral(integral.tri_, mesh.tri_, solver_supplemental.thermo_model_, solver.tri_);
  }
  if constexpr (HasQuad<MeshT>) {
    calElemIntegral(integral.quad_, mesh.quad_, solver_supplemental.thermo_model_, solver.quad_);
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CAL_ELEM_INTEGRAL_HPP_
