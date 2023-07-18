/**
 * @file write_raw_buffer.hpp
 * @brief The head file to write raw buffer.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-07-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_WRITE_RAW_BUFFER_HPP_
#define SUBROSA_DG_WRITE_RAW_BUFFER_HPP_

#include <fstream>

#include "basic/concept.hpp"
#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "mesh/mesh_structure.hpp"
#include "solver/solver_structure.hpp"

namespace SubrosaDG {

template <int Dim, PolyOrder P, ElemType ElemT>
inline void writeElemRawBuffer(const ElemMesh<Dim, P, ElemT>& elem_mesh,
                               const ElemSolver<Dim, P, ElemT, EquModel::Euler>& elem_solver, std::ofstream& fout) {
  for (Isize i = 0; i < elem_mesh.num_; i++) {
    fout << elem_solver.elem_(i).basis_fun_coeff_(1) << std::endl << std::endl;
  }
}

template <PolyOrder P, MeshType MeshT>
inline void writeRawBuffer(const Mesh<2, P, MeshT>& mesh, const Solver<2, P, EquModel::Euler, MeshT>& solver,
                           std::ofstream& fout) {
  if constexpr (HasTri<MeshT>) {
    writeElemRawBuffer(mesh.tri_, solver.tri_, fout);
  }
  if constexpr (HasQuad<MeshT>) {
    writeElemRawBuffer(mesh.quad_, solver.quad_, fout);
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_WRITE_RAW_BUFFER_HPP_
