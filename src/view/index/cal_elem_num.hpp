/**
 * @file cal_elem_num.hpp
 * @brief The calculate element number header file.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-08-15
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CAL_ELEM_NUM_HPP_
#define SUBROSA_DG_CAL_ELEM_NUM_HPP_

#include "basic/concept.hpp"
#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "mesh/mesh_structure.hpp"
#include "view/index/get_subelem_num.hpp"

namespace SubrosaDG {

template <PolyOrder P, ElemType ElemT>
inline Isize calElemNum(const ElemMesh<2, P, ElemT>& elem_mesh) {
  return elem_mesh.num_ * getSubElemNum<ElemT>(P);
}

template <PolyOrder P, MeshType MeshT>
inline Isize calElemNum(const Mesh<2, P, MeshT>& mesh) {
  Isize elem_num = 0;
  if constexpr (HasTri<MeshT>) {
    elem_num += calElemNum(mesh.tri_);
  }
  if constexpr (HasQuad<MeshT>) {
    elem_num += calElemNum(mesh.quad_);
  }
  return elem_num;
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CAL_ELEM_NUM_HPP_
