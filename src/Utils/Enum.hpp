/**
 * @file Enum.hpp
 * @brief The header file of SubroseDG enum.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_ENUM_HPP_
#define SUBROSA_DG_ENUM_HPP_

namespace SubrosaDG {

enum class Element {
  Line = 1,
  Triangle,
  Quadrangle,
  Tetrahedron,
  Pyramid,
  Hexahedron,
};

enum class MeshModel {
  Triangle = 1,
  Quadrangle,
  TriangleQuadrangle,
  Tetrahedron,
  Hexahedron,
  TetrahedronPyramidHexahedron,
};

enum class MeshHighOrderModel {
  Straight = 1,
  Curved,
};

enum class BoundaryCondition {
  NormalFarfield = 1,
  RiemannFarfield,
  NoSlipWall,
  FreeSlipWall,
  Periodic,
};

enum class ConvectiveFlux {
  Central = 1,
  LaxFriedrichs,
  HLLC,
  Roe,
};

enum class PolynomialOrder {
  P1 = 1,
  P2,
  P3,
  P4,
  P5,
};

enum class EquationModel {
  Euler = 1,
  NS,
  RANS,
};

enum class ThermodynamicModel {
  ConstantE = 1,
  ConstantH,
};

enum class EquationOfState {
  IdealGas = 1,
};

enum class TransportModel {
  Constant = 1,
  Sutherland,
};

enum class TimeIntegration {
  ForwardEuler = 1,
  RK3SSP,
  BackwardEuler,
};

enum class TurbulenceModel {
  SA = 1,
};

enum class ViscousFlux {
  BR1 = 1,
  BR2,
};

enum class ViewModel {
  Msh = 1,
  Dat,
  Plt,
  Vtk,
};

enum class ViewElementVariable {
  SpeedSound = 1,
  MachNumber,
  Vorticity,
  VorticityX,
  VorticityY,
  VorticityZ,
};

enum class ViewAdjacencyElementVariable {
  PressureCoefficient = 1,
  FrictionCoefficient,
  EntropyIncrease,
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_ENUM_HPP_
