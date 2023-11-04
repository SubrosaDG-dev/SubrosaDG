/**
 * @file init_view.hpp
 * @brief The head file to initialize the view structure.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-08-11
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_INIT_VIEW_HPP_
#define SUBROSA_DG_INIT_VIEW_HPP_

#include "basic/concept.hpp"
#include "basic/enum.hpp"
#include "mesh/mesh_structure.hpp"
#include "view/variable/get_output_var_num.hpp"
#include "view/view_structure.hpp"

namespace SubrosaDG {

template <int Dim, PolyOrder P, ElemType ElemT, EquModel EquModelT>
inline void initElemSolverView(const ElemMesh<Dim, P, ElemT>& elem_mesh,
                               ElemSolverView<Dim, P, ElemT, EquModelT>& elem_solver_view) {
  elem_solver_view.elem_.resize(elem_mesh.num_);
}

template <PolyOrder P, MeshType MeshT, EquModel EquModelT>
inline void initView(const Mesh<2, P, MeshT>& mesh, View<2, P, MeshT, EquModelT>& view) {
  view.node_.output_var_.resize(getOutputVarNum<EquModelT>(2), mesh.node_num_);
  if constexpr (HasTri<MeshT>) {
    initElemSolverView(mesh.tri_, view.tri_);
  }
  if constexpr (HasQuad<MeshT>) {
    initElemSolverView(mesh.quad_, view.quad_);
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_INIT_VIEW_HPP_
