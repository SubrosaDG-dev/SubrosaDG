/**
 * @file mesh_structure.hpp
 * @brief The mesh structure head file.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2022-11-21
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_MESH_STRUCTURE_HPP_
#define SUBROSA_DG_MESH_STRUCTURE_HPP_

// clang-format off

#include <gmsh.h>               // for clear, open
#include <Eigen/Core>           // for Vector, Dynamic, Matrix
#include <filesystem>           // for path
#include <utility>              // for pair

#include "basic/data_type.hpp"  // for Isize, Real
#include "basic/enum.hpp"       // for MeshType
#include "mesh/elem_type.hpp"   // for ElemInfo, kLine, kQuad, kTri

// clang-format on

namespace SubrosaDG {

template <int Dim, ElemInfo ElemT>
struct PerElemMesh {
  Eigen::Matrix<Real, Dim, ElemT.kNodeNum> node_;
  Eigen::Vector<Isize, ElemT.kNodeNum> index_;
  Real jacobian_;
};

template <int Dim, ElemInfo ElemT>
struct ElemMesh {
  std::pair<Isize, Isize> range_;
  Isize num_;
  Eigen::Vector<PerElemMesh<Dim, ElemT>, Eigen::Dynamic> elem_;
};

template <int Dim, ElemInfo ElemT, bool IsInternal>
struct PerAdjacencyElemMesh {
  Eigen::Matrix<Real, Dim, ElemT.kNodeNum> node_;
  Eigen::Vector<Real, Dim> norm_vec_;
  Eigen::Vector<Isize, ElemT.kNodeNum + 4 + 2 * static_cast<Isize>(IsInternal)> index_;
  Real jacobian_;
};

template <int Dim, ElemInfo ElemT, bool IsInternal>
struct AdjacencyElemTypeMesh {
  std::pair<Isize, Isize> range_;
  Isize num_;
  Eigen::Vector<PerAdjacencyElemMesh<Dim, ElemT, IsInternal>, Eigen::Dynamic> elem_;
};

template <int Dim, ElemInfo ElemT>
struct AdjacencyElemMesh {
  AdjacencyElemTypeMesh<Dim, ElemT, true> internal_;
  AdjacencyElemTypeMesh<Dim, ElemT, false> boundary_;
};

template <int Dim>
using TriElemMesh = ElemMesh<Dim, kTri>;

template <int Dim>
using QuadElemMesh = ElemMesh<Dim, kQuad>;

template <int Dim>
using AdjacencyLineElemMesh = AdjacencyElemMesh<Dim, kLine>;

template <int Dim>
struct MeshBase {
  Isize node_num_;
  Eigen::Matrix<Real, Dim, Eigen::Dynamic> node_;

  inline MeshBase(const std::filesystem::path& mesh_file) { gmsh::open(mesh_file.string()); }
  inline ~MeshBase() { gmsh::clear(); }
};

template <int Dim, MeshType MeshT>
struct Mesh {};

template <>
struct Mesh<2, MeshType::Tri> : MeshBase<2> {
  TriElemMesh<2> tri_;
  AdjacencyLineElemMesh<2> line_;

  using MeshBase<2>::MeshBase;
};

template <>
struct Mesh<2, MeshType::Quad> : MeshBase<2> {
  QuadElemMesh<2> quad_;
  AdjacencyLineElemMesh<2> line_;

  using MeshBase<2>::MeshBase;
};

template <>
struct Mesh<2, MeshType::TriQuad> : MeshBase<2> {
  TriElemMesh<2> tri_;
  QuadElemMesh<2> quad_;
  AdjacencyLineElemMesh<2> line_;

  using MeshBase<2>::MeshBase;
};

template <int ElemDim>
struct MeshSupplemental {
  std::pair<Isize, Isize> range_;
  Isize num_;
  Eigen::Vector<int, Eigen::Dynamic> index_;
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_MESH_STRUCTURE_HPP_