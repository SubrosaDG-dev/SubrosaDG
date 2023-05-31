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
#include <utility>                         // for make_pair
#include <algorithm>                       // for max, min
#include <cstdlib>                         // for size_t
#include <stdexcept>                       // for out_of_range
#include <unordered_map>                   // for unordered_map

#include "basic/data_type.hpp"             // for Isize, Real, Usize
#include "mesh/mesh_structure.hpp"         // for Mesh2d (ptr only), Mesh (ptr only), MeshSupplemental
#include "mesh/element/get_integral.hpp"   // for getElemIntegral, getAdjacencyElemIntegral
#include "mesh/elem_type.hpp"              // for kLine, kQuad, kTri, ElemInfo
#include "mesh/get_mesh_supplemental.hpp"  // for getMeshSupplemental
#include "mesh/element/get_jacobian.hpp"   // for getElemJacobian
#include "mesh/element/get_elem_mesh.hpp"  // for getElemMesh, getAdjacencyElemMesh
#include "mesh/element/cal_norm_vec.hpp"   // for calAdjacencyElemNormVec

// clang-format on

namespace SubrosaDG {

enum class Boundary : SubrosaDG::Isize;

template <Isize Dim, Isize PolyOrder>
inline void getNodes(Mesh<Dim, PolyOrder>& mesh) {
  std::vector<std::size_t> node_tags;
  std::vector<double> node_coords;
  std::vector<double> node_params;
  gmsh::model::mesh::getNodes(node_tags, node_coords, node_params);
  std::size_t nodes_num;
  gmsh::model::mesh::getMaxNodeTag(nodes_num);
  mesh.node_num_ = static_cast<Isize>(nodes_num);
  mesh.node_.resize(Dim, mesh.node_num_);
  for (const auto node_tag : node_tags) {
    for (Isize i = 0; i < Dim; i++) {
      mesh.node_(i, static_cast<Isize>(node_tag - 1)) =
          static_cast<Real>(node_coords[3 * (node_tag - 1) + static_cast<Usize>(i)]);
    }
  }
}

template <Isize PolyOrder>
inline void getMeshElements(Mesh2d<PolyOrder>& mesh) {
  mesh.elem_range_ = std::make_pair(std::ranges::min(mesh.tri_.range_.first, mesh.quad_.range_.first),
                                    std::ranges::max(mesh.tri_.range_.second, mesh.quad_.range_.second));
  mesh.elem_num_ = mesh.tri_.num_ + mesh.quad_.num_;
  mesh.elem_type_.resize(mesh.elem_num_);
  for (Isize i = 0; i < mesh.elem_num_; i++) {
    if ((i + mesh.elem_range_.first) >= mesh.tri_.range_.first &&
        (i + mesh.elem_range_.first) <= mesh.tri_.range_.second) {
      mesh.elem_type_(i) = kTri.kTag;
    } else if ((i + mesh.elem_range_.first) >= mesh.quad_.range_.first &&
               (i + mesh.elem_range_.first) <= mesh.quad_.range_.second) {
      mesh.elem_type_(i) = kQuad.kTag;
    }
  }
}

template <Isize PolyOrder>
inline void getMesh(const std::unordered_map<std::string_view, Boundary>& boundary_type_map, Mesh2d<PolyOrder>& mesh) {
  getNodes(mesh);

  getElemIntegral<kTri, PolyOrder>();
  getElemIntegral<kQuad, PolyOrder>();
  getAdjacencyElemIntegral<kLine, PolyOrder>();

  MeshSupplemental<kLine> boundary_supplemental;
  getMeshSupplemental<kLine, Boundary>(boundary_type_map, boundary_supplemental);
  getAdjacencyElemMesh<2, kLine>(mesh.node_, mesh.line_, boundary_supplemental);

  getElemMesh<2, kTri>(mesh.node_, mesh.tri_);
  getElemMesh<2, kQuad>(mesh.node_, mesh.quad_);

  getMeshElements(mesh);

  calAdjacencyElemNormVec(mesh.line_);

  getElemJacobian<2, kLine>(mesh.line_);
  getElemJacobian<2, kTri>(mesh.tri_);
  getElemJacobian<2, kQuad>(mesh.quad_);
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_MESH_HPP_
