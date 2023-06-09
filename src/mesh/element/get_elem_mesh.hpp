/**
 * @file get_elem_mesh.hpp
 * @brief The get element mesh header file.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-22
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_GET_ELEM_MESH_HPP_
#define SUBROSA_DG_GET_ELEM_MESH_HPP_

#include <gmsh.h>

#include <Eigen/Core>
#include <utility>
#include <vector>

#include "basic/data_type.hpp"
#include "mesh/elem_type.hpp"

namespace SubrosaDG {

template <int Dim, ElemInfo ElemT>
struct ElemMesh;

template <int Dim, ElemInfo ElemT>
inline void getElemMesh(const Eigen::Matrix<Real, Dim, Eigen::Dynamic>& nodes, ElemMesh<Dim, ElemT>& elem_mesh) {
  std::vector<Usize> elem_tags;
  std::vector<Usize> elem_node_tag;
  gmsh::model::mesh::getElementsByType(ElemT.kTopology, elem_tags, elem_node_tag);
  if (elem_tags.empty()) {
    elem_mesh.range_ = std::make_pair(0, 0);
    elem_mesh.num_ = 0;
  } else {
    elem_mesh.range_ = std::make_pair(elem_tags.front(), elem_tags.back());
    elem_mesh.num_ = static_cast<Isize>(elem_tags.size());
    elem_mesh.elem_.resize(elem_mesh.num_);
    for (Isize i = 0; i < elem_mesh.num_; i++) {
      for (Isize j = 0; j < ElemT.kNodeNum; j++) {
        auto node_tag = static_cast<Isize>(elem_node_tag[static_cast<Usize>(i * ElemT.kNodeNum + j)]);
        elem_mesh.elem_(i).node_.col(j) = nodes.col(node_tag - 1);
        elem_mesh.elem_(i).index_(j) = node_tag;
      }
    }
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_ELEM_MESH_HPP_
