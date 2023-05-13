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

#include <gmsh.h>  // for getElementType, open
#include <memory>  // for allocator

// clang-format on

namespace SubrosaDG::Internal {

ElementInfo::ElementInfo(const std::string_view& name, Isize nodes_num_per_element)
    : name_(name), nodes_num_per_element_(nodes_num_per_element) {
  this->element_type_ = gmsh::model::mesh::getElementType(this->name_.data(), 1);
}

ElementMesh::ElementMesh(const std::string_view& name, Isize nodes_num_per_element)
    : ElementInfo(name, nodes_num_per_element) {}

ElementIntegral::ElementIntegral(const std::string_view& name, Isize nodes_num_per_element)
    : ElementInfo(name, nodes_num_per_element) {}

ElementGradIntegral::ElementGradIntegral(const std::string_view& name, Isize nodes_num_per_element)
    : ElementInfo(name, nodes_num_per_element), ElementIntegral(name, nodes_num_per_element) {}

Element::Element(const std::string_view& name, const Isize nodes_num_per_element)
    : ElementInfo(name, nodes_num_per_element),
      ElementMesh(name, nodes_num_per_element),
      ElementGradIntegral(name, nodes_num_per_element) {}

AdjanencyElement::AdjanencyElement(const std::string_view& name, const Isize nodes_num_per_element)
    : ElementInfo(name, nodes_num_per_element),
      ElementMesh(name, nodes_num_per_element),
      ElementIntegral(name, nodes_num_per_element) {}

Mesh2d::Mesh2d(const std::filesystem::path& mesh_file, const Isize polynomial_order)
    : polynomial_order_(polynomial_order), gauss_integral_accuracy_(2 * polynomial_order + 1) {
  gmsh::open(mesh_file.string());
}

MeshSupplemental::MeshSupplemental(const bool is_adjanency_element, const Isize dimension)
    : is_adjanency_element_(is_adjanency_element), dimension_(dimension) {}

}  // namespace SubrosaDG::Internal
