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

#include <Eigen/Core>          // for Matrix, Dynamic
#include <memory>              // for unique_ptr
#include <vector>              // for vector

#include "basic/data_types.h"  // for Usize, Real

// clang-format on

namespace SubrosaDG::Internal {

struct Mesh2d {
  Usize num_nodes_;
  Usize num_edges_interior_;
  std::vector<Usize> num_edges_boundary_;
  Usize num_cells_tri_;
  Usize num_cells_quad_;

  std::unique_ptr<Eigen::Matrix<Real, 2, Eigen::Dynamic>> nodes_;
  std::unique_ptr<Eigen::Matrix<Usize, 4, Eigen::Dynamic>> iedges_interior_;
  std::vector<Eigen::Matrix<Usize, 3, Eigen::Dynamic>> iedges_boundary_;
  std::unique_ptr<Eigen::Matrix<Usize, 3, Eigen::Dynamic>> icell_tri_;
  std::unique_ptr<Eigen::Matrix<Usize, 4, Eigen::Dynamic>> icell_quad_;
};

}  // namespace SubrosaDG::Internal

#endif  // SUBROSA_DG_MESH_STRUCTURE_H_
