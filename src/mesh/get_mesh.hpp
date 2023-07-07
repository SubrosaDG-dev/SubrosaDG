/**
 * @file get_mesh.hpp
 * @brief This head file to get mesh.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2022-11-02
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_GET_MESH_HPP_
#define SUBROSA_DG_GET_MESH_HPP_

#include <gmsh.h>

#include <string_view>
#include <unordered_map>
#include <vector>

#include "basic/concept.hpp"
#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "mesh/element/cal_norm_vec.hpp"
#include "mesh/element/cal_projection_measure.hpp"
#include "mesh/element/get_adjacency_mesh.hpp"
#include "mesh/element/get_elem_mesh.hpp"
#include "mesh/element/get_jacobian.hpp"
#include "mesh/mesh_structure.hpp"

namespace SubrosaDG {

template <int Dim, PolyOrder P>
inline void getNodes(MeshBase<Dim, P>& mesh_base) {
  std::vector<Usize> node_tags;
  std::vector<double> node_coords;
  std::vector<double> node_params;
  gmsh::model::mesh::getNodes(node_tags, node_coords, node_params);
  mesh_base.node_num_ = static_cast<Isize>(node_tags.size());
  mesh_base.node_.resize(Dim, static_cast<Isize>(node_tags.size()));
  for (const auto node_tag : node_tags) {
    for (Usize i = 0; i < Dim; i++) {
      mesh_base.node_(static_cast<Isize>(i), static_cast<Isize>(node_tag - 1)) =
          static_cast<Real>(node_coords[3 * (node_tag - 1) + i]);
    }
  }
}

template <PolyOrder P, MeshType MeshT>
inline void getElemNum(Mesh<2, P, MeshT>& mesh) {
  if constexpr (HasTri<MeshT>) {
    mesh.elem_num_ += mesh.tri_.num_;
  }
  if constexpr (HasQuad<MeshT>) {
    mesh.elem_num_ += mesh.quad_.num_;
  }
}

template <PolyOrder P, MeshType MeshT>
inline void getMesh(const std::unordered_map<std::string_view, Boundary>& boundary_type_map, Mesh<2, P, MeshT>& mesh) {
  getNodes(mesh);
  if constexpr (HasTri<MeshT>) {
    getElemMesh(mesh.node_, mesh.tri_);
    calElemProjectionMeasure(mesh.tri_);
    getElemJacobian(mesh.tri_);
  }
  if constexpr (HasQuad<MeshT>) {
    getElemMesh(mesh.node_, mesh.quad_);
    calElemProjectionMeasure(mesh.quad_);
    getElemJacobian(mesh.quad_);
  }
  getElemNum(mesh);
  getAdjacencyElemMesh<2, ElemType::Line, MeshT>(mesh.node_, boundary_type_map, mesh.line_);
  calAdjacencyElemNormVec(mesh.line_);
  getElemJacobian(mesh.line_);
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_MESH_HPP_
