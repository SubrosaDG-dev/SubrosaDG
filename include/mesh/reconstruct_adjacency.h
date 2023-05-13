/**
 * @file reconstruct_adjacency.h
 * @brief The head file to reconstrcuct adjacency from mesh.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-04-27
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_RECONSTRUCT_ADJACENCY_H_
#define SUBROSA_DG_RECONSTRUCT_ADJACENCY_H_

// clang-format off

#include <map>                 // for map
#include <Eigen/Core>          // for Dynamic, Matrix
#include <vector>              // for vector
#include <string_view>         // for string_view
#include <utility>             // for pair

#include "basic/data_types.h"  // for Usize, Isize, Real

// clang-format on

namespace SubrosaDG::Internal {

struct AdjanencyElement;
struct MeshSupplemental;

void reconstructAdjacency(const Eigen::Matrix<Real, 3, Eigen::Dynamic>& nodes, AdjanencyElement& interior_line,
                          AdjanencyElement& boundary_line, const MeshSupplemental& boundary_supplemental);

void getEdgeElementMap(std::map<Usize, std::pair<bool, std::vector<Isize>>>& edge_element_map,
                       const std::pair<std::string_view, Usize>& element_type_info);

}  // namespace SubrosaDG::Internal

#endif  // SUBROSA_DG_RECONSTRUCT_ADJACENCY_H_
