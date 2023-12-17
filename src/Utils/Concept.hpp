/**
 * @file Concept.hpp
 * @brief The header file of SubrosaDG concept.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CONCEPT_HPP_
#define SUBROSA_DG_CONCEPT_HPP_

#include <Utils/Enum.hpp>

namespace SubrosaDG {

template <MeshModel MeshModelType>
concept IsUniform = MeshModelType == MeshModel::Line || MeshModelType == MeshModel::Triangle ||
                    MeshModelType == MeshModel::Quadrangle || MeshModelType == MeshModel::Tetrahedron ||
                    MeshModelType == MeshModel::Hexahedron;

template <MeshModel MeshModelType>
concept IsMixed =
    MeshModelType == MeshModel::TriangleQuadrangle || MeshModelType == MeshModel::TetrahedronPyramidHexahedron;

template <MeshModel MeshModelType>
concept HasTriangle = MeshModelType == MeshModel::Triangle || MeshModelType == MeshModel::TriangleQuadrangle;

template <MeshModel MeshModelType>
concept HasQuadrangle = MeshModelType == MeshModel::Quadrangle || MeshModelType == MeshModel::TriangleQuadrangle;

template <MeshModel MeshModelType>
concept HasTetrahedron =
    MeshModelType == MeshModel::Tetrahedron || MeshModelType == MeshModel::TetrahedronPyramidHexahedron;

template <MeshModel MeshModelType>
concept HasPyramid = MeshModelType == MeshModel::TetrahedronPyramidHexahedron;

template <MeshModel MeshModelType>
concept HasHexahedron =
    MeshModelType == MeshModel::Hexahedron || MeshModelType == MeshModel::TetrahedronPyramidHexahedron;

template <Element ElementType>
concept Is0dElement = ElementType == Element::Point;

template <Element ElementType>
concept Is1dElement = ElementType == Element::Line;

template <Element ElementType>
concept Is2dElement = ElementType == Element::Triangle || ElementType == Element::Quadrangle;

template <Element ElementType>
concept Is3dElement =
    ElementType == Element::Tetrahedron || ElementType == Element::Pyramid || ElementType == Element::Hexahedron;

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CONCEPT_HPP_
