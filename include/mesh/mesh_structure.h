/**
 * @file mesh_structure.h
 * @brief The mesh structure head file.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2022-11-21
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2022 by SubrosaDG developers
 */

#ifndef SUBROSA_DG_MESH_STRUCTURE_H_
#define SUBROSA_DG_MESH_STRUCTURE_H_

#include <cgnslib.h>

#include <Eigen/Core>
#include <memory>
#include <string>
#include <vector>

#include "core/basic.h"

namespace SubrosaDG {

struct MeshStructure {
  int filetype_;
  int fn_;

  float cgns_version_;

  int cell_dim_;
  int physical_dim_;

  int ncoords_;
  int nbocos_;
  int nsections_;
  int nfamilies_;

  std::unique_ptr<std::string> descriptor_text_;

  std::array<cgsize_t, 3> zone_size_;

  std::vector<std::string> coord_name_;
  std::vector<DataType_t> coord_type_;
  std::vector<std::unique_ptr<Eigen::Vector<real, Eigen::Dynamic>>> coord_;

  std::vector<std::array<cgsize_t, 2>> points_index_;
  std::vector<std::string> zonebc_family_name_;

  std::vector<int> element_type_num_;
  std::vector<cgsize_t> element_start_;
  std::vector<cgsize_t> element_end_;
  std::vector<std::unique_ptr<Eigen::Matrix<cgsize_t, Eigen::Dynamic, Eigen::Dynamic>>> element_connectivity_;

  std::vector<std::string> family_;
  std::vector<std::string> family_name_;
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_MESH_STRUCTURE_H_
