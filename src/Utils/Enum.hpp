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

#include <magic_enum/magic_enum.hpp>

using namespace magic_enum::bitwise_operators;

namespace SubrosaDG {

enum class DimensionEnum {
  D1 = 1,
  D2,
  D3,
};

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

enum class PolynomialOrderEnum {
  P1 = 1,
  P2,
  P3,
  P4,
  P5,
};

enum class EquationModelEnum {
  CompresibleEuler = 1,
  CompresibleNS,
  IncompresibleEuler,
  IncompresibleNS,
  CompresibleRANS,
  IdealMHD,
  ViscousMHD,
};

enum class BasisFunctionEnum {
  Nodal = 1,
  Modal,
};

enum class SourceTermEnum {
  None = 1,
  Boussinesq,
};

enum class ShockCapturingEnum {
  None = 1,
  ArtificialViscosity,
};

enum class LimiterEnum {
  None = 1,
  PositivityPreserving,
};

enum class InitialConditionEnum {
  Function = 1,
  SpecificFile,
  LastStep,
};

enum class BoundaryConditionEnum {
  RiemannFarfield = 1,
  VelocityInflow,
  PressureOutflow,
  IsoThermalNonSlipWall,
  AdiabaticSlipWall,
  AdiabaticNonSlipWall,
  Periodic,
};

enum class BoundaryTimeEnum {
  Steady = 1,
  TimeVarying,
};

enum class ConvectiveFluxEnum {
  Central = 1,
  LaxFriedrichs,
  HLLC,
  Roe,
  Exact,
};

enum class ViscousFluxEnum {
  None = 1,
  BR1,
  BR2,
};

enum class ThermodynamicModelEnum {
  Constant = 1,
};

enum class EquationOfStateEnum {
  IdealGas = 1,
  WeakCompressibleFluid,
};

enum class TransportModelEnum {
  None,
  Constant = 1,
  Sutherland,
};

enum class TimeIntegrationEnum {
  ForwardEuler = 1,
  HeunRK2,
  SSPRK3,
};

enum class TurbulenceModelEnum {
  SA = 1,
};

enum class ConservedVariableEnum {
  // Compresible Euler/Navier-Stokes
  Density = 1,
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
  Density = 1,
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
  Density = 1,
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
  X = 1,
  Y,
  Z,
};

enum class ViewVariableEnum {
  Density = 1,
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

#endif  // SUBROSA_DG_ENUM_HPP_
