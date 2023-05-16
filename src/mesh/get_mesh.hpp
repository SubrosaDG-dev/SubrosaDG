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

#include <gmsh.h>                          // for getElementsByType, getEntitiesForPhysicalGroup, getMaxNodeTag, get...
#include <Eigen/Core>                      // for DenseBase::col, Matrix, DenseCoeffsBase, Dynamic
#include <string>                          // for string
#include <vector>                          // for vector
#include <string_view>                     // for operator==, string_view
#include <utility>                         // for pair, make_pair
#include <iostream>                        // for operator<<, endl, basic_ostream, cerr, ostream
#include <algorithm>                       // for copy, max
#include <cstdlib>                         // for size_t, exit, EXIT_FAILURE
#include <stdexcept>                       // for out_of_range
#include <unordered_map>                   // for unordered_map

#include "basic/data_types.hpp"            // for Isize, Real, Usize
#include "mesh/mesh_structure.hpp"         // for ElementMesh (ptr only), Mesh (ptr only), Mesh2d (ptr only), MeshSu...
#include "mesh/reconstruct_adjacency.hpp"  // for reconstructAdjacency
#include "mesh/get_integration.hpp"        // for getElementJacobian, getElementGradIntegral, getElementIntegral
#include "mesh/element_types.hpp"          // for kLine, ElementType, kQuadrangle, kTriangle

// clang-format on

namespace SubrosaDG {

enum class BoundaryType : SubrosaDG::Isize;

/**
 * @brief Get element entity tags for specific physical group.
 *
 * @tparam Type The element type.
 * @param [in] physical_group_entity_tag The physical group entity tag.
 * @param [out] elem_entity_tags The element entity tags for specific physical group.
 *
 * @details // TODO: Add details.
 */
template <ElementType Type>
inline void concatenateElementEntityTags(const std::vector<int>& physical_group_entity_tag,
                                         std::vector<std::size_t>& elem_entity_tags) {
  std::vector<std::size_t> elem_tags;
  std::vector<std::size_t> elem_node_tags;
  for (const auto& entity_tag : physical_group_entity_tag) {
    gmsh::model::mesh::getElementsByType(Type.kElementTag, elem_tags, elem_node_tags, entity_tag);
    if (!elem_tags.empty()) {
      elem_entity_tags.insert(elem_entity_tags.end(), elem_tags.begin(), elem_tags.end());
    }
  }
}

/**
 * @brief Get the Physical Group all info.
 *
 * @tparam Dimension The dimension of physical group.
 * @param [in] physical_group_tag The physical group tag.
 * @return std::pair<std::string, std::vector<std::size_t>> The physical group info(name, entity tags).
 *
 * @details
 * This function's purpose is to get all the elements tags of the physical group and store it into a vector. which
 * taking in a `physical_group_tag` variable. It uses `getPhysicalName` function to get the `physical_group_name` and
 * `getEntitiesForPhysicalGroup` function to get the `physical_group_entity_tag`. Then, it uses the
 * `concatenateElementEntityTags` function to put all the entity tags in `physical_group_entity_tag` into
 * `elem_entity_tags`, which refers to all the element tags bound to this entity. For entities with a dimension of 1,
 * `elem_entity_tags` include the Line tags. For entities with a dimension of 2, `elem_entity_tags` include Triangle and
 * Quadrangle tags. Finally, it returns a pair containing `physical_group_name`, and `elem_entity_tags`.
 */
template <Isize Dimension>
inline std::pair<std::string, std::vector<std::size_t>> getPhysicalGroups(int physical_group_tag) {
  std::string physical_group_name;
  std::vector<int> physical_group_entity_tag;
  gmsh::model::getPhysicalName(Dimension, physical_group_tag, physical_group_name);
  gmsh::model::getEntitiesForPhysicalGroup(Dimension, physical_group_tag, physical_group_entity_tag);
  std::vector<std::size_t> elem_entity_tags;
  if constexpr (Dimension == 1) {
    concatenateElementEntityTags<kLine>(physical_group_entity_tag, elem_entity_tags);
  } else if constexpr (Dimension == 2) {
    concatenateElementEntityTags<kTriangle>(physical_group_entity_tag, elem_entity_tags);
    concatenateElementEntityTags<kQuadrangle>(physical_group_entity_tag, elem_entity_tags);
  }
  return {physical_group_name, elem_entity_tags};
}

/**
 * @brief Get the Mesh Supplemental information.
 *
 * @tparam Type The element type.
 * @tparam T The name map type, BoundaryType for boundary name map, Isize for region name map.
 * @param [in] name_map The name map to get the relationship between name and tag.
 * @param [out] mesh_supplemental The MeshSupplemental structure.
 *
 * @exception std::out_of_range The boundary name or region name is not found in config file.
 *
 * @details // TODO: Add details.
 *
 */
template <ElementType Type, typename T>
inline void getMeshSupplemental(const std::unordered_map<std::string_view, T>& name_map,
                                MeshSupplemental<Type>& mesh_supplemental) {
  std::vector<std::pair<int, int>> physical_group_tags;
  gmsh::model::getPhysicalGroups(physical_group_tags, static_cast<int>(Type.kDimension));
  std::vector<std::pair<std::string, std::vector<std::size_t>>> physical_groups;
  physical_groups.reserve(physical_group_tags.size());
  for (const auto& [dimension, physical_group_tag] : physical_group_tags) {
    physical_groups.emplace_back(getPhysicalGroups<Type.kDimension>(physical_group_tag));
  }
  std::vector<std::size_t> all_elem_entity_tags;
  for (const auto& [name, elem_entity_tags] : physical_groups) {
    all_elem_entity_tags.insert(all_elem_entity_tags.end(), elem_entity_tags.begin(), elem_entity_tags.end());
  }
  mesh_supplemental.range_ = std::make_pair(all_elem_entity_tags.front(), all_elem_entity_tags.back());
  mesh_supplemental.num_ = mesh_supplemental.range_.second - mesh_supplemental.range_.first + 1;
  mesh_supplemental.index_.resize(mesh_supplemental.num_);
  try {
    for (const auto& [name, elem_entity_tags] : physical_groups) {
      {
        for (const auto& elem_entity_tag : elem_entity_tags) {
          mesh_supplemental.index_(static_cast<Isize>(elem_entity_tag) - mesh_supplemental.range_.first) =
              static_cast<Isize>(name_map.at(name));
        }
      }
    }
  } catch (const std::out_of_range& error) {
    std::cerr << error.what() << std::endl;
    std::exit(EXIT_FAILURE);
  }
}

template <Isize Dimension, Isize PolynomialOrder>
inline void getNodes(Mesh<Dimension, PolynomialOrder>& mesh) {
  std::vector<std::size_t> node_tags;
  std::vector<double> node_coords;
  std::vector<double> node_params;
  gmsh::model::mesh::getNodes(node_tags, node_coords, node_params);
  std::size_t nodes_num;
  gmsh::model::mesh::getMaxNodeTag(nodes_num);
  mesh.nodes_num_ = static_cast<Isize>(nodes_num);
  mesh.nodes_.resize(3, mesh.nodes_num_);
  for (const auto& node_tag : node_tags) {
    mesh.nodes_.col(static_cast<Isize>(node_tag - 1)) << static_cast<Real>(node_coords[3 * (node_tag - 1)]),
        static_cast<Real>(node_coords[3 * (node_tag - 1) + 1]), static_cast<Real>(node_coords[3 * (node_tag - 1) + 2]);
  }
}

template <ElementType Type>
inline void getElement(const Eigen::Matrix<Real, 3, Eigen::Dynamic>& nodes, ElementMesh<Type>& element_mesh) {
  std::vector<std::size_t> elem_tags;
  std::vector<std::size_t> elem_node_tag;
  gmsh::model::mesh::getElementsByType(Type.kElementTag, elem_tags, elem_node_tag);
  if (elem_tags.empty()) {
    element_mesh.elements_range_ = std::make_pair(0, 0);
    element_mesh.elements_num_ = 0;
  } else {
    element_mesh.elements_range_ = std::make_pair(elem_tags.front(), elem_tags.back());
    element_mesh.elements_num_ = static_cast<Isize>(elem_tags.size());
    element_mesh.elements_nodes_.resize(Type.kNodesNumPerElement * 3, element_mesh.elements_num_);
    element_mesh.elements_index_.resize(Type.kNodesNumPerElement, element_mesh.elements_num_);
    for (const auto& elem_tag : elem_tags) {
      for (Isize i = 0; i < Type.kNodesNumPerElement; i++) {
        auto node_tag = static_cast<Isize>(elem_node_tag[static_cast<Usize>(
            (static_cast<Isize>(elem_tag) - element_mesh.elements_range_.first) * Type.kNodesNumPerElement + i)]);
        for (Isize j = 0; j < 3; j++) {
          element_mesh.elements_nodes_(i * 3 + j, static_cast<Isize>(elem_tag) - element_mesh.elements_range_.first) =
              nodes(j, node_tag - 1);
        }
        element_mesh.elements_index_(i, static_cast<Isize>(elem_tag) - element_mesh.elements_range_.first) = node_tag;
      }
    }
  }
}

template <Isize PolynomialOrder>
inline void getMesh(const std::unordered_map<std::string_view, BoundaryType>& boundary_type_map,
                    Mesh2d<PolynomialOrder>& mesh) {
  getNodes(mesh);
  getElementIntegral<kLine, PolynomialOrder>();
  getElementGradIntegral<kTriangle, PolynomialOrder>();
  getElementGradIntegral<kQuadrangle, PolynomialOrder>();
  MeshSupplemental<kLine> boundary_supplemental;
  getMeshSupplemental<kLine, BoundaryType>(boundary_type_map, boundary_supplemental);
  reconstructAdjacency(mesh.nodes_, mesh.internal_line_, mesh.boundary_line_, boundary_supplemental);
  getElement(mesh.nodes_, mesh.triangle_);
  getElement(mesh.nodes_, mesh.quadrangle_);
  getElementJacobian(mesh.internal_line_);
  getElementJacobian(mesh.boundary_line_);
  getElementJacobian(mesh.triangle_);
  getElementJacobian(mesh.quadrangle_);
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GRT_MESH_HPP_
