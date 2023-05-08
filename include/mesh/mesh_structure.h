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

#include <Eigen/Core>          // for Matrix, Dynamic, Vector, VectorXd
#include <filesystem>          // for path
#include <memory>              // for unique_ptr
#include <utility>             // for pair

#include "basic/data_types.h"  // for Usize, Isize, Real

// clang-format on

namespace SubrosaDG::Internal {

struct IElement {
  std::pair<Usize, Usize> num_elements_;
  std::unique_ptr<Eigen::Matrix<Isize, Eigen::Dynamic, Eigen::Dynamic>> ielements_;
};

struct Mesh2d {
  Usize num_nodes_;
  Usize num_edges_;
  Usize num_elements_;

  IElement ielement_triangle_;
  IElement ielement_quadrangle_;

  std::unique_ptr<Eigen::Matrix<Real, 3, Eigen::Dynamic>> nodes_;
  std::unique_ptr<Eigen::Matrix<Isize, 4, Eigen::Dynamic>> iedges_;
  std::unique_ptr<Eigen::VectorXd> element_area_;

  Mesh2d(const std::filesystem::path& mesh_file);
};

struct MeshSupplementalInfo {
  std::pair<Usize, Usize> num_boundary_;
  std::pair<Usize, Usize> num_region_;

  std::unique_ptr<Eigen::Vector<Isize, Eigen::Dynamic>> iboundary_;
  std::unique_ptr<Eigen::Vector<Isize, Eigen::Dynamic>> iregion_;
};

}  // namespace SubrosaDG::Internal

#endif  // SUBROSA_DG_MESH_STRUCTURE_H_
