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

// clang-format off

#include <gmsh.h>                                   // for getMaxNodeTag, getNodes
#include <vector>                                   // for vector
#include <string_view>                              // for string_view
#include <unordered_map>                            // for unordered_map

#include "basic/data_type.hpp"                      // for Usize, Isize, Real
#include "basic/enum.hpp"                           // for MeshType, Boundary (ptr only)
#include "mesh/element/get_jacobian.hpp"            // for getElemJacobian
#include "mesh/element/get_elem_mesh.hpp"           // for getElemMesh
#include "mesh/element/get_adjacency_mesh.hpp"      // for getAdjacencyElemMesh
#include "mesh/element/cal_norm_vec.hpp"            // for calAdjacencyElemNormVec
#include "mesh/element/cal_projection_measure.hpp"  // for calElemProjectionMeasure
#include "mesh/elem_type.hpp"                       // for kLine

// clang-format on

namespace SubrosaDG {

template <int Dim>
struct MeshBase;
template <int Dim, MeshType MeshT>
struct Mesh;

template <int Dim>
inline void getNodes(MeshBase<Dim>& mesh_base) {
  std::vector<Usize> node_tags;
  std::vector<double> node_coords;
  std::vector<double> node_params;
  gmsh::model::mesh::getNodes(node_tags, node_coords, node_params);
  Usize nodes_num;
  gmsh::model::mesh::getMaxNodeTag(nodes_num);
  mesh_base.node_num_ = static_cast<Isize>(nodes_num);
  mesh_base.node_.resize(Dim, static_cast<Isize>(mesh_base.node_num_));
  for (const auto node_tag : node_tags) {
    for (Usize i = 0; i < Dim; i++) {
      mesh_base.node_(static_cast<Isize>(i), static_cast<Isize>(node_tag - 1)) =
          static_cast<Real>(node_coords[3 * (node_tag - 1) + i]);
    }
  }
}

template <int Dim, MeshType MeshT>
inline void getMesh(const std::unordered_map<std::string_view, Boundary>& boundary_type_map, Mesh<Dim, MeshT>& mesh) {
  getNodes(mesh);
  if constexpr (MeshT == MeshType::Tri) {
    getElemMesh(mesh.node_, mesh.tri_);
    calElemProjectionMeasure(mesh.tri_);
    getElemJacobian(mesh.tri_);
  } else if constexpr (MeshT == MeshType::Quad) {
    getElemMesh(mesh.node_, mesh.quad_);
    calElemProjectionMeasure(mesh.quad_);
    getElemJacobian(mesh.quad_);
  } else if constexpr (MeshT == MeshType::TriQuad) {
    getElemMesh(mesh.node_, mesh.tri_);
    calElemProjectionMeasure(mesh.tri_);
    getElemJacobian(mesh.tri_);
    getElemMesh(mesh.node_, mesh.quad_);
    calElemProjectionMeasure(mesh.quad_);
    getElemJacobian(mesh.quad_);
  }
  if constexpr (MeshT == MeshType::Tri || MeshT == MeshType::Quad || MeshT == MeshType::TriQuad) {
    getAdjacencyElemMesh<2, kLine, MeshT>(mesh.node_, boundary_type_map, mesh.line_);
    calAdjacencyElemNormVec(mesh.line_);
    getElemJacobian(mesh.line_);
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_MESH_HPP_
