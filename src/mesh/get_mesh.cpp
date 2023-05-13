/**
 * @file get_mesh.cpp
 * @brief The source file to get mesh.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2022-11-02
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

// clang-format off

#include "mesh/get_mesh.h"

#include <fmt/core.h>                    // for format
#include <gmsh.h>                        // for getElementsByType, getElementType, getEntitiesForPhysicalGroup, getM...
#include <Eigen/Core>                    // for Block, Matrix, DenseCoeffsBase, Vector, CommaInitializer, DenseBase
#include <cstddef>                       // for size_t
#include <cstdlib>                       // for exit, EXIT_FAILURE
#include <iostream>                      // for endl, operator<<, basic_ostream, cerr, ostream
#include <stdexcept>                     // for out_of_range
#include <unordered_map>                 // for unordered_map
#include <utility>                       // for pair, make_pair
#include <string_view>                   // for string_view
#include <filesystem>                    // for path

#include "config/config_structure.h"     // for Config
#include "mesh/get_integration.h"        // for getElementIntegral
#include "mesh/mesh_structure.h"         // for ElementMesh, Mesh2d, MeshSupplemental, Element
#include "basic/data_types.h"            // for Isize, Real, Usize
#include "mesh/reconstruct_adjacency.h"  // for reconstructAdjacency

// clang-format on

namespace SubrosaDG::Internal {

void getMesh(const Config& config, Mesh2d& mesh) {
  getNodes(mesh);
  getElement(mesh.nodes_, mesh.triangle_);
  getElement(mesh.nodes_, mesh.quadrangle_);
  getElementIntegral(mesh.polynomial_order_, mesh.gauss_integral_accuracy_, mesh.triangle_);
  getElementIntegral(mesh.polynomial_order_, mesh.gauss_integral_accuracy_, mesh.quadrangle_);
  MeshSupplemental boundary_supplemental{true, 1};
  readMeshSupplemental(config, boundary_supplemental);
  reconstructAdjacency(mesh.nodes_, mesh.interior_line_, mesh.boundary_line_, boundary_supplemental);
}

void readMeshSupplemental(const Config& config, MeshSupplemental& supplemental) {
  std::vector<std::pair<int, int>> physical_group_tags;
  gmsh::model::getPhysicalGroups(physical_group_tags, static_cast<int>(supplemental.dimension_));
  std::vector<std::pair<std::string, std::vector<std::size_t>>> physical_groups;
  physical_groups.reserve(physical_group_tags.size());
  for (const auto& [dim, physical_group_tag] : physical_group_tags) {
    physical_groups.emplace_back(getPhysicalGroups(dim, physical_group_tag));
  }
  std::vector<std::size_t> all_elem_entity_tags;
  for (const auto& [name, elem_entity_tags] : physical_groups) {
    all_elem_entity_tags.insert(all_elem_entity_tags.end(), elem_entity_tags.begin(), elem_entity_tags.end());
  }
  supplemental.range_ = std::make_pair(all_elem_entity_tags.front(), all_elem_entity_tags.back());
  supplemental.num_ = supplemental.range_.second - supplemental.range_.first + 1;
  supplemental.index_.resize(supplemental.num_);
  try {
    for (const auto& [name, elem_entity_tags] : physical_groups) {
      if (supplemental.is_adjanency_element_) {
        for (const auto& elem_entity_tag : elem_entity_tags) {
          supplemental.index_(static_cast<Isize>(elem_entity_tag) - supplemental.range_.first) =
              static_cast<Isize>(config.boundary_condition_.at(name));
        }
      } else {
        for (const auto& elem_entity_tag : elem_entity_tags) {
          supplemental.index_(static_cast<Isize>(elem_entity_tag) - supplemental.range_.first) =
              static_cast<Isize>(config.region_name_map_.at(name));
        }
      }
    }
  } catch (const std::out_of_range& error) {
    std::cerr << std::endl << fmt::format("Error parsing file '{}':", config.config_file_.string()) << std::endl;
    std::cerr << error.what() << std::endl;
    std::exit(EXIT_FAILURE);
  }
}

/**
 * @details
 * This function's purpose is to get all the elements tags of the physical group and store it into a vector. It takes in
 * `dimension` and a `physical_group_tag`. It uses `getPhysicalName` function to get the `physical_group_name` and
 * `getEntitiesForPhysicalGroup` function to get the `physical_group_entity_tag`. Then, it uses the
 * `concatenateElementEntityTags` function to put all the entity tags in `physical_group_entity_tag` into
 * `elem_entity_tags`, which refers to all the element tags bound to this entity. For entities with a dimension of 1,
 * `elem_entity_tags` include the Line tags. For entities with a dimension of 2, `elem_entity_tags` include Triangle and
 * Quadrangle tags. Finally, it returns a pair containing `physical_group_name`, and `elem_entity_tags`.
 */
std::pair<std::string, std::vector<std::size_t>> getPhysicalGroups(const int dim, const int physical_group_tag) {
  std::string physical_group_name;
  std::vector<int> physical_group_entity_tag;
  gmsh::model::getPhysicalName(dim, physical_group_tag, physical_group_name);
  gmsh::model::getEntitiesForPhysicalGroup(dim, physical_group_tag, physical_group_entity_tag);
  std::vector<std::size_t> elem_entity_tags;
  if (dim == 1) {
    concatenateElementEntityTags(elem_entity_tags, physical_group_entity_tag, "Line");
  } else if (dim == 2) {
    concatenateElementEntityTags(elem_entity_tags, physical_group_entity_tag, "Triangle");
    concatenateElementEntityTags(elem_entity_tags, physical_group_entity_tag, "Quadrangle");
  }
  return {physical_group_name, elem_entity_tags};
}

void concatenateElementEntityTags(std::vector<std::size_t>& elem_entity_tags,
                                  const std::vector<int>& physical_group_entity_tag,
                                  const std::string_view& element_type_name) {
  std::vector<std::size_t> elem_tags;
  std::vector<std::size_t> elem_node_tags;
  int element_type = gmsh::model::mesh::getElementType(element_type_name.data(), 1);
  for (const auto& entity_tag : physical_group_entity_tag) {
    gmsh::model::mesh::getElementsByType(element_type, elem_tags, elem_node_tags, entity_tag);
    if (!elem_tags.empty()) {
      elem_entity_tags.insert(elem_entity_tags.end(), elem_tags.begin(), elem_tags.end());
    }
  }
}

void getNodes(Mesh2d& mesh) {
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

void getElement(const Eigen::Matrix<Real, 3, Eigen::Dynamic>& nodes, ElementMesh& element_mesh) {
  std::vector<std::size_t> elem_tags;
  std::vector<std::size_t> elem_node_tag;
  gmsh::model::mesh::getElementsByType(element_mesh.element_type_, elem_tags, elem_node_tag);
  if (elem_tags.empty()) {
    element_mesh.elements_range_ = std::make_pair(0, 0);
    element_mesh.elements_num_ = 0;
  } else {
    element_mesh.elements_range_ = std::make_pair(elem_tags.front(), elem_tags.back());
    element_mesh.elements_num_ = static_cast<Isize>(elem_tags.size());
    element_mesh.elements_nodes_.resize(element_mesh.nodes_num_per_element_ * 3, element_mesh.elements_num_);
    element_mesh.elements_index_.resize(element_mesh.nodes_num_per_element_, element_mesh.elements_num_);
    for (const auto& elem_tag : elem_tags) {
      for (Isize i = 0; i < element_mesh.nodes_num_per_element_; i++) {
        auto node_tag = static_cast<Isize>(elem_node_tag[static_cast<Usize>(
            (static_cast<Isize>(elem_tag) - element_mesh.elements_range_.first) * element_mesh.nodes_num_per_element_ +
            i)]);
        for (Isize j = 0; j < 3; j++) {
          element_mesh.elements_nodes_(i * 3 + j, static_cast<Isize>(elem_tag) - element_mesh.elements_range_.first) =
              nodes(j, node_tag - 1);
        }
        element_mesh.elements_index_(i, static_cast<Isize>(elem_tag) - element_mesh.elements_range_.first) = node_tag;
      }
    }
  }
}

}  // namespace SubrosaDG::Internal
