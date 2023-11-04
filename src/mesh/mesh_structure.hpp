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

#include "basic/data_type.hpp"
#include "basic/enum.hpp"
#include "integral/cal_basisfun_num.hpp"
#include "integral/get_integral_num.hpp"
#include "mesh/get_elem_info.hpp"

namespace SubrosaDG {

template <int Dim, PolyOrder P, ElemType ElemT>
struct PerElemMeshBase {
  Eigen::Matrix<Real, Dim, getNodeNum<ElemT>(P)> node_;
  Eigen::Vector<Isize, getNodeNum<ElemT>(P)> index_;
  Eigen::Vector<Real, getElemIntegralNum<ElemT>(P)> jacobian_det_;
};

template <int Dim, PolyOrder P, ElemType ElemT>
struct PerElemMesh : PerElemMeshBase<Dim, P, ElemT> {
  Eigen::Matrix<Real, calBasisFunNum<ElemT>(P), calBasisFunNum<ElemT>(P)> local_mass_mat_inv_;
  Eigen::Matrix<Real, Dim, getElemIntegralNum<ElemT>(P) * Dim> jacobian_trans_inv_;
  Eigen::Vector<Real, Dim> projection_measure_;
};

template <int Dim, PolyOrder P, ElemType ElemT>
struct ElemMesh {
  std::pair<Isize, Isize> range_;
  Isize num_;
  Eigen::Array<PerElemMesh<Dim, P, ElemT>, Eigen::Dynamic, 1> elem_;
  Eigen::Matrix<int, getNodeNum<ElemT>(PolyOrder::P1), getSubElemNum<ElemT>(P)> subelem_index_;
};

template <int Dim, PolyOrder P, ElemType ElemT, MeshType MeshT, bool IsInternal>
struct PerAdjacencyElemMesh;

template <int Dim, PolyOrder P, ElemType ElemT, MeshType MeshT, bool IsInternal>
  requires IsUniform<MeshT>
struct PerAdjacencyElemMesh<Dim, P, ElemT, MeshT, IsInternal> : PerElemMeshBase<Dim, P, ElemT> {
  Eigen::Vector<Isize, 2> parent_index_;
  Eigen::Vector<Isize, 1 + static_cast<Isize>(IsInternal)> adjacency_index_;
  Eigen::Vector<Real, Dim> norm_vec_;
};

template <int Dim, PolyOrder P, ElemType ElemT, MeshType MeshT, bool IsInternal>
  requires IsMixed<MeshT>
struct PerAdjacencyElemMesh<Dim, P, ElemT, MeshT, IsInternal> : PerElemMeshBase<Dim, P, ElemT> {
  Eigen::Vector<Isize, 2> parent_index_;
  Eigen::Vector<Isize, 1 + static_cast<Isize>(IsInternal)> adjacency_index_;
  Eigen::Vector<int, 1 + static_cast<Isize>(IsInternal)> typology_index_;
  Eigen::Vector<Real, Dim> norm_vec_;
};

template <int Dim, PolyOrder P, ElemType ElemT, MeshType MeshT, bool IsInternal>
struct AdjacencyElemTypeMesh {
  std::pair<Isize, Isize> range_;
  Isize num_;
  Eigen::Array<PerAdjacencyElemMesh<Dim, P, ElemT, MeshT, IsInternal>, Eigen::Dynamic, 1> elem_;
};

template <int Dim, PolyOrder P, ElemType ElemT, MeshType MeshT>
struct AdjacencyElemMesh {
  AdjacencyElemTypeMesh<Dim, P, ElemT, MeshT, true> internal_;
  AdjacencyElemTypeMesh<Dim, P, ElemT, MeshT, false> boundary_;
};

template <PolyOrder P>
using TriElemMesh = ElemMesh<2, P, ElemType::Tri>;

template <PolyOrder P>
using QuadElemMesh = ElemMesh<2, P, ElemType::Quad>;

template <PolyOrder P, MeshType MeshT>
using AdjacencyLineElemMesh = AdjacencyElemMesh<2, P, ElemType::Line, MeshT>;

template <int Dim, PolyOrder P>
struct MeshBase {
  Isize node_num_;
  Eigen::Matrix<Real, Dim, Eigen::Dynamic> node_;
  Eigen::Vector<int, Eigen::Dynamic> node_elem_num_;

  Isize elem_num_{0};

  inline MeshBase(const std::filesystem::path& mesh_file) {
    gmsh::clear();
    gmsh::open(mesh_file.string());
  }
  inline ~MeshBase() { gmsh::clear(); }
};

template <int Dim, PolyOrder P, MeshType MeshT>
struct Mesh;

template <PolyOrder P>
struct Mesh<2, P, MeshType::Tri> : MeshBase<2, P> {
  TriElemMesh<P> tri_;
  AdjacencyLineElemMesh<P, MeshType::Tri> line_;

  using MeshBase<2, P>::MeshBase;
};

template <PolyOrder P>
struct Mesh<2, P, MeshType::Quad> : MeshBase<2, P> {
  QuadElemMesh<P> quad_;
  AdjacencyLineElemMesh<P, MeshType::Quad> line_;

  using MeshBase<2, P>::MeshBase;
};

template <PolyOrder P>
struct Mesh<2, P, MeshType::TriQuad> : MeshBase<2, P> {
  TriElemMesh<P> tri_;
  QuadElemMesh<P> quad_;
  AdjacencyLineElemMesh<P, MeshType::TriQuad> line_;

  using MeshBase<2, P>::MeshBase;
};

template <ElemType ElemT>
struct MeshSupplemental {
  std::pair<Isize, Isize> range_;
  Isize num_;
  Eigen::Vector<int, Eigen::Dynamic> index_;
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_MESH_STRUCTURE_HPP_
