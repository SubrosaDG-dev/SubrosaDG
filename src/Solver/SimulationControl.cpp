/**
 * @file SimulationControl.cpp
 * @brief The header file of SimulationControl.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_SIMULATION_CONTROL_CPP_
#define SUBROSA_DG_SIMULATION_CONTROL_CPP_

#include <array>
#include <magic_enum/magic_enum.hpp>
#include <numeric>

#include "Utils/BasicDataType.cpp"
#include "Utils/Enum.cpp"

namespace SubrosaDG {

inline constexpr std::array<int, 5> kLineGmshTypeNumber{1, 8, 26, 27, 28};
inline constexpr std::array<int, 5> kTriangleGmshTypeNumber{2, 9, 21, 23, 25};
inline constexpr std::array<int, 5> kQuadrangleGmshTypeNumber{3, 10, 36, 37, 38};
inline constexpr std::array<int, 5> kTetrahedronGmshTypeNumber{4, 11, 29, 30, 31};
inline constexpr std::array<int, 5> kPyramidGmshTypeNumber{7, 14, 118, 119, 120};
inline constexpr std::array<int, 5> kHexahedronGmshTypeNumber{5, 12, 92, 93, 94};

template <ElementEnum ElementType>
consteval int getElementDimension() {
  if constexpr (ElementType == ElementEnum::Point) {
    return 0;
  }
  if constexpr (ElementType == ElementEnum::Line) {
    return 1;
  }
  if constexpr (ElementType == ElementEnum::Triangle || ElementType == ElementEnum::Quadrangle) {
    return 2;
  }
  if constexpr (ElementType == ElementEnum::Tetrahedron || ElementType == ElementEnum::Pyramid ||
                ElementType == ElementEnum::Hexahedron) {
    return 3;
  }
}

template <ElementEnum ElementType, int PolynomialOrder>
consteval int getElementGmshTypeNumber() {
  if constexpr (ElementType == ElementEnum::Point) {
    return 15;
  }
  if constexpr (ElementType == ElementEnum::Line) {
    return kLineGmshTypeNumber[PolynomialOrder - 1];
  }
  if constexpr (ElementType == ElementEnum::Triangle) {
    return kTriangleGmshTypeNumber[PolynomialOrder - 1];
  }
  if constexpr (ElementType == ElementEnum::Quadrangle) {
    return kQuadrangleGmshTypeNumber[PolynomialOrder - 1];
  }
  if constexpr (ElementType == ElementEnum::Tetrahedron) {
    return kTetrahedronGmshTypeNumber[PolynomialOrder - 1];
  }
  if constexpr (ElementType == ElementEnum::Pyramid) {
    return kPyramidGmshTypeNumber[PolynomialOrder - 1];
  }
  if constexpr (ElementType == ElementEnum::Hexahedron) {
    return kHexahedronGmshTypeNumber[PolynomialOrder - 1];
  }
}

template <ElementEnum ElementType, int PolynomialOrder>
consteval int getElementNodeNumber() {
  if constexpr (ElementType == ElementEnum::Point) {
    return 1;
  }
  if constexpr (ElementType == ElementEnum::Line) {
    return PolynomialOrder + 1;
  }
  if constexpr (ElementType == ElementEnum::Triangle) {
    return (PolynomialOrder + 1) * (PolynomialOrder + 2) / 2;
  }
  if constexpr (ElementType == ElementEnum::Quadrangle) {
    return (PolynomialOrder + 1) * (PolynomialOrder + 1);
  }
  if constexpr (ElementType == ElementEnum::Tetrahedron) {
    return (PolynomialOrder + 1) * (PolynomialOrder + 2) * (PolynomialOrder + 3) / 6;
  }
  if constexpr (ElementType == ElementEnum::Pyramid) {
    return (PolynomialOrder + 1) * (PolynomialOrder + 2) * (2 * PolynomialOrder + 3) / 6;
  }
  if constexpr (ElementType == ElementEnum::Hexahedron) {
    return (PolynomialOrder + 1) * (PolynomialOrder + 1) * (PolynomialOrder + 1);
  }
}

template <ElementEnum ElementType>
consteval int getVolumeElementAdjacencyNumber() {
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
  if constexpr (ElementType == ElementEnum::Tetrahedron) {
    return 4;
  }
  if constexpr (ElementType == ElementEnum::Pyramid) {
    return 5;
  }
  if constexpr (ElementType == ElementEnum::Hexahedron) {
    return 6;
  }
}

template <ElementEnum ElementType>
consteval std::array<ElementEnum, getVolumeElementAdjacencyNumber<ElementType>()> getVolumeElementPerAdjacencyType() {
  if constexpr (ElementType == ElementEnum::Line) {
    return {ElementEnum::Point, ElementEnum::Point};
  }
  if constexpr (ElementType == ElementEnum::Triangle) {
    return {ElementEnum::Line, ElementEnum::Line, ElementEnum::Line};
  }
  if constexpr (ElementType == ElementEnum::Quadrangle) {
    return {ElementEnum::Line, ElementEnum::Line, ElementEnum::Line, ElementEnum::Line};
  }
  if constexpr (ElementType == ElementEnum::Tetrahedron) {
    return {ElementEnum::Triangle, ElementEnum::Triangle, ElementEnum::Triangle, ElementEnum::Triangle};
  }
  if constexpr (ElementType == ElementEnum::Pyramid) {
    return {ElementEnum::Triangle, ElementEnum::Triangle, ElementEnum::Triangle, ElementEnum::Triangle,
            ElementEnum::Quadrangle};
  }
  if constexpr (ElementType == ElementEnum::Hexahedron) {
    return {ElementEnum::Quadrangle, ElementEnum::Quadrangle, ElementEnum::Quadrangle,
            ElementEnum::Quadrangle, ElementEnum::Quadrangle, ElementEnum::Quadrangle};
  }
}

template <ElementEnum ElementType>
consteval std::array<int, getVolumeElementAdjacencyNumber<ElementType>()> getVolumeElementPerAdjacencyNodeNumber() {
  if constexpr (ElementType == ElementEnum::Line) {
    return {1, 1};
  }
  if constexpr (ElementType == ElementEnum::Triangle) {
    return {2, 2, 2};
  }
  if constexpr (ElementType == ElementEnum::Quadrangle) {
    return {2, 2, 2, 2};
  }
  if constexpr (ElementType == ElementEnum::Tetrahedron) {
    return {3, 3, 3, 3};
  }
  if constexpr (ElementType == ElementEnum::Pyramid) {
    return {3, 3, 3, 3, 4};
  }
  if constexpr (ElementType == ElementEnum::Hexahedron) {
    return {4, 4, 4, 4, 4, 4};
  }
}

template <ElementEnum ElementType>
consteval int getVolumeElementAllAdjacencyNodeNumber() {
  constexpr std::array<int, getVolumeElementAdjacencyNumber<ElementType>()> kElementPerAdjacencyNodeNumber{
      getVolumeElementPerAdjacencyNodeNumber<ElementType>()};
  return std::accumulate(kElementPerAdjacencyNodeNumber.begin(), kElementPerAdjacencyNodeNumber.end(), 0);
}

template <ElementEnum ElementType>
consteval std::array<int, getVolumeElementAllAdjacencyNodeNumber<ElementType>()>
getVolumeElementPerAdjacencyNodeIndex() {
  // clang-format off
  if constexpr (ElementType == ElementEnum::Line) {
    return {0, 1};
  }
  if constexpr (ElementType == ElementEnum::Triangle) {
    return {0, 1,
            1, 2,
            2, 0};
  }
  if constexpr (ElementType == ElementEnum::Quadrangle) {
    return {0, 1,
            1, 2,
            2, 3,
            3, 0};
  }
  if constexpr (ElementType == ElementEnum::Tetrahedron) {
    return {0, 2, 1,
            0, 1, 3,
            0, 3, 2,
            3, 1, 2};
  }
  if constexpr (ElementType == ElementEnum::Pyramid) {
    return {0, 1, 4,
            3, 0, 4,
            1, 2, 4,
            2, 3, 4,
            0, 3, 2, 1};
  }
  if constexpr (ElementType == ElementEnum::Hexahedron) {
    return {0, 3, 2, 1,
            0, 1, 5, 4,
            0, 4, 7, 3,
            1, 2, 6, 5,
            2, 3, 7, 6,
            4, 5, 6, 7};
  }
  // clang-format on
}

template <ElementEnum ElementType>
consteval Real getElementMeasure() {
  if constexpr (ElementType == ElementEnum::Point) {
    return 1.0_r;
  }
  if constexpr (ElementType == ElementEnum::Line) {
    return 2.0_r;
  }
  if constexpr (ElementType == ElementEnum::Triangle) {
    return 0.5_r;
  }
  if constexpr (ElementType == ElementEnum::Quadrangle) {
    return 4.0_r;
  }
  if constexpr (ElementType == ElementEnum::Tetrahedron) {
    return 1.0_r / 6.0_r;
  }
  if constexpr (ElementType == ElementEnum::Pyramid) {
    return 1.0_r / 3.0_r;
  }
  if constexpr (ElementType == ElementEnum::Hexahedron) {
    return 8.0_r;
  }
}

template <ElementEnum ElementType, int PolynomialOrder>
consteval int getElementBasisFunctionNumber() {
  if constexpr (ElementType == ElementEnum::Point) {
    return 1;
  }
  if constexpr (ElementType == ElementEnum::Line) {
    return PolynomialOrder + 1;
  }
  if constexpr (ElementType == ElementEnum::Triangle) {
    return (PolynomialOrder + 1) * (PolynomialOrder + 2) / 2;
  }
  if constexpr (ElementType == ElementEnum::Quadrangle) {
    return (PolynomialOrder + 1) * (PolynomialOrder + 1);
  }
  if constexpr (ElementType == ElementEnum::Tetrahedron) {
    return (PolynomialOrder + 1) * (PolynomialOrder + 2) * (PolynomialOrder + 3) / 6;
  }
  if constexpr (ElementType == ElementEnum::Pyramid) {
    return (PolynomialOrder + 1) * (PolynomialOrder + 2) * (2 * PolynomialOrder + 3) / 6;
  }
  if constexpr (ElementType == ElementEnum::Hexahedron) {
    return (PolynomialOrder + 1) * (PolynomialOrder + 1) * (PolynomialOrder + 1);
  }
}

inline constexpr std::array<int, 26> kLineQuadratureNumber{1, 1, 2, 2, 3, 3,  4,  4,  5,  5,  6,  6,  7,
                                                           7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
inline constexpr std::array<int, 21> kTriangleQuadratureNumber{1,  1,  3,  4,  6,  7,  12, 13, 16, 19, 25,
                                                               27, 33, 37, 42, 48, 52, 61, 70, 73, 79};
inline constexpr std::array<int, 26> kQuadrangleQuadratureNumber{
    1, 3, 7, 4, 9, 9, 16, 16, 25, 25, 36, 36, 49, 49, 64, 64, 81, 81, 100, 100, 121, 121, 144, 144, 169, 169};
inline constexpr std::array<int, 22> kTetrahedronQuadratureNumber{
    1, 1, 4, 5, 11, 14, 24, 31, 43, 53, 126, 126, 210, 210, 330, 330, 495, 495, 715, 715, 1001, 1001};
inline constexpr std::array<int, 26> kPyramidQuadratureNumber{1,    1,    8,    8,    27,   27,   64,   64,  125,
                                                              125,  216,  216,  343,  343,  512,  512,  729, 729,
                                                              1000, 1000, 1331, 1331, 1728, 1728, 2197, 2197};
inline constexpr std::array<int, 26> kHexahedronQuadratureNumber{1,    6,    8,    8,    27,   27,   64,   64,  125,
                                                                 125,  216,  216,  343,  343,  512,  512,  729, 729,
                                                                 1000, 1000, 1331, 1331, 1728, 1728, 2197, 2197};

template <int PolynomialOrder>
consteval int getVolumeElementQuadratureOrder() {
  return 2 * PolynomialOrder + 2;
}

template <int PolynomialOrder>
consteval int getAdjacencyElementQuadratureOrder() {
  return 2 * PolynomialOrder + 3;

  // NOTE: More accurate quadrature order can be used for the interface element, which shows better convergence behavior
  // in the rotating mesh test.
  // return 4 * PolynomialOrder + 3;
}

template <ElementEnum ElementType, int PolynomialOrder>
consteval int getVolumeElementQuadratureNumber() {
  if constexpr (ElementType == ElementEnum::Point) {
    return 1;
  }
  if constexpr (ElementType == ElementEnum::Line) {
    return kLineQuadratureNumber[static_cast<Usize>(getVolumeElementQuadratureOrder<PolynomialOrder>())];
  }
  if constexpr (ElementType == ElementEnum::Triangle) {
    return kTriangleQuadratureNumber[static_cast<Usize>(getVolumeElementQuadratureOrder<PolynomialOrder>())];
  }
  if constexpr (ElementType == ElementEnum::Quadrangle) {
    return kQuadrangleQuadratureNumber[static_cast<Usize>(getVolumeElementQuadratureOrder<PolynomialOrder>())];
  }
  if constexpr (ElementType == ElementEnum::Tetrahedron) {
    return kTetrahedronQuadratureNumber[static_cast<Usize>(getVolumeElementQuadratureOrder<PolynomialOrder>())];
  }
  if constexpr (ElementType == ElementEnum::Pyramid) {
    return kPyramidQuadratureNumber[static_cast<Usize>(getVolumeElementQuadratureOrder<PolynomialOrder>())];
  }
  if constexpr (ElementType == ElementEnum::Hexahedron) {
    return kHexahedronQuadratureNumber[static_cast<Usize>(getVolumeElementQuadratureOrder<PolynomialOrder>())];
  }
}

template <ElementEnum ElementType, int PolynomialOrder>
consteval int getAdjacencyElementQuadratureNumber() {
  if constexpr (ElementType == ElementEnum::Point) {
    return 1;
  }
  if constexpr (ElementType == ElementEnum::Line) {
    return kLineQuadratureNumber[static_cast<Usize>(getAdjacencyElementQuadratureOrder<PolynomialOrder>())];
  }
  if constexpr (ElementType == ElementEnum::Triangle) {
    return kTriangleQuadratureNumber[static_cast<Usize>(getAdjacencyElementQuadratureOrder<PolynomialOrder>())];
  }
  if constexpr (ElementType == ElementEnum::Quadrangle) {
    return kQuadrangleQuadratureNumber[static_cast<Usize>(getAdjacencyElementQuadratureOrder<PolynomialOrder>())];
  }
}

template <ElementEnum ElementType, int PolynomialOrder>
consteval std::array<int, getVolumeElementAdjacencyNumber<ElementType>()>
getVolumeElementPerAdjacencyQuadratureNumber() {
  if constexpr (ElementType == ElementEnum::Line) {
    return {1, 1};
  }
  if constexpr (ElementType == ElementEnum::Triangle) {
    constexpr int kLineQuadrature{
        kLineQuadratureNumber[static_cast<Usize>(getAdjacencyElementQuadratureOrder<PolynomialOrder>())]};
    return {kLineQuadrature, kLineQuadrature, kLineQuadrature};
  }
  if constexpr (ElementType == ElementEnum::Quadrangle) {
    constexpr int kLineQuadrature{
        kLineQuadratureNumber[static_cast<Usize>(getAdjacencyElementQuadratureOrder<PolynomialOrder>())]};
    return {kLineQuadrature, kLineQuadrature, kLineQuadrature, kLineQuadrature};
  }
  if constexpr (ElementType == ElementEnum::Tetrahedron) {
    constexpr int kTriangleQuadrature{
        kTriangleQuadratureNumber[static_cast<Usize>(getAdjacencyElementQuadratureOrder<PolynomialOrder>())]};
    return {kTriangleQuadrature, kTriangleQuadrature, kTriangleQuadrature, kTriangleQuadrature};
  }
  if constexpr (ElementType == ElementEnum::Pyramid) {
    constexpr int kTriangleQuadrature{
        kTriangleQuadratureNumber[static_cast<Usize>(getAdjacencyElementQuadratureOrder<PolynomialOrder>())]};
    constexpr int kQuadrangleQuadrature{
        kQuadrangleQuadratureNumber[static_cast<Usize>(getAdjacencyElementQuadratureOrder<PolynomialOrder>())]};
    return {kTriangleQuadrature, kTriangleQuadrature, kTriangleQuadrature, kTriangleQuadrature, kQuadrangleQuadrature};
  }
  if constexpr (ElementType == ElementEnum::Hexahedron) {
    constexpr int kQuadrangleQuadrature{
        kQuadrangleQuadratureNumber[static_cast<Usize>(getAdjacencyElementQuadratureOrder<PolynomialOrder>())]};
    return {kQuadrangleQuadrature, kQuadrangleQuadrature, kQuadrangleQuadrature,
            kQuadrangleQuadrature, kQuadrangleQuadrature, kQuadrangleQuadrature};
  }
}

template <ElementEnum ElementType, int PolynomialOrder>
consteval int getVolumeElementAllAdjacencyQuadratureNumber() {
  constexpr std::array<int, getVolumeElementAdjacencyNumber<ElementType>()> kElementPerAdjacencyQuadratureNumber{
      getVolumeElementPerAdjacencyQuadratureNumber<ElementType, PolynomialOrder>()};
  return std::accumulate(kElementPerAdjacencyQuadratureNumber.begin(), kElementPerAdjacencyQuadratureNumber.end(), 0);
}

template <ElementEnum ElementType, int PolynomialOrder>
consteval std::array<int, getVolumeElementAdjacencyNumber<ElementType>() + 1>
getVolumeElementAdjacencyQuadratureSequence() {
  std::array<int, getVolumeElementAdjacencyNumber<ElementType>() + 1> accumulate_adjacency_quadrature_number{};
  accumulate_adjacency_quadrature_number[0] = 0;
  for (int i = 0; i < getVolumeElementAdjacencyNumber<ElementType>(); i++) {
    accumulate_adjacency_quadrature_number[static_cast<Usize>(i + 1)] =
        accumulate_adjacency_quadrature_number[static_cast<Usize>(i)] +
        getVolumeElementPerAdjacencyQuadratureNumber<ElementType, PolynomialOrder>()[static_cast<Usize>(i)];
  }
  return accumulate_adjacency_quadrature_number;
}

template <ElementEnum ElementType, int PolynomialOrder>
constexpr std::array<int, getAdjacencyElementQuadratureNumber<ElementType, PolynomialOrder>()>
getAdjacencyElementQuadratureSequence([[maybe_unused]] int rotation) {
  constexpr int kAdjacencyElementQuadratureNumber{getAdjacencyElementQuadratureNumber<ElementType, PolynomialOrder>()};
  if constexpr (ElementType == ElementEnum::Point) {
    return {0};
  } else if constexpr (ElementType == ElementEnum::Line) {
    if constexpr (kAdjacencyElementQuadratureNumber == 2) {
      return {1, 0};
    } else if constexpr (kAdjacencyElementQuadratureNumber == 3) {
      return {2, 1, 0};
    } else if constexpr (kAdjacencyElementQuadratureNumber == 4) {
      return {3, 2, 1, 0};
    } else if constexpr (kAdjacencyElementQuadratureNumber == 5) {
      return {4, 3, 2, 1, 0};
    } else if constexpr (kAdjacencyElementQuadratureNumber == 6) {
      return {5, 4, 3, 2, 1, 0};
    } else if constexpr (kAdjacencyElementQuadratureNumber == 7) {
      return {6, 5, 4, 3, 2, 1, 0};
    } else if constexpr (kAdjacencyElementQuadratureNumber == 8) {
      return {7, 6, 5, 4, 3, 2, 1, 0};
    } else if constexpr (kAdjacencyElementQuadratureNumber == 9) {
      return {8, 7, 6, 5, 4, 3, 2, 1, 0};
    } else if constexpr (kAdjacencyElementQuadratureNumber == 10) {
      return {9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    } else if constexpr (kAdjacencyElementQuadratureNumber == 11) {
      return {10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    } else if constexpr (kAdjacencyElementQuadratureNumber == 12) {
      return {11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    } else if constexpr (kAdjacencyElementQuadratureNumber == 13) {
      return {12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
    }
  } else if constexpr (ElementType == ElementEnum::Triangle) {
    if constexpr (kAdjacencyElementQuadratureNumber == 4) {
      switch (rotation) {
      case 0:
        return {0, 1, 3, 2};
      case 1:
        return {0, 3, 2, 1};
      case 2:
        return {0, 2, 1, 3};
      }
    } else if constexpr (kAdjacencyElementQuadratureNumber == 7) {
      switch (rotation) {
      case 0:
        return {0, 1, 3, 2, 4, 6, 5};
      case 1:
        return {0, 3, 2, 1, 6, 5, 4};
      case 2:
        return {0, 2, 1, 3, 5, 4, 6};
      }
    } else if constexpr (kAdjacencyElementQuadratureNumber == 13) {
      switch (rotation) {
      case 0:
        return {0, 1, 3, 2, 4, 6, 5, 11, 12, 10, 9, 7, 8};
      case 1:
        return {0, 3, 2, 1, 6, 5, 4, 12, 10, 11, 8, 9, 7};
      case 2:
        return {0, 2, 1, 3, 5, 4, 6, 10, 11, 12, 7, 8, 9};
      }
    } else if constexpr (kAdjacencyElementQuadratureNumber == 19) {
      switch (rotation) {
      case 0:
        return {0, 1, 3, 2, 4, 6, 5, 7, 9, 8, 10, 12, 11, 17, 18, 16, 15, 13, 14};
      case 1:
        return {0, 3, 2, 1, 6, 5, 4, 9, 8, 7, 12, 11, 10, 18, 16, 17, 14, 15, 13};
      case 2:
        return {0, 2, 1, 3, 5, 4, 6, 8, 7, 9, 11, 10, 12, 16, 17, 18, 13, 14, 15};
      }
    } else if constexpr (kAdjacencyElementQuadratureNumber == 27) {
      switch (rotation) {
      case 0:
        return {0, 2, 1, 3, 5, 4, 6, 8, 7, 9, 11, 10, 12, 14, 13, 19, 20, 18, 17, 15, 16, 25, 26, 24, 23, 21, 22};
      case 1:
        return {1, 0, 2, 4, 3, 5, 7, 6, 8, 10, 9, 11, 13, 12, 14, 18, 19, 20, 15, 16, 17, 24, 25, 26, 21, 22, 23};
      case 2:
        return {2, 1, 0, 5, 4, 3, 8, 7, 6, 11, 10, 9, 14, 13, 12, 20, 18, 19, 16, 17, 15, 26, 24, 25, 22, 23, 21};
      }
    } else if constexpr (kAdjacencyElementQuadratureNumber == 37) {
      switch (rotation) {
      case 0:
        return {0,  1,  3,  2,  4,  6,  5,  7,  9,  8,  10, 12, 11, 13, 15, 14, 16, 18, 17,
                23, 24, 22, 21, 19, 20, 29, 30, 28, 27, 25, 26, 35, 36, 34, 33, 31, 32};
      case 1:
        return {0,  2,  1,  3,  5,  4,  6,  8,  7,  9,  11, 10, 12, 14, 13, 15, 17, 16, 18,
                22, 23, 24, 19, 20, 21, 28, 29, 30, 25, 26, 27, 34, 35, 36, 31, 32, 33};
      case 2:
        return {0,  3,  2,  1,  6,  5,  4,  9,  8,  7,  12, 11, 10, 15, 14, 13, 18, 17, 16,
                24, 22, 23, 20, 21, 19, 30, 28, 29, 26, 27, 25, 36, 34, 35, 32, 33, 31};
      }
    } else if constexpr (kAdjacencyElementQuadratureNumber == 48) {
      switch (rotation) {
      case 0:
        return {0,  2,  1,  3,  5,  4,  6,  8,  7,  9,  11, 10, 12, 14, 13, 15, 17, 16, 22, 23, 21, 20, 18, 19,
                28, 29, 27, 26, 24, 25, 34, 35, 33, 32, 30, 31, 40, 41, 39, 38, 36, 37, 46, 47, 45, 44, 42, 43};
      case 1:
        return {1,  0,  2,  4,  3,  5,  7,  6,  8,  10, 9,  11, 13, 12, 14, 16, 15, 17, 21, 22, 23, 18, 19, 20,
                27, 28, 29, 24, 25, 26, 33, 34, 35, 30, 31, 32, 39, 40, 41, 36, 37, 38, 45, 46, 47, 42, 43, 44};
      case 2:
        return {2,  1,  0,  5,  4,  3,  8,  7,  6,  11, 10, 9,  14, 13, 12, 17, 16, 15, 23, 21, 22, 19, 20, 18,
                29, 27, 28, 25, 26, 24, 35, 33, 34, 31, 32, 30, 41, 39, 40, 37, 38, 36, 47, 45, 46, 43, 44, 42};
      }
    } else if constexpr (kAdjacencyElementQuadratureNumber == 52) {
      switch (rotation) {
      case 0:
        return {0,  1,  3,  2,  4,  6,  5,  7,  9,  8,  10, 12, 11, 13, 15, 14, 16, 18, 17, 19, 21, 20, 26, 27, 25, 24,
                22, 23, 32, 33, 31, 30, 28, 29, 38, 39, 37, 36, 34, 35, 44, 45, 43, 42, 40, 41, 50, 51, 49, 48, 46, 47};
      case 1:
        return {0,  2,  1,  3,  5,  4,  6,  8,  7,  9,  11, 10, 12, 14, 13, 15, 17, 16, 18, 20, 19, 21, 25, 26, 27, 22,
                23, 24, 31, 32, 33, 28, 29, 30, 37, 38, 39, 34, 35, 36, 43, 44, 45, 40, 41, 42, 49, 50, 51, 46, 47, 48};
      case 2:
        return {0,  3,  2,  1,  6,  5,  4,  9,  8,  7,  12, 11, 10, 15, 14, 13, 18, 17, 16, 21, 20, 19, 27, 25, 26, 23,
                24, 22, 33, 31, 32, 29, 30, 28, 39, 37, 38, 35, 36, 34, 45, 43, 44, 41, 42, 40, 51, 49, 50, 47, 48, 46};
      }
    } else if constexpr (kAdjacencyElementQuadratureNumber == 61) {
      switch (rotation) {
      case 0:
        return {0,  1,  3,  2,  4,  6,  5,  7,  9,  8,  10, 12, 11, 13, 15, 14, 16, 18, 17, 19, 21,
                20, 22, 24, 23, 29, 30, 28, 27, 25, 26, 35, 36, 34, 33, 31, 32, 41, 42, 40, 39, 37,
                38, 47, 48, 46, 45, 43, 44, 53, 54, 52, 51, 49, 50, 59, 60, 58, 57, 55, 56};
      case 1:
        return {0,  2,  1,  3,  5,  4,  6,  8,  7,  9,  11, 10, 12, 14, 13, 15, 17, 16, 18, 20, 19,
                21, 23, 22, 24, 28, 29, 30, 25, 26, 27, 34, 35, 36, 31, 32, 33, 40, 41, 42, 37, 38,
                39, 46, 47, 48, 43, 44, 45, 52, 53, 54, 49, 50, 51, 58, 59, 60, 55, 56, 57};
      case 2:
        return {0,  3,  2,  1,  6,  5,  4,  9,  8,  7,  12, 11, 10, 15, 14, 13, 18, 17, 16, 21, 20,
                19, 24, 23, 22, 30, 28, 29, 26, 27, 25, 36, 34, 35, 32, 33, 31, 42, 40, 41, 38, 39,
                37, 48, 46, 47, 44, 45, 43, 54, 52, 53, 50, 51, 49, 60, 58, 59, 56, 57, 55};
      }
    } else if constexpr (kAdjacencyElementQuadratureNumber == 70) {
      switch (rotation) {
      case 0:
        return {0,  1,  3,  2,  4,  6,  5,  7,  9,  8,  10, 12, 11, 13, 15, 14, 16, 18, 17, 19, 21, 20, 22, 24,
                23, 25, 27, 26, 32, 33, 31, 30, 28, 29, 38, 39, 37, 36, 34, 35, 44, 45, 43, 42, 40, 41, 50, 51,
                49, 48, 46, 47, 56, 57, 55, 54, 52, 53, 62, 63, 61, 60, 58, 59, 68, 69, 67, 66, 64, 65};
      case 1:
        return {0,  2,  1,  3,  5,  4,  6,  8,  7,  9,  11, 10, 12, 14, 13, 15, 17, 16, 18, 20, 19, 21, 23, 22,
                24, 26, 25, 27, 31, 32, 33, 28, 29, 30, 37, 38, 39, 34, 35, 36, 43, 44, 45, 40, 41, 42, 49, 50,
                51, 46, 47, 48, 55, 56, 57, 52, 53, 54, 61, 62, 63, 58, 59, 60, 67, 68, 69, 64, 65, 66};
      case 2:
        return {0,  3,  2,  1,  6,  5,  4,  9,  8,  7,  12, 11, 10, 15, 14, 13, 18, 17, 16, 21, 20, 19, 24, 23,
                22, 27, 26, 25, 33, 31, 32, 29, 30, 28, 39, 37, 38, 35, 36, 34, 45, 43, 44, 41, 42, 40, 51, 49,
                50, 47, 48, 46, 57, 55, 56, 53, 54, 52, 63, 61, 62, 59, 60, 58, 69, 67, 68, 65, 66, 64};
      }
    } else if constexpr (kAdjacencyElementQuadratureNumber == 73) {
      switch (rotation) {
      case 0:
        return {0,  1,  3,  2,  4,  6,  5,  7,  9,  8,  10, 12, 11, 13, 15, 14, 16, 18, 17, 19, 21, 20, 22, 24, 23,
                29, 30, 28, 27, 25, 26, 35, 36, 34, 33, 31, 32, 41, 42, 40, 39, 37, 38, 47, 48, 46, 45, 43, 44, 53,
                54, 52, 51, 49, 50, 59, 60, 58, 57, 55, 56, 65, 66, 64, 63, 61, 62, 71, 72, 70, 69, 67, 68};
      case 1:
        return {0,  2,  1,  3,  5,  4,  6,  8,  7,  9,  11, 10, 12, 14, 13, 15, 17, 16, 18, 20, 19, 21, 23, 22, 24,
                28, 29, 30, 25, 26, 27, 34, 35, 36, 31, 32, 33, 40, 41, 42, 37, 38, 39, 46, 47, 48, 43, 44, 45, 52,
                53, 54, 49, 50, 51, 58, 59, 60, 55, 56, 57, 64, 65, 66, 61, 62, 63, 70, 71, 72, 67, 68, 69};
      case 2:
        return {0,  3,  2,  1,  6,  5,  4,  9,  8,  7,  12, 11, 10, 15, 14, 13, 18, 17, 16, 21, 20, 19, 24, 23, 22,
                30, 28, 29, 26, 27, 25, 36, 34, 35, 32, 33, 31, 42, 40, 41, 38, 39, 37, 48, 46, 47, 44, 45, 43, 54,
                52, 53, 50, 51, 49, 60, 58, 59, 56, 57, 55, 66, 64, 65, 62, 63, 61, 72, 70, 71, 68, 69, 67};
      }
    } else if constexpr (kAdjacencyElementQuadratureNumber == 79) {
      switch (rotation) {
      case 0:
        return {0,  1,  3,  2,  4,  6,  5,  7,  9,  8,  10, 12, 11, 13, 15, 14, 16, 18, 17, 19,
                21, 20, 22, 24, 23, 25, 27, 26, 28, 30, 29, 35, 36, 34, 33, 31, 32, 41, 42, 40,
                39, 37, 38, 47, 48, 46, 45, 43, 44, 53, 54, 52, 51, 49, 50, 59, 60, 58, 57, 55,
                56, 65, 66, 64, 63, 61, 62, 71, 72, 70, 69, 67, 68, 77, 78, 76, 75, 73, 74};
      case 1:
        return {0,  2,  1,  3,  5,  4,  6,  8,  7,  9,  11, 10, 12, 14, 13, 15, 17, 16, 18, 20,
                19, 21, 23, 22, 24, 26, 25, 27, 29, 28, 30, 34, 35, 36, 31, 32, 33, 40, 41, 42,
                37, 38, 39, 46, 47, 48, 43, 44, 45, 52, 53, 54, 49, 50, 51, 58, 59, 60, 55, 56,
                57, 64, 65, 66, 61, 62, 63, 70, 71, 72, 67, 68, 69, 76, 77, 78, 73, 74, 75};
      case 2:
        return {0,  3,  2,  1,  6,  5,  4,  9,  8,  7,  12, 11, 10, 15, 14, 13, 18, 17, 16, 21,
                20, 19, 24, 23, 22, 27, 26, 25, 30, 29, 28, 36, 34, 35, 32, 33, 31, 42, 40, 41,
                38, 39, 37, 48, 46, 47, 44, 45, 43, 54, 52, 53, 50, 51, 49, 60, 58, 59, 56, 57,
                55, 66, 64, 65, 62, 63, 61, 72, 70, 71, 68, 69, 67, 78, 76, 77, 74, 75, 73};
      }
    }
  } else if constexpr (ElementType == ElementEnum::Quadrangle) {
    if constexpr (kAdjacencyElementQuadratureNumber == 4) {
      switch (rotation) {
      case 0:
        return {0, 2, 1, 3};
      case 1:
        return {2, 3, 0, 1};
      case 2:
        return {3, 1, 2, 0};
      case 3:
        return {1, 0, 3, 2};
      }
    } else if constexpr (kAdjacencyElementQuadratureNumber == 9) {
      switch (rotation) {
      case 0:
        return {0, 3, 6, 1, 4, 7, 2, 5, 8};
      case 1:
        return {6, 7, 8, 3, 4, 5, 0, 1, 2};
      case 2:
        return {8, 5, 2, 7, 4, 1, 6, 3, 0};
      case 3:
        return {2, 1, 0, 5, 4, 3, 8, 7, 6};
      }
    } else if constexpr (kAdjacencyElementQuadratureNumber == 16) {
      switch (rotation) {
      case 0:
        return {0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15};
      case 1:
        return {12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3};
      case 2:
        return {15, 11, 7, 3, 14, 10, 6, 2, 13, 9, 5, 1, 12, 8, 4, 0};
      case 3:
        return {3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12};
      }
    } else if constexpr (kAdjacencyElementQuadratureNumber == 25) {
      switch (rotation) {
      case 0:
        return {0, 5, 10, 15, 20, 1, 6, 11, 16, 21, 2, 7, 12, 17, 22, 3, 8, 13, 18, 23, 4, 9, 14, 19, 24};
      case 1:
        return {20, 21, 22, 23, 24, 15, 16, 17, 18, 19, 10, 11, 12, 13, 14, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4};
      case 2:
        return {24, 19, 14, 9, 4, 23, 18, 13, 8, 3, 22, 17, 12, 7, 2, 21, 16, 11, 6, 1, 20, 15, 10, 5, 0};
      case 3:
        return {4, 3, 2, 1, 0, 9, 8, 7, 6, 5, 14, 13, 12, 11, 10, 19, 18, 17, 16, 15, 24, 23, 22, 21, 20};
      }
    } else if constexpr (kAdjacencyElementQuadratureNumber == 36) {
      switch (rotation) {
      case 0:
        return {0, 6, 12, 18, 24, 30, 1, 7,  13, 19, 25, 31, 2, 8,  14, 20, 26, 32,
                3, 9, 15, 21, 27, 33, 4, 10, 16, 22, 28, 34, 5, 11, 17, 23, 29, 35};
      case 1:
        return {30, 31, 32, 33, 34, 35, 24, 25, 26, 27, 28, 29, 18, 19, 20, 21, 22, 23,
                12, 13, 14, 15, 16, 17, 6,  7,  8,  9,  10, 11, 0,  1,  2,  3,  4,  5};
      case 2:
        return {35, 29, 23, 17, 11, 5, 34, 28, 22, 16, 10, 4, 33, 27, 21, 15, 9, 3,
                32, 26, 20, 14, 8,  2, 31, 25, 19, 13, 7,  1, 30, 24, 18, 12, 6, 0};
      case 3:
        return {5,  4,  3,  2,  1,  0,  11, 10, 9,  8,  7,  6,  17, 16, 15, 14, 13, 12,
                23, 22, 21, 20, 19, 18, 29, 28, 27, 26, 25, 24, 35, 34, 33, 32, 31, 30};
      }
    } else if constexpr (kAdjacencyElementQuadratureNumber == 49) {
      switch (rotation) {
      case 0:
        return {0,  7,  14, 21, 28, 35, 42, 1,  8,  15, 22, 29, 36, 43, 2,  9,  16, 23, 30, 37, 44, 3,  10, 17, 24,
                31, 38, 45, 4,  11, 18, 25, 32, 39, 46, 5,  12, 19, 26, 33, 40, 47, 6,  13, 20, 27, 34, 41, 48};
      case 1:
        return {42, 43, 44, 45, 46, 47, 48, 35, 36, 37, 38, 39, 40, 41, 28, 29, 30, 31, 32, 33, 34, 21, 22, 23, 24,
                25, 26, 27, 14, 15, 16, 17, 18, 19, 20, 7,  8,  9,  10, 11, 12, 13, 0,  1,  2,  3,  4,  5,  6};
      case 2:
        return {48, 41, 34, 27, 20, 13, 6,  47, 40, 33, 26, 19, 12, 5,  46, 39, 32, 25, 18, 11, 4,  45, 38, 31, 24,
                17, 10, 3,  44, 37, 30, 23, 16, 9,  2,  43, 36, 29, 22, 15, 8,  1,  42, 35, 28, 21, 14, 7,  0};
      case 3:
        return {6,  5,  4,  3,  2,  1,  0,  13, 12, 11, 10, 9,  8,  7,  20, 19, 18, 17, 16, 15, 14, 27, 26, 25, 24,
                23, 22, 21, 34, 33, 32, 31, 30, 29, 28, 41, 40, 39, 38, 37, 36, 35, 48, 47, 46, 45, 44, 43, 42};
      }
    } else if constexpr (kAdjacencyElementQuadratureNumber == 64) {
      switch (rotation) {
      case 0:
        return {0,  8,  16, 24, 32, 40, 48, 56, 1,  9,  17, 25, 33, 41, 49, 57, 2,  10, 18, 26, 34, 42,
                50, 58, 3,  11, 19, 27, 35, 43, 51, 59, 4,  12, 20, 28, 36, 44, 52, 60, 5,  13, 21, 29,
                37, 45, 53, 61, 6,  14, 22, 30, 38, 46, 54, 62, 7,  15, 23, 31, 39, 47, 55, 63};
      case 1:
        return {56, 57, 58, 59, 60, 61, 62, 63, 48, 49, 50, 51, 52, 53, 54, 55, 40, 41, 42, 43, 44, 45,
                46, 47, 32, 33, 34, 35, 36, 37, 38, 39, 24, 25, 26, 27, 28, 29, 30, 31, 16, 17, 18, 19,
                20, 21, 22, 23, 8,  9,  10, 11, 12, 13, 14, 15, 0,  1,  2,  3,  4,  5,  6,  7};
      case 2:
        return {63, 55, 47, 39, 31, 23, 15, 7,  62, 54, 46, 38, 30, 22, 14, 6,  61, 53, 45, 37, 29, 21,
                13, 5,  60, 52, 44, 36, 28, 20, 12, 4,  59, 51, 43, 35, 27, 19, 11, 3,  58, 50, 42, 34,
                26, 18, 10, 2,  57, 49, 41, 33, 25, 17, 9,  1,  56, 48, 40, 32, 24, 16, 8,  0};
      case 3:
        return {7,  6,  5,  4,  3,  2,  1,  0,  15, 14, 13, 12, 11, 10, 9,  8,  23, 22, 21, 20, 19, 18,
                17, 16, 31, 30, 29, 28, 27, 26, 25, 24, 39, 38, 37, 36, 35, 34, 33, 32, 47, 46, 45, 44,
                43, 42, 41, 40, 55, 54, 53, 52, 51, 50, 49, 48, 63, 62, 61, 60, 59, 58, 57, 56};
      }
    } else if constexpr (kAdjacencyElementQuadratureNumber == 81) {
      switch (rotation) {
      case 0:
        return {0,  9,  18, 27, 36, 45, 54, 63, 72, 1,  10, 19, 28, 37, 46, 55, 64, 73, 2,  11, 20,
                29, 38, 47, 56, 65, 74, 3,  12, 21, 30, 39, 48, 57, 66, 75, 4,  13, 22, 31, 40, 49,
                58, 67, 76, 5,  14, 23, 32, 41, 50, 59, 68, 77, 6,  15, 24, 33, 42, 51, 60, 69, 78,
                7,  16, 25, 34, 43, 52, 61, 70, 79, 8,  17, 26, 35, 44, 53, 62, 71, 80};
      case 1:
        return {72, 73, 74, 75, 76, 77, 78, 79, 80, 63, 64, 65, 66, 67, 68, 69, 70, 71, 54, 55, 56,
                57, 58, 59, 60, 61, 62, 45, 46, 47, 48, 49, 50, 51, 52, 53, 36, 37, 38, 39, 40, 41,
                42, 43, 44, 27, 28, 29, 30, 31, 32, 33, 34, 35, 18, 19, 20, 21, 22, 23, 24, 25, 26,
                9,  10, 11, 12, 13, 14, 15, 16, 17, 0,  1,  2,  3,  4,  5,  6,  7,  8};
      case 2:
        return {80, 71, 62, 53, 44, 35, 26, 17, 8,  79, 70, 61, 52, 43, 34, 25, 16, 7,  78, 69, 60,
                51, 42, 33, 24, 15, 6,  77, 68, 59, 50, 41, 32, 23, 14, 5,  76, 67, 58, 49, 40, 31,
                22, 13, 4,  75, 66, 57, 48, 39, 30, 21, 12, 3,  74, 65, 56, 47, 38, 29, 20, 11, 2,
                73, 64, 55, 46, 37, 28, 19, 10, 1,  72, 63, 54, 45, 36, 27, 18, 9,  0};
      case 3:
        return {8,  7,  6,  5,  4,  3,  2,  1,  0,  17, 16, 15, 14, 13, 12, 11, 10, 9,  26, 25, 24,
                23, 22, 21, 20, 19, 18, 35, 34, 33, 32, 31, 30, 29, 28, 27, 44, 43, 42, 41, 40, 39,
                38, 37, 36, 53, 52, 51, 50, 49, 48, 47, 46, 45, 62, 61, 60, 59, 58, 57, 56, 55, 54,
                71, 70, 69, 68, 67, 66, 65, 64, 63, 80, 79, 78, 77, 76, 75, 74, 73, 72};
      }
    } else if constexpr (kAdjacencyElementQuadratureNumber == 100) {
      switch (rotation) {
      case 0:
        return {0,  10, 20, 30, 40, 50, 60, 70, 80, 90, 1,  11, 21, 31, 41, 51, 61, 71, 81, 91, 2,  12, 22, 32, 42,
                52, 62, 72, 82, 92, 3,  13, 23, 33, 43, 53, 63, 73, 83, 93, 4,  14, 24, 34, 44, 54, 64, 74, 84, 94,
                5,  15, 25, 35, 45, 55, 65, 75, 85, 95, 6,  16, 26, 36, 46, 56, 66, 76, 86, 96, 7,  17, 27, 37, 47,
                57, 67, 77, 87, 97, 8,  18, 28, 38, 48, 58, 68, 78, 88, 98, 9,  19, 29, 39, 49, 59, 69, 79, 89, 99};
      case 1:
        return {90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 70, 71, 72, 73, 74,
                75, 76, 77, 78, 79, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
                40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 20, 21, 22, 23, 24,
                25, 26, 27, 28, 29, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9};
      case 2:
        return {99, 89, 79, 69, 59, 49, 39, 29, 19, 9,  98, 88, 78, 68, 58, 48, 38, 28, 18, 8,  97, 87, 77, 67, 57,
                47, 37, 27, 17, 7,  96, 86, 76, 66, 56, 46, 36, 26, 16, 6,  95, 85, 75, 65, 55, 45, 35, 25, 15, 5,
                94, 84, 74, 64, 54, 44, 34, 24, 14, 4,  93, 83, 73, 63, 53, 43, 33, 23, 13, 3,  92, 82, 72, 62, 52,
                42, 32, 22, 12, 2,  91, 81, 71, 61, 51, 41, 31, 21, 11, 1,  90, 80, 70, 60, 50, 40, 30, 20, 10, 0};
      case 3:
        return {9,  8,  7,  6,  5,  4,  3,  2,  1,  0,  19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 29, 28, 27, 26, 25,
                24, 23, 22, 21, 20, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40,
                59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 79, 78, 77, 76, 75,
                74, 73, 72, 71, 70, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80, 99, 98, 97, 96, 95, 94, 93, 92, 91, 90};
      }
    } else if constexpr (kAdjacencyElementQuadratureNumber == 121) {
      switch (rotation) {
      case 0:
        return {0,   11,  22,  33,  44,  55, 66, 77, 88, 99, 110, 1,   12,  23,  34,  45, 56, 67, 78, 89, 100,
                111, 2,   13,  24,  35,  46, 57, 68, 79, 90, 101, 112, 3,   14,  25,  36, 47, 58, 69, 80, 91,
                102, 113, 4,   15,  26,  37, 48, 59, 70, 81, 92,  103, 114, 5,   16,  27, 38, 49, 60, 71, 82,
                93,  104, 115, 6,   17,  28, 39, 50, 61, 72, 83,  94,  105, 116, 7,   18, 29, 40, 51, 62, 73,
                84,  95,  106, 117, 8,   19, 30, 41, 52, 63, 74,  85,  96,  107, 118, 9,  20, 31, 42, 53, 64,
                75,  86,  97,  108, 119, 10, 21, 32, 43, 54, 65,  76,  87,  98,  109, 120};
      case 1:
        return {110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108,
                109, 88,  89,  90,  91,  92,  93,  94,  95,  96,  97,  98, 77,  78,  79,  80,  81,  82,  83,  84,  85,
                86,  87,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75, 76,  55,  56,  57,  58,  59,  60,  61,  62,
                63,  64,  65,  44,  45,  46,  47,  48,  49,  50,  51,  52, 53,  54,  33,  34,  35,  36,  37,  38,  39,
                40,  41,  42,  43,  22,  23,  24,  25,  26,  27,  28,  29, 30,  31,  32,  11,  12,  13,  14,  15,  16,
                17,  18,  19,  20,  21,  0,   1,   2,   3,   4,   5,   6,  7,   8,   9,   10};
      case 2:
        return {120, 109, 98,  87,  76,  65,  54, 43, 32, 21, 10, 119, 108, 97,  86,  75,  64,  53, 42, 31, 20,
                9,   118, 107, 96,  85,  74,  63, 52, 41, 30, 19, 8,   117, 106, 95,  84,  73,  62, 51, 40, 29,
                18,  7,   116, 105, 94,  83,  72, 61, 50, 39, 28, 17,  6,   115, 104, 93,  82,  71, 60, 49, 38,
                27,  16,  5,   114, 103, 92,  81, 70, 59, 48, 37, 26,  15,  4,   113, 102, 91,  80, 69, 58, 47,
                36,  25,  14,  3,   112, 101, 90, 79, 68, 57, 46, 35,  24,  13,  2,   111, 100, 89, 78, 67, 56,
                45,  34,  23,  12,  1,   110, 99, 88, 77, 66, 55, 44,  33,  22,  11,  0};
      case 3:
        return {10,  9,   8,   7,   6,  5,   4,   3,   2,   1,   0,   21,  20,  19,  18,  17,  16,  15,  14,  13,  12,
                11,  32,  31,  30,  29, 28,  27,  26,  25,  24,  23,  22,  43,  42,  41,  40,  39,  38,  37,  36,  35,
                34,  33,  54,  53,  52, 51,  50,  49,  48,  47,  46,  45,  44,  65,  64,  63,  62,  61,  60,  59,  58,
                57,  56,  55,  76,  75, 74,  73,  72,  71,  70,  69,  68,  67,  66,  87,  86,  85,  84,  83,  82,  81,
                80,  79,  78,  77,  98, 97,  96,  95,  94,  93,  92,  91,  90,  89,  88,  109, 108, 107, 106, 105, 104,
                103, 102, 101, 100, 99, 120, 119, 118, 117, 116, 115, 114, 113, 112, 111, 110};
      }
    } else if constexpr (kAdjacencyElementQuadratureNumber == 144) {
      switch (rotation) {
      case 0:
        return {0,  12, 24, 36, 48, 60, 72, 84, 96,  108, 120, 132, 1,  13, 25, 37, 49, 61, 73, 85, 97,  109, 121, 133,
                2,  14, 26, 38, 50, 62, 74, 86, 98,  110, 122, 134, 3,  15, 27, 39, 51, 63, 75, 87, 99,  111, 123, 135,
                4,  16, 28, 40, 52, 64, 76, 88, 100, 112, 124, 136, 5,  17, 29, 41, 53, 65, 77, 89, 101, 113, 125, 137,
                6,  18, 30, 42, 54, 66, 78, 90, 102, 114, 126, 138, 7,  19, 31, 43, 55, 67, 79, 91, 103, 115, 127, 139,
                8,  20, 32, 44, 56, 68, 80, 92, 104, 116, 128, 140, 9,  21, 33, 45, 57, 69, 81, 93, 105, 117, 129, 141,
                10, 22, 34, 46, 58, 70, 82, 94, 106, 118, 130, 142, 11, 23, 35, 47, 59, 71, 83, 95, 107, 119, 131, 143};
      case 1:
        return {132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 120, 121, 122, 123, 124, 125, 126, 127, 128,
                129, 130, 131, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 96,  97,  98,  99,  100, 101,
                102, 103, 104, 105, 106, 107, 84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  72,  73,  74,
                75,  76,  77,  78,  79,  80,  81,  82,  83,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,
                48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  36,  37,  38,  39,  40,  41,  42,  43,  44,
                45,  46,  47,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  12,  13,  14,  15,  16,  17,
                18,  19,  20,  21,  22,  23,  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11};
      case 2:
        return {143, 131, 119, 107, 95, 83, 71, 59, 47, 35, 23, 11, 142, 130, 118, 106, 94, 82, 70, 58, 46, 34, 22, 10,
                141, 129, 117, 105, 93, 81, 69, 57, 45, 33, 21, 9,  140, 128, 116, 104, 92, 80, 68, 56, 44, 32, 20, 8,
                139, 127, 115, 103, 91, 79, 67, 55, 43, 31, 19, 7,  138, 126, 114, 102, 90, 78, 66, 54, 42, 30, 18, 6,
                137, 125, 113, 101, 89, 77, 65, 53, 41, 29, 17, 5,  136, 124, 112, 100, 88, 76, 64, 52, 40, 28, 16, 4,
                135, 123, 111, 99,  87, 75, 63, 51, 39, 27, 15, 3,  134, 122, 110, 98,  86, 74, 62, 50, 38, 26, 14, 2,
                133, 121, 109, 97,  85, 73, 61, 49, 37, 25, 13, 1,  132, 120, 108, 96,  84, 72, 60, 48, 36, 24, 12, 0};
      case 3:
        return {11,  10,  9,   8,   7,   6,   5,   4,   3,   2,   1,   0,   23,  22,  21,  20,  19,  18,  17,  16,  15,
                14,  13,  12,  35,  34,  33,  32,  31,  30,  29,  28,  27,  26,  25,  24,  47,  46,  45,  44,  43,  42,
                41,  40,  39,  38,  37,  36,  59,  58,  57,  56,  55,  54,  53,  52,  51,  50,  49,  48,  71,  70,  69,
                68,  67,  66,  65,  64,  63,  62,  61,  60,  83,  82,  81,  80,  79,  78,  77,  76,  75,  74,  73,  72,
                95,  94,  93,  92,  91,  90,  89,  88,  87,  86,  85,  84,  107, 106, 105, 104, 103, 102, 101, 100, 99,
                98,  97,  96,  119, 118, 117, 116, 115, 114, 113, 112, 111, 110, 109, 108, 131, 130, 129, 128, 127, 126,
                125, 124, 123, 122, 121, 120, 143, 142, 141, 140, 139, 138, 137, 136, 135, 134, 133, 132};
      }
    } else if constexpr (kAdjacencyElementQuadratureNumber == 169) {
      switch (rotation) {
      case 0:
        return {0,   13,  26,  39,  52,  65,  78,  91,  104, 117, 130, 143, 156, 1,   14,  27,  40,  53,  66,
                79,  92,  105, 118, 131, 144, 157, 2,   15,  28,  41,  54,  67,  80,  93,  106, 119, 132, 145,
                158, 3,   16,  29,  42,  55,  68,  81,  94,  107, 120, 133, 146, 159, 4,   17,  30,  43,  56,
                69,  82,  95,  108, 121, 134, 147, 160, 5,   18,  31,  44,  57,  70,  83,  96,  109, 122, 135,
                148, 161, 6,   19,  32,  45,  58,  71,  84,  97,  110, 123, 136, 149, 162, 7,   20,  33,  46,
                59,  72,  85,  98,  111, 124, 137, 150, 163, 8,   21,  34,  47,  60,  73,  86,  99,  112, 125,
                138, 151, 164, 9,   22,  35,  48,  61,  74,  87,  100, 113, 126, 139, 152, 165, 10,  23,  36,
                49,  62,  75,  88,  101, 114, 127, 140, 153, 166, 11,  24,  37,  50,  63,  76,  89,  102, 115,
                128, 141, 154, 167, 12,  25,  38,  51,  64,  77,  90,  103, 116, 129, 142, 155, 168};
      case 1:
        return {156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 143, 144, 145, 146, 147, 148,
                149, 150, 151, 152, 153, 154, 155, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141,
                142, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 104, 105, 106, 107, 108,
                109, 110, 111, 112, 113, 114, 115, 116, 91,  92,  93,  94,  95,  96,  97,  98,  99,  100, 101,
                102, 103, 78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  65,  66,  67,  68,
                69,  70,  71,  72,  73,  74,  75,  76,  77,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,
                62,  63,  64,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  26,  27,  28,
                29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  13,  14,  15,  16,  17,  18,  19,  20,  21,
                22,  23,  24,  25,  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12};
      case 2:
        return {168, 155, 142, 129, 116, 103, 90,  77,  64,  51,  38,  25,  12,  167, 154, 141, 128, 115, 102,
                89,  76,  63,  50,  37,  24,  11,  166, 153, 140, 127, 114, 101, 88,  75,  62,  49,  36,  23,
                10,  165, 152, 139, 126, 113, 100, 87,  74,  61,  48,  35,  22,  9,   164, 151, 138, 125, 112,
                99,  86,  73,  60,  47,  34,  21,  8,   163, 150, 137, 124, 111, 98,  85,  72,  59,  46,  33,
                20,  7,   162, 149, 136, 123, 110, 97,  84,  71,  58,  45,  32,  19,  6,   161, 148, 135, 122,
                109, 96,  83,  70,  57,  44,  31,  18,  5,   160, 147, 134, 121, 108, 95,  82,  69,  56,  43,
                30,  17,  4,   159, 146, 133, 120, 107, 94,  81,  68,  55,  42,  29,  16,  3,   158, 145, 132,
                119, 106, 93,  80,  67,  54,  41,  28,  15,  2,   157, 144, 131, 118, 105, 92,  79,  66,  53,
                40,  27,  14,  1,   156, 143, 130, 117, 104, 91,  78,  65,  52,  39,  26,  13,  0};
      case 3:
        return {12,  11,  10,  9,   8,   7,   6,   5,   4,   3,   2,   1,   0,   25,  24,  23,  22,  21,  20,
                19,  18,  17,  16,  15,  14,  13,  38,  37,  36,  35,  34,  33,  32,  31,  30,  29,  28,  27,
                26,  51,  50,  49,  48,  47,  46,  45,  44,  43,  42,  41,  40,  39,  64,  63,  62,  61,  60,
                59,  58,  57,  56,  55,  54,  53,  52,  77,  76,  75,  74,  73,  72,  71,  70,  69,  68,  67,
                66,  65,  90,  89,  88,  87,  86,  85,  84,  83,  82,  81,  80,  79,  78,  103, 102, 101, 100,
                99,  98,  97,  96,  95,  94,  93,  92,  91,  116, 115, 114, 113, 112, 111, 110, 109, 108, 107,
                106, 105, 104, 129, 128, 127, 126, 125, 124, 123, 122, 121, 120, 119, 118, 117, 142, 141, 140,
                139, 138, 137, 136, 135, 134, 133, 132, 131, 130, 155, 154, 153, 152, 151, 150, 149, 148, 147,
                146, 145, 144, 143, 168, 167, 166, 165, 164, 163, 162, 161, 160, 159, 158, 157, 156};
      }
    }
  }
  return {};
}

template <ElementEnum ElementType, int PolynomialOrder>
constexpr std::array<int, getElementNodeNumber<ElementType, PolynomialOrder>()>
getAdjacencyElementViewNodeSequenceInParent([[maybe_unused]] int parent, int sequence) {
  if constexpr (ElementType == ElementEnum::Point) {
    switch (sequence) {
    case 0:
      return {0};
    case 1:
      return {1};
    }
  }
  if constexpr (ElementType == ElementEnum::Line) {
    if (parent == getElementGmshTypeNumber<ElementEnum::Triangle, PolynomialOrder>()) {
      if constexpr (PolynomialOrder == 1) {
        switch (sequence) {
        case 0:
          return {0, 1};
        case 1:
          return {1, 2};
        case 2:
          return {2, 0};
        }
      }
      if constexpr (PolynomialOrder == 2) {
        switch (sequence) {
        case 0:
          return {0, 1, 3};
        case 1:
          return {1, 2, 4};
        case 2:
          return {2, 0, 5};
        }
      }
      if constexpr (PolynomialOrder == 3) {
        switch (sequence) {
        case 0:
          return {0, 1, 3, 4};
        case 1:
          return {1, 2, 5, 6};
        case 2:
          return {2, 0, 7, 8};
        }
      }
      if constexpr (PolynomialOrder == 4) {
        switch (sequence) {
        case 0:
          return {0, 1, 3, 4, 5};
        case 1:
          return {1, 2, 6, 7, 8};
        case 2:
          return {2, 0, 9, 10, 11};
        }
      }
      if constexpr (PolynomialOrder == 5) {
        switch (sequence) {
        case 0:
          return {0, 1, 3, 4, 5, 6};
        case 1:
          return {1, 2, 7, 8, 9, 10};
        case 2:
          return {2, 0, 11, 12, 13, 14};
        }
      }
    }
    if (parent == getElementGmshTypeNumber<ElementEnum::Quadrangle, PolynomialOrder>()) {
      if constexpr (PolynomialOrder == 1) {
        switch (sequence) {
        case 0:
          return {0, 1};
        case 1:
          return {1, 2};
        case 2:
          return {2, 3};
        case 3:
          return {3, 0};
        }
      }
      if constexpr (PolynomialOrder == 2) {
        switch (sequence) {
        case 0:
          return {0, 1, 4};
        case 1:
          return {1, 2, 5};
        case 2:
          return {2, 3, 6};
        case 3:
          return {3, 0, 7};
        }
      }
      if constexpr (PolynomialOrder == 3) {
        switch (sequence) {
        case 0:
          return {0, 1, 4, 5};
        case 1:
          return {1, 2, 6, 7};
        case 2:
          return {2, 3, 8, 9};
        case 3:
          return {3, 0, 10, 11};
        }
      }
      if constexpr (PolynomialOrder == 4) {
        switch (sequence) {
        case 0:
          return {0, 1, 4, 5, 6};
        case 1:
          return {1, 2, 7, 8, 9};
        case 2:
          return {2, 3, 10, 11, 12};
        case 3:
          return {3, 0, 13, 14, 15};
        }
      }
      if constexpr (PolynomialOrder == 5) {
        switch (sequence) {
        case 0:
          return {0, 1, 4, 5, 6, 7};
        case 1:
          return {1, 2, 8, 9, 10, 11};
        case 2:
          return {2, 3, 12, 13, 14, 15};
        case 3:
          return {3, 0, 16, 17, 18, 19};
        }
      }
    }
  }
  if constexpr (ElementType == ElementEnum::Triangle) {
    if (parent == getElementGmshTypeNumber<ElementEnum::Tetrahedron, PolynomialOrder>()) {
      if constexpr (PolynomialOrder == 1) {
        switch (sequence) {
        case 0:
          return {0, 2, 1};
        case 1:
          return {0, 1, 3};
        case 2:
          return {0, 3, 2};
        case 3:
          return {3, 1, 2};
        }
      }
      if constexpr (PolynomialOrder == 2) {
        switch (sequence) {
        case 0:
          return {0, 2, 1, 6, 5, 4};
        case 1:
          return {0, 1, 3, 4, 9, 7};
        case 2:
          return {0, 3, 2, 7, 8, 6};
        case 3:
          return {3, 1, 2, 9, 5, 8};
        }
      }
      if constexpr (PolynomialOrder == 3) {
        switch (sequence) {
        case 0:
          return {0, 2, 1, 9, 8, 7, 6, 5, 4, 16};
        case 1:
          return {0, 1, 3, 4, 5, 15, 14, 10, 11, 17};
        case 2:
          return {0, 3, 2, 11, 10, 12, 13, 8, 9, 18};
        case 3:
          return {3, 1, 2, 14, 15, 6, 7, 13, 12, 19};
        }
      }
      if constexpr (PolynomialOrder == 4) {
        switch (sequence) {
        case 0:
          return {0, 2, 1, 12, 11, 10, 9, 8, 7, 6, 5, 4, 22, 23, 24};
        case 1:
          return {0, 1, 3, 4, 5, 6, 21, 20, 19, 13, 14, 15, 25, 26, 27};
        case 2:
          return {0, 3, 2, 15, 14, 13, 16, 17, 18, 10, 11, 12, 28, 29, 30};
        case 3:
          return {3, 1, 2, 19, 20, 21, 7, 8, 9, 18, 17, 16, 31, 32, 33};
        }
      }
      if constexpr (PolynomialOrder == 5) {
        switch (sequence) {
        case 0:
          return {0, 2, 1, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 28, 29, 30, 31, 32, 33};
        case 1:
          return {0, 1, 3, 4, 5, 6, 7, 27, 26, 25, 24, 16, 17, 18, 19, 34, 35, 36, 37, 38, 39};
        case 2:
          return {0, 3, 2, 19, 18, 17, 16, 20, 21, 22, 23, 12, 13, 14, 15, 40, 41, 42, 43, 44, 45};
        case 3:
          return {3, 1, 2, 24, 25, 26, 27, 8, 9, 10, 11, 23, 22, 21, 20, 46, 47, 48, 49, 50, 51};
        }
      }
    }
    if (parent == getElementGmshTypeNumber<ElementEnum::Pyramid, PolynomialOrder>()) {
      if constexpr (PolynomialOrder == 1) {
        switch (sequence) {
        case 0:
          return {0, 1, 4};
        case 1:
          return {3, 0, 4};
        case 2:
          return {1, 2, 4};
        case 3:
          return {2, 3, 4};
        }
      }
      if constexpr (PolynomialOrder == 2) {
        switch (sequence) {
        case 0:
          return {0, 1, 4, 5, 9, 7};
        case 1:
          return {3, 0, 4, 6, 7, 12};
        case 2:
          return {1, 2, 4, 8, 11, 9};
        case 3:
          return {2, 3, 4, 10, 12, 11};
        }
      }
      if constexpr (PolynomialOrder == 3) {
        switch (sequence) {
        case 0:
          return {0, 1, 4, 5, 6, 13, 14, 10, 9, 21};
        case 1:
          return {3, 0, 4, 8, 7, 9, 10, 20, 19, 22};
        case 2:
          return {1, 2, 4, 11, 12, 17, 18, 14, 13, 23};
        case 3:
          return {2, 3, 4, 15, 16, 19, 20, 18, 17, 24};
        }
      }
      if constexpr (PolynomialOrder == 4) {
        switch (sequence) {
        case 0:
          return {0, 1, 4, 5, 6, 7, 17, 18, 19, 13, 12, 11, 29, 30, 31};
        case 1:
          return {3, 0, 4, 10, 9, 8, 11, 12, 13, 28, 27, 26, 32, 33, 34};
        case 2:
          return {1, 2, 4, 14, 15, 16, 23, 24, 25, 19, 18, 17, 35, 36, 37};
        case 3:
          return {2, 3, 4, 20, 21, 22, 26, 27, 28, 25, 24, 23, 38, 39, 40};
        }
      }
      if constexpr (PolynomialOrder == 5) {
        switch (sequence) {
        case 0:
          return {0, 1, 4, 5, 6, 7, 8, 21, 22, 23, 24, 16, 15, 14, 13, 37, 38, 39, 40, 41, 42};
        case 1:
          return {3, 0, 4, 12, 11, 10, 9, 13, 14, 15, 16, 36, 35, 34, 33, 43, 44, 45, 46, 47, 48};
        case 2:
          return {1, 2, 4, 17, 18, 19, 20, 29, 30, 31, 32, 24, 23, 22, 21, 49, 50, 51, 52, 53, 54};
        case 3:
          return {2, 3, 4, 25, 26, 27, 28, 33, 34, 35, 36, 32, 31, 30, 29, 55, 56, 57, 58, 59, 60};
        }
      }
    }
  }
  if constexpr (ElementType == ElementEnum::Quadrangle) {
    if (parent == getElementGmshTypeNumber<ElementEnum::Pyramid, PolynomialOrder>()) {
      if constexpr (PolynomialOrder == 1) {
        return {0, 3, 2, 1};
      }
      if constexpr (PolynomialOrder == 2) {
        return {0, 3, 2, 1, 6, 10, 8, 5, 13};
      }
      if constexpr (PolynomialOrder == 3) {
        return {0, 3, 2, 1, 7, 8, 16, 15, 12, 11, 6, 5, 25, 26, 27, 28};
      }
      if constexpr (PolynomialOrder == 4) {
        return {0, 3, 2, 1, 8, 9, 10, 22, 21, 20, 16, 15, 14, 7, 6, 5, 41, 42, 43, 44, 45, 46, 47, 48, 49};
      }
      if constexpr (PolynomialOrder == 5) {
        return {0, 3, 2,  1,  9,  10, 11, 12, 28, 27, 26, 25, 20, 19, 18, 17, 8,  7,
                6, 5, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76};
      }
    }
    if (parent == getElementGmshTypeNumber<ElementEnum::Hexahedron, PolynomialOrder>()) {
      if constexpr (PolynomialOrder == 1) {
        switch (sequence) {
        case 0:
          return {0, 3, 2, 1};
        case 1:
          return {0, 1, 5, 4};
        case 2:
          return {0, 4, 7, 3};
        case 3:
          return {1, 2, 6, 5};
        case 4:
          return {2, 3, 7, 6};
        case 5:
          return {4, 5, 6, 7};
        }
      }
      if constexpr (PolynomialOrder == 2) {
        switch (sequence) {
        case 0:
          return {0, 3, 2, 1, 9, 13, 11, 8, 20};
        case 1:
          return {0, 1, 5, 4, 8, 12, 16, 10, 21};
        case 2:
          return {0, 4, 7, 3, 10, 17, 15, 9, 22};
        case 3:
          return {1, 2, 6, 5, 11, 14, 18, 12, 23};
        case 4:
          return {2, 3, 7, 6, 13, 15, 19, 14, 24};
        case 5:
          return {4, 5, 6, 7, 16, 18, 19, 17, 25};
        }
      }
      if constexpr (PolynomialOrder == 3) {
        switch (sequence) {
        case 0:
          return {0, 3, 2, 1, 10, 11, 19, 18, 15, 14, 9, 8, 32, 33, 34, 35};
        case 1:
          return {0, 1, 5, 4, 8, 9, 16, 17, 25, 24, 13, 12, 36, 37, 38, 39};
        case 2:
          return {0, 4, 7, 3, 12, 13, 26, 27, 23, 22, 11, 10, 40, 41, 42, 43};
        case 3:
          return {1, 2, 6, 5, 14, 15, 20, 21, 29, 28, 17, 16, 44, 45, 46, 47};
        case 4:
          return {2, 3, 7, 6, 18, 19, 22, 23, 31, 30, 21, 20, 48, 49, 50, 51};
        case 5:
          return {4, 5, 6, 7, 24, 25, 28, 29, 30, 31, 27, 26, 52, 53, 54, 55};
        }
      }
      if constexpr (PolynomialOrder == 4) {
        switch (sequence) {
        case 0:
          return {0, 3, 2, 1, 11, 12, 13, 25, 24, 23, 19, 18, 17, 10, 9, 8, 44, 45, 46, 47, 48, 49, 50, 51, 52};
        case 1:
          return {0, 1, 5, 4, 8, 9, 10, 20, 21, 22, 34, 33, 32, 16, 15, 14, 53, 54, 55, 56, 57, 58, 59, 60, 61};
        case 2:
          return {0, 4, 7, 3, 14, 15, 16, 35, 36, 37, 31, 30, 29, 13, 12, 11, 62, 63, 64, 65, 66, 67, 68, 69, 70};
        case 3:
          return {1, 2, 6, 5, 17, 18, 19, 26, 27, 28, 40, 39, 38, 22, 21, 20, 71, 72, 73, 74, 75, 76, 77, 78, 79};
        case 4:
          return {2, 3, 7, 6, 23, 24, 25, 29, 30, 31, 43, 42, 41, 28, 27, 26, 80, 81, 82, 83, 84, 85, 86, 87, 88};
        case 5:
          return {4, 5, 6, 7, 32, 33, 34, 38, 39, 40, 41, 42, 43, 37, 36, 35, 89, 90, 91, 92, 93, 94, 95, 96, 97};
        }
      }
      if constexpr (PolynomialOrder == 5) {
        switch (sequence) {
        case 0:
          return {0, 3, 2,  1,  12, 13, 14, 15, 31, 30, 29, 28, 23, 22, 21, 20, 11, 10,
                  9, 8, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71};
        case 1:
          return {0,  1,  5,  4,  8,  9,  10, 11, 24, 25, 26, 27, 43, 42, 41, 40, 19, 18,
                  17, 16, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87};
        case 2:
          return {0,  4,  7,  3,  16, 17, 18, 19, 44, 45, 46, 47, 39, 38, 37,  36,  15,  14,
                  13, 12, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103};
        case 3:
          return {1,  2,  6,   5,   20,  21,  22,  23,  32,  33,  34,  35,  51,  50,  49,  48,  27,  26,
                  25, 24, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119};
        case 4:
          return {2,  3,  7,   6,   28,  29,  30,  31,  36,  37,  38,  39,  55,  54,  53,  52,  35,  34,
                  33, 32, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135};
        case 5:
          return {4,  5,  6,   7,   40,  41,  42,  43,  48,  49,  50,  51,  52,  53,  54,  55,  47,  46,
                  45, 44, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151};
        }
      }
    }
  }
  return {};
}

template <ElementEnum ElementType>
consteval int getElementVtkElementNumber() {
  if constexpr (ElementType == ElementEnum::Pyramid) {
    return 2;
  } else {
    return 1;
  }
}

template <ElementEnum ElementType>
consteval std::array<int, getElementVtkElementNumber<ElementType>()> getElementVtkTypeNumber() {
  if constexpr (ElementType == ElementEnum::Line) {
    return {68};
  }
  if constexpr (ElementType == ElementEnum::Triangle) {
    return {69};
  }
  if constexpr (ElementType == ElementEnum::Quadrangle) {
    return {70};
  }
  if constexpr (ElementType == ElementEnum::Tetrahedron) {
    return {71};
  }
  if constexpr (ElementType == ElementEnum::Pyramid) {
    return {71, 71};
  }
  if constexpr (ElementType == ElementEnum::Hexahedron) {
    return {72};
  }
}

template <ElementEnum ElementType, int PolynomialOrder>
consteval std::array<int, getElementVtkElementNumber<ElementType>()> getElementVtkPerNodeNumber() {
  if constexpr (ElementType == ElementEnum::Pyramid) {
    return {getElementNodeNumber<ElementEnum::Tetrahedron, PolynomialOrder>(),
            getElementNodeNumber<ElementEnum::Tetrahedron, PolynomialOrder>()};
  } else {
    return {getElementNodeNumber<ElementType, PolynomialOrder>()};
  }
}

template <ElementEnum ElementType, int PolynomialOrder>
consteval int getElementVtkAllNodeNumber() {
  constexpr std::array<int, getElementVtkElementNumber<ElementType>()> kElementVtkPerNodeNumber{
      getElementVtkPerNodeNumber<ElementType, PolynomialOrder>()};
  return std::accumulate(kElementVtkPerNodeNumber.begin(), kElementVtkPerNodeNumber.end(), 0);
}

template <ElementEnum ElementType, int PolynomialOrder>
consteval std::array<int, getElementVtkAllNodeNumber<ElementType, PolynomialOrder>()> getElementVTKConnectivity() {
  if constexpr (ElementType == ElementEnum::Line) {
    if constexpr (PolynomialOrder == 1) {
      return {0, 1};
    }
    if constexpr (PolynomialOrder == 2) {
      return {0, 1, 2};
    }
    if constexpr (PolynomialOrder == 3) {
      return {0, 1, 2, 3};
    }
    if constexpr (PolynomialOrder == 4) {
      return {0, 1, 2, 3, 4};
    }
    if constexpr (PolynomialOrder == 5) {
      return {0, 1, 2, 3, 4, 5};
    }
  }
  if constexpr (ElementType == ElementEnum::Triangle) {
    if constexpr (PolynomialOrder == 1) {
      return {0, 1, 2};
    }
    if constexpr (PolynomialOrder == 2) {
      return {0, 1, 2, 3, 4, 5};
    }
    if constexpr (PolynomialOrder == 3) {
      return {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    }
    if constexpr (PolynomialOrder == 4) {
      return {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
    }
    if constexpr (PolynomialOrder == 5) {
      return {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
    }
  }
  if constexpr (ElementType == ElementEnum::Quadrangle) {
    if constexpr (PolynomialOrder == 1) {
      return {0, 1, 2, 3};
    }
    if constexpr (PolynomialOrder == 2) {
      return {0, 1, 2, 3, 4, 5, 6, 7, 8};
    }
    if constexpr (PolynomialOrder == 3) {
      return {0, 1, 2, 3, 4, 5, 6, 7, 9, 8, 11, 10, 12, 13, 15, 14};
    }
    if constexpr (PolynomialOrder == 4) {
      return {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 11, 10, 15, 14, 13, 16, 20, 17, 23, 24, 21, 19, 22, 18};
    }
    if constexpr (PolynomialOrder == 5) {
      return {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 15, 14, 13, 12, 19, 18,
              17, 16, 20, 24, 25, 21, 31, 32, 33, 26, 30, 35, 34, 27, 23, 29, 28, 22};
    }
  }
  if constexpr (ElementType == ElementEnum::Tetrahedron) {
    if constexpr (PolynomialOrder == 1) {
      return {0, 1, 2, 3};
    }
    if constexpr (PolynomialOrder == 2) {
      return {0, 1, 2, 3, 4, 5, 6, 7, 9, 8};
    }
    if constexpr (PolynomialOrder == 3) {
      return {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 10, 15, 14, 13, 12, 17, 19, 18, 16};
    }
    if constexpr (PolynomialOrder == 4) {
      return {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 15, 14, 13, 21, 20,
              19, 18, 17, 16, 25, 26, 27, 33, 31, 32, 28, 29, 30, 22, 23, 24, 34};
    }
    if constexpr (PolynomialOrder == 5) {
      return {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 19, 18, 17,
              16, 27, 26, 25, 24, 23, 22, 21, 20, 34, 35, 36, 37, 38, 39, 48, 46, 47, 51,
              49, 50, 40, 41, 42, 43, 44, 45, 28, 29, 30, 31, 32, 33, 52, 53, 54, 55};
    }
  }
  if constexpr (ElementType == ElementEnum::Pyramid) {
    if constexpr (PolynomialOrder == 1) {
      return {0, 1, 3, 4, 2, 3, 1, 4};
    }
    if constexpr (PolynomialOrder == 2) {
      return {0, 1, 3, 4, 5, 13, 6, 7, 9, 12, 2, 3, 1, 4, 10, 13, 8, 11, 12, 9};
    }
    if constexpr (PolynomialOrder == 3) {
      return {0, 1, 3, 4, 5,  6,  28, 26, 8,  7,  9,  10, 13, 14, 19, 20, 21, 29, 22, 25,
              2, 3, 1, 4, 15, 16, 26, 28, 11, 12, 17, 18, 19, 20, 13, 14, 24, 29, 23, 27};
    }
    if constexpr (PolynomialOrder == 4) {
      return {0,  1,  3,  4,  5,  6,  7,  44, 49, 42, 10, 9,  8,  11, 12, 13, 17, 18, 19, 26, 27, 28, 29, 30,
              31, 53, 54, 51, 33, 34, 32, 41, 45, 48, 50, 2,  3,  1,  4,  20, 21, 22, 42, 49, 44, 14, 15, 16,
              23, 24, 25, 26, 27, 28, 17, 18, 19, 38, 39, 40, 51, 54, 53, 36, 37, 35, 43, 47, 46, 52};
    }
    if constexpr (PolynomialOrder == 5) {
      return {0,  1,  3,  4,  5,  6,  7,  8,  64, 76, 74, 62, 12, 11, 10, 9,  13, 14, 15, 16, 21, 22, 23,
              24, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 80, 81, 78, 89, 86, 90, 44, 45, 43, 47, 48, 46,
              61, 66, 71, 65, 73, 72, 77, 82, 83, 84, 2,  3,  1,  4,  25, 26, 27, 28, 62, 74, 76, 64, 17,
              18, 19, 20, 29, 30, 31, 32, 33, 34, 35, 36, 21, 22, 23, 24, 55, 56, 57, 58, 59, 60, 78, 81,
              80, 86, 89, 90, 50, 51, 49, 53, 54, 52, 63, 70, 67, 69, 75, 68, 79, 87, 85, 88};
    }
  }
  if constexpr (ElementType == ElementEnum::Hexahedron) {
    if constexpr (PolynomialOrder == 1) {
      return {0, 1, 2, 3, 4, 5, 6, 7};
    }
    if constexpr (PolynomialOrder == 2) {
      return {0, 1, 2, 3, 4, 5, 6, 7, 8, 11, 13, 9, 16, 18, 19, 17, 10, 12, 15, 14, 22, 23, 21, 24, 20, 25, 26};
    }
    if constexpr (PolynomialOrder == 3) {
      return {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  14, 15, 19, 18, 10, 11, 24, 25, 28, 29, 31, 30,
              26, 27, 12, 13, 16, 17, 22, 23, 20, 21, 40, 43, 41, 42, 44, 45, 47, 46, 36, 37, 39, 38,
              49, 48, 50, 51, 32, 35, 33, 34, 52, 53, 55, 54, 56, 57, 59, 58, 60, 61, 63, 62};
    }
    if constexpr (PolynomialOrder == 4) {
      return {0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  17,  18,  19,  25,  24,  23,  11,  12,  13,  32,
              33,  34,  38,  39,  40,  43,  42,  41,  35,  36,  37,  14,  15,  16,  20,  21,  22,  29,  30,  31,  26,
              27,  28,  62,  69,  65,  66,  70,  68,  63,  67,  64,  71,  75,  72,  78,  79,  76,  74,  77,  73,  53,
              57,  54,  60,  61,  58,  56,  59,  55,  81,  84,  80,  85,  88,  87,  82,  86,  83,  44,  51,  47,  48,
              52,  50,  45,  49,  46,  89,  93,  90,  96,  97,  94,  92,  95,  91,  98,  106, 99,  107, 118, 109, 101,
              111, 100, 108, 119, 110, 120, 124, 121, 113, 122, 112, 102, 114, 103, 115, 123, 116, 105, 117, 104};
    }
    if constexpr (PolynomialOrder == 5) {
      return {0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  20,  21,  22,  23,  31,  30,  29,  28,
              12,  13,  14,  15,  40,  41,  42,  43,  48,  49,  50,  51,  55,  54,  53,  52,  44,  45,  46,  47,
              16,  17,  18,  19,  24,  25,  26,  27,  36,  37,  38,  39,  32,  33,  34,  35,  88,  99,  98,  91,
              92,  100, 103, 97,  93,  101, 102, 96,  89,  94,  95,  90,  104, 108, 109, 105, 115, 116, 117, 110,
              114, 119, 118, 111, 107, 113, 112, 106, 72,  76,  77,  73,  83,  84,  85,  78,  82,  87,  86,  79,
              75,  81,  80,  74,  121, 125, 124, 120, 126, 133, 132, 131, 127, 134, 135, 130, 122, 128, 129, 123,
              56,  67,  66,  59,  60,  68,  71,  65,  61,  69,  70,  64,  57,  62,  63,  58,  136, 140, 141, 137,
              147, 148, 149, 142, 146, 151, 150, 143, 139, 145, 144, 138, 152, 160, 161, 153, 162, 184, 187, 166,
              163, 185, 186, 167, 155, 171, 170, 154, 164, 188, 189, 168, 192, 208, 209, 196, 195, 211, 210, 197,
              174, 201, 200, 172, 165, 191, 190, 169, 193, 212, 213, 199, 194, 215, 214, 198, 175, 202, 203, 173,
              156, 176, 177, 157, 178, 204, 205, 180, 179, 207, 206, 181, 159, 183, 182, 158};
    }
  }
}

template <ElementEnum ElementType, int PolynomialOrder>
struct ElementTrait {
  static constexpr int kDimension{getElementDimension<ElementType>()};
  static constexpr ElementEnum kElementType{ElementType};
  static constexpr int kPolynomialOrder{PolynomialOrder};
  static constexpr int kGmshTypeNumber{getElementGmshTypeNumber<ElementType, PolynomialOrder>()};
  static constexpr int kBasicNodeNumber{getElementNodeNumber<ElementType, 1>()};
  static constexpr int kAllNodeNumber{getElementNodeNumber<ElementType, PolynomialOrder>()};
  static constexpr int kP1BasisFunctionNumber{getElementBasisFunctionNumber<ElementType, 1>()};
  static constexpr int kAllBasisFunctionNumber{getElementBasisFunctionNumber<ElementType, PolynomialOrder>()};
  static constexpr int kVtkElementNumber{getElementVtkElementNumber<ElementType>()};
  static constexpr int kVtkAllNodeNumber{getElementVtkAllNodeNumber<ElementType, PolynomialOrder>()};
};

template <ElementEnum ElementType, int PolynomialOrder>
struct VolumeElementTrait : ElementTrait<ElementType, PolynomialOrder> {
  static constexpr int kQuadratureOrder{getVolumeElementQuadratureOrder<PolynomialOrder>()};
  static constexpr int kQuadratureNumber{getVolumeElementQuadratureNumber<ElementType, PolynomialOrder>()};
  static constexpr int kAdjacencyNumber{getVolumeElementAdjacencyNumber<ElementType>()};
  static constexpr int kAllAdjacencyNodeNumber{getVolumeElementAllAdjacencyNodeNumber<ElementType>()};
  static constexpr int kAllAdjacencyQuadratureNumber{
      getVolumeElementAllAdjacencyQuadratureNumber<ElementType, PolynomialOrder>()};
};

template <ElementEnum ElementType, int PolynomialOrder>
struct AdjacencyElementTrait : ElementTrait<ElementType, PolynomialOrder> {
  static constexpr int kQuadratureOrder{getAdjacencyElementQuadratureOrder<PolynomialOrder>()};
  static constexpr int kQuadratureNumber{getAdjacencyElementQuadratureNumber<ElementType, PolynomialOrder>()};
};

template <int PolynomialOrder>
using VolumeLineTrait = VolumeElementTrait<ElementEnum::Line, PolynomialOrder>;

template <int PolynomialOrder>
using VolumeTriangleTrait = VolumeElementTrait<ElementEnum::Triangle, PolynomialOrder>;

template <int PolynomialOrder>
using VolumeQuadrangleTrait = VolumeElementTrait<ElementEnum::Quadrangle, PolynomialOrder>;

template <int PolynomialOrder>
using VolumeTetrahedronTrait = VolumeElementTrait<ElementEnum::Tetrahedron, PolynomialOrder>;

template <int PolynomialOrder>
using VolumePyramidTrait = VolumeElementTrait<ElementEnum::Pyramid, PolynomialOrder>;

template <int PolynomialOrder>
using VolumeHexahedronTrait = VolumeElementTrait<ElementEnum::Hexahedron, PolynomialOrder>;

template <int PolynomialOrder>
using AdjacencyPointTrait = AdjacencyElementTrait<ElementEnum::Point, PolynomialOrder>;

template <int PolynomialOrder>
using AdjacencyLineTrait = AdjacencyElementTrait<ElementEnum::Line, PolynomialOrder>;

template <int PolynomialOrder>
using AdjacencyTriangleTrait = AdjacencyElementTrait<ElementEnum::Triangle, PolynomialOrder>;

template <int PolynomialOrder>
using AdjacencyQuadrangleTrait = AdjacencyElementTrait<ElementEnum::Quadrangle, PolynomialOrder>;

template <int Dimension, EquationModelEnum EquationModelType>
consteval int getConservedVariableNumber() {
  if constexpr (EquationModelType == EquationModelEnum::CompressibleEuler ||
                EquationModelType == EquationModelEnum::CompressibleNS ||
                EquationModelType == EquationModelEnum::IncompressibleEuler ||
                EquationModelType == EquationModelEnum::IncompressibleNS) {
    return Dimension + 2;
  }
}

template <int Dimension, EquationModelEnum EquationModelType, TurbulenceModelEnum TurbulenceModelType>
  requires(EquationModelType == EquationModelEnum::CompressibleRANS)
consteval int getConservedVariableNumber() {
  if constexpr (TurbulenceModelType == TurbulenceModelEnum::SA) {
    return Dimension + 3;
  }
}

template <int Dimension, EquationModelEnum EquationModelType>
consteval int getComputationalVariableNumber() {
  if constexpr (EquationModelType == EquationModelEnum::CompressibleEuler ||
                EquationModelType == EquationModelEnum::CompressibleNS ||
                EquationModelType == EquationModelEnum::IncompressibleEuler ||
                EquationModelType == EquationModelEnum::IncompressibleNS) {
    return Dimension + 3;
  }
}

template <int Dimension, EquationModelEnum EquationModelType, TurbulenceModelEnum TurbulenceModelType>
  requires(EquationModelType == EquationModelEnum::CompressibleRANS)
consteval int getComputationalVariableNumber() {
  if constexpr (TurbulenceModelType == TurbulenceModelEnum::SA) {
    return Dimension + 4;
  }
}

template <int Dimension, EquationModelEnum EquationModelType>
consteval int getPrimitiveVariableNumber() {
  if constexpr (EquationModelType == EquationModelEnum::CompressibleEuler ||
                EquationModelType == EquationModelEnum::CompressibleNS ||
                EquationModelType == EquationModelEnum::IncompressibleEuler ||
                EquationModelType == EquationModelEnum::IncompressibleNS) {
    return Dimension + 2;
  }
}

template <int Dimension, EquationModelEnum EquationModelType, TurbulenceModelEnum TurbulenceModelType>
  requires(EquationModelType == EquationModelEnum::CompressibleRANS)
consteval int getPrimitiveVariableNumber() {
  if constexpr (TurbulenceModelType == TurbulenceModelEnum::SA) {
    return Dimension + 3;
  }
}

template <DimensionEnum Dimension, PolynomialOrderEnum PolynomialOrder, BoundaryTimeEnum BoundaryTimeType,
          SourceTermEnum SourceTermType>
struct SolveControl {
  static constexpr int kDimension{magic_enum::enum_integer(Dimension)};
  static constexpr int kPolynomialOrder{magic_enum::enum_integer(PolynomialOrder)};
  static constexpr BoundaryTimeEnum kBoundaryTime{BoundaryTimeType};
  static constexpr SourceTermEnum kSourceTerm{SourceTermType};
};

template <MeshModelEnum MeshModelType, InitialConditionEnum InitialConditionType,
          TimeIntegrationEnum TimeIntegrationType>
struct NumericalControl {
  static constexpr MeshModelEnum kMeshModel{MeshModelType};
  static constexpr InitialConditionEnum kInitialCondition{InitialConditionType};
  static constexpr TimeIntegrationEnum kTimeIntegration{TimeIntegrationType};
};

template <ThermodynamicModelEnum ThermodynamicModelType, EquationOfStateEnum EquationOfStateType,
          ConvectiveFluxEnum ConvectiveFluxType>
struct CompressibleEulerVariable {
  static constexpr EquationModelEnum kEquationModel{EquationModelEnum::CompressibleEuler};
  static constexpr ThermodynamicModelEnum kThermodynamicModel{ThermodynamicModelType};
  static constexpr EquationOfStateEnum kEquationOfState{EquationOfStateType};
  static constexpr TransportModelEnum kTransportModel{TransportModelEnum::None};
  static constexpr ConvectiveFluxEnum kConvectiveFlux{ConvectiveFluxType};
};

template <ThermodynamicModelEnum ThermodynamicModelType, EquationOfStateEnum EquationOfStateType,
          TransportModelEnum TransportModelType, ConvectiveFluxEnum ConvectiveFluxType, ViscousFluxEnum ViscousFluxType>
struct CompressibleNSVariable {
  static constexpr EquationModelEnum kEquationModel{EquationModelEnum::CompressibleNS};
  static constexpr ThermodynamicModelEnum kThermodynamicModel{ThermodynamicModelType};
  static constexpr EquationOfStateEnum kEquationOfState{EquationOfStateType};
  static constexpr TransportModelEnum kTransportModel{TransportModelType};
  static constexpr ConvectiveFluxEnum kConvectiveFlux{ConvectiveFluxType};
  static constexpr ViscousFluxEnum kViscousFlux{ViscousFluxType};
};

template <ThermodynamicModelEnum ThermodynamicModelType, EquationOfStateEnum EquationOfStateType,
          ConvectiveFluxEnum ConvectiveFluxType>
struct IncompressibleEulerVariable {
  static constexpr EquationModelEnum kEquationModel{EquationModelEnum::IncompressibleEuler};
  static constexpr ThermodynamicModelEnum kThermodynamicModel{ThermodynamicModelType};
  static constexpr EquationOfStateEnum kEquationOfState{EquationOfStateType};
  static constexpr TransportModelEnum kTransportModel{TransportModelEnum::None};
  static constexpr ConvectiveFluxEnum kConvectiveFlux{ConvectiveFluxType};
};

template <ThermodynamicModelEnum ThermodynamicModelType, EquationOfStateEnum EquationOfStateType,
          TransportModelEnum TransportModelType, ConvectiveFluxEnum ConvectiveFluxType, ViscousFluxEnum ViscousFluxType>
struct IncompressibleNSVariable {
  static constexpr EquationModelEnum kEquationModel{EquationModelEnum::IncompressibleNS};
  static constexpr ThermodynamicModelEnum kThermodynamicModel{ThermodynamicModelType};
  static constexpr EquationOfStateEnum kEquationOfState{EquationOfStateType};
  static constexpr TransportModelEnum kTransportModel{TransportModelType};
  static constexpr ConvectiveFluxEnum kConvectiveFlux{ConvectiveFluxType};
  static constexpr ViscousFluxEnum kViscousFlux{ViscousFluxType};
};

template <ThermodynamicModelEnum ThermodynamicModelType, EquationOfStateEnum EquationOfStateType,
          TransportModelEnum TransportModelType, TurbulenceModelEnum TurbulenceModelType,
          ConvectiveFluxEnum ConvectiveFluxType, ViscousFluxEnum ViscousFluxType>
struct CompressibleRANSVariable {
  static constexpr EquationModelEnum kEquationModel{EquationModelEnum::CompressibleRANS};
  static constexpr ThermodynamicModelEnum kThermodynamicModel{ThermodynamicModelType};
  static constexpr EquationOfStateEnum kEquationOfState{EquationOfStateType};
  static constexpr TransportModelEnum kTransportModel{TransportModelType};
  static constexpr TurbulenceModelEnum kTurbulenceModel{TurbulenceModelType};
  static constexpr ConvectiveFluxEnum kConvectiveFlux{ConvectiveFluxType};
  static constexpr ViscousFluxEnum kViscousFlux{ViscousFluxType};
};

template <typename SolveControl, typename NumericalControl, typename EquationVariable>
struct SimulationControl : SolveControl, NumericalControl, EquationVariable {
  static constexpr int kConservedVariableNumber{
      getConservedVariableNumber<SolveControl::kDimension, EquationVariable::kEquationModel>()};
  static constexpr int kComputationalVariableNumber{
      getComputationalVariableNumber<SolveControl::kDimension, EquationVariable::kEquationModel>()};
  static constexpr int kPrimitiveVariableNumber{
      getPrimitiveVariableNumber<SolveControl::kDimension, EquationVariable::kEquationModel>()};
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_SIMULATION_CONTROL_CPP_
