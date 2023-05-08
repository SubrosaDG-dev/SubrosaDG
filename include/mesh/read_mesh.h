/**
 * @file read_mesh.h
 * @brief This head file to read mesh.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2022-11-02
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_READ_MESH_H_
#define SUBROSA_DG_READ_MESH_H_

// clang-format off

#include <string>   // for string
#include <vector>   // for vector
#include <cstddef>  // for size_t
#include <tuple>    // for tuple

// clang-format on

namespace SubrosaDG::Internal {

struct Config;
struct IElement;
struct Mesh2d;
struct MeshSupplementalInfo;

void readMesh(Mesh2d& mesh);

void readMeshSupplementalInfo(const Config& config, MeshSupplementalInfo& mesh_supplemental_info);

std::tuple<int, std::string, std::vector<std::size_t>> getPhysicalGroups(int dim, int physical_group_tag);

void getNodes(Mesh2d& mesh);

void getElements(IElement& i_element, const std::string& element_name);

}  // namespace SubrosaDG::Internal

#endif  // SUBROSA_DG_READ_MESH_H_
