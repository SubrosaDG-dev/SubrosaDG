/**
 * @file read_raw_buffer.hpp
 * @brief The head file to define read raw buffer function.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-08-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_READ_RAW_BUFFER_HPP_
#define SUBROSA_DG_READ_RAW_BUFFER_HPP_

#include <fstream>

#include "basic/concept.hpp"
#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "integral/cal_basisfun_num.hpp"
#include "mesh/mesh_structure.hpp"
#include "solver/variable/get_var_num.hpp"
#include "view/view_structure.hpp"

namespace SubrosaDG {

template <int Dim, PolyOrder P, ElemType ElemT, EquModel EquModelT>
inline void readElemRawBuffer(const ElemMesh<Dim, P, ElemT>& elem_mesh,
                              ElemSolverView<Dim, P, ElemT, EquModelT>& elem_solver_view, std::ifstream& fin) {
  for (Isize i = 0; i < elem_mesh.num_; i++) {
#ifndef SUBROSA_DG_DEVELOP
    fin.read(
        reinterpret_cast<char*>(elem_solver_view.elem_(i).basis_fun_coeff_.data()),
        getConservedVarNum<EquModelT>(Dim) * calBasisFunNum<ElemT>(P) * static_cast<std::streamsize>(sizeof(Real)));
#else
    for (Isize j = 0; j < getConservedVarNum<EquModelT>(Dim); j++) {
      for (Isize k = 0; k < calBasisFunNum<ElemT>(P); k++) {
        fin >> elem_solver_view.elem_(i).basis_fun_coeff_(j, k);
      }
    }
#endif
  }
}

template <PolyOrder P, MeshType MeshT, EquModel EquModelT>
inline void readRawBuffer(const Mesh<2, P, MeshT>& mesh, View<2, P, MeshT, EquModelT>& view, std::ifstream& fin) {
  if constexpr (HasTri<MeshT>) {
    readElemRawBuffer<2, P, ElemType::Tri, EquModelT>(mesh.tri_, view.tri_, fin);
  }
  if constexpr (HasQuad<MeshT>) {
    readElemRawBuffer<2, P, ElemType::Quad, EquModelT>(mesh.quad_, view.quad_, fin);
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_READ_RAW_BUFFER_HPP_
