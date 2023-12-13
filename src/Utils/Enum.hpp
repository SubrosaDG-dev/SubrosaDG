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
  CharacteristicFarfield,
  AdiabaticWall,
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
  HeunRK2,
  SSPRK3,
  BackwardEuler,
};

enum class TurbulenceModel {
  SA = 1,
};

enum class ConservedVariable {
  Density = 1,
  MomentumX,
  MomentumY,
  MomentumZ,
  DensityTotalEnergy,
};

enum class ComputationalVariable {
  Density = 1,
  VelocityX,
  VelocityY,
  VelocityZ,
  InternalEnergy,
  Pressure,
};

enum class PrimitiveVariable { Density = 1, VelocityX, VelocityY, VelocityZ, Temperature };

enum class ViscousFlux {
  BR1 = 1,
  BR2,
};

enum class ViewModel {
  Msh = 1,
  Dat,
  Plt,
  Vtu,
};

enum class ViewConfig { HighOrderReconstruction = 1, SolverSmoothness };

enum class ViewVariable {
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
