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

#include <gmsh.h>

#include <Eigen/Core>
#include <filesystem>
#include <utility>

#include "basic/concept.hpp"  // IWYU pragma: keep
#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "mesh/get_elem_info.hpp"

namespace SubrosaDG {

template <int Dim, ElemType ElemT>
struct PerElemMeshBase {
  Eigen::Matrix<Real, Dim, getNodeNum<ElemT>()> node_;
  Eigen::Vector<Isize, getNodeNum<ElemT>()> index_;
  Real jacobian_;
};

template <int Dim, ElemType ElemT>
struct PerElemMesh : PerElemMeshBase<Dim, ElemT> {
  Eigen::Vector<Real, Dim> projection_measure_;
};

template <int Dim, ElemType ElemT>
struct ElemMesh {
  std::pair<Isize, Isize> range_;
  Isize num_;
  Eigen::Vector<PerElemMesh<Dim, ElemT>, Eigen::Dynamic> elem_;
};

template <int Dim, ElemType ElemT, MeshType MeshT, bool IsInternal>
struct PerAdjacencyElemMesh;

template <int Dim, ElemType ElemT, MeshType MeshT, bool IsInternal>
  requires IsUniform<MeshT>
struct PerAdjacencyElemMesh<Dim, ElemT, MeshT, IsInternal> : PerElemMeshBase<Dim, ElemT> {
  Eigen::Vector<Isize, 2> parent_index_;
  Eigen::Vector<Isize, 1 + static_cast<Isize>(IsInternal)> adjacency_index_;
  Eigen::Vector<Real, Dim> norm_vec_;
};

template <int Dim, ElemType ElemT, MeshType MeshT, bool IsInternal>
  requires IsMixed<MeshT>
struct PerAdjacencyElemMesh<Dim, ElemT, MeshT, IsInternal> : PerElemMeshBase<Dim, ElemT> {
  Eigen::Vector<Isize, 2> parent_index_;
  Eigen::Vector<Isize, 1 + static_cast<Isize>(IsInternal)> adjacency_index_;
  Eigen::Vector<int, 1 + static_cast<Isize>(IsInternal)> typology_index_;
  Eigen::Vector<Real, Dim> norm_vec_;
};

template <int Dim, ElemType ElemT, MeshType MeshT, bool IsInternal>
struct AdjacencyElemTypeMesh {
  std::pair<Isize, Isize> range_;
  Isize num_;
  Eigen::Vector<PerAdjacencyElemMesh<Dim, ElemT, MeshT, IsInternal>, Eigen::Dynamic> elem_;
};

template <int Dim, ElemType ElemT, MeshType MeshT>
struct AdjacencyElemMesh {
  AdjacencyElemTypeMesh<Dim, ElemT, MeshT, true> internal_;
  AdjacencyElemTypeMesh<Dim, ElemT, MeshT, false> boundary_;
};

using TriElemMesh = ElemMesh<2, ElemType::Tri>;

using QuadElemMesh = ElemMesh<2, ElemType::Quad>;

template <MeshType MeshT>
using AdjacencyLineElemMesh = AdjacencyElemMesh<2, ElemType::Line, MeshT>;

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
  TriElemMesh tri_;
  AdjacencyLineElemMesh<MeshType::Tri> line_;

  using MeshBase<2>::MeshBase;
};

template <>
struct Mesh<2, MeshType::Quad> : MeshBase<2> {
  QuadElemMesh quad_;
  AdjacencyLineElemMesh<MeshType::Quad> line_;

  using MeshBase<2>::MeshBase;
};

template <>
struct Mesh<2, MeshType::TriQuad> : MeshBase<2> {
  TriElemMesh tri_;
  QuadElemMesh quad_;
  AdjacencyLineElemMesh<MeshType::TriQuad> line_;

  using MeshBase<2>::MeshBase;
};

template <ElemType ElemT>
struct MeshSupplemental {
  std::pair<Isize, Isize> range_;
  Isize num_;
  Eigen::Vector<int, Eigen::Dynamic> index_;
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_MESH_STRUCTURE_HPP_
