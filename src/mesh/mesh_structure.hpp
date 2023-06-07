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

#include "basic/concept.hpp"    // IWYU pragma: keep
#include "basic/data_type.hpp"  // for Isize, Real
#include "basic/enum.hpp"       // for MeshType
#include "mesh/elem_type.hpp"   // for ElemInfo, kLine, kQuad, kTri

// clang-format on

namespace SubrosaDG {

template <int Dim, ElemInfo ElemT>
struct PerElemMeshBase {
  Eigen::Matrix<Real, Dim, ElemT.kNodeNum> node_;
  Eigen::Vector<Isize, ElemT.kNodeNum> index_;
  Real jacobian_;
};

template <int Dim, ElemInfo ElemT>
struct PerElemMesh : PerElemMeshBase<Dim, ElemT> {
  Eigen::Vector<Real, Dim> projection_measure_;
};

template <int Dim, ElemInfo ElemT>
struct ElemMesh {
  std::pair<Isize, Isize> range_;
  Isize num_;
  Eigen::Vector<PerElemMesh<Dim, ElemT>, Eigen::Dynamic> elem_;
};

template <int Dim, ElemInfo ElemT, MeshType MeshT, bool IsInternal>
struct PerAdjacencyElemMesh;

template <int Dim, ElemInfo ElemT, MeshType MeshT, bool IsInternal>
  requires IsUniform<MeshT>
struct PerAdjacencyElemMesh<Dim, ElemT, MeshT, IsInternal> : PerElemMeshBase<Dim, ElemT> {
  Eigen::Vector<Isize, 2> parent_index_;
  Eigen::Vector<Isize, 1 + static_cast<Isize>(IsInternal)> adjacency_index_;
  Eigen::Vector<Real, Dim> norm_vec_;
};

template <int Dim, ElemInfo ElemT, MeshType MeshT, bool IsInternal>
  requires IsMixed<MeshT>
struct PerAdjacencyElemMesh<Dim, ElemT, MeshT, IsInternal> : PerElemMeshBase<Dim, ElemT> {
  Eigen::Vector<Isize, 2> parent_index_;
  Eigen::Vector<Isize, 1 + static_cast<Isize>(IsInternal)> adjacency_index_;
  Eigen::Vector<int, 1 + static_cast<Isize>(IsInternal)> typology_index_;
  Eigen::Vector<Real, Dim> norm_vec_;
};

template <int Dim, ElemInfo ElemT, MeshType MeshT, bool IsInternal>
struct AdjacencyElemTypeMesh {
  std::pair<Isize, Isize> range_;
  Isize num_;
  Eigen::Vector<PerAdjacencyElemMesh<Dim, ElemT, MeshT, IsInternal>, Eigen::Dynamic> elem_;
};

template <int Dim, ElemInfo ElemT, MeshType MeshT>
struct AdjacencyElemMesh {
  AdjacencyElemTypeMesh<Dim, ElemT, MeshT, true> internal_;
  AdjacencyElemTypeMesh<Dim, ElemT, MeshT, false> boundary_;
};

template <int Dim>
using TriElemMesh = ElemMesh<Dim, kTri>;

template <int Dim>
using QuadElemMesh = ElemMesh<Dim, kQuad>;

template <int Dim, MeshType MeshT>
using AdjacencyLineElemMesh = AdjacencyElemMesh<Dim, kLine, MeshT>;

template <int Dim>
struct MeshBase {
  Isize node_num_;
  Eigen::Matrix<Real, Dim, Eigen::Dynamic> node_;

  inline MeshBase(const std::filesystem::path& mesh_file) { gmsh::open(mesh_file.string()); }
  inline ~MeshBase() { gmsh::clear(); }
};

template <int Dim, MeshType MeshT>
struct Mesh;

template <>
struct Mesh<2, MeshType::Tri> : MeshBase<2> {
  TriElemMesh<2> tri_;
  AdjacencyLineElemMesh<2, MeshType::Tri> line_;

  using MeshBase<2>::MeshBase;
};

template <>
struct Mesh<2, MeshType::Quad> : MeshBase<2> {
  QuadElemMesh<2> quad_;
  AdjacencyLineElemMesh<2, MeshType::Quad> line_;

  using MeshBase<2>::MeshBase;
};

template <>
struct Mesh<2, MeshType::TriQuad> : MeshBase<2> {
  TriElemMesh<2> tri_;
  QuadElemMesh<2> quad_;
  AdjacencyLineElemMesh<2, MeshType::TriQuad> line_;

  using MeshBase<2>::MeshBase;
};

template <ElemInfo ElemT>
struct MeshSupplemental {
  std::pair<Isize, Isize> range_;
  Isize num_;
  Eigen::Vector<int, Eigen::Dynamic> index_;
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_MESH_STRUCTURE_HPP_
