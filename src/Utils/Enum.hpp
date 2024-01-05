/**
 * @file Enum.hpp
 * @brief The header file of SubrosaDG enum.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_ENUM_HPP_
#define SUBROSA_DG_ENUM_HPP_

#include <magic_enum.hpp>

using namespace magic_enum::bitwise_operators;

namespace SubrosaDG {

enum class ElementEnum {
  Point = 1,
  Line,
  Triangle,
  Quadrangle,
  Tetrahedron,
  Pyramid,
  Hexahedron,
};

enum class MeshModelEnum {
  Line = 1,
  Triangle,
  Quadrangle,
  TriangleQuadrangle,
  Tetrahedron,
  Hexahedron,
  TetrahedronPyramidHexahedron,
};

enum class BoundaryConditionEnum {
  NormalFarfield = 1,
  RiemannFarfield,
  AdiabaticWall,
  Periodic,
};

enum class ConvectiveFluxEnum {
  Central = 1,
  LaxFriedrichs,
  HLLC,
  Roe,
};

enum class PolynomialOrderEnum {
  P1 = 1,
  P2,
  P3,
  P4,
  P5,
};

enum class EquationModelEnum {
  Euler = 1,
  NS,
  RANS,
};

enum class ThermodynamicModelEnum {
  ConstantE = 1,
  ConstantH,
};

enum class EquationOfStateEnum {
  IdealGas = 1,
};

enum class TransportModelEnum {
  Constant = 1,
  Sutherland,
};

enum class TimeIntegrationEnum {
  TestInitialization = 1,
  ForwardEuler,
  HeunRK2,
  SSPRK3,
  BackwardEuler,
};

enum class TurbulenceModelEnum {
  SA = 1,
};

enum class ConservedVariableEnum {
  Density = 1,
  Momentum,
  MomentumX,
  MomentumY,
  MomentumZ,
  DensityTotalEnergy,
};

enum class ComputationalVariableEnum {
  Density = 1,
  Velocity,
  VelocityX,
  VelocityY,
  VelocityZ,
  VelocitySquareSummation,
  InternalEnergy,
  Pressure,
};

enum class PrimitiveVariableEnum { Density = 1, Velocity, VelocityX, VelocityY, VelocityZ, Temperature };

enum class ViscousFluxEnum {
  BR1 = 1,
  BR2,
};

enum class ViewModelEnum {
  Msh = 1,
  Dat,
  Plt,
  Vtu,
};

enum class ViewConfigEnum {
  Default = 0,
  DoNotTruncate = 1 << 0,
  SolverSmoothness = 1 << 1,
};

enum class ViewVariableEnum {
  Density = 1,
  Velocity,
  Temperature,
  Pressure,
  SoundSpeed,
  MachNumber,
  Vorticity,
  Entropy,
  VelocityX,
  VelocityY,
  VelocityZ,
  MachNumberX,
  MachNumberY,
  MachNumberZ,
  VorticityX,
  VorticityY,
  VorticityZ,
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_ENUM_HPP_
