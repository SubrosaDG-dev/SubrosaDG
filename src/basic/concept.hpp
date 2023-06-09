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

#include <basic/enum.hpp>

#include "mesh/elem_type.hpp"

namespace SubrosaDG {

template <TimeDiscrete TimeDiscreteT>
concept IsExplicit = TimeDiscreteT == TimeDiscrete::ExplicitEuler || TimeDiscreteT == TimeDiscrete::RungeKutta3;

template <TimeDiscrete TimeDiscreteT>
concept IsImplicit = TimeDiscreteT == TimeDiscrete::ImplicitEuler;

template <MeshType MeshT>
concept IsUniform =
    MeshT == MeshType::Tri || MeshT == MeshType::Quad || MeshT == MeshType::Tet || MeshT == MeshType::Hex;

template <MeshType MeshT>
concept IsMixed = MeshT == MeshType::TriQuad || MeshT == MeshType::TetPyrHex;

template <ElemInfo ElemT>
concept Is1dElem = ElemT.kDim == 1;

template <ElemInfo ElemT>
concept Is2dElem = ElemT.kDim == 2;

template <ElemInfo ElemT>
concept Is3dElem = ElemT.kDim == 3;

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CONCEPT_HPP_
