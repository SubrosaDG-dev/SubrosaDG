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

#include <gmsh.h>                          // for getMaxNodeTag, getNodes
#include <vector>                          // for vector
#include <string_view>                     // for string_view
#include <stdexcept>                       // for out_of_range
#include <unordered_map>                   // for unordered_map
#include <map>                             // for map

#include "basic/data_type.hpp"             // for Usize, Isize, Real
#include "basic/enum.hpp"                  // for MeshType, Boundary (ptr only)
#include "mesh/mesh_structure.hpp"         // for Mesh, MeshBase (ptr only)
#include "mesh/element/get_jacobian.hpp"   // for getElemJacobian
#include "mesh/element/get_elem_mesh.hpp"  // for getElemMesh, getAdjacencyElemMesh
#include "mesh/element/cal_norm_vec.hpp"   // for calAdjacencyElemNormVec

// clang-format on

namespace SubrosaDG {

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
inline void getMesh(const std::unordered_map<std::string_view, Boundary>& boundary_type_map, Mesh<Dim, MeshT>& mesh);

template <>
inline void getMesh(const std::unordered_map<std::string_view, Boundary>& boundary_type_map,
                    Mesh<2, MeshType::Tri>& mesh) {
  getNodes(mesh);

  getElemMesh<2>(mesh.node_, mesh.tri_);

  getAdjacencyElemMesh<2, MeshType::Tri>(mesh.node_, boundary_type_map, mesh.line_);

  calAdjacencyElemNormVec(mesh.line_);

  getElemJacobian<2>(mesh.line_);
  getElemJacobian<2>(mesh.tri_);
}

template <>
inline void getMesh(const std::unordered_map<std::string_view, Boundary>& boundary_type_map,
                    Mesh<2, MeshType::Quad>& mesh) {
  getNodes(mesh);

  getElemMesh<2>(mesh.node_, mesh.quad_);

  getAdjacencyElemMesh<2, MeshType::Quad>(mesh.node_, boundary_type_map, mesh.line_);

  calAdjacencyElemNormVec(mesh.line_);

  getElemJacobian<2>(mesh.line_);
  getElemJacobian<2>(mesh.quad_);
}

template <>
inline void getMesh(const std::unordered_map<std::string_view, Boundary>& boundary_type_map,
                    Mesh<2, MeshType::TriQuad>& mesh) {
  getNodes(mesh);

  getElemMesh<2>(mesh.node_, mesh.tri_);
  getElemMesh<2>(mesh.node_, mesh.quad_);

  getAdjacencyElemMesh<2, MeshType::TriQuad>(mesh.node_, boundary_type_map, mesh.line_);

  calAdjacencyElemNormVec(mesh.line_);

  getElemJacobian<2>(mesh.line_);
  getElemJacobian<2>(mesh.tri_);
  getElemJacobian<2>(mesh.quad_);
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_MESH_HPP_
