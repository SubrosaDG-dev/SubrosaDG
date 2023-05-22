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

#ifndef SUBROSA_DG_GRT_MESH_HPP_
#define SUBROSA_DG_GRT_MESH_HPP_

// clang-format off

#include <gmsh.h>                                  // for getMaxNodeTag, getNodes
#include <vector>                                  // for vector
#include <string_view>                             // for string_view
#include <utility>                                 // for make_pair
#include <algorithm>                               // for max, min
#include <cstdlib>                                 // for size_t
#include <stdexcept>                               // for out_of_range
#include <unordered_map>                           // for unordered_map
#include <Eigen/LU>                                // for MatrixBase::inverse

#include "basic/data_types.hpp"                    // for Isize, Real, Usize
#include "mesh/mesh_structure.hpp"                 // for Mesh2d (ptr only), Mesh (ptr only), MeshSupplemental
#include "mesh/element/reconstruct_adjacency.hpp"  // for reconstructAdjacency
#include "mesh/element/get_element_integral.hpp"   // for FElementIntegral
#include "mesh/element_types.hpp"                  // for kLine, kQuadrangle, kTriangle, ElementType
#include "mesh/get_mesh_supplemental.hpp"          // for getMeshSupplemental
#include "mesh/element/get_element_jacobian.hpp"   // for getElementJacobian
#include "mesh/element/get_element_mesh.hpp"       // for getElementMesh

// clang-format on

namespace SubrosaDG {

enum class Boundary : SubrosaDG::Isize;

template <Isize Dimension, Isize PolynomialOrder>
inline void getNodes(Mesh<Dimension, PolynomialOrder>& mesh) {
  std::vector<std::size_t> node_tags;
  std::vector<double> node_coords;
  std::vector<double> node_params;
  gmsh::model::mesh::getNodes(node_tags, node_coords, node_params);
  std::size_t nodes_num;
  gmsh::model::mesh::getMaxNodeTag(nodes_num);
  mesh.nodes_num_ = static_cast<Isize>(nodes_num);
  mesh.nodes_.resize(Dimension, mesh.nodes_num_);
  for (const auto node_tag : node_tags) {
    for (Isize i = 0; i < Dimension; i++) {
      mesh.nodes_(i, static_cast<Isize>(node_tag - 1)) =
          static_cast<Real>(node_coords[3 * (node_tag - 1) + static_cast<Usize>(i)]);
    }
  }
}

template <Isize PolynomialOrder>
inline void getMeshElements(Mesh2d<PolynomialOrder>& mesh) {
  mesh.elements_range_ =
      std::make_pair(std::ranges::min(mesh.triangle_.elements_range_.first, mesh.quadrangle_.elements_range_.first),
                     std::ranges::max(mesh.triangle_.elements_range_.second, mesh.quadrangle_.elements_range_.second));
  mesh.elements_num_ = mesh.triangle_.elements_num_ + mesh.quadrangle_.elements_num_;
  mesh.elements_type_.resize(mesh.elements_num_);
  for (Isize i = 0; i < mesh.elements_num_; i++) {
    if ((i + mesh.elements_range_.first) >= mesh.triangle_.elements_range_.first &&
        (i + mesh.elements_range_.first) <= mesh.triangle_.elements_range_.second) {
      mesh.elements_type_(i) = kTriangle.kElementTag;
    } else if ((i + mesh.elements_range_.first) >= mesh.quadrangle_.elements_range_.first &&
               (i + mesh.elements_range_.first) <= mesh.quadrangle_.elements_range_.second) {
      mesh.elements_type_(i) = kQuadrangle.kElementTag;
    }
  }
}

template <Isize PolynomialOrder>
inline void getMesh(const std::unordered_map<std::string_view, Boundary>& boundary_type_map,
                    Mesh2d<PolynomialOrder>& mesh) {
  getNodes(mesh);

  FElementIntegral<true, kLine, PolynomialOrder>::get();
  FElementIntegral<false, kTriangle, PolynomialOrder>::get();
  FElementIntegral<false, kQuadrangle, PolynomialOrder>::get();

  MeshSupplemental<kLine> boundary_supplemental;
  getMeshSupplemental<kLine, Boundary>(boundary_type_map, boundary_supplemental);
  reconstructAdjacency<2, kLine>(mesh.nodes_, mesh.line_, boundary_supplemental);

  getElementMesh<2, kTriangle>(mesh.nodes_, mesh.triangle_);
  getElementMesh<2, kQuadrangle>(mesh.nodes_, mesh.quadrangle_);

  getMeshElements(mesh);

  getElementJacobian<2, kLine>(mesh.line_);
  getElementJacobian<2, kTriangle>(mesh.triangle_);
  getElementJacobian<2, kQuadrangle>(mesh.quadrangle_);
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GRT_MESH_HPP_
