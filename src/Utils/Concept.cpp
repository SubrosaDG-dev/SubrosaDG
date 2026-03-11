/**
 * @file Concept.cpp
 * @brief The header file of SubrosaDG concept.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CONCEPT_CPP_
#define SUBROSA_DG_CONCEPT_CPP_

#include <Utils/Enum.cpp>

namespace SubrosaDG {

template <typename ElementTrait>
concept Is0dElement = ElementTrait::kElementType == ElementEnum::Point;

template <typename ElementTrait>
concept Is1dElement = ElementTrait::kElementType == ElementEnum::Line;

template <typename ElementTrait>
concept Is2dElement =
    ElementTrait::kElementType == ElementEnum::Triangle || ElementTrait::kElementType == ElementEnum::Quadrangle;

template <typename ElementTrait>
concept Is3dElement =
    ElementTrait::kElementType == ElementEnum::Tetrahedron || ElementTrait::kElementType == ElementEnum::Pyramid ||
    ElementTrait::kElementType == ElementEnum::Hexahedron;

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

template <EquationModelEnum EquationModelType>
concept IsEuler = EquationModelType == EquationModelEnum::CompressibleEuler ||
                  EquationModelType == EquationModelEnum::IncompressibleEuler;

template <EquationModelEnum EquationModelType>
concept IsNS =
    EquationModelType == EquationModelEnum::CompressibleNS || EquationModelType == EquationModelEnum::IncompressibleNS;

template <EquationModelEnum EquationModelType>
concept IsCompressible =
    EquationModelType == EquationModelEnum::CompressibleEuler || EquationModelType == EquationModelEnum::CompressibleNS;

template <EquationModelEnum EquationModelType>
concept IsIncompressible = EquationModelType == EquationModelEnum::IncompressibleEuler ||
                           EquationModelType == EquationModelEnum::IncompressibleNS;

bool isWall(const BoundaryConditionEnum boundary_condition_type) {
  return boundary_condition_type == BoundaryConditionEnum::IsoThermalNonSlipWall ||
         boundary_condition_type == BoundaryConditionEnum::AdiabaticSlipWall ||
         boundary_condition_type == BoundaryConditionEnum::AdiabaticNonSlipWall;
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CONCEPT_CPP_
