/**
 * @file SimulationControl.hpp
 * @brief The header file of SimulationControl.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_SIMULATION_CONTROL_HPP_
#define SUBROSA_DG_SIMULATION_CONTROL_HPP_

#include <array>
#include <magic_enum.hpp>
#include <numeric>

#include "Utils/BasicDataType.hpp"
#include "Utils/Concept.hpp"
#include "Utils/Enum.hpp"

namespace SubrosaDG {

inline constexpr std::array<int, 5> kLineGmshTypeNumber{1, 8, 26, 27, 28};
inline constexpr std::array<int, 5> kTriangleGmshTypeNumber{2, 9, 21, 23, 25};
inline constexpr std::array<int, 5> kQuadrangleGmshTypeNumber{3, 10, 36, 37, 38};

template <ElementEnum ElementType>
inline consteval int getElementDimension() {
  if constexpr (Is0dElement<ElementType>) {
    return 0;
  }
  if constexpr (Is1dElement<ElementType>) {
    return 1;
  }
  if constexpr (Is2dElement<ElementType>) {
    return 2;
  }
  if constexpr (Is3dElement<ElementType>) {
    return 3;
  }
}

template <ElementEnum ElementType, PolynomialOrderEnum P>
inline consteval int getElementGmshTypeNumber() {
  if constexpr (ElementType == ElementEnum::Point) {
    return 15;
  }
  if constexpr (ElementType == ElementEnum::Line) {
    return kLineGmshTypeNumber[static_cast<Usize>(P) - 1];
  }
  if constexpr (ElementType == ElementEnum::Triangle) {
    return kTriangleGmshTypeNumber[static_cast<Usize>(P) - 1];
  }
  if constexpr (ElementType == ElementEnum::Quadrangle) {
    return kQuadrangleGmshTypeNumber[static_cast<Usize>(P) - 1];
  }
}

template <ElementEnum ElementType>
inline consteval int getElementVtkTypeNumber() {
  if constexpr (ElementType == ElementEnum::Line) {
    return 68;
  }
  if constexpr (ElementType == ElementEnum::Triangle) {
    return 69;
  }
  if constexpr (ElementType == ElementEnum::Quadrangle) {
    return 70;
  }
}

template <ElementEnum ElementType, PolynomialOrderEnum P>
inline consteval int getElementNodeNumber() {
  if constexpr (ElementType == ElementEnum::Point) {
    return 1;
  }
  if constexpr (ElementType == ElementEnum::Line) {
    return magic_enum::enum_integer(P) + 1;
  }
  if constexpr (ElementType == ElementEnum::Triangle) {
    return (magic_enum::enum_integer(P) + 1) * (magic_enum::enum_integer(P) + 2) / 2;
  }
  if constexpr (ElementType == ElementEnum::Quadrangle) {
    return (magic_enum::enum_integer(P) + 1) * (magic_enum::enum_integer(P) + 1);
  }
}

template <int Dimension>
inline consteval int getElementTecplotBasicNodeNumber() {
  if constexpr (Dimension == 0) {
    return 1;
  }
  if constexpr (Dimension == 1) {
    return 2;
  }
  if constexpr (Dimension == 2) {
    return 4;
  }
  if constexpr (Dimension == 3) {
    return 8;
  }
}

template <ElementEnum ElementType>
inline consteval int getElementTecplotBasicNodeNumber() {
  return getElementTecplotBasicNodeNumber<getElementDimension<ElementType>()>();
}

template <ElementEnum ElementType>
inline consteval int getElementAdjacencyNumber() {
  if constexpr (ElementType == ElementEnum::Point) {
    return 0;
  }
  if constexpr (ElementType == ElementEnum::Line) {
    return 2;
  }
  if constexpr (ElementType == ElementEnum::Triangle) {
    return 3;
  }
  if constexpr (ElementType == ElementEnum::Quadrangle) {
    return 4;
  }
}

template <ElementEnum ElementType>
inline consteval std::array<ElementEnum, getElementAdjacencyNumber<ElementType>()> getElementPerAdjacencyType() {
  if constexpr (ElementType == ElementEnum::Line) {
    return {ElementEnum::Point, ElementEnum::Point};
  }
  if constexpr (ElementType == ElementEnum::Triangle) {
    return {ElementEnum::Line, ElementEnum::Line, ElementEnum::Line};
  }
  if constexpr (ElementType == ElementEnum::Quadrangle) {
    return {ElementEnum::Line, ElementEnum::Line, ElementEnum::Line, ElementEnum::Line};
  }
}

template <ElementEnum ElementType>
inline consteval std::array<int, getElementAdjacencyNumber<ElementType>()> getElementPerAdjacencyNodeNumber() {
  if constexpr (ElementType == ElementEnum::Line) {
    return {1, 1};
  }
  if constexpr (ElementType == ElementEnum::Triangle) {
    return {2, 2, 2};
  }
  if constexpr (ElementType == ElementEnum::Quadrangle) {
    return {2, 2, 2, 2};
  }
}

template <ElementEnum ElementType>
inline consteval int getElementAllAdjacencyNodeNumber() {
  constexpr std::array<int, getElementAdjacencyNumber<ElementType>()> kElementPerAdjacencyNodeNumber{
      getElementPerAdjacencyNodeNumber<ElementType>()};
  return std::accumulate(kElementPerAdjacencyNodeNumber.begin(), kElementPerAdjacencyNodeNumber.end(), 0);
}

template <ElementEnum ElementType>
inline consteval std::array<int, getElementAllAdjacencyNodeNumber<ElementType>()> getElementPerAdjacencyNodeIndex() {
  if constexpr (ElementType == ElementEnum::Line) {
    return {0, 1};
  }
  if constexpr (ElementType == ElementEnum::Triangle) {
    return {0, 1, 1, 2, 2, 0};
  }
  if constexpr (ElementType == ElementEnum::Quadrangle) {
    return {0, 1, 1, 2, 2, 3, 3, 0};
  }
}

template <ElementEnum ElementType, PolynomialOrderEnum P>
inline consteval int getElementSubNumber() {
  if constexpr (Is0dElement<ElementType>) {
    return 1;
  }
  if constexpr (Is1dElement<ElementType>) {
    return (magic_enum::enum_integer(P));
  }
  if constexpr (Is2dElement<ElementType>) {
    return (magic_enum::enum_integer(P) * magic_enum::enum_integer(P));
  }
  if constexpr (Is3dElement<ElementType>) {
    return (magic_enum::enum_integer(P) * magic_enum::enum_integer(P) * magic_enum::enum_integer(P));
  }
}

template <int Dimension, PolynomialOrderEnum P>
inline consteval int getElementSubNumber() {
  if constexpr (Dimension == 1) {
    return (magic_enum::enum_integer(P));
  }
  if constexpr (Dimension == 2) {
    return (magic_enum::enum_integer(P) * magic_enum::enum_integer(P));
  }
  if constexpr (Dimension == 3) {
    return (magic_enum::enum_integer(P) * magic_enum::enum_integer(P) * magic_enum::enum_integer(P));
  }
}

template <ElementEnum ElementType>
inline consteval Real getElementMeasure() {
  if constexpr (ElementType == ElementEnum::Line) {
    return 2.0;
  }
  if constexpr (ElementType == ElementEnum::Triangle) {
    return 0.5;
  }
  if constexpr (ElementType == ElementEnum::Quadrangle) {
    return 4.0;
  }
}

template <ElementEnum ElementType, PolynomialOrderEnum P>
inline consteval std::array<Real, getElementNodeNumber<ElementType, P>() * getElementDimension<ElementType>()>
getElementNodeCoordinate() {
  if constexpr (ElementType == ElementEnum::Line) {
    if constexpr (P == PolynomialOrderEnum::P1) {
      return {-1.0, 1.0};
    }
    if constexpr (P == PolynomialOrderEnum::P2) {
      return {-1.0, -1.0, 0.0};
    }
    if constexpr (P == PolynomialOrderEnum::P3) {
      return {-1.0, 1.0, -1.0 / 3.0, 1.0 / 3.0};
    }
    if constexpr (P == PolynomialOrderEnum::P4) {
      return {-1.0, 1.0, -0.5, 0.0, 0.5};
    }
    if constexpr (P == PolynomialOrderEnum::P5) {
      return {-1.0, 1.0, -0.6, -0.2, 0.2, 0.6};
    }
  }
  if constexpr (ElementType == ElementEnum::Triangle) {
    if constexpr (P == PolynomialOrderEnum::P1) {
      return {0.0, 0.0, 1.0, 0.0, 0.0, 1.0};
    }
    if constexpr (P == PolynomialOrderEnum::P2) {
      return {0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.5, 0.0, 0.5, 0.5, 0.0, 0.5};
    }
    if constexpr (P == PolynomialOrderEnum::P3) {
      return {0.0,       0.0,       1.0,       0.0,       0.0, 1.0,       1.0 / 3.0, 0.0,       2.0 / 3.0, 0.0,
              2.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0, 2.0 / 3.0, 0.0, 2.0 / 3.0, 0.0,       1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0};
    }
    if constexpr (P == PolynomialOrderEnum::P4) {
      return {
          0.0, 0.0,  1.0,  0.0, 0.0,  1.0, 0.25, 0.0, 0.5,  0.0,  0.75, 0.0, 0.75, 0.25, 0.5,
          0.5, 0.25, 0.75, 0.0, 0.75, 0.0, 0.5,  0.0, 0.25, 0.25, 0.25, 0.5, 0.25, 0.25, 0.5,
      };
    }
    if constexpr (P == PolynomialOrderEnum::P5) {
      return {0.0, 0.0, 1.,  0.0, 0.0, 1.,  0.2, 0.0, 0.4, 0.0, 0.6, 0.0, 0.8, 0.0, 0.8, 0.2, 0.6, 0.4, 0.4, 0.6, 0.2,
              0.8, 0.0, 0.8, 0.0, 0.6, 0.0, 0.4, 0.0, 0.2, 0.2, 0.2, 0.6, 0.2, 0.2, 0.6, 0.4, 0.2, 0.4, 0.4, 0.2, 0.4};
    }
  }
  if constexpr (ElementType == ElementEnum::Quadrangle) {
    if constexpr (P == PolynomialOrderEnum::P1) {
      return {-1.0, -1.0, 1.0, -1.0, 1.0, 1.0, -1.0, 1.0};
    }
    if constexpr (P == PolynomialOrderEnum::P2) {
      return {-1.0, -1.0, 1.0, -1.0, 1.0, 1.0, -1.0, 1.0, 0.0, -1.0, 1.0, 0.0, 0.0, 1.0, -1.0, 0.0, 0.0, 0.0};
    }
    if constexpr (P == PolynomialOrderEnum::P3) {
      return {-1.0,       -1.0,       1.0,        -1.0,       1.0,       1.0,        -1.0,       1.0,
              -1.0 / 3.0, -1.0,       1.0 / 3.0,  -1.0,       1.0,       -1.0 / 3.0, 1.0,        1.0 / 3.0,
              1.0 / 3.0,  1.0,        -1.0 / 3.0, 1.0,        -1.0,      1.0 / 3.0,  -1.0,       -1.0 / 3.0,
              -1.0 / 3.0, -1.0 / 3.0, 1.0 / 3.0,  -1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0,  -1.0 / 3.0, 1.0 / 3.0};
    }
    if constexpr (P == PolynomialOrderEnum::P4) {
      return {-1.0, -1.0, 1.0, -1.0, 1.0,  1.0, -1.0, 1.0,  -0.5, -1.0, 0.0, -1.0, 0.5,  -1.0, 1.0,  -0.5, 1.0,
              0.0,  1.0,  0.5, 0.5,  1.0,  0.0, 1.0,  -0.5, 1.0,  -1.0, 0.5, -1.0, 0.0,  -1.0, -0.5, -0.5, -0.5,
              0.5,  -0.5, 0.5, 0.5,  -0.5, 0.5, 0.0,  -0.5, 0.5,  0.0,  0.0, 0.5,  -0.5, 0.0,  0.0,  0.0};
    }
    if constexpr (P == PolynomialOrderEnum::P5) {
      return {-1.0, -1.0, 1.0,  -1.0, 1.0,  1.0,  -1.0, 1.0,  -0.6, -1.0, -0.2, -1.0, 0.2, -1.0, 0.6,
              -1.0, 1.0,  -0.6, 1.0,  -0.2, 1.0,  0.2,  1.0,  0.6,  0.6,  1.0,  0.2,  1.0, -0.2, 1.0,
              -0.6, 1.0,  -1.0, 0.6,  -1.0, 0.2,  -1.0, -0.2, -1.0, -0.6, -0.6, -0.6, 0.6, -0.6, 0.6,
              0.6,  -0.6, 0.6,  -0.2, -0.6, 0.2,  -0.6, 0.6,  -0.2, 0.6,  0.2,  0.2,  0.6, -0.2, 0.6,
              -0.6, 0.2,  -0.6, -0.2, -0.2, -0.2, 0.2,  -0.2, 0.2,  0.2,  -0.2, 0.2};
    }
  }
}

template <ElementEnum ElementType, PolynomialOrderEnum P>
inline consteval std::array<int,
                            getElementTecplotBasicNodeNumber<ElementType>() * getElementSubNumber<ElementType, P>()>
getSubElementConnectivity() {
  // clang-format off
  if constexpr (ElementType == ElementEnum::Line) {
    if constexpr (P == PolynomialOrderEnum::P1) {
      return {0, 1};
    }
    if constexpr (P == PolynomialOrderEnum::P2) {
      return {0, 2,
              2, 1};
    }
    if constexpr (P == PolynomialOrderEnum::P3) {
      return {0, 2,
              2, 3,
              3, 1};
    }
    if constexpr (P == PolynomialOrderEnum::P4) {
      return {0, 2,
              2, 3,
              3, 4,
              4, 1};
    }
    if constexpr (P == PolynomialOrderEnum::P5) {
      return {0, 2,
              2, 3,
              3, 4,
              4, 5,
              5, 1};
    }
  }
  if constexpr (ElementType == ElementEnum::Triangle) {
    if constexpr (P == PolynomialOrderEnum::P1) {
      return {0, 1, 2, 2};
    }
    if constexpr (P == PolynomialOrderEnum::P2) {
      return {0, 3, 5, 5,
              3, 4, 5, 5,
              3, 1, 4, 4,
              5, 4, 2, 2,};
    }
    if constexpr (P == PolynomialOrderEnum::P3) {
      return {0, 3, 8, 8,
              3, 9, 8, 8,
              3, 4, 9, 9,
              4, 5, 9, 9,
              4, 1, 5, 5,
              8, 9, 7, 7,
              9, 6, 7, 7,
              9, 5, 6, 6,
              7, 6, 2, 2};
    }
    if constexpr (P == PolynomialOrderEnum::P4) {
      return {0,  3,  11, 11,
              3,  12, 11, 11,
              3,  4,  12, 12,
              4,  13, 12, 12,
              4,  5,  13, 13,
              5,  6,  13, 13,
              5,  1,  6,  6,
              11, 12, 10, 10,
              12, 14, 10, 10,
              12, 13, 14, 14,
              13, 7,  14, 14,
              13, 6,  7,  7,
              10, 14, 9,  9,
              14, 8,  9,  9,
              14, 7,  8,  8,
              9,  8,  2,  2};
    }
    if constexpr (P == PolynomialOrderEnum::P5) {
      return {0,  3,  14, 14,
              3,  15, 14, 14,
              3,  4,  15, 15,
              4,  18, 15, 15,
              4,  5,  18, 18,
              5,  16, 18, 18,
              5,  6,  16, 16,
              6,  17, 16, 16,
              6,  1,  7,  7,
              14, 15, 13, 13,
              15, 20, 13, 13,
              15, 18, 20, 20,
              18, 19, 20, 20,
              18, 16, 19, 19,
              16, 8,  19, 19,
              16, 7,  8,  8,
              13, 20, 12, 12,
              20, 17, 12, 12,
              20, 19, 17, 17,
              19, 9,  17, 17,
              19, 8,  9,  9,
              12, 17, 11, 11,
              17, 10, 11, 11,
              17, 9,  10, 10,
              11, 10, 2,  2};
    }
  }
  if constexpr (ElementType == ElementEnum::Quadrangle) {
    if constexpr (P == PolynomialOrderEnum::P1) {
      return {0, 1, 2, 3};
    }
    if constexpr (P == PolynomialOrderEnum::P2) {
      return {0, 4, 8, 7,
              4, 1, 5, 8,
              7, 8, 6, 3,
              8, 5, 2, 6};
    }
    if constexpr (P == PolynomialOrderEnum::P3) {
      return {0,  4,  12, 11,
              4,  5,  13, 12,
              5,  1,  6,  13,
              11, 12, 15, 10,
              12, 13, 14, 15,
              13, 6,  7,  14,
              10, 15, 9,  3,
              15, 14, 8,  9,
              14, 7,  2,  8};
    }
    if constexpr (P == PolynomialOrderEnum::P4) {
      return {0,  4,  16, 15,
              4,  5,  20, 16,
              5,  6,  17, 20,
              6,  1,  7,  17,
              15, 16, 23, 14,
              16, 20, 24, 23,
              20, 17, 21, 24,
              17, 7,  8,  21,
              14, 23, 19, 13,
              23, 24, 22, 19,
              24, 21, 18, 22,
              21, 8,  9,  18,
              13, 19, 12, 3,
              19, 22, 11, 12,
              22, 18, 10, 11,
              18, 9,  2,  10};
    }
    if constexpr (P == PolynomialOrderEnum::P5) {
      return {0,  4,  20, 19,
              4,  5,  24, 20,
              5,  6,  25, 24,
              6,  7,  21, 25,
              7,  1,  8,  21,
              19, 20, 31, 18,
              20, 24, 32, 31,
              24, 25, 33, 32,
              25, 21, 26, 33,
              21, 8,  9,  26,
              18, 31, 30, 17,
              31, 32, 35, 30,
              32, 33, 34, 35,
              33, 26, 27, 34,
              26, 9,  10, 27,
              17, 30, 23, 16,
              30, 35, 29, 23,
              35, 34, 28, 29,
              34, 27, 22, 28,
              27, 10, 11, 22,
              16, 23, 15, 3,
              23, 29, 14, 15,
              29, 28, 13, 14,
              28, 22, 12, 13,
              22, 11, 2,  12};
    }
  }
  // clang-format on
}

template <ElementEnum ElementType, PolynomialOrderEnum P>
inline consteval std::array<int, getElementNodeNumber<ElementType, P>()> getElementVTKConnectivity() {
  if constexpr (ElementType == ElementEnum::Line) {
    if constexpr (P == PolynomialOrderEnum::P1) {
      return {0, 1};
    }
    if constexpr (P == PolynomialOrderEnum::P2) {
      return {0, 1, 2};
    }
    if constexpr (P == PolynomialOrderEnum::P3) {
      return {0, 1, 2, 3};
    }
    if constexpr (P == PolynomialOrderEnum::P4) {
      return {0, 1, 2, 3, 4};
    }
    if constexpr (P == PolynomialOrderEnum::P5) {
      return {0, 1, 2, 3, 4, 5};
    }
  }
  if constexpr (ElementType == ElementEnum::Triangle) {
    if constexpr (P == PolynomialOrderEnum::P1) {
      return {0, 1, 2};
    }
    if constexpr (P == PolynomialOrderEnum::P2) {
      return {0, 1, 2, 3, 4, 5};
    }
    if constexpr (P == PolynomialOrderEnum::P3) {
      return {0, 1, 2, 3, 4, 5, 6, 7, 8};
    }
    if constexpr (P == PolynomialOrderEnum::P4) {
      return {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
    }
    if constexpr (P == PolynomialOrderEnum::P5) {
      return {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
    }
  }
  if constexpr (ElementType == ElementEnum::Quadrangle) {
    if constexpr (P == PolynomialOrderEnum::P1) {
      return {0, 1, 2, 3};
    }
    if constexpr (P == PolynomialOrderEnum::P2) {
      return {0, 1, 2, 3, 4, 5, 6, 7, 8};
    }
    if constexpr (P == PolynomialOrderEnum::P3) {
      return {0, 1, 2, 3, 4, 5, 6, 7, 9, 8, 11, 10, 12, 13, 15, 14};
    }
    if constexpr (P == PolynomialOrderEnum::P4) {
      return {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 11, 10, 15, 14, 13, 16, 20, 17, 23, 24, 21, 19, 22, 18};
    }
    if constexpr (P == PolynomialOrderEnum::P5) {
      return {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 15, 14, 13, 12, 19, 18,
              17, 16, 20, 24, 25, 21, 31, 32, 33, 26, 30, 35, 34, 27, 23, 29, 28, 22};
    }
  }
}

template <ElementEnum ElementType, PolynomialOrderEnum P>
inline consteval int getElementBasisFunctionNumber() {
  if constexpr (ElementType == ElementEnum::Point) {
    return 1;
  }
  if constexpr (ElementType == ElementEnum::Line) {
    return magic_enum::enum_integer(P) + 1;
  }
  if constexpr (ElementType == ElementEnum::Triangle) {
    return (magic_enum::enum_integer(P) + 1) * (magic_enum::enum_integer(P) + 2) / 2;
  }
  if constexpr (ElementType == ElementEnum::Quadrangle) {
    return (magic_enum::enum_integer(P) + 1) * (magic_enum::enum_integer(P) + 1);
  }
}

inline constexpr std::array<int, 12> kLineQuadratureNumber{1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6};
inline constexpr std::array<int, 12> kTriangleQuadratureNumber{1, 1, 3, 4, 6, 7, 12, 13, 16, 19, 25, 27};
inline constexpr std::array<int, 12> kQuadrangleQuadratureNumber{1, 3, 7, 4, 9, 9, 16, 16, 25, 25, 36, 36};

template <PolynomialOrderEnum P>
inline consteval int getElementQuadratureOrder() {
  return 2 * magic_enum::enum_integer(P);
}

template <PolynomialOrderEnum P>
inline consteval int getAdjacencyElementQuadratureOrder() {
  return 2 * magic_enum::enum_integer(P) + 1;
}

template <ElementEnum ElementType, PolynomialOrderEnum P>
inline consteval int getElementQuadratureNumber() {
  if constexpr (ElementType == ElementEnum::Line) {
    return kLineQuadratureNumber[static_cast<Usize>(getElementQuadratureOrder<P>())];
  }
  if constexpr (ElementType == ElementEnum::Triangle) {
    return kTriangleQuadratureNumber[static_cast<Usize>(getElementQuadratureOrder<P>())];
  }
  if constexpr (ElementType == ElementEnum::Quadrangle) {
    return kQuadrangleQuadratureNumber[static_cast<Usize>(getElementQuadratureOrder<P>())];
  }
}

template <ElementEnum ElementType, PolynomialOrderEnum P>
inline consteval int getAdjacencyElementQuadratureNumber() {
  if constexpr (ElementType == ElementEnum::Point) {
    return 1;
  }
  if constexpr (ElementType == ElementEnum::Line) {
    return kLineQuadratureNumber[static_cast<Usize>(getAdjacencyElementQuadratureOrder<P>())];
  }
}

template <ElementEnum ElementType, PolynomialOrderEnum P>
inline consteval std::array<int, getElementAdjacencyNumber<ElementType>()> getElementPerAdjacencyQuadratureNumber() {
  if constexpr (ElementType == ElementEnum::Line) {
    return {1, 1};
  }
  if constexpr (ElementType == ElementEnum::Triangle) {
    constexpr int kLineQuadrature{kLineQuadratureNumber[static_cast<Usize>(getAdjacencyElementQuadratureOrder<P>())]};
    return {kLineQuadrature, kLineQuadrature, kLineQuadrature};
  }
  if constexpr (ElementType == ElementEnum::Quadrangle) {
    constexpr int kLineQuadrature{kLineQuadratureNumber[static_cast<Usize>(getAdjacencyElementQuadratureOrder<P>())]};
    return {kLineQuadrature, kLineQuadrature, kLineQuadrature, kLineQuadrature};
  }
}

template <ElementEnum ElementType, PolynomialOrderEnum P>
inline consteval int getElementAllAdjacencyQuadratureNumber() {
  constexpr std::array<int, getElementAdjacencyNumber<ElementType>()> kElementPerAdjacencyQuadratureNumber{
      getElementPerAdjacencyQuadratureNumber<ElementType, P>()};
  return std::accumulate(kElementPerAdjacencyQuadratureNumber.begin(), kElementPerAdjacencyQuadratureNumber.end(), 0);
}

template <ElementEnum ElementType, PolynomialOrderEnum P>
inline consteval std::array<int, getElementAdjacencyNumber<ElementType>() + 1>
getElementAccumulateAdjacencyQuadratureNumber() {
  std::array<int, getElementAdjacencyNumber<ElementType>() + 1> accumulate_adjacency_quadrature_number{};
  accumulate_adjacency_quadrature_number[0] = 0;
  for (int i = 0; i < getElementAdjacencyNumber<ElementType>(); ++i) {
    accumulate_adjacency_quadrature_number[static_cast<Usize>(i) + 1] =
        accumulate_adjacency_quadrature_number[static_cast<Usize>(i)] +
        getElementPerAdjacencyQuadratureNumber<ElementType, P>()[static_cast<Usize>(i)];
  }
  return accumulate_adjacency_quadrature_number;
}

template <int Dimension, EquationModelEnum EquationModelType>
inline consteval int getConservedVariableNumber() {
  if constexpr (EquationModelType == EquationModelEnum::Euler) {
    return Dimension + 2;
  }
  if constexpr (EquationModelType == EquationModelEnum::NavierStokes) {
    return Dimension + 2;
  }
}

template <int Dimension, EquationModelEnum EquationModelType, TurbulenceModelEnum TurbulenceModelType>
  requires(EquationModelType == EquationModelEnum::RANS)
inline consteval int getConservedVariableNumber() {
  if constexpr (TurbulenceModelType == TurbulenceModelEnum::SA) {
    return Dimension + 3;
  }
}

template <int Dimension, EquationModelEnum EquationModelType>
inline consteval int getComputationalVariableNumber() {
  if constexpr (EquationModelType == EquationModelEnum::Euler) {
    return Dimension + 3;
  }
  if constexpr (EquationModelType == EquationModelEnum::NavierStokes) {
    return Dimension + 3;
  }
}

template <int Dimension, EquationModelEnum EquationModelType, TurbulenceModelEnum TurbulenceModelType>
  requires(EquationModelType == EquationModelEnum::RANS)
inline consteval int getComputationalVariableNumber() {
  if constexpr (TurbulenceModelType == TurbulenceModelEnum::SA) {
    return Dimension + 4;
  }
}

template <int Dimension, EquationModelEnum EquationModelType>
inline consteval int getPrimitiveVariableNumber() {
  if constexpr (EquationModelType == EquationModelEnum::Euler) {
    return Dimension + 2;
  }
  if constexpr (EquationModelType == EquationModelEnum::NavierStokes) {
    return Dimension + 2;
  }
}

template <int Dimension, EquationModelEnum EquationModelType, TurbulenceModelEnum TurbulenceModelType>
  requires(EquationModelType == EquationModelEnum::RANS)
inline consteval int getPrimitiveVariableNumber() {
  if constexpr (TurbulenceModelType == TurbulenceModelEnum::SA) {
    return Dimension + 3;
  }
}

template <ElementEnum ElementType, PolynomialOrderEnum P>
struct ElementTraitBase {
  inline static constexpr int kDimension{getElementDimension<ElementType>()};
  inline static constexpr ElementEnum kElementType{ElementType};
  inline static constexpr PolynomialOrderEnum kPolynomialOrder{P};
  inline static constexpr int kGmshTypeNumber{getElementGmshTypeNumber<ElementType, P>()};
  inline static constexpr int kVtkTypeNumber{getElementVtkTypeNumber<ElementType>()};
  inline static constexpr int kBasicNodeNumber{getElementNodeNumber<ElementType, PolynomialOrderEnum::P1>()};
  inline static constexpr int kAllNodeNumber{getElementNodeNumber<ElementType, P>()};
  inline static constexpr int kTecplotBasicNodeNumber{getElementTecplotBasicNodeNumber<ElementType>()};
  inline static constexpr int kAdjacencyNumber{getElementAdjacencyNumber<ElementType>()};
  inline static constexpr int kSubNumber{getElementSubNumber<ElementType, P>()};
};

template <ElementEnum ElementType, PolynomialOrderEnum P>
struct AdjacencyElementTrait : ElementTraitBase<ElementType, P> {
  inline static constexpr int kBasisFunctionNumber{getElementBasisFunctionNumber<ElementType, P>()};
  inline static constexpr int kQuadratureOrder{getAdjacencyElementQuadratureOrder<P>()};
  inline static constexpr int kQuadratureNumber{getAdjacencyElementQuadratureNumber<ElementType, P>()};
};

template <ElementEnum ElementType, PolynomialOrderEnum P>
struct ElementTrait : ElementTraitBase<ElementType, P> {
  inline static constexpr int kAllAdjacencyNodeNumber{getElementAllAdjacencyNodeNumber<ElementType>()};
  inline static constexpr int kBasisFunctionNumber{getElementBasisFunctionNumber<ElementType, P>()};
  inline static constexpr int kQuadratureOrder{getElementQuadratureOrder<P>()};
  inline static constexpr int kQuadratureNumber{getElementQuadratureNumber<ElementType, P>()};
  inline static constexpr int kAllAdjacencyQuadratureNumber{getElementAllAdjacencyQuadratureNumber<ElementType, P>()};
};

template <PolynomialOrderEnum P>
using AdjacencyPointTrait = AdjacencyElementTrait<ElementEnum::Point, P>;

template <PolynomialOrderEnum P>
using LineTrait = ElementTrait<ElementEnum::Line, P>;

template <PolynomialOrderEnum P>
using AdjacencyLineTrait = AdjacencyElementTrait<ElementEnum::Line, P>;

template <PolynomialOrderEnum P>
using TriangleTrait = ElementTrait<ElementEnum::Triangle, P>;

template <PolynomialOrderEnum P>
using QuadrangleTrait = ElementTrait<ElementEnum::Quadrangle, P>;

template <int Dimension, PolynomialOrderEnum P, EquationModelEnum EquationModelType,
          ThermodynamicModelEnum ThermodynamicModelType, EquationOfStateEnum EquationOfStateType,
          TimeIntegrationEnum TimeIntegrationType>
struct SolveControl {
  inline static constexpr int kDimension{Dimension};
  inline static constexpr PolynomialOrderEnum kPolynomialOrder{P};
  inline static constexpr EquationModelEnum kEquationModel{EquationModelType};
  inline static constexpr ThermodynamicModelEnum kThermodynamicModel{ThermodynamicModelType};
  inline static constexpr EquationOfStateEnum kEquationOfState{EquationOfStateType};
  inline static constexpr TimeIntegrationEnum kTimeIntegration{TimeIntegrationType};
};

template <EquationModelEnum EquationModelType>
struct EquationVariable {};

template <int Dimension, ConvectiveFluxEnum ConvectiveFluxType>
struct EulerVariable : EquationVariable<EquationModelEnum::Euler> {
  inline static constexpr ConvectiveFluxEnum kConvectiveFlux{ConvectiveFluxType};
  inline static constexpr int kConservedVariableNumber{
      getConservedVariableNumber<Dimension, EquationModelEnum::Euler>()};
  inline static constexpr int kComputationalVariableNumber{
      getComputationalVariableNumber<Dimension, EquationModelEnum::Euler>()};
  inline static constexpr int kPrimitiveVariableNumber{
      getPrimitiveVariableNumber<Dimension, EquationModelEnum::Euler>()};
};

template <int Dimension, TransportModelEnum TransportModelType, ConvectiveFluxEnum ConvectiveFluxType,
          ViscousFluxEnum ViscousFluxType>
struct NavierStokesVariable : EquationVariable<EquationModelEnum::NavierStokes> {
  inline static constexpr TransportModelEnum kTransportModel{TransportModelType};
  inline static constexpr ConvectiveFluxEnum kConvectiveFlux{ConvectiveFluxType};
  inline static constexpr ViscousFluxEnum kViscousFlux{ViscousFluxType};
  inline static constexpr int kConservedVariableNumber{
      getConservedVariableNumber<Dimension, EquationModelEnum::NavierStokes>()};
  inline static constexpr int kComputationalVariableNumber{
      getComputationalVariableNumber<Dimension, EquationModelEnum::NavierStokes>()};
  inline static constexpr int kPrimitiveVariableNumber{
      getPrimitiveVariableNumber<Dimension, EquationModelEnum::NavierStokes>()};
};

template <int Dimension, TransportModelEnum TransportModelType, ConvectiveFluxEnum ConvectiveFluxType,
          ViscousFluxEnum ViscousFluxType, TurbulenceModelEnum TurbulenceModelType>
struct RANSVariable : EquationVariable<EquationModelEnum::RANS> {
  inline static constexpr TransportModelEnum kTransportModel{TransportModelType};
  inline static constexpr ConvectiveFluxEnum kConvectiveFlux{ConvectiveFluxType};
  inline static constexpr ViscousFluxEnum kViscousFlux{ViscousFluxType};
  inline static constexpr TurbulenceModelEnum kTurbulenceModel{TurbulenceModelType};
  inline static constexpr int kConservedVariableNumber{
      getConservedVariableNumber<Dimension, EquationModelEnum::RANS, TurbulenceModelType>()};
  inline static constexpr int kComputationalVariableNumber{
      getComputationalVariableNumber<Dimension, EquationModelEnum::RANS, TurbulenceModelType>()};
  inline static constexpr int kPrimitiveVariableNumber{
      getPrimitiveVariableNumber<Dimension, EquationModelEnum::RANS, TurbulenceModelType>()};
};

template <int Dimension, PolynomialOrderEnum P, MeshModelEnum MeshModelType, EquationModelEnum EquationModelType,
          ThermodynamicModelEnum ThermodynamicModelType, EquationOfStateEnum EquationOfStateType,
          TimeIntegrationEnum TimeIntegrationType, ViewModelEnum ViewModelType>
struct SimulationControl
    : SolveControl<Dimension, P, EquationModelType, ThermodynamicModelType, EquationOfStateType, TimeIntegrationType> {
  inline static constexpr MeshModelEnum kMeshModel{MeshModelType};
  inline static constexpr ViewModelEnum kViewModel{ViewModelType};
};

template <int Dimension, PolynomialOrderEnum P, MeshModelEnum MeshModelType,
          ThermodynamicModelEnum ThermodynamicModelType, EquationOfStateEnum EquationOfStateType,
          ConvectiveFluxEnum ConvectiveFluxType, TimeIntegrationEnum TimeIntegrationType, ViewModelEnum ViewModelType>
struct SimulationControlEuler
    : SimulationControl<Dimension, P, MeshModelType, EquationModelEnum::Euler, ThermodynamicModelType,
                        EquationOfStateType, TimeIntegrationType, ViewModelType>,
      EulerVariable<Dimension, ConvectiveFluxType> {};

template <int Dimension, PolynomialOrderEnum P, MeshModelEnum MeshModelType,
          ThermodynamicModelEnum ThermodynamicModelType, EquationOfStateEnum EquationOfStateType,
          TransportModelEnum TransportModelType, ConvectiveFluxEnum ConvectiveFluxType, ViscousFluxEnum ViscousFluxType,
          TimeIntegrationEnum TimeIntegrationType, ViewModelEnum ViewModelType>
struct SimulationControlNavierStokes
    : SimulationControl<Dimension, P, MeshModelType, EquationModelEnum::NavierStokes, ThermodynamicModelType,
                        EquationOfStateType, TimeIntegrationType, ViewModelType>,
      NavierStokesVariable<Dimension, TransportModelType, ConvectiveFluxType, ViscousFluxType> {};

template <int Dimension, PolynomialOrderEnum P, MeshModelEnum MeshModelType,
          ThermodynamicModelEnum ThermodynamicModelType, EquationOfStateEnum EquationOfStateType,
          TransportModelEnum TransportModelType, ConvectiveFluxEnum ConvectiveFluxType, ViscousFluxEnum ViscousFluxType,
          TurbulenceModelEnum TurbulenceModelType, TimeIntegrationEnum TimeIntegrationType, ViewModelEnum ViewModelType>
struct SimulationControlRANS
    : SimulationControl<Dimension, P, MeshModelType, EquationModelEnum::NavierStokes, ThermodynamicModelType,
                        EquationOfStateType, TimeIntegrationType, ViewModelType>,
      RANSVariable<Dimension, TransportModelType, ConvectiveFluxType, ViscousFluxType, TurbulenceModelType> {};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_SIMULATION_CONTROL_HPP_
