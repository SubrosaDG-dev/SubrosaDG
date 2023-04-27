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

namespace SubrosaDG::Internal {

struct Config;
struct Mesh2d;

void readMesh(const Config& config, Mesh2d& mesh);

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_READ_MESH_H_
