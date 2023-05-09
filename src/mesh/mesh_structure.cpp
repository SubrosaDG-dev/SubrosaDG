/**
 * @file mesh_structure.cpp
 * @brief The mesh structure source file.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-06
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

// clang-format off

#include "mesh/mesh_structure.h"

#include <gmsh.h>  // for open

// clang-format on

namespace SubrosaDG::Internal {

Element::Element(std::string_view element_name, Isize element_num) {
  this->element_type_info_ = std::make_pair(element_name, element_num);
}

Mesh2d::Mesh2d(const std::filesystem::path& mesh_file) { gmsh::open(mesh_file.string()); }

}  // namespace SubrosaDG::Internal
