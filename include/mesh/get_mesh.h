/**
 * @file get_mesh.h
 * @brief This head file to get mesh.
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

#include <Eigen/Core>          // for Dynamic, Matrix
#include <string>              // for string
#include <vector>              // for vector
#include <cstddef>             // for size_t
#include <string_view>         // for string_view
#include <utility>             // for pair

#include "basic/data_types.h"  // for Real

// clang-format on

namespace SubrosaDG::Internal {

struct Config;
struct ElementMesh;
struct Mesh2d;
struct MeshSupplemental;

void getMesh(const Config& config, Mesh2d& mesh);

/**
 * @brief Read mesh supplemental information.
 *
 * @param [in] config The Config structure.
 * @param [out] supplemental The MeshSupplemental structure.
 *
 * @exception std::out_of_range The boundary name or region name is not found in config file.
 */
void readMeshSupplemental(const Config& config, MeshSupplemental& supplemental);

/**
 * @brief Get the Physical Group all info.
 *
 * @param [in] dim The dimension of mesh.
 * @param [in] physical_group_tag The physical group tag.
 * @return std::pair<std::string, std::vector<std::size_t>> The physical group info(name, entity tags).
 */
std::pair<std::string, std::vector<std::size_t>> getPhysicalGroups(int dim, int physical_group_tag);

/**
 * @brief Get element entity tags for specific physical group.
 *
 * @param [out] elem_entity_tags The element entity tags for specific physical group.
 * @param [in] physical_group_entity_tag The physical group entity tag.
 * @param [in] element_type_name The element type name.
 */
void concatenateElementEntityTags(std::vector<std::size_t>& elem_entity_tags,
                                  const std::vector<int>& physical_group_entity_tag,
                                  const std::string_view& element_type_name);

void getNodes(Mesh2d& mesh);

void getElement(const Eigen::Matrix<Real, 3, Eigen::Dynamic>& nodes, ElementMesh& element_mesh);

}  // namespace SubrosaDG::Internal

#endif  // SUBROSA_DG_READ_MESH_H_
