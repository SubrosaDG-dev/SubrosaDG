/**
 * @file cal_nodal_var.hpp
 * @brief The calculate nodal variable header file.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-07-16
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CAL_NODAL_VAR_HPP_
#define SUBROSA_DG_CAL_NODAL_VAR_HPP_

#include <Eigen/Core>

#include "basic/concept.hpp"
#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "config/thermo_model.hpp"
#include "mesh/get_elem_info.hpp"
#include "mesh/mesh_structure.hpp"
#include "solver/variable/get_var_num.hpp"
#include "view/variable/cal_output_var.hpp"
#include "view/variable/get_output_var_num.hpp"
#include "view/view_structure.hpp"

namespace SubrosaDG {

template <int Dim, PolyOrder P, ElemType ElemT, EquModel EquModelT>
inline void addElemNodalVar(const ElemMesh<Dim, P, ElemT>& elem_mesh,
                            const ElemSolverView<Dim, P, ElemT, EquModelT>& elem_solver_view,
                            const ThermoModel<EquModelT>& thermo_model,
                            NodeSolverView<Dim, EquModelT>& node_solver_view) {
  Eigen::Vector<Real, getConservedVarNum<EquModelT>(Dim)> conserved_var;
  Eigen::Vector<Real, getOutputVarNum<EquModelT>(Dim)> output_var;
  for (Isize i = 0; i < elem_mesh.num_; i++) {
    for (Isize j = 0; j < getNodeNum<ElemT>(P); j++) {
      conserved_var = elem_solver_view.elem_(i).basis_fun_coeff_.col(j);
      calOutputVar<EquModelT>(thermo_model, conserved_var, output_var);
      node_solver_view.output_var_.col(elem_mesh.elem_(i).index_(j) - 1) += output_var;
    }
  }
}

template <PolyOrder P, MeshType MeshT, EquModel EquModelT>
inline void calNodalVar(const Mesh<2, P, MeshT>& mesh, const ThermoModel<EquModelT>& thermo_model,
                        View<2, P, MeshT, EquModelT>& view) {
  view.node_.output_var_.setZero();
  if constexpr (HasTri<MeshT>) {
    addElemNodalVar(mesh.tri_, view.tri_, thermo_model, view.node_);
  }
  if constexpr (HasQuad<MeshT>) {
    addElemNodalVar(mesh.quad_, view.quad_, thermo_model, view.node_);
  }
  view.node_.output_var_.array().rowwise() /= mesh.node_elem_num_.template cast<Real>().transpose().array();
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CAL_NODAL_VAR_HPP_
