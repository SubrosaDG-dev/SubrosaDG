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
  if constexpr (ElementType == ElementEnum::Point) {
    return -1;
  }
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

template <ElementEnum ElementType, PolynomialOrderEnum P>
inline consteval int getElementAdjacencyNodeNumber() {
  if constexpr (ElementType == ElementEnum::Line) {
    return 2;
  }
  if constexpr (ElementType == ElementEnum::Triangle) {
    return 3 * getElementNodeNumber<ElementEnum::Line, P>();
  }
  if constexpr (ElementType == ElementEnum::Quadrangle) {
    return 4 * getElementNodeNumber<ElementEnum::Line, P>();
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
              1.0 / 3.0,  1.0,        -1.0 / 3.0, 1.0,        -1.0,      -1.0 / 3.0, -1.0,       -1.0 / 3.0,
              -1.0 / 3.0, -1.0 / 3.0, 1.0 / 3.0,  -1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0,  -1.0 / 3.0, 1.0 / 3.0};
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
  }
}

template <ElementEnum ElementType, PolynomialOrderEnum P>
inline consteval int getElementBasisFunctionNumber() {
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
inline consteval int getElementGaussianQuadratureOrder() {
  return 2 * magic_enum::enum_integer(P);
}

template <PolynomialOrderEnum P>
inline consteval int getAdjacencyElementGaussianQuadratureOrder() {
  return 2 * magic_enum::enum_integer(P) + 1;
}

template <ElementEnum ElementType, PolynomialOrderEnum P>
inline consteval int getElementGaussianQuadratureNumber() {
  if constexpr (ElementType == ElementEnum::Line) {
    return kLineQuadratureNumber[static_cast<Usize>(getElementGaussianQuadratureOrder<P>())];
  }
  if constexpr (ElementType == ElementEnum::Triangle) {
    return kTriangleQuadratureNumber[static_cast<Usize>(getElementGaussianQuadratureOrder<P>())];
  }
  if constexpr (ElementType == ElementEnum::Quadrangle) {
    return kQuadrangleQuadratureNumber[static_cast<Usize>(getElementGaussianQuadratureOrder<P>())];
  }
}

template <ElementEnum ElementType, PolynomialOrderEnum P>
inline consteval int getAdjacencyElementGaussianQuadratureNumber() {
  if constexpr (Is0dElement<ElementType>) {
    return 1;
  }
  if constexpr (Is1dElement<ElementType>) {
    return kLineQuadratureNumber[static_cast<Usize>(getAdjacencyElementGaussianQuadratureOrder<P>())];
  }
}

template <ElementEnum ElementType, PolynomialOrderEnum P>
inline consteval int getElementAdjacencyQuadratureNumber() {
  if constexpr (Is1dElement<ElementType>) {
    return 2;
  }
  if constexpr (Is2dElement<ElementType>) {
    return kLineQuadratureNumber[static_cast<Usize>(getAdjacencyElementGaussianQuadratureOrder<P>())] *
           getElementAdjacencyNumber<ElementType>();
  }
}

template <int Dimension, EquationModelEnum EquationModelType>
inline consteval int getConservedVariableNumber() {
  if constexpr (EquationModelType == EquationModelEnum::Euler) {
    return Dimension + 2;
  }
  if constexpr (EquationModelType == EquationModelEnum::NS) {
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
inline constexpr int getComputationalVariableNumber() {
  if constexpr (EquationModelType == EquationModelEnum::Euler) {
    return Dimension + 3;
  }
  if constexpr (EquationModelType == EquationModelEnum::NS) {
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
inline constexpr int getPrimitiveVariableNumber() {
  if constexpr (EquationModelType == EquationModelEnum::Euler) {
    return Dimension + 2;
  }
  if constexpr (EquationModelType == EquationModelEnum::NS) {
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
  inline static constexpr int kQuadratureOrder{getAdjacencyElementGaussianQuadratureOrder<P>()};
  inline static constexpr int kQuadratureNumber{getAdjacencyElementGaussianQuadratureNumber<ElementType, P>()};
};

template <ElementEnum ElementType, PolynomialOrderEnum P>
struct ElementTrait : ElementTraitBase<ElementType, P> {
  inline static constexpr int kAdjacencyNodeNumber{getElementAdjacencyNodeNumber<ElementType, P>()};
  inline static constexpr int kBasisFunctionNumber{getElementBasisFunctionNumber<ElementType, P>()};
  inline static constexpr int kQuadratureOrder{getElementGaussianQuadratureOrder<P>()};
  inline static constexpr int kQuadratureNumber{getElementGaussianQuadratureNumber<ElementType, P>()};
  inline static constexpr int kAdjacencyQuadratureNumber{getElementAdjacencyQuadratureNumber<ElementType, P>()};
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
struct NSVariable : EquationVariable<EquationModelEnum::NS> {
  inline static constexpr TransportModelEnum kTransportModel{TransportModelType};
  inline static constexpr ConvectiveFluxEnum kConvectiveFlux{ConvectiveFluxType};
  inline static constexpr ViscousFluxEnum kViscousFlux{ViscousFluxType};
  inline static constexpr int kConservedVariableNumber{getConservedVariableNumber<Dimension, EquationModelEnum::NS>()};
  inline static constexpr int kComputationalVariableNumber{
      getComputationalVariableNumber<Dimension, EquationModelEnum::NS>()};
  inline static constexpr int kPrimitiveVariableNumber{getPrimitiveVariableNumber<Dimension, EquationModelEnum::NS>()};
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
  requires(Dimension == 2) || (Dimension == 3)
struct SimulationControlNS
    : SimulationControl<Dimension, P, MeshModelType, EquationModelEnum::NS, ThermodynamicModelType, EquationOfStateType,
                        TimeIntegrationType, ViewModelType>,
      NSVariable<Dimension, TransportModelType, ConvectiveFluxType, ViscousFluxType> {};

template <int Dimension, PolynomialOrderEnum P, MeshModelEnum MeshModelType,
          ThermodynamicModelEnum ThermodynamicModelType, EquationOfStateEnum EquationOfStateType,
          TransportModelEnum TransportModelType, ConvectiveFluxEnum ConvectiveFluxType, ViscousFluxEnum ViscousFluxType,
          TurbulenceModelEnum TurbulenceModelType, TimeIntegrationEnum TimeIntegrationType, ViewModelEnum ViewModelType>
  requires(Dimension == 2) || (Dimension == 3)
struct SimulationControlRANS
    : SimulationControl<Dimension, P, MeshModelType, EquationModelEnum::NS, ThermodynamicModelType, EquationOfStateType,
                        TimeIntegrationType, ViewModelType>,
      RANSVariable<Dimension, TransportModelType, ConvectiveFluxType, ViscousFluxType, TurbulenceModelType> {};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_SIMULATION_CONTROL_HPP_
