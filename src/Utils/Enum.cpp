/**
 * @file Enum.cpp
 * @brief The header file of SubrosaDG enum.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2025 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_ENUM_CPP_
#define SUBROSA_DG_ENUM_CPP_

#include <magic_enum/magic_enum.hpp>

using namespace magic_enum::bitwise_operators;

namespace SubrosaDG {

enum class DimensionEnum {
  D1 = 1,
  D2,
  D3,
};

enum class ElementEnum {
  Point,
  Line,
  Triangle,
  Quadrangle,
  Tetrahedron,
  Pyramid,
  Hexahedron,
};

enum class MeshModelEnum {
  Line,
  Triangle,
  Quadrangle,
  TriangleQuadrangle,
  Tetrahedron,
  Hexahedron,
  TetrahedronPyramidHexahedron,
};

enum class PolynomialOrderEnum {
  P1 = 1,
  P2,
  P3,
  P4,
  P5,
};

enum class EquationModelEnum {
  CompresibleEuler,
  CompresibleNS,
  IncompresibleEuler,
  IncompresibleNS,
  CompresibleRANS,
  IdealMHD,
  ViscousMHD,
};

enum class BasisFunctionEnum {
  Nodal,
  Modal,
};

enum class SourceTermEnum {
  None,
  Boussinesq,
};

enum class ShockCapturingEnum {
  None,
  ArtificialViscosity,
};

enum class LimiterEnum {
  None,
  PositivityPreserving,
};

enum class InitialConditionEnum {
  Function,
  SpecificFile,
  LastStep,
};

enum class BoundaryConditionEnum {
  RiemannFarfield,
  VelocityInflow,
  PressureOutflow,
  IsoThermalNonSlipWall,
  AdiabaticSlipWall,
  AdiabaticNonSlipWall,
  Periodic,
};

enum class BoundaryTimeEnum {
  Steady,
  TimeVarying,
};

enum class ConvectiveFluxEnum {
  Central,
  LaxFriedrichs,
  HLLC,
  Roe,
  Exact,
};

enum class ViscousFluxEnum {
  None,
  BR1,
  BR2,
};

enum class ThermodynamicModelEnum {
  Constant,
};

enum class EquationOfStateEnum {
  IdealGas,
  WeakCompressibleFluid,
};

enum class TransportModelEnum {
  None,
  Constant,
  Sutherland,
};

enum class TimeIntegrationEnum {
  ForwardEuler,
  HeunRK2,
  SSPRK3,
};

enum class TurbulenceModelEnum {
  SA,
};

enum class ConservedVariableEnum {
  // Compresible Euler/Navier-Stokes
  Density,
  Momentum,
  MomentumX,
  MomentumY,
  MomentumZ,
  DensityTotalEnergy,

  // Incompressible Euler/Navier-Stokes
  // Density,
  // Momentum,
  // MomentumX,
  // MomentumY,
  // MomentumZ,
  DensityInternalEnergy,
};

enum class ComputationalVariableEnum {
  // Compresible Euler/Navier-Stokes
  Density,
  Velocity,
  VelocityX,
  VelocityY,
  VelocityZ,
  VelocitySquaredNorm,
  InternalEnergy,
  Pressure,

  // Incompressible Euler/Navier-Stokes
  // Density,
  // Velocity,
  // VelocityX,
  // VelocityY,
  // VelocityZ,
  // VelocitySquaredNorm,
  // InternalEnergy,
  // Pressure,
};

enum class PrimitiveVariableEnum {
  // Compresible Euler/Navier-Stokes
  Density,
  Velocity,
  VelocityX,
  VelocityY,
  VelocityZ,
  Temperature,

  // Incompressible Euler/Navier-Stokes
  // Density,
  // Velocity,
  // VelocityX,
  // VelocityY,
  // VelocityZ,
  // Temperature,
};

enum class VariableGradientEnum {
  X,
  Y,
  Z,
};

enum class ViewVariableEnum {
  Density,
  Velocity,
  Temperature,
  Pressure,
  SoundSpeed,
  MachNumber,
  Entropy,
  Vorticity,
  HeatFlux,
  ArtificialViscosity,
  VelocityX,
  VelocityY,
  VelocityZ,
  MachNumberX,
  MachNumberY,
  MachNumberZ,
  VorticityX,
  VorticityY,
  VorticityZ,
  HeatFluxX,
  HeatFluxY,
  HeatFluxZ,
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_ENUM_CPP_
