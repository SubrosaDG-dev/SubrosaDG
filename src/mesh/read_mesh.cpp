/**
 * @file read_mesh.cpp
 * @brief The source file to read mesh.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2022-11-02
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

// clang-format off

#include "mesh/read_mesh.h"

#include <fmt/core.h>                 // for format
#include <gmsh.h>                     // for getElementType, getElementsByType, getEntitiesForPhysicalGroup, getMaxE...
#include <Eigen/Core>                 // for Matrix, Vector, Block, Dynamic, CommaInitializer, DenseBase, DenseBase:...
#include <cstddef>                    // for size_t
#include <cstdlib>                    // for exit, EXIT_FAILURE
#include <iostream>                   // for endl, operator<<, basic_ostream, cerr, ostream
#include <memory>                     // for make_unique, unique_ptr, allocator, shared_ptr, make_shared, __shared_p...
#include <stdexcept>                  // for out_of_range
#include <unordered_map>              // for unordered_map
#include <utility>                    // for pair, make_pair
#include <string_view>                // for string_view, basic_string_view
#include <filesystem>                 // for path

#include "config/config_structure.h"  // for Config
#include "mesh/mesh_structure.h"      // for Element, MeshSupplementalInfo, Mesh2d
#include "basic/data_types.h"         // for Isize, Real, Usize

// clang-format on

namespace SubrosaDG::Internal {

void readMesh(Mesh2d& mesh) {
  getNodes(mesh);
  std::size_t element_num;
  gmsh::model::mesh::getMaxElementTag(element_num);
  mesh.element_num_ = static_cast<Isize>(element_num);
  getElements(mesh.nodes_, mesh.triangle_element_);
  getElements(mesh.nodes_, mesh.quadrangle_element_);
}

void readMeshSupplementalInfo(const Config& config, MeshSupplementalInfo& mesh_supplemental_info) {
  std::vector<std::pair<int, int>> physical_group_tags;
  gmsh::model::getPhysicalGroups(physical_group_tags);
  std::vector<std::tuple<int, std::string, std::vector<std::size_t>>> physical_groups;
  physical_groups.reserve(physical_group_tags.size());
  for (const auto& [dim, physical_group_tag] : physical_group_tags) {
    physical_groups.emplace_back(getPhysicalGroups(dim, physical_group_tag));
  }
  std::vector<std::size_t> elem_entity_tags_1d;
  std::vector<std::size_t> elem_entity_tags_2d;
  for (const auto& [dim, name, elem_entity_tags] : physical_groups) {
    if (dim == 1) {
      elem_entity_tags_1d.insert(elem_entity_tags_1d.end(), elem_entity_tags.begin(), elem_entity_tags.end());
    } else if (dim == 2) {
      elem_entity_tags_2d.insert(elem_entity_tags_2d.end(), elem_entity_tags.begin(), elem_entity_tags.end());
    }
  }
  mesh_supplemental_info.boundary_num_ = std::make_pair(elem_entity_tags_1d.front(), elem_entity_tags_1d.back());
  mesh_supplemental_info.region_num_ = std::make_pair(elem_entity_tags_2d.front(), elem_entity_tags_2d.back());
  mesh_supplemental_info.boundary_index_ = std::make_unique<Eigen::Vector<Isize, Eigen::Dynamic>>(
      mesh_supplemental_info.boundary_num_.second - mesh_supplemental_info.boundary_num_.first + 1);
  mesh_supplemental_info.region_index_ = std::make_unique<Eigen::Vector<Isize, Eigen::Dynamic>>(
      mesh_supplemental_info.region_num_.second - mesh_supplemental_info.region_num_.first + 1);
  try {
    for (const auto& [dim, name, elem_entity_tags] : physical_groups) {
      if (dim == 1) {
        for (const auto& elem_entity_tag : elem_entity_tags) {
          mesh_supplemental_info.boundary_index_->operator()(static_cast<Isize>(elem_entity_tag) -
                                                             mesh_supplemental_info.boundary_num_.first) =
              static_cast<Isize>(config.boundary_condition_.at(name));
        }
      } else if (dim == 2) {
        for (const auto& elem_entity_tag : elem_entity_tags) {
          mesh_supplemental_info.region_index_->operator()(static_cast<Isize>(elem_entity_tag) -
                                                           mesh_supplemental_info.region_num_.first) =
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
 * This function's purpose is to get all the element tags of the physical group and store it into a vector. It takes in
 * `dimension` and a `physical_group_tag`. It uses `getPhysicalName` function to get the `physical_group_name` and
 * `getEntitiesForPhysicalGroup` function to get the `physical_group_entity_tag`. Then, it uses the
 * `concatenateElementEntityTags` function to put all the entity tags in `physical_group_entity_tag` into
 * `elem_entity_tags`, which refers to all the element tags bound to this entity. For entities with a dimension of 1,
 * `elem_entity_tags` include the Line tags. For entities with a dimension of 2, `elem_entity_tags` include Triangle and
 * Quadrangle tags. Finally, it returns a tuple containing `dimension`, `physical_group_name`, and `elem_entity_tags`.
 */
std::tuple<int, std::string, std::vector<std::size_t>> getPhysicalGroups(const int dim, const int physical_group_tag) {
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
  return {dim, physical_group_name, elem_entity_tags};
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
  std::size_t node_num;
  gmsh::model::mesh::getMaxNodeTag(node_num);
  mesh.node_num_ = static_cast<Isize>(node_num);
  mesh.nodes_ = std::make_shared<Eigen::Matrix<Real, 3, Eigen::Dynamic>>(3, mesh.node_num_);
  for (const auto& node_tag : node_tags) {
    mesh.nodes_->col(static_cast<Isize>(node_tag - 1)) << static_cast<Real>(node_coords[3 * (node_tag - 1)]),
        static_cast<Real>(node_coords[3 * (node_tag - 1) + 1]), static_cast<Real>(node_coords[3 * (node_tag - 1) + 2]);
  }
}

void getElements(const std::shared_ptr<Eigen::Matrix<Real, 3, Eigen::Dynamic>>& nodes, Element& element) {
  int element_type = gmsh::model::mesh::getElementType(element.element_type_info_.first.data(), 1);
  std::vector<std::size_t> elem_tags;
  std::vector<std::size_t> elem_node_tag;
  gmsh::model::mesh::getElementsByType(element_type, elem_tags, elem_node_tag);
  if (elem_tags.empty()) {
    element.element_num_ = std::make_pair(0, 0);
    element.element_index_ = nullptr;
  } else {
    element.element_num_ = std::make_pair(elem_tags.front(), elem_tags.back());
    element.element_index_ = std::make_unique<Eigen::Matrix<Isize, Eigen::Dynamic, Eigen::Dynamic>>(
        element.element_type_info_.second, elem_tags.size());
    element.element_nodes_ = std::make_unique<Eigen::Matrix<Real, Eigen::Dynamic, Eigen::Dynamic>>(
        element.element_type_info_.second * 3, elem_tags.size());
    for (const auto& elem_tag : elem_tags) {
      for (Isize i = 0; i < element.element_type_info_.second; i++) {
        auto node_tag = static_cast<Isize>(elem_node_tag[static_cast<Usize>(
            (static_cast<Isize>(elem_tag) - element.element_num_.first) * element.element_type_info_.second + i)]);
        element.element_index_->operator()(i, static_cast<Isize>(elem_tag) - element.element_num_.first) = node_tag;
        for (Isize j = 0; j < 3; j++) {
          element.element_nodes_->operator()(i * 3 + j, static_cast<Isize>(elem_tag) - element.element_num_.first) =
              nodes->operator()(j, node_tag - 1);
        }
      }
    }
  }
}

}  // namespace SubrosaDG::Internal
