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

#include <gmsh.h>

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "mesh/get_elem_info.hpp"
#include "mesh/mesh_structure.hpp"

namespace SubrosaDG {

struct PhysicalGroup {
  std::string name_;
  std::vector<Usize> elem_entity_tags_;
};

template <ElemType ElemT>
inline void concatenateElemEntityTags(const std::vector<int>& physical_group_entity_tag,
                                      std::vector<Usize>& elem_entity_tags) {
  std::vector<Usize> elem_tags;
  std::vector<Usize> elem_node_tags;
  for (const auto entity_tag : physical_group_entity_tag) {
    gmsh::model::mesh::getElementsByType(getTopology<ElemT>(), elem_tags, elem_node_tags, entity_tag);
    if (!elem_tags.empty()) {
      elem_entity_tags.insert(elem_entity_tags.end(), elem_tags.begin(), elem_tags.end());
    }
  }
}

template <ElemType ElemT>
inline PhysicalGroup getPhysicalGroup(int physical_group_tag) {
  std::string physical_group_name;
  std::vector<int> physical_group_entity_tag;
  gmsh::model::getPhysicalName(getDim<ElemT>(), physical_group_tag, physical_group_name);
  gmsh::model::getEntitiesForPhysicalGroup(getDim<ElemT>(), physical_group_tag, physical_group_entity_tag);
  std::vector<Usize> elem_entity_tags;
  concatenateElemEntityTags<ElemT>(physical_group_entity_tag, elem_entity_tags);
  return {physical_group_name, elem_entity_tags};
}

template <typename T, ElemType ElemT>
inline void getMeshSupplemental(const std::unordered_map<std::string_view, T>& name_map,
                                MeshSupplemental<ElemT>& mesh_supplemental) {
  std::vector<std::pair<int, int>> physical_group_tags;
  gmsh::model::getPhysicalGroups(physical_group_tags, getDim<ElemT>());
  std::vector<PhysicalGroup> physical_groups;
  physical_groups.reserve(physical_group_tags.size());
  for (const auto& [dimension, physical_group_tag] : physical_group_tags) {
    physical_groups.emplace_back(getPhysicalGroup<ElemT>(physical_group_tag));
  }
  std::vector<Usize> all_elem_entity_tags;
  for (const auto& physical_group : physical_groups) {
    all_elem_entity_tags.insert(all_elem_entity_tags.end(), physical_group.elem_entity_tags_.begin(),
                                physical_group.elem_entity_tags_.end());
  }
  mesh_supplemental.range_ = std::make_pair(all_elem_entity_tags.front(), all_elem_entity_tags.back());
  mesh_supplemental.num_ = mesh_supplemental.range_.second - mesh_supplemental.range_.first + 1;
  mesh_supplemental.index_.resize(mesh_supplemental.num_);
  try {
    for (const auto& physical_group : physical_groups) {
      for (const auto elem_entity_tag : physical_group.elem_entity_tags_) {
        mesh_supplemental.index_(static_cast<Isize>(elem_entity_tag) - mesh_supplemental.range_.first) =
            static_cast<int>(name_map.at(physical_group.name_));
      }
    }
  } catch (const std::out_of_range& error) {
    std::cerr << error.what() << std::endl;
    std::exit(EXIT_FAILURE);
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_MESH_SUPPLEMENTAL_HPP_
