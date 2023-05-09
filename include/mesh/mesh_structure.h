/**
 * @file mesh_structure.h
 * @brief The mesh structure head file.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2022-11-21
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_MESH_STRUCTURE_H_
#define SUBROSA_DG_MESH_STRUCTURE_H_

// clang-format off

#include <Eigen/Core>          // for Matrix, Dynamic, Vector
#include <filesystem>          // for path
#include <memory>              // for unique_ptr, shared_ptr
#include <utility>             // for pair
#include <string_view>         // for string_view

#include "basic/data_types.h"  // for Isize, Real

// clang-format on

namespace SubrosaDG::Internal {

struct Edge {
  std::pair<Isize, Isize> edge_num_;
  std::unique_ptr<Eigen::Matrix<Real, 6, Eigen::Dynamic>> edge_nodes_;
  std::unique_ptr<Eigen::Matrix<Isize, 4, Eigen::Dynamic>> edge_index_;
};

struct Element {
  std::pair<std::string_view, Isize> element_type_info_;
  std::pair<Isize, Isize> element_num_;
  std::unique_ptr<Eigen::Matrix<Isize, Eigen::Dynamic, Eigen::Dynamic>> element_index_;
  std::unique_ptr<Eigen::Matrix<Real, Eigen::Dynamic, Eigen::Dynamic>> element_nodes_;
  std::unique_ptr<Eigen::Vector<Real, Eigen::Dynamic>> element_area_;

  Element(std::string_view element_name, Isize element_num);
};

struct Mesh2d {
  Isize node_num_;
  Isize edge_num_;
  Isize element_num_;

  Edge interior_edge_;
  Edge boundary_edge_;

  Element triangle_element_{"Triangle", 3};
  Element quadrangle_element_{"Quadrangle", 4};

  std::shared_ptr<Eigen::Matrix<Real, 3, Eigen::Dynamic>> nodes_;

  Mesh2d(const std::filesystem::path& mesh_file);
};

struct MeshSupplementalInfo {
  std::pair<Isize, Isize> boundary_num_;
  std::pair<Isize, Isize> region_num_;

  std::unique_ptr<Eigen::Vector<Isize, Eigen::Dynamic>> boundary_index_;
  std::unique_ptr<Eigen::Vector<Isize, Eigen::Dynamic>> region_index_;
};

}  // namespace SubrosaDG::Internal

#endif  // SUBROSA_DG_MESH_STRUCTURE_H_
