/**
 * @file Concept.hpp
 * @brief The header file of SubrosaDG concept.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CONCEPT_HPP_
#define SUBROSA_DG_CONCEPT_HPP_

#include <Utils/Enum.hpp>

namespace SubrosaDG {

template <MeshModelEnum MeshModelType>
concept IsUniform = MeshModelType == MeshModelEnum::Line || MeshModelType == MeshModelEnum::Triangle ||
                    MeshModelType == MeshModelEnum::Quadrangle || MeshModelType == MeshModelEnum::Tetrahedron ||
                    MeshModelType == MeshModelEnum::Hexahedron;

template <MeshModelEnum MeshModelType>
concept IsMixed =
    MeshModelType == MeshModelEnum::TriangleQuadrangle || MeshModelType == MeshModelEnum::TetrahedronPyramidHexahedron;

template <MeshModelEnum MeshModelType>
concept HasTriangle = MeshModelType == MeshModelEnum::Triangle || MeshModelType == MeshModelEnum::TriangleQuadrangle;

template <MeshModelEnum MeshModelType>
concept HasQuadrangle =
    MeshModelType == MeshModelEnum::Quadrangle || MeshModelType == MeshModelEnum::TriangleQuadrangle;

template <MeshModelEnum MeshModelType>
concept HasTetrahedron =
    MeshModelType == MeshModelEnum::Tetrahedron || MeshModelType == MeshModelEnum::TetrahedronPyramidHexahedron;

template <MeshModelEnum MeshModelType>
concept HasPyramid = MeshModelType == MeshModelEnum::TetrahedronPyramidHexahedron;

template <MeshModelEnum MeshModelType>
concept HasHexahedron =
    MeshModelType == MeshModelEnum::Hexahedron || MeshModelType == MeshModelEnum::TetrahedronPyramidHexahedron;

template <MeshModelEnum MeshModelType>
concept HasAdjacencyTriangle =
    MeshModelType == MeshModelEnum::Tetrahedron || MeshModelType == MeshModelEnum::TetrahedronPyramidHexahedron;

template <MeshModelEnum MeshModelType>
concept HasAdjacencyQuadrangle =
    MeshModelType == MeshModelEnum::Hexahedron || MeshModelType == MeshModelEnum::TetrahedronPyramidHexahedron;

template <ElementEnum ElementType>
concept Is0dElement = ElementType == ElementEnum::Point;

template <ElementEnum ElementType>
concept Is1dElement = ElementType == ElementEnum::Line;

template <ElementEnum ElementType>
concept Is2dElement = ElementType == ElementEnum::Triangle || ElementType == ElementEnum::Quadrangle;

template <ElementEnum ElementType>
concept Is3dElement = ElementType == ElementEnum::Tetrahedron || ElementType == ElementEnum::Pyramid ||
                      ElementType == ElementEnum::Hexahedron;

bool isWall(const BoundaryConditionEnum boundary_condition_type) {
  return boundary_condition_type == BoundaryConditionEnum::IsothermalNoslipWall ||
         boundary_condition_type == BoundaryConditionEnum::AdiabaticSlipWall ||
         boundary_condition_type == BoundaryConditionEnum::AdiabaticNoSlipWall;
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CONCEPT_HPP_
