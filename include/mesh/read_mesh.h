/**
 * @file read_mesh.h
 * @brief This head file to read mesh.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2022-11-02
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers
 */

#ifndef SUBROSA_DG_READ_MESH_H_
#define SUBROSA_DG_READ_MESH_H_

#include <cgnslib.h>

#include <Eigen/Dense>
#include <array>
#include <filesystem>
#include <memory>
#include <vector>

#include "mesh/mesh_structure.h"

namespace SubrosaDG {

inline constexpr cgsize_t kIndex1 = 1;

void openCgnsFile(const std::shared_ptr<MeshStructure>& mesh, const std::filesystem::path& meshfile);

void readBasicData(const std::shared_ptr<MeshStructure>& mesh);

void readBaseNode(const std::shared_ptr<MeshStructure>& mesh);

void readZone(const std::shared_ptr<MeshStructure>& mesh);

void readCoord(const std::shared_ptr<MeshStructure>& mesh);

void readZoneBC(const std::shared_ptr<MeshStructure>& mesh);

void readElement(const std::shared_ptr<MeshStructure>& mesh);

void readFamily(const std::shared_ptr<MeshStructure>& mesh);

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_READ_TRI_MESH_H_
