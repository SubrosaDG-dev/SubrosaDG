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
inline constexpr std::array<int, 5> kTetrahedronGmshTypeNumber{4, 11, 29, 30, 31};
inline constexpr std::array<int, 5> kPyramidGmshTypeNumber{7, 14, 118, 119, 120};
inline constexpr std::array<int, 5> kHexahedronGmshTypeNumber{5, 12, 92, 93, 94};

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

template <ElementEnum ElementType, int PolynomialOrder>
inline consteval int getElementGmshTypeNumber() {
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
inline consteval int getElementNodeNumber() {
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
inline consteval int getElementAllAdjacencyNodeNumber() {
  constexpr std::array<int, getElementAdjacencyNumber<ElementType>()> kElementPerAdjacencyNodeNumber{
      getElementPerAdjacencyNodeNumber<ElementType>()};
  return std::accumulate(kElementPerAdjacencyNodeNumber.begin(), kElementPerAdjacencyNodeNumber.end(), 0);
}

template <ElementEnum ElementType>
inline consteval std::array<int, getElementAllAdjacencyNodeNumber<ElementType>()> getElementPerAdjacencyNodeIndex() {
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
inline consteval Real getElementMeasure() {
  if constexpr (ElementType == ElementEnum::Point) {
    return 1.0;
  }
  if constexpr (ElementType == ElementEnum::Line) {
    return 2.0;
  }
  if constexpr (ElementType == ElementEnum::Triangle) {
    return 0.5;
  }
  if constexpr (ElementType == ElementEnum::Quadrangle) {
    return 4.0;
  }
  if constexpr (ElementType == ElementEnum::Tetrahedron) {
    return 1.0 / 6.0;
  }
  if constexpr (ElementType == ElementEnum::Pyramid) {
    return 1.0 / 3.0;
  }
  if constexpr (ElementType == ElementEnum::Hexahedron) {
    return 8.0;
  }
}

template <ElementEnum ElementType, int PolynomialOrder>
inline consteval int getElementBasisFunctionNumber() {
  return getElementNodeNumber<ElementType, PolynomialOrder>();
}

template <ElementEnum ElementType, int PolynomialOrder>
inline constexpr std::array<int, getElementBasisFunctionNumber<ElementType, PolynomialOrder>()>
getAdjacencyElementBasisFunctionSequence([[maybe_unused]] int parent, int sequence) {
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

inline constexpr std::array<int, 12> kLineQuadratureNumber{1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6};
inline constexpr std::array<int, 12> kTriangleQuadratureNumber{1, 1, 3, 4, 6, 7, 12, 13, 16, 19, 25, 27};
inline constexpr std::array<int, 12> kQuadrangleQuadratureNumber{1, 3, 7, 4, 9, 9, 16, 16, 25, 25, 36, 36};
inline constexpr std::array<int, 12> kTetrahedronQuadratureNumber{1, 1, 4, 5, 11, 14, 24, 31, 43, 53, 126, 126};
inline constexpr std::array<int, 12> kPyramidQuadratureNumber{1, 1, 8, 8, 27, 27, 64, 64, 125, 125, 216, 216};
inline constexpr std::array<int, 12> kHexahedronQuadratureNumber{1, 6, 8, 8, 27, 27, 64, 64, 125, 125, 216, 216};

template <int PolynomialOrder>
inline consteval int getElementQuadratureOrder() {
  return 2 * PolynomialOrder;
}

template <int PolynomialOrder>
inline consteval int getAdjacencyElementQuadratureOrder() {
  return 2 * PolynomialOrder + 1;
}

template <ElementEnum ElementType, int PolynomialOrder>
inline consteval int getElementQuadratureNumber() {
  if constexpr (ElementType == ElementEnum::Point) {
    return 1;
  }
  if constexpr (ElementType == ElementEnum::Line) {
    return kLineQuadratureNumber[static_cast<Usize>(getElementQuadratureOrder<PolynomialOrder>())];
  }
  if constexpr (ElementType == ElementEnum::Triangle) {
    return kTriangleQuadratureNumber[static_cast<Usize>(getElementQuadratureOrder<PolynomialOrder>())];
  }
  if constexpr (ElementType == ElementEnum::Quadrangle) {
    return kQuadrangleQuadratureNumber[static_cast<Usize>(getElementQuadratureOrder<PolynomialOrder>())];
  }
  if constexpr (ElementType == ElementEnum::Tetrahedron) {
    return kTetrahedronQuadratureNumber[static_cast<Usize>(getElementQuadratureOrder<PolynomialOrder>())];
  }
  if constexpr (ElementType == ElementEnum::Pyramid) {
    return kPyramidQuadratureNumber[static_cast<Usize>(getElementQuadratureOrder<PolynomialOrder>())];
  }
  if constexpr (ElementType == ElementEnum::Hexahedron) {
    return kHexahedronQuadratureNumber[static_cast<Usize>(getElementQuadratureOrder<PolynomialOrder>())];
  }
}

template <ElementEnum ElementType, int PolynomialOrder>
inline consteval int getAdjacencyElementQuadratureNumber() {
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
inline consteval std::array<int, getElementAdjacencyNumber<ElementType>()> getElementPerAdjacencyQuadratureNumber() {
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
inline consteval int getElementAllAdjacencyQuadratureNumber() {
  constexpr std::array<int, getElementAdjacencyNumber<ElementType>()> kElementPerAdjacencyQuadratureNumber{
      getElementPerAdjacencyQuadratureNumber<ElementType, PolynomialOrder>()};
  return std::accumulate(kElementPerAdjacencyQuadratureNumber.begin(), kElementPerAdjacencyQuadratureNumber.end(), 0);
}

template <ElementEnum ElementType, int PolynomialOrder>
inline consteval std::array<int, getElementAdjacencyNumber<ElementType>() + 1>
getElementAccumulateAdjacencyQuadratureNumber() {
  std::array<int, getElementAdjacencyNumber<ElementType>() + 1> accumulate_adjacency_quadrature_number{};
  accumulate_adjacency_quadrature_number[0] = 0;
  for (int i = 0; i < getElementAdjacencyNumber<ElementType>(); ++i) {
    accumulate_adjacency_quadrature_number[static_cast<Usize>(i) + 1] =
        accumulate_adjacency_quadrature_number[static_cast<Usize>(i)] +
        getElementPerAdjacencyQuadratureNumber<ElementType, PolynomialOrder>()[static_cast<Usize>(i)];
  }
  return accumulate_adjacency_quadrature_number;
}

template <ElementEnum ElementType, int PolynomialOrder>
inline constexpr std::array<int, getAdjacencyElementQuadratureNumber<ElementType, PolynomialOrder>()>
getAdjacencyElementQuadratureSequence([[maybe_unused]] int rotation) {
  if constexpr (ElementType == ElementEnum::Point) {
    return {0};
  }
  if constexpr (ElementType == ElementEnum::Line) {
    if constexpr (PolynomialOrder == 1) {
      return {1, 0};
    }
    if constexpr (PolynomialOrder == 2) {
      return {2, 1, 0};
    }
    if constexpr (PolynomialOrder == 3) {
      return {3, 2, 1, 0};
    }
    if constexpr (PolynomialOrder == 4) {
      return {4, 3, 2, 1, 0};
    }
    if constexpr (PolynomialOrder == 5) {
      return {5, 4, 3, 2, 1, 0};
    }
  }
  if constexpr (ElementType == ElementEnum::Triangle) {
    if constexpr (PolynomialOrder == 1) {
      switch (rotation) {
      case 0:
        return {0, 1, 3, 2};
      case 1:
        return {0, 3, 2, 1};
      case 2:
        return {0, 2, 1, 3};
      }
    }
    if constexpr (PolynomialOrder == 2) {
      switch (rotation) {
      case 0:
        return {0, 1, 3, 2, 4, 6, 5};
      case 1:
        return {0, 3, 2, 1, 6, 5, 4};
      case 2:
        return {0, 2, 1, 3, 5, 4, 6};
      }
    }
    if constexpr (PolynomialOrder == 3) {
      switch (rotation) {
      case 0:
        return {0, 1, 3, 2, 4, 6, 5, 11, 12, 10, 9, 7, 8};
      case 1:
        return {0, 3, 2, 1, 6, 5, 4, 12, 10, 11, 8, 9, 7};
      case 2:
        return {0, 2, 1, 3, 5, 4, 6, 10, 11, 12, 7, 8, 9};
      }
    }
    if constexpr (PolynomialOrder == 4) {
      switch (rotation) {
      case 0:
        return {0, 1, 3, 2, 4, 6, 5, 7, 9, 8, 10, 12, 11, 17, 18, 16, 15, 13, 14};
      case 1:
        return {0, 3, 2, 1, 6, 5, 4, 9, 8, 7, 12, 11, 10, 18, 16, 17, 14, 15, 13};
      case 2:
        return {0, 2, 1, 3, 5, 4, 6, 8, 7, 9, 11, 10, 12, 16, 17, 18, 13, 14, 15};
      }
    }
    if constexpr (PolynomialOrder == 5) {
      switch (rotation) {
      case 0:
        return {0, 2, 1, 3, 5, 4, 6, 8, 7, 9, 11, 10, 12, 14, 13, 19, 20, 18, 17, 15, 16, 25, 26, 24, 23, 21, 22};
      case 1:
        return {1, 0, 2, 4, 3, 5, 7, 6, 8, 10, 9, 11, 13, 12, 14, 18, 19, 20, 15, 16, 17, 24, 25, 26, 21, 22, 23};
      case 2:
        return {2, 1, 0, 5, 4, 3, 8, 7, 6, 11, 10, 9, 14, 13, 12, 20, 18, 19, 16, 17, 15, 26, 24, 25, 22, 23, 21};
      }
    }
  }
  if constexpr (ElementType == ElementEnum::Quadrangle) {
    if constexpr (PolynomialOrder == 1) {
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
    }
    if constexpr (PolynomialOrder == 2) {
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
    }
    if constexpr (PolynomialOrder == 3) {
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
    }
    if constexpr (PolynomialOrder == 4) {
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
    }
    if constexpr (PolynomialOrder == 5) {
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
    }
  }
  return {};
}

template <ElementEnum ElementType>
inline consteval int getElementVtkElementNumber() {
  if constexpr (ElementType == ElementEnum::Pyramid) {
    return 2;
  } else {
    return 1;
  }
}

template <ElementEnum ElementType>
inline consteval std::array<int, getElementVtkElementNumber<ElementType>()> getElementVtkTypeNumber() {
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
inline consteval std::array<int, getElementVtkElementNumber<ElementType>()> getElementVtkPerNodeNumber() {
  if constexpr (ElementType == ElementEnum::Pyramid) {
    return {getElementNodeNumber<ElementEnum::Tetrahedron, PolynomialOrder>(),
            getElementNodeNumber<ElementEnum::Tetrahedron, PolynomialOrder>()};
  } else {
    return {getElementNodeNumber<ElementType, PolynomialOrder>()};
  }
}

template <ElementEnum ElementType, int PolynomialOrder>
inline consteval int getElementVtkAllNodeNumber() {
  constexpr std::array<int, getElementVtkElementNumber<ElementType>()> kElementVtkPerNodeNumber{
      getElementVtkPerNodeNumber<ElementType, PolynomialOrder>()};
  return std::accumulate(kElementVtkPerNodeNumber.begin(), kElementVtkPerNodeNumber.end(), 0);
}

template <ElementEnum ElementType, int PolynomialOrder>
inline consteval std::array<int, getElementVtkAllNodeNumber<ElementType, PolynomialOrder>()>
getElementVTKConnectivity() {
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

template <ElementEnum ElementType, int PolynomialOrder>
struct ElementTraitBase {
  inline static constexpr int kDimension{getElementDimension<ElementType>()};
  inline static constexpr ElementEnum kElementType{ElementType};
  inline static constexpr int kPolynomialOrder{PolynomialOrder};
  inline static constexpr int kGmshTypeNumber{getElementGmshTypeNumber<ElementType, PolynomialOrder>()};
  inline static constexpr int kVtkElementNumber{getElementVtkElementNumber<ElementType>()};
  inline static constexpr int kVtkAllNodeNumber{getElementVtkAllNodeNumber<ElementType, PolynomialOrder>()};
  inline static constexpr int kBasicNodeNumber{getElementNodeNumber<ElementType, 1>()};
  inline static constexpr int kAllNodeNumber{getElementNodeNumber<ElementType, PolynomialOrder>()};
  inline static constexpr int kAdjacencyNumber{getElementAdjacencyNumber<ElementType>()};
};

template <ElementEnum ElementType, int PolynomialOrder>
struct AdjacencyElementTrait : ElementTraitBase<ElementType, PolynomialOrder> {
  inline static constexpr int kBasisFunctionNumber{getElementBasisFunctionNumber<ElementType, PolynomialOrder>()};
  inline static constexpr int kQuadratureOrder{getAdjacencyElementQuadratureOrder<PolynomialOrder>()};
  inline static constexpr int kQuadratureNumber{getAdjacencyElementQuadratureNumber<ElementType, PolynomialOrder>()};
};

template <ElementEnum ElementType, int PolynomialOrder>
struct ElementTrait : ElementTraitBase<ElementType, PolynomialOrder> {
  inline static constexpr int kAllAdjacencyNodeNumber{getElementAllAdjacencyNodeNumber<ElementType>()};
  inline static constexpr int kBasisFunctionNumber{getElementBasisFunctionNumber<ElementType, PolynomialOrder>()};
  inline static constexpr int kQuadratureOrder{getElementQuadratureOrder<PolynomialOrder>()};
  inline static constexpr int kQuadratureNumber{getElementQuadratureNumber<ElementType, PolynomialOrder>()};
  inline static constexpr int kAllAdjacencyQuadratureNumber{
      getElementAllAdjacencyQuadratureNumber<ElementType, PolynomialOrder>()};
};

template <int PolynomialOrder>
using LineTrait = ElementTrait<ElementEnum::Line, PolynomialOrder>;

template <int PolynomialOrder>
using TriangleTrait = ElementTrait<ElementEnum::Triangle, PolynomialOrder>;

template <int PolynomialOrder>
using QuadrangleTrait = ElementTrait<ElementEnum::Quadrangle, PolynomialOrder>;

template <int PolynomialOrder>
using TetrahedronTrait = ElementTrait<ElementEnum::Tetrahedron, PolynomialOrder>;

template <int PolynomialOrder>
using PyramidTrait = ElementTrait<ElementEnum::Pyramid, PolynomialOrder>;

template <int PolynomialOrder>
using HexahedronTrait = ElementTrait<ElementEnum::Hexahedron, PolynomialOrder>;

template <int PolynomialOrder>
using AdjacencyPointTrait = AdjacencyElementTrait<ElementEnum::Point, PolynomialOrder>;

template <int PolynomialOrder>
using AdjacencyLineTrait = AdjacencyElementTrait<ElementEnum::Line, PolynomialOrder>;

template <int PolynomialOrder>
using AdjacencyTriangleTrait = AdjacencyElementTrait<ElementEnum::Triangle, PolynomialOrder>;

template <int PolynomialOrder>
using AdjacencyQuadrangleTrait = AdjacencyElementTrait<ElementEnum::Quadrangle, PolynomialOrder>;

template <DimensionEnum Dimension, PolynomialOrderEnum PolynomialOrder, EquationModelEnum EquationModelType,
          SourceTermEnum SourceTermType, InitialConditionEnum InitialConditionType,
          ThermodynamicModelEnum ThermodynamicModelType, EquationOfStateEnum EquationOfStateType,
          TimeIntegrationEnum TimeIntegrationType>
struct SolveControl {
  inline static constexpr int kDimension{magic_enum::enum_integer(Dimension)};
  inline static constexpr int kPolynomialOrder{magic_enum::enum_integer(PolynomialOrder)};
  inline static constexpr SourceTermEnum kSourceTerm{SourceTermType};
  inline static constexpr EquationModelEnum kEquationModel{EquationModelType};
  inline static constexpr InitialConditionEnum kInitialCondition{InitialConditionType};
  inline static constexpr ThermodynamicModelEnum kThermodynamicModel{ThermodynamicModelType};
  inline static constexpr EquationOfStateEnum kEquationOfState{EquationOfStateType};
  inline static constexpr TimeIntegrationEnum kTimeIntegration{TimeIntegrationType};
};

template <DimensionEnum Dimension, ConvectiveFluxEnum ConvectiveFluxType>
struct EulerVariable {
  inline static constexpr ConvectiveFluxEnum kConvectiveFlux{ConvectiveFluxType};
  inline static constexpr int kConservedVariableNumber{
      getConservedVariableNumber<magic_enum::enum_integer(Dimension), EquationModelEnum::Euler>()};
  inline static constexpr int kComputationalVariableNumber{
      getComputationalVariableNumber<magic_enum::enum_integer(Dimension), EquationModelEnum::Euler>()};
  inline static constexpr int kPrimitiveVariableNumber{
      getPrimitiveVariableNumber<magic_enum::enum_integer(Dimension), EquationModelEnum::Euler>()};
};

template <DimensionEnum Dimension, TransportModelEnum TransportModelType, ConvectiveFluxEnum ConvectiveFluxType,
          ViscousFluxEnum ViscousFluxType>
struct NavierStokesVariable {
  inline static constexpr TransportModelEnum kTransportModel{TransportModelType};
  inline static constexpr ConvectiveFluxEnum kConvectiveFlux{ConvectiveFluxType};
  inline static constexpr ViscousFluxEnum kViscousFlux{ViscousFluxType};
  inline static constexpr int kConservedVariableNumber{
      getConservedVariableNumber<magic_enum::enum_integer(Dimension), EquationModelEnum::NavierStokes>()};
  inline static constexpr int kComputationalVariableNumber{
      getComputationalVariableNumber<magic_enum::enum_integer(Dimension), EquationModelEnum::NavierStokes>()};
  inline static constexpr int kPrimitiveVariableNumber{
      getPrimitiveVariableNumber<magic_enum::enum_integer(Dimension), EquationModelEnum::NavierStokes>()};
};

template <DimensionEnum Dimension, TransportModelEnum TransportModelType, ConvectiveFluxEnum ConvectiveFluxType,
          ViscousFluxEnum ViscousFluxType, TurbulenceModelEnum TurbulenceModelType>
struct RANSVariable {
  inline static constexpr TransportModelEnum kTransportModel{TransportModelType};
  inline static constexpr ConvectiveFluxEnum kConvectiveFlux{ConvectiveFluxType};
  inline static constexpr ViscousFluxEnum kViscousFlux{ViscousFluxType};
  inline static constexpr TurbulenceModelEnum kTurbulenceModel{TurbulenceModelType};
  inline static constexpr int kConservedVariableNumber{
      getConservedVariableNumber<magic_enum::enum_integer(Dimension), EquationModelEnum::RANS, TurbulenceModelType>()};
  inline static constexpr int kComputationalVariableNumber{
      getComputationalVariableNumber<magic_enum::enum_integer(Dimension), EquationModelEnum::RANS,
                                     TurbulenceModelType>()};
  inline static constexpr int kPrimitiveVariableNumber{
      getPrimitiveVariableNumber<magic_enum::enum_integer(Dimension), EquationModelEnum::RANS, TurbulenceModelType>()};
};

template <DimensionEnum Dimension, PolynomialOrderEnum PolynomialOrder, MeshModelEnum MeshModelType,
          EquationModelEnum EquationModelType, SourceTermEnum SourceTermType, InitialConditionEnum InitialConditionType,
          ThermodynamicModelEnum ThermodynamicModelType, EquationOfStateEnum EquationOfStateType,
          TimeIntegrationEnum TimeIntegrationType>
struct SimulationControl
    : SolveControl<Dimension, PolynomialOrder, EquationModelType, SourceTermType, InitialConditionType,
                   ThermodynamicModelType, EquationOfStateType, TimeIntegrationType> {
  inline static constexpr MeshModelEnum kMeshModel{MeshModelType};
};

template <DimensionEnum Dimension, PolynomialOrderEnum PolynomialOrder, MeshModelEnum MeshModelType,
          SourceTermEnum SourceTermType, InitialConditionEnum InitialConditionType,
          ThermodynamicModelEnum ThermodynamicModelType, EquationOfStateEnum EquationOfStateType,
          ConvectiveFluxEnum ConvectiveFluxType, TimeIntegrationEnum TimeIntegrationType>
struct SimulationControlEuler
    : SimulationControl<Dimension, PolynomialOrder, MeshModelType, EquationModelEnum::Euler, SourceTermType,
                        InitialConditionType, ThermodynamicModelType, EquationOfStateType, TimeIntegrationType>,
      EulerVariable<Dimension, ConvectiveFluxType> {};

template <DimensionEnum Dimension, PolynomialOrderEnum PolynomialOrder, MeshModelEnum MeshModelType,
          SourceTermEnum SourceTermType, InitialConditionEnum InitialConditionType,
          ThermodynamicModelEnum ThermodynamicModelType, EquationOfStateEnum EquationOfStateType,
          TransportModelEnum TransportModelType, ConvectiveFluxEnum ConvectiveFluxType, ViscousFluxEnum ViscousFluxType,
          TimeIntegrationEnum TimeIntegrationType>
struct SimulationControlNavierStokes
    : SimulationControl<Dimension, PolynomialOrder, MeshModelType, EquationModelEnum::NavierStokes, SourceTermType,
                        InitialConditionType, ThermodynamicModelType, EquationOfStateType, TimeIntegrationType>,
      NavierStokesVariable<Dimension, TransportModelType, ConvectiveFluxType, ViscousFluxType> {};

template <DimensionEnum Dimension, PolynomialOrderEnum PolynomialOrder, MeshModelEnum MeshModelType,
          SourceTermEnum SourceTermType, InitialConditionEnum InitialConditionType,
          ThermodynamicModelEnum ThermodynamicModelType, EquationOfStateEnum EquationOfStateType,
          TransportModelEnum TransportModelType, ConvectiveFluxEnum ConvectiveFluxType, ViscousFluxEnum ViscousFluxType,
          TurbulenceModelEnum TurbulenceModelType, TimeIntegrationEnum TimeIntegrationType>
struct SimulationControlRANS
    : SimulationControl<Dimension, PolynomialOrder, MeshModelType, EquationModelEnum::NavierStokes, SourceTermType,
                        InitialConditionType, ThermodynamicModelType, EquationOfStateType, TimeIntegrationType>,
      RANSVariable<Dimension, TransportModelType, ConvectiveFluxType, ViscousFluxType, TurbulenceModelType> {};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_SIMULATION_CONTROL_HPP_
