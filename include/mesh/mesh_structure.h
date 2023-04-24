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

#include <Eigen/Core>
#include <memory>
#include "basic/data_types.h"

// clang-format on

namespace SubrosaDG {

struct Mesh {
  Isize num_nodes_;
  Isize num_edges_;
  Isize num_elements_;

  std::unique_ptr<Eigen::Matrix2Xd> nodes_;
  std::unique_ptr<Eigen::Matrix<Isize, 4, Eigen::Dynamic>> edges_;
  std::unique_ptr<Eigen::Matrix<Isize, 3, Eigen::Dynamic>> elements_;
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_MESH_STRUCTURE_H_
