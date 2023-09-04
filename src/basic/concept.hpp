/**
 * @file concept.hpp
 * @brief The concepts head file to define some concepts.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-16
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CONCEPT_HPP_
#define SUBROSA_DG_CONCEPT_HPP_

#include <concepts>

#include "basic/enum.hpp"
#include "config/spatial_discrete.hpp"

namespace SubrosaDG {

template <TimeDiscrete TimeDiscreteT>
concept IsExplicit = TimeDiscreteT ==
TimeDiscrete::ForwardEuler || TimeDiscreteT == TimeDiscrete::RK3SSP;

template <TimeDiscrete TimeDiscreteT>
concept IsImplicit = TimeDiscreteT ==
TimeDiscrete::BackwardEuler;

template <typename T, EquModel EquModelT>
concept DerivedFromSpatialDiscrete = std::derived_from<T, SpatialDiscrete<EquModelT>>;

template <MeshType MeshT>
concept IsUniform = MeshT ==
MeshType::Tri || MeshT == MeshType::Quad || MeshT == MeshType::Tet || MeshT == MeshType::Hex;

template <MeshType MeshT>
concept IsMixed = MeshT ==
MeshType::TriQuad || MeshT == MeshType::TetPyrHex;

template <MeshType MeshT>
concept HasTri = MeshT ==
MeshType::Tri || MeshT == MeshType::TriQuad;

template <MeshType MeshT>
concept HasQuad = MeshT ==
MeshType::Quad || MeshT == MeshType::TriQuad;

template <ElemType ElemT>
concept Is1dElem = ElemT ==
ElemType::Line;

template <ElemType ElemT>
concept Is2dElem = ElemT ==
ElemType::Tri || ElemT == ElemType::Quad;

template <ElemType ElemT>
concept Is3dElem = ElemT ==
ElemType::Tet || ElemT == ElemType::Pyr || ElemT == ElemType::Hex;

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CONCEPT_HPP_
