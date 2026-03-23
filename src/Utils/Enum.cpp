/**
 * @file Enum.cpp
 * @brief The header file of SubrosaDG enum.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-07
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_ENUM_CPP_
#define SUBROSA_DG_ENUM_CPP_

#include <magic_enum/magic_enum.hpp>

using namespace magic_enum::bitwise_operators;

namespace SubrosaDG {

enum class CommandLineEnum : std::uint8_t {
  Open,
  Close,
};

enum class DimensionEnum : std::uint8_t {
  D1 = 1,
  D2,
  D3,
};

enum class ElementEnum : std::uint8_t {
  Point,
  Line,
  Triangle,
  Quadrangle,
  Tetrahedron,
  Pyramid,
  Hexahedron,
};

enum class MeshModelEnum : std::uint8_t {
  Line,
  Triangle,
  Quadrangle,
  TriangleQuadrangle,
  Tetrahedron,
  Hexahedron,
  TetrahedronPyramidHexahedron,
};

enum class PolynomialOrderEnum : std::uint8_t {
  P1 = 1,
  P2,
  P3,
  P4,
  P5,
};

enum class EquationModelEnum : std::uint8_t {
  CompressibleEuler,
  CompressibleNS,
  IncompressibleEuler,
  IncompressibleNS,
  CompressibleRANS,
  IdealMHD,
  ViscousMHD,

  Euler,
  NS,
  MHD
};

enum class SourceTermEnum : std::uint8_t {
  None,
  Boussinesq,
};

enum class InitialConditionEnum : std::uint8_t {
  Function,
  LastStep,
  LowOrder,
};

enum class BoundaryConditionEnum : std::uint8_t {
  RiemannFarfield,
  VelocityInflow,
  PressureOutflow,
  AdiabaticSlipWall,
  IsoThermalNonSlipWall,
  AdiabaticNonSlipWall,
  Periodic,
};

enum class BoundaryTimeEnum : std::uint8_t {
  Steady,
  TimeVarying,
};

enum class ConvectiveFluxEnum : std::uint8_t {
  Central,
  LaxFriedrichs,
  HLLC,
  Roe,
  Exact,
};

enum class ViscousFluxEnum : std::uint8_t {
  None,
  BR1,
  BR2,
};

enum class ThermodynamicModelEnum : std::uint8_t {
  Constant,
};

enum class EquationOfStateEnum : std::uint8_t {
  IdealGas,
  WeakCompressibleFluid,
};

enum class TransportModelEnum : std::uint8_t {
  None,
  Constant,
  Sutherland,
};

enum class TimeIntegrationEnum : std::uint8_t {
  ForwardEuler,
  HeunRK2,
  SSPRK3,
};

enum class TurbulenceModelEnum : std::uint8_t {
  SA,
};

enum class ConservedVariableEnum : std::uint8_t {
  // Compressible Euler/Navier-Stokes
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

enum class ComputationalVariableEnum : std::uint8_t {
  // Compressible Euler/Navier-Stokes
  Density,
  Velocity,
  VelocityX,
  VelocityY,
  VelocityZ,
  InternalEnergy,
  Pressure,

  // Incompressible Euler/Navier-Stokes
  // Density,
  // Velocity,
  // VelocityX,
  // VelocityY,
  // VelocityZ,
  // InternalEnergy,
  // Pressure,
};

enum class PrimitiveVariableEnum : std::uint8_t {
  // Compressible Euler/Navier-Stokes
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

enum class VariableGradientEnum : std::uint8_t {
  X,
  Y,
  Z,
};

enum class ViewVariableEnum : std::uint8_t {
  Density,
  Velocity,
  Temperature,
  Pressure,
  SoundSpeed,
  MachNumber,
  Entropy,
  Vorticity,
  HeatFlux,
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
  QCriterion,
  Lambda2,
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_ENUM_CPP_
