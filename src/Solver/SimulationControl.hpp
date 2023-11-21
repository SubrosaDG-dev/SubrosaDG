/**
 * @file SimulationControl.hpp
 * @brief The header file of SimulationControl.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_SIMULATION_CONTROL_HPP_
#define SUBROSA_DG_SIMULATION_CONTROL_HPP_

#include <array>
#include <unordered_map>

#include "Utils/BasicDataType.hpp"
#include "Utils/Concept.hpp"
#include "Utils/Enum.hpp"

namespace SubrosaDG {

inline const std::unordered_map<Isize, Isize> kGmshTypeNumberToNodeNumber{
    {1, 2},   {8, 3},   {26, 4}, {27, 5}, {28, 6},  {2, 3},   {9, 6},  {21, 10},
    {23, 15}, {25, 21}, {3, 4},  {10, 9}, {36, 16}, {37, 25}, {38, 36}};

inline constexpr std::array<int, 5> kLineGmshTypeNumber{1, 8, 26, 27, 28};
inline constexpr std::array<int, 5> kTriangleGmshTypeNumber{2, 9, 21, 23, 25};
inline constexpr std::array<int, 5> kQuadrangleGmshTypeNumber{3, 10, 36, 37, 38};

template <Element ElementType>
inline consteval int getElementDimension() {
  if constexpr (Is1dElement<ElementType>) {
    return 1;
  } else if constexpr (Is2dElement<ElementType>) {
    return 2;
  } else if constexpr (Is3dElement<ElementType>) {
    return 3;
  }
}

template <Element ElementType, PolynomialOrder P>
inline consteval int getElementGmshTypeNumber() {
  if constexpr (ElementType == Element::Line) {
    return kLineGmshTypeNumber[static_cast<Usize>(P) - 1];
  } else if constexpr (ElementType == Element::Triangle) {
    return kTriangleGmshTypeNumber[static_cast<Usize>(P) - 1];
  } else if constexpr (ElementType == Element::Quadrangle) {
    return kQuadrangleGmshTypeNumber[static_cast<Usize>(P) - 1];
  }
}

template <Element ElementType, PolynomialOrder P>
inline consteval int getElementNodeNumber() {
  if constexpr (ElementType == Element::Line) {
    return static_cast<int>(P) + 1;
  } else if constexpr (ElementType == Element::Triangle) {
    return (static_cast<int>(P) + 1) * (static_cast<int>(P) + 2) / 2;
  } else if constexpr (ElementType == Element::Quadrangle) {
    return (static_cast<int>(P) + 1) * (static_cast<int>(P) + 1);
  }
}

template <Element ElementType>
inline consteval int getElementAdjacencyNumber() {
  if constexpr (ElementType == Element::Line) {
    return 2;
  } else if constexpr (ElementType == Element::Triangle) {
    return 3;
  } else if constexpr (ElementType == Element::Quadrangle) {
    return 4;
  }
}

template <Element ElementType, PolynomialOrder P>
inline consteval int getElementAdjacencyNodeNumber() {
  if constexpr (ElementType == Element::Triangle) {
    return 3 * getElementNodeNumber<Element::Line, P>();
  } else if constexpr (ElementType == Element::Quadrangle) {
    return 4 * getElementNodeNumber<Element::Line, P>();
  }
}

template <Element ElementType, PolynomialOrder P>
inline consteval int getElementSubNumber() {
  if constexpr (Is1dElement<ElementType>) {
    return (static_cast<int>(P));
  } else if constexpr (Is2dElement<ElementType>) {
    return (static_cast<int>(P) * static_cast<int>(P));
  } else if constexpr (Is3dElement<ElementType>) {
    return (static_cast<int>(P) * static_cast<int>(P) * static_cast<int>(P));
  }
}

template <Element ElementType>
inline consteval Real getElementMeasure() {
  if constexpr (ElementType == Element::Line) {
    return 2.0;
  } else if constexpr (ElementType == Element::Triangle) {
    return 0.5;
  } else if constexpr (ElementType == Element::Quadrangle) {
    return 4.0;
  }
}

template <Element ElementType, PolynomialOrder P>
inline consteval std::array<Real, getElementNodeNumber<ElementType, P>() * getElementDimension<ElementType>()>
getElementNodeCoordinate() {
  if constexpr (ElementType == Element::Line) {
    if constexpr (P == PolynomialOrder::P1) {
      return {-1.0, 1.0};
    } else if constexpr (P == PolynomialOrder::P2) {
      return {-1.0, -1.0, 0.0};
    } else if constexpr (P == PolynomialOrder::P3) {
      return {-1.0, 1.0, -1.0 / 3.0, 1.0 / 3.0};
    }
  } else if constexpr (ElementType == Element::Triangle) {
    if constexpr (P == PolynomialOrder::P1) {
      return {0.0, 0.0, 1.0, 0.0, 0.0, 1.0};
    } else if constexpr (P == PolynomialOrder::P2) {
      return {0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.5, 0.0, 0.5, 0.5, 0.0, 0.5};
    } else if constexpr (P == PolynomialOrder::P3) {
      return {0.0,       0.0,       1.0,       0.0,       0.0, 1.0,       1.0 / 3.0, 0.0,       2.0 / 3.0, 0.0,
              2.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0, 2.0 / 3.0, 0.0, 2.0 / 3.0, 0.0,       1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0};
    }
  } else if constexpr (ElementType == Element::Quadrangle) {
    if constexpr (P == PolynomialOrder::P1) {
      return {-1.0, -1.0, 1.0, -1.0, 1.0, 1.0, -1.0, 1.0};
    } else if constexpr (P == PolynomialOrder::P2) {
      return {-1.0, -1.0, 1.0, -1.0, 1.0, 1.0, -1.0, 1.0, 0.0, -1.0, 1.0, 0.0, 0.0, 1.0, -1.0, 0.0, 0.0, 0.0};
    } else if constexpr (P == PolynomialOrder::P3) {
      return {-1.0,       -1.0,       1.0,        -1.0,       1.0,       1.0,        -1.0,       1.0,
              -1.0 / 3.0, -1.0,       1.0 / 3.0,  -1.0,       1.0,       -1.0 / 3.0, 1.0,        1.0 / 3.0,
              1.0 / 3.0,  1.0,        -1.0 / 3.0, 1.0,        -1.0,      -1.0 / 3.0, -1.0,       -1.0 / 3.0,
              -1.0 / 3.0, -1.0 / 3.0, 1.0 / 3.0,  -1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0,  -1.0 / 3.0, 1.0 / 3.0};
    }
  }
}

template <Element ElementType, PolynomialOrder P>
inline consteval std::array<int, getElementNodeNumber<ElementType, PolynomialOrder::P1>() *
                                     getElementSubNumber<ElementType, P>()>
getSubElementConnectivity() {
  // clang-format off
  if constexpr (ElementType == Element::Line) {
    if constexpr (P == PolynomialOrder::P1) {
      return {0,
              1};
    } else if constexpr (P == PolynomialOrder::P2) {
      return {0, 2,
              2, 1};
    } else if constexpr (P == PolynomialOrder::P3) {
      return {0, 2, 3,
              2, 3, 1};
    }
  } else if constexpr (ElementType == Element::Triangle) {
    if constexpr (P == PolynomialOrder::P1) {
      return {0,
              1,
              2};
    } else if constexpr (P == PolynomialOrder::P2) {
      return {0, 3, 3, 5,
              3, 4, 1, 4,
              5, 5, 4, 2};
    } else if constexpr (P == PolynomialOrder::P3) {
      return {0, 3, 3, 4, 4, 8, 9, 9, 7,
              3, 9, 4, 5, 1, 9, 6, 5, 6,
              8, 8, 9, 9, 5, 7, 7, 6, 2};
    }
  } else if constexpr (ElementType == Element::Quadrangle) {
    if constexpr (P == PolynomialOrder::P1) {
      return {0,
              1,
              2,
              3};
    } else if constexpr (P == PolynomialOrder::P2) {
      return {0, 4, 7, 8,
              4, 1, 8, 5,
              8, 5, 6, 2,
              7, 8, 3, 6};
    } else if constexpr (P == PolynomialOrder::P3) {
      return {0,  4,  5,  11, 12, 13, 10, 15, 14,
              4,  5,  1,  12, 13,  6, 15, 14,  7,
              12, 13, 6,  15, 14,  7,  9,  8,  2,
              11, 12, 13, 10, 15, 14,  3,  9,  8};
    }
  }
  // clang-format on
}

template <Element ElementType, PolynomialOrder P>
inline consteval int getElementBasisFunctionNumber() {
  if constexpr (ElementType == Element::Line) {
    return static_cast<int>(P) + 1;
  } else if constexpr (ElementType == Element::Triangle) {
    return (static_cast<int>(P) + 1) * (static_cast<int>(P) + 2) / 2;
  } else if constexpr (ElementType == Element::Quadrangle) {
    return (static_cast<int>(P) + 1) * (static_cast<int>(P) + 1);
  }
}

inline constexpr std::array<int, 12> kLineQuadratureNumber{1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6};
inline constexpr std::array<int, 12> kTriangleQuadratureNumber{1, 1, 3, 4, 6, 7, 12, 13, 16, 19, 25, 27};
inline constexpr std::array<int, 12> kQuadrangleQuadratureNumber{1, 3, 7, 4, 9, 9, 16, 16, 25, 25, 36, 36};

template <PolynomialOrder P>
inline consteval int getElementGaussianQuadratureOrder() {
  return 2 * static_cast<int>(P);
}

template <PolynomialOrder P>
inline consteval int getAdjacencyElementGaussianQuadratureOrder() {
  return 2 * static_cast<int>(P) + 1;
}

template <Element ElementType, PolynomialOrder P>
inline consteval int getElementGaussianQuadratureNumber() {
  if constexpr (ElementType == Element::Line) {
    return kLineQuadratureNumber[static_cast<Usize>(getElementGaussianQuadratureOrder<P>())];
  } else if constexpr (ElementType == Element::Triangle) {
    return kTriangleQuadratureNumber[static_cast<Usize>(getElementGaussianQuadratureOrder<P>())];
  } else if constexpr (ElementType == Element::Quadrangle) {
    return kQuadrangleQuadratureNumber[static_cast<Usize>(getElementGaussianQuadratureOrder<P>())];
  }
}

template <Element ElementType, PolynomialOrder P>
  requires Is1dElement<ElementType> || Is2dElement<ElementType>
inline consteval int getAdjacencyElementGaussianQuadratureNumber() {
  if constexpr (Is1dElement<ElementType>) {
    return kLineQuadratureNumber[static_cast<Usize>(getAdjacencyElementGaussianQuadratureOrder<P>())];
  }
}

template <Element ElementType, PolynomialOrder P>
  requires Is2dElement<ElementType> || Is3dElement<ElementType>
inline consteval int getElementAdjacencyQuadratureNumber() {
  if constexpr (Is2dElement<ElementType>) {
    return kLineQuadratureNumber[static_cast<Usize>(getAdjacencyElementGaussianQuadratureOrder<P>())] *
           getElementAdjacencyNumber<ElementType>();
  }
}

template <int Dimension, EquationModel EquationModelType>
inline static consteval int getConservedVariableNumber() {
  if constexpr (EquationModelType == EquationModel::Euler) {
    return Dimension + 2;
  } else if constexpr (EquationModelType == EquationModel::NS) {
    return Dimension + 2;
  }
}

template <int Dimension, EquationModel EquationModelType, TurbulenceModel TurbulenceModelType>
  requires(EquationModelType == EquationModel::RANS)
inline static consteval int getConservedVariableNumber() {
  if constexpr (TurbulenceModelType == TurbulenceModel::SA) {
    return Dimension + 3;
  }
}

template <int Dimension, EquationModel EquationModelType>
inline static constexpr int getPrimitiveVariableNumber() {
  if constexpr (EquationModelType == EquationModel::Euler) {
    return Dimension + 3;
  } else if constexpr (EquationModelType == EquationModel::NS) {
    return Dimension + 3;
  }
}

template <int Dimension, EquationModel EquationModelType, TurbulenceModel TurbulenceModelType>
  requires(EquationModelType == EquationModel::RANS)
inline static consteval int getPrimitiveVariableNumber() {
  if constexpr (TurbulenceModelType == TurbulenceModel::SA) {
    return Dimension + 4;
  }
}

template <Element ElementType, PolynomialOrder P>
struct ElementTraitBase {
  inline static constexpr int kDimension{getElementDimension<ElementType>()};
  inline static constexpr Element kElementType{ElementType};
  inline static constexpr PolynomialOrder kPolynomialOrder{P};
  inline static constexpr int kGmshTypeNumber{getElementGmshTypeNumber<ElementType, P>()};
  inline static constexpr int kBasicNodeNumber{getElementNodeNumber<ElementType, PolynomialOrder::P1>()};
  inline static constexpr int kAllNodeNumber{getElementNodeNumber<ElementType, P>()};
  inline static constexpr std::array<Real, kDimension * kBasicNodeNumber> kBasicNodeCoordinate{
      getElementNodeCoordinate<ElementType, PolynomialOrder::P1>()};
  inline static constexpr int kAdjacencyNumber{getElementAdjacencyNumber<ElementType>()};
};

template <Element ElementType, PolynomialOrder P>
struct AdjacencyElementTrait : ElementTraitBase<ElementType, P> {
  inline static constexpr int kQuadratureOrder{getAdjacencyElementGaussianQuadratureOrder<P>()};
  inline static constexpr int kQuadratureNumber{getAdjacencyElementGaussianQuadratureNumber<ElementType, P>()};
};

template <Element ElementType, PolynomialOrder P>
struct ElementTrait : ElementTraitBase<ElementType, P> {
  inline static constexpr int kAdjacencyNodeNumber{getElementAdjacencyNodeNumber<ElementType, P>()};
  inline static constexpr int kSubNumber{getElementSubNumber<ElementType, P>()};
  inline static constexpr int kBasisFunctionNumber{getElementBasisFunctionNumber<ElementType, P>()};
  inline static constexpr int kQuadratureOrder{getElementGaussianQuadratureOrder<P>()};
  inline static constexpr int kQuadratureNumber{getElementGaussianQuadratureNumber<ElementType, P>()};
  inline static constexpr int kAdjacencyQuadratureNumber{getElementAdjacencyQuadratureNumber<ElementType, P>()};
};

template <PolynomialOrder P>
using AdjacencyLineTrait = AdjacencyElementTrait<Element::Line, P>;

template <PolynomialOrder P>
using TriangleTrait = ElementTrait<Element::Triangle, P>;

template <PolynomialOrder P>
using QuadrangleTrait = ElementTrait<Element::Quadrangle, P>;

template <int Dimension, PolynomialOrder P, EquationModel EquationModelType, ThermodynamicModel ThermodynamicModelType,
          EquationOfState EquationOfStateType, TimeIntegration TimeIntegrationType>
struct SolveControl {
  inline static constexpr int kDimension{Dimension};
  inline static constexpr PolynomialOrder kPolynomialOrder{P};
  inline static constexpr EquationModel kEquationModel{EquationModelType};
  inline static constexpr ThermodynamicModel kThermodynamicModel{ThermodynamicModelType};
  inline static constexpr EquationOfState kEquationOfState{EquationOfStateType};
  inline static constexpr TimeIntegration kTimeIntegration{TimeIntegrationType};
};

template <EquationModel EquationModelType>
struct EquationVariable {};

template <int Dimension, ConvectiveFlux ConvectiveFluxType>
struct EulerVariable : EquationVariable<EquationModel::Euler> {
  inline static constexpr ConvectiveFlux kConvectiveFlux{ConvectiveFluxType};
  inline static constexpr int kConservedVariableNumber{getConservedVariableNumber<Dimension, EquationModel::Euler>()};
  inline static constexpr int kPrimitiveVariableNumber{getPrimitiveVariableNumber<Dimension, EquationModel::Euler>()};
};

template <int Dimension, TransportModel TransportModelType, ConvectiveFlux ConvectiveFluxType,
          ViscousFlux ViscousFluxType>
struct NSVariable : EquationVariable<EquationModel::NS> {
  inline static constexpr TransportModel kTransportModel{TransportModelType};
  inline static constexpr ConvectiveFlux kConvectiveFlux{ConvectiveFluxType};
  inline static constexpr ViscousFlux kViscousFlux{ViscousFluxType};
  inline static constexpr int kConservedVariableNumber{getConservedVariableNumber<Dimension, EquationModel::NS>()};
  inline static constexpr int kPrimitiveVariableNumber{getPrimitiveVariableNumber<Dimension, EquationModel::NS>()};
};

template <int Dimension, TransportModel TransportModelType, ConvectiveFlux ConvectiveFluxType,
          ViscousFlux ViscousFluxType, TurbulenceModel TurbulenceModelType>
struct RANSVariable : EquationVariable<EquationModel::RANS> {
  inline static constexpr TransportModel kTransportModel{TransportModelType};
  inline static constexpr ConvectiveFlux kConvectiveFlux{ConvectiveFluxType};
  inline static constexpr ViscousFlux kViscousFlux{ViscousFluxType};
  inline static constexpr TurbulenceModel kTurbulenceModel{TurbulenceModelType};
  inline static constexpr int kConservedVariableNumber{
      getConservedVariableNumber<Dimension, EquationModel::RANS, TurbulenceModelType>()};
  inline static constexpr int kPrimitiveVariableNumber{
      getPrimitiveVariableNumber<Dimension, EquationModel::RANS, TurbulenceModelType>()};
};

template <int Dimension, PolynomialOrder P, MeshModel MeshModelType, MeshHighOrderModel MeshHighOrderModelType,
          EquationModel EquationModelType, ThermodynamicModel ThermodynamicModelType,
          EquationOfState EquationOfStateType, TimeIntegration TimeIntegrationType, ViewModel ViewModelType>
struct SimulationControl
    : SolveControl<Dimension, P, EquationModelType, ThermodynamicModelType, EquationOfStateType, TimeIntegrationType> {
  inline static constexpr MeshModel kMeshModel{MeshModelType};
  inline static constexpr MeshHighOrderModel kMeshHighOrderModel{MeshHighOrderModelType};
  inline static constexpr ViewModel kViewModel{ViewModelType};
};

template <int Dimension, PolynomialOrder P, MeshModel MeshModelType, MeshHighOrderModel MeshHighOrderModelType,
          ThermodynamicModel ThermodynamicModelType, EquationOfState EquationOfStateType,
          ConvectiveFlux ConvectiveFluxType, TimeIntegration TimeIntegrationType, ViewModel ViewModelType>
struct SimulationControlEuler
    : SimulationControl<Dimension, P, MeshModelType, MeshHighOrderModelType, EquationModel::Euler,
                        ThermodynamicModelType, EquationOfStateType, TimeIntegrationType, ViewModelType>,
      EulerVariable<Dimension, ConvectiveFluxType> {};

template <int Dimension, PolynomialOrder P, MeshModel MeshModelType, MeshHighOrderModel MeshHighOrderModelType,
          ThermodynamicModel ThermodynamicModelType, EquationOfState EquationOfStateType,
          TransportModel TransportModelType, ConvectiveFlux ConvectiveFluxType, ViscousFlux ViscousFluxType,
          TimeIntegration TimeIntegrationType, ViewModel ViewModelType>
struct SimulationControlNS
    : SimulationControl<Dimension, P, MeshModelType, MeshHighOrderModelType, EquationModel::NS, ThermodynamicModelType,
                        EquationOfStateType, TimeIntegrationType, ViewModelType>,
      NSVariable<Dimension, TransportModelType, ConvectiveFluxType, ViscousFluxType> {};

template <int Dimension, PolynomialOrder P, MeshModel MeshModelType, MeshHighOrderModel MeshHighOrderModelType,
          ThermodynamicModel ThermodynamicModelType, EquationOfState EquationOfStateType,
          TransportModel TransportModelType, ConvectiveFlux ConvectiveFluxType, ViscousFlux ViscousFluxType,
          TurbulenceModel TurbulenceModelType, TimeIntegration TimeIntegrationType, ViewModel ViewModelType>
struct SimulationControlRANS
    : SimulationControl<Dimension, P, MeshModelType, MeshHighOrderModelType, EquationModel::NS, ThermodynamicModelType,
                        EquationOfStateType, TimeIntegrationType, ViewModelType>,
      RANSVariable<Dimension, TransportModelType, ConvectiveFluxType, ViscousFluxType, TurbulenceModelType> {};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_SIMULATION_CONTROL_HPP_
