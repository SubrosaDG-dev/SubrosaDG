/**
 * @file write_ascii_tec.hpp
 * @brief The write ascii tecplot header file.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-07-16
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_WRITE_ASCII_TEC_HPP_
#define SUBROSA_DG_WRITE_ASCII_TEC_HPP_

#include <fmt/core.h>

#include <Eigen/Core>
#include <fstream>
#include <string>

#include "basic/concept.hpp"
#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "mesh/get_elem_info.hpp"
#include "mesh/mesh_structure.hpp"
#include "view/index/cal_elem_num.hpp"
#include "view/variable/get_output_var_num.hpp"
#include "view/view_structure.hpp"

namespace SubrosaDG {

template <EquModel EquModelT>
inline std::string getVarList(int dim) {
  std::string var_list;
  switch (EquModelT) {
  case EquModel::Euler:
    switch (dim) {
    case 2:
      var_list = R"(VARIABLES = "x", "y", "rho", "u", "v", "p", "T")";
      break;
    case 3:
      var_list = R"(VARIABLES = "x", "y", "z", "rho", "u", "v", "w", "p", "T")";
      break;
    }
    break;
  case EquModel::NS:
    switch (dim) {
    case 2:
      var_list = R"(VARIABLES = "x", "y", "rho", "u", "v", "w", "p", "T")";
      break;
    case 3:
      var_list = R"(VARIABLES = "x", "y", "z", "rho", "u", "v", "w", "p", "T")";
      break;
    }
    break;
  }
  return var_list;
}

template <PolyOrder P, MeshType MeshT, EquModel EquModelT>
inline void writeAsciiTecHeader(const int step, const Mesh<2, P, MeshT>& mesh, std::ofstream& fout) {
  fout << getVarList<EquModelT>(2) << std::endl;
  fout << fmt::format(R"(Zone T="Step {}", ZONETYPE=FEQUADRILATERAL, NODES={}, ELEMENTS={}, DATAPACKING=POINT)", step,
                      mesh.node_num_, calElemNum(mesh))
       << std::endl;
}

template <int Dim, PolyOrder P, MeshType MeshT, EquModel EquModelT>
inline void writeAsciiTecNodeVar(const Mesh<Dim, P, MeshT>& mesh, const View<Dim, P, MeshT, EquModelT>& view,
                                 std::ofstream& fout) {
  Eigen::Matrix<Real, Dim + getOutputVarNum<EquModelT>(Dim), Eigen::Dynamic> node_all_var(
      Dim + getOutputVarNum<EquModelT>(Dim), mesh.node_num_);
  node_all_var << mesh.node_, view.node_.output_var_;
  fout << node_all_var.transpose() << std::endl;
}

template <PolyOrder P, ElemType ElemT>
inline void writeAsciiTecIndex(const ElemMesh<2, P, ElemT>& elem_mesh, std::ofstream& fout) {
  // Eigen::Matrix<Isize, 4, Eigen::Dynamic> elem_index(4, elem_mesh.num_ * getSubElemNum<ElemT>(PolyOrder::P1));
  Eigen::Matrix<Isize, 4, Eigen::Dynamic> elem_index(4, elem_mesh.num_ * getSubElemNum<ElemT>(P));
  for (Isize i = 0; i < elem_mesh.num_; i++) {
    // elem_index(Eigen::seqN(Eigen::fix<0>, Eigen::fix<getNodeNum<ElemT>(PolyOrder::P1)>), i) =
    //     elem_mesh.elem_(i).index_(Eigen::seqN(Eigen::fix<0>, Eigen::fix<getNodeNum<ElemT>(PolyOrder::P1)>));
    // if constexpr (ElemT == ElemType::Tri) {
    //   elem_index(3, i) = elem_mesh.elem_(i).index_(2);
    // }
    for (Isize j = 0; j < getSubElemNum<ElemT>(P); j++) {
      if constexpr (ElemT == ElemType::Tri) {
        elem_index.col(i * getSubElemNum<ElemT>(P) + j) << elem_mesh.elem_(i).index_(elem_mesh.subelem_index_.col(j)),
            elem_mesh.elem_(i).index_(elem_mesh.subelem_index_(Eigen::last, j));
      } else if constexpr (ElemT == ElemType::Quad) {
        elem_index.col(i * getSubElemNum<ElemT>(P) + j) << elem_mesh.elem_(i).index_(elem_mesh.subelem_index_.col(j));
      }
    }
  }
  fout << elem_index.transpose() << std::endl;
}

template <PolyOrder P, MeshType MeshT, EquModel EquModelT>
inline void writeAsciiTec(const int step, const Mesh<2, P, MeshT>& mesh, const View<2, P, MeshT, EquModelT>& view,
                          std::ofstream& fout) {
  writeAsciiTecHeader<P, MeshT, EquModelT>(step, mesh, fout);
  writeAsciiTecNodeVar(mesh, view, fout);
  if constexpr (HasTri<MeshT>) {
    writeAsciiTecIndex(mesh.tri_, fout);
  }
  if constexpr (HasQuad<MeshT>) {
    writeAsciiTecIndex(mesh.quad_, fout);
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_WRITE_ASCII_TEC_HPP_
