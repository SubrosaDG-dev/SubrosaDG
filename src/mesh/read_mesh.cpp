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
#include <memory>                     // for make_unique, unique_ptr
#include <stdexcept>                  // for out_of_range
#include <unordered_map>              // for unordered_map
#include <utility>                    // for pair, make_pair
#include <string_view>                // for string_view, hash, operator==
#include <filesystem>                 // for path

#include "config/config_structure.h"  // for Config
#include "mesh/mesh_map.h"            // for kElementPointNumMap
#include "mesh/mesh_structure.h"      // for MeshSupplementalInfo, IElement, Mesh2d
#include "basic/data_types.h"         // for Isize, Real, Usize

// clang-format on
namespace SubrosaDG::Internal {

void readMesh(Mesh2d& mesh) {
  getNodes(mesh);
  gmsh::model::mesh::getMaxElementTag(mesh.num_elements_);
  getElements(mesh.ielement_triangle_, "Triangle");
  getElements(mesh.ielement_quadrangle_, "Quadrangle");
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
  mesh_supplemental_info.num_boundary_ = std::make_pair(elem_entity_tags_1d.front(), elem_entity_tags_1d.back());
  mesh_supplemental_info.num_region_ = std::make_pair(elem_entity_tags_2d.front(), elem_entity_tags_2d.back());
  mesh_supplemental_info.iboundary_ = std::make_unique<Eigen::Vector<Isize, Eigen::Dynamic>>(
      mesh_supplemental_info.num_boundary_.second - mesh_supplemental_info.num_boundary_.first + 1);
  mesh_supplemental_info.iregion_ = std::make_unique<Eigen::Vector<Isize, Eigen::Dynamic>>(
      mesh_supplemental_info.num_region_.second - mesh_supplemental_info.num_region_.first + 1);
  try {
    for (const auto& [dim, name, elem_entity_tags] : physical_groups) {
      if (dim == 1) {
        for (const auto& elem_entity_tag : elem_entity_tags) {
          mesh_supplemental_info.iboundary_->operator()(
              static_cast<Isize>(elem_entity_tag - mesh_supplemental_info.num_boundary_.first)) =
              static_cast<Isize>(config.boundary_condition_.at(name));
        }
      } else if (dim == 2) {
        for (const auto& elem_entity_tag : elem_entity_tags) {
          mesh_supplemental_info.iregion_->operator()(
              static_cast<Isize>(elem_entity_tag - mesh_supplemental_info.num_region_.first)) =
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

std::tuple<int, std::string, std::vector<std::size_t>> getPhysicalGroups(const int dim, const int physical_group_tag) {
  std::string physical_group_name;
  std::vector<int> physical_group_entity_tag;
  gmsh::model::getPhysicalName(dim, physical_group_tag, physical_group_name);
  gmsh::model::getEntitiesForPhysicalGroup(dim, physical_group_tag, physical_group_entity_tag);
  std::vector<std::size_t> elem_tags;
  std::vector<std::size_t> elem_node_tags;
  std::vector<std::size_t> elem_entity_tags;
  if (dim == 1) {
    int element_type_line = gmsh::model::mesh::getElementType("Line", 1);
    for (const auto& entity_tag : physical_group_entity_tag) {
      gmsh::model::mesh::getElementsByType(element_type_line, elem_tags, elem_node_tags, entity_tag);
      elem_entity_tags.insert(elem_entity_tags.end(), elem_tags.begin(), elem_tags.end());
    }
  } else if (dim == 2) {
    int element_type_tri = gmsh::model::mesh::getElementType("Triangle", 1);
    for (const auto& entity_tag : physical_group_entity_tag) {
      gmsh::model::mesh::getElementsByType(element_type_tri, elem_tags, elem_node_tags, entity_tag);
      if (!elem_tags.empty()) {
        elem_entity_tags.insert(elem_entity_tags.end(), elem_tags.begin(), elem_tags.end());
      }
    }
    int element_type_quad = gmsh::model::mesh::getElementType("Quadrangle", 1);
    for (const auto& entity_tag : physical_group_entity_tag) {
      gmsh::model::mesh::getElementsByType(element_type_quad, elem_tags, elem_node_tags, entity_tag);
      if (!elem_tags.empty()) {
        elem_entity_tags.insert(elem_entity_tags.end(), elem_tags.begin(), elem_tags.end());
      }
    }
  }
  return {dim, physical_group_name, elem_entity_tags};
}

void getNodes(Mesh2d& mesh) {
  std::vector<std::size_t> node_tags;
  std::vector<double> node_coords;
  std::vector<double> node_params;
  gmsh::model::mesh::getNodes(node_tags, node_coords, node_params);
  gmsh::model::mesh::getMaxNodeTag(mesh.num_nodes_);
  mesh.nodes_ = std::make_unique<Eigen::Matrix<Real, 3, Eigen::Dynamic>>(3, mesh.num_nodes_);
  for (const auto& node_tag : node_tags) {
    mesh.nodes_->col(static_cast<Isize>(node_tag - 1)) << static_cast<Real>(node_coords[3 * (node_tag - 1)]),
        static_cast<Real>(node_coords[3 * (node_tag - 1) + 1]), static_cast<Real>(node_coords[3 * (node_tag - 1) + 2]);
  }
}

void getElements(IElement& i_element, const std::string& element_name) {
  int element_type = gmsh::model::mesh::getElementType(element_name, 1);
  Usize element_point_num = kElementPointNumMap.at(element_name);
  std::vector<std::size_t> elem_tags;
  std::vector<std::size_t> elem_node_tag;
  gmsh::model::mesh::getElementsByType(element_type, elem_tags, elem_node_tag);
  if (elem_tags.empty()) {
    i_element.num_elements_ = std::make_pair(0, 0);
    i_element.ielements_ = nullptr;
  } else {
    i_element.num_elements_ = std::make_pair(elem_tags.front(), elem_tags.back());
    i_element.ielements_ =
        std::make_unique<Eigen::Matrix<Isize, Eigen::Dynamic, Eigen::Dynamic>>(element_point_num, elem_tags.size());
    for (const auto& elem_tag : elem_tags) {
      for (Usize i = 0; i < element_point_num; i++) {
        i_element.ielements_->operator()(static_cast<Isize>(i),
                                         static_cast<Isize>(elem_tag - i_element.num_elements_.first)) =
            static_cast<Isize>(elem_node_tag[(elem_tag - i_element.num_elements_.first) * element_point_num + i]);
      }
    }
  }
}

}  // namespace SubrosaDG::Internal
