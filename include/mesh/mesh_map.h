/**
 * @file mesh_map.h
 * @brief The header file for mesh map.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-06
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_MESH_MAP_H_
#define SUBROSA_DG_MESH_MAP_H_

// clang-format off

#include <unordered_map>       // for unordered_map
#include <string_view>         // for string_view, hash, operator==

#include "basic/data_types.h"  // for Usize

// clang-format on

namespace SubrosaDG::Internal {

inline const std::unordered_map<std::string_view, Usize> kElementPointNumMap{
    {"Point", 1},       {"Line", 2},    {"Triangle", 3}, {"Quadrangle", 4},
    {"Tetrahedron", 4}, {"Pyramid", 5}, {"Prism", 6},    {"Hexahedron", 8},
};

}  // namespace SubrosaDG::Internal

#endif  // SUBROSA_DG_MESH_MAP_H_
