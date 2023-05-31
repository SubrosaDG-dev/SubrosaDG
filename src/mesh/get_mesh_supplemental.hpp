/**
 * @file get_mesh_supplemental.hpp
 * @brief The header file to get mesh supplemental.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-16
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_GET_MESH_SUPPLEMENTAL_HPP_
#define SUBROSA_DG_GET_MESH_SUPPLEMENTAL_HPP_

// clang-format off

#include <gmsh.h>               // for getElementsByType, getEntitiesForPhysicalGroup, getPhysicalGroups, getPhysica...
#include <iostream>             // for operator<<, endl, basic_ostream, cerr, ostream
#include <algorithm>            // for copy, max
#include <cstdlib>              // for size_t, exit, EXIT_FAILURE
#include <stdexcept>            // for out_of_range
#include <string>               // for string
#include <string_view>          // for operator==, string_view
#include <unordered_map>        // for unordered_map
#include <utility>              // for pair, make_pair
#include <vector>               // for vector

#include "basic/data_type.hpp"  // for Isize
#include "mesh/elem_type.hpp"   // for ElemInfo, kLine, kQuad, kTri

// clang-format on

namespace SubrosaDG {

template <ElemInfo ElemT>
struct MeshSupplemental;

/**
 * @brief Get element entity tags for specific physical group.
 *
 * @tparam ElemT The element type.
 * @param [in] physical_group_entity_tag The physical group entity tag.
 * @param [out] elem_entity_tags The element entity tags for specific physical group.
 *
 * @details // TODO: Add details.
 */
template <ElemInfo ElemT>
inline void concatenateElementEntityTags(const std::vector<int>& physical_group_entity_tag,
                                         std::vector<std::size_t>& elem_entity_tags) {
  std::vector<std::size_t> elem_tags;
  std::vector<std::size_t> elem_node_tags;
  for (const auto entity_tag : physical_group_entity_tag) {
    gmsh::model::mesh::getElementsByType(ElemT.kTag, elem_tags, elem_node_tags, entity_tag);
    if (!elem_tags.empty()) {
      elem_entity_tags.insert(elem_entity_tags.end(), elem_tags.begin(), elem_tags.end());
    }
  }
}

/**
 * @brief Get the Physical Group all info.
 *
 * @tparam Dim The dimension of physical group.
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
template <Isize Dim>
inline std::pair<std::string, std::vector<std::size_t>> getPhysicalGroups(int physical_group_tag) {
  std::string physical_group_name;
  std::vector<int> physical_group_entity_tag;
  gmsh::model::getPhysicalName(Dim, physical_group_tag, physical_group_name);
  gmsh::model::getEntitiesForPhysicalGroup(Dim, physical_group_tag, physical_group_entity_tag);
  std::vector<std::size_t> elem_entity_tags;
  if constexpr (Dim == 1) {
    concatenateElementEntityTags<kLine>(physical_group_entity_tag, elem_entity_tags);
  } else if constexpr (Dim == 2) {
    concatenateElementEntityTags<kTri>(physical_group_entity_tag, elem_entity_tags);
    concatenateElementEntityTags<kQuad>(physical_group_entity_tag, elem_entity_tags);
  }
  return {physical_group_name, elem_entity_tags};
}

/**
 * @brief Get the Mesh Supplemental information.
 *
 * @tparam ElemT The element type.
 * @tparam T The name map type, BoundaryType for boundary name map, Isize for region name map.
 * @param [in] name_map The name map to get the relationship between name and tag.
 * @param [out] mesh_supplemental The MeshSupplemental structure.
 *
 * @exception std::out_of_range The boundary name or region name is not found in config file.
 *
 * @details // TODO: Add details.
 *
 */
template <ElemInfo ElemT, typename T>
inline void getMeshSupplemental(const std::unordered_map<std::string_view, T>& name_map,
                                MeshSupplemental<ElemT>& mesh_supplemental) {
  std::vector<std::pair<int, int>> physical_group_tags;
  gmsh::model::getPhysicalGroups(physical_group_tags, static_cast<int>(ElemT.kDim));
  std::vector<std::pair<std::string, std::vector<std::size_t>>> physical_groups;
  physical_groups.reserve(physical_group_tags.size());
  for (const auto& [dimension, physical_group_tag] : physical_group_tags) {
    physical_groups.emplace_back(getPhysicalGroups<ElemT.kDim>(physical_group_tag));
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
        for (const auto elem_entity_tag : elem_entity_tags) {
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

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_MESH_SUPPLEMENTAL_HPP_
