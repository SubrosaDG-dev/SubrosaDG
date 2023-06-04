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
#include <cstdlib>              // for exit, EXIT_FAILURE
#include <stdexcept>            // for out_of_range
#include <string>               // for string
#include <string_view>          // for operator==, string_view
#include <unordered_map>        // for unordered_map
#include <utility>              // for pair, make_pair
#include <vector>               // for vector

#include "basic/data_type.hpp"  // for Usize, Isize
#include "mesh/elem_type.hpp"   // for ElemInfo, kLine, kQuad, kTri

// clang-format on

namespace SubrosaDG {

template <int ElemDim>
struct MeshSupplemental;

template <ElemInfo ElemT>
inline void concatenateElemEntityTags(const std::vector<int>& physical_group_entity_tag,
                                      std::vector<Usize>& elem_entity_tags) {
  std::vector<Usize> elem_tags;
  std::vector<Usize> elem_node_tags;
  for (const auto entity_tag : physical_group_entity_tag) {
    gmsh::model::mesh::getElementsByType(ElemT.kTopology, elem_tags, elem_node_tags, entity_tag);
    if (!elem_tags.empty()) {
      elem_entity_tags.insert(elem_entity_tags.end(), elem_tags.begin(), elem_tags.end());
    }
  }
}

template <int ElemDim>
inline std::pair<std::string, std::vector<Usize>> getPhysicalGroup(int physical_group_tag) {
  std::string physical_group_name;
  std::vector<int> physical_group_entity_tag;
  gmsh::model::getPhysicalName(ElemDim, physical_group_tag, physical_group_name);
  gmsh::model::getEntitiesForPhysicalGroup(ElemDim, physical_group_tag, physical_group_entity_tag);
  std::vector<Usize> elem_entity_tags;
  if constexpr (ElemDim == 1) {
    concatenateElemEntityTags<kLine>(physical_group_entity_tag, elem_entity_tags);
  } else if constexpr (ElemDim == 2) {
    concatenateElemEntityTags<kTri>(physical_group_entity_tag, elem_entity_tags);
    concatenateElemEntityTags<kQuad>(physical_group_entity_tag, elem_entity_tags);
  }
  return {physical_group_name, elem_entity_tags};
}

template <typename T, int ElemDim>
inline void getMeshSupplemental(const std::unordered_map<std::string_view, T>& name_map,
                                MeshSupplemental<ElemDim>& mesh_supplemental) {
  std::vector<std::pair<int, int>> physical_group_tags;
  gmsh::model::getPhysicalGroups(physical_group_tags, ElemDim);
  std::vector<std::pair<std::string, std::vector<Usize>>> physical_groups;
  physical_groups.reserve(physical_group_tags.size());
  for (const auto& [dimension, physical_group_tag] : physical_group_tags) {
    physical_groups.emplace_back(getPhysicalGroup<ElemDim>(physical_group_tag));
  }
  std::vector<Usize> all_elem_entity_tags;
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
              static_cast<int>(name_map.at(name));
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
