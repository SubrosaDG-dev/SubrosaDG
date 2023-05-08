/**
 * @file reconstruct_edge.h
 * @brief The head file to reconstrcuct edge from mesh.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-04-27
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_RECONSTRUCT_EDGE_H_
#define SUBROSA_DG_RECONSTRUCT_EDGE_H_

// clang-format off

#include <map>                 // for map
#include <string>              // for string
#include <vector>              // for vector
#include <cstddef>             // for size_t

#include "basic/data_types.h"  // for Usize

// clang-format on

namespace SubrosaDG::Internal {

struct Mesh2d;
struct MeshSupplementalInfo;

struct EdgeInfo {
  std::string element_name_;
  std::vector<std::size_t> edge_nodes_;
  std::vector<std::size_t> edge_tags_;
  std::vector<std::size_t> element_tags_;

  EdgeInfo(std::string element_name);
};

void reconstructEdge(Mesh2d& mesh, const MeshSupplementalInfo& mesh_supplemental_info);

Usize getMaxEdgeTag(EdgeInfo& edge_info);

void reconstructElementEdge(std::map<Usize, bool>& edges2_elements, EdgeInfo& edge_info, Mesh2d& mesh);

}  // namespace SubrosaDG::Internal

#endif  // SUBROSA_DG_RECONSTRUCT_EDGE_H_
