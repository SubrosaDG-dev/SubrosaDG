/**
 * @file unsteadycavity_2d_incns.cpp
 * @brief The source file for SubrosaDG example unsteadycavity_2d_incns.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2024-11-30
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

namespace SubrosaDG {

struct PhysicalModelData {
  // EquationOfState
  static constexpr double kReferenceSoundSpeed = 10.0;
  static constexpr double kReferenceDensity = 1.0;

  // ThermodynamicModel
  static constexpr double kSpecificHeatConstantPressure = 1.0;
  static constexpr double kSpecificHeatConstantVolume = 1.0;

  // TransportModel
  static constexpr double kRayleighNumber = 3.4e5;
  static constexpr double kPrandtlNumber = 0.71;
  static constexpr double kDynamicViscosity = 491.32473986153;  // std::sqrt(kPrandtlNumber / kRayleighNumber);
  static constexpr double kThermalConductivity = kSpecificHeatConstantPressure * kDynamicViscosity / kPrandtlNumber;
};

}  // namespace SubrosaDG

#include "SubrosaDG.cpp"

inline const std::string kExampleName{"unsteadycavity_2d_incns"};

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory / "build/out" / kExampleName};

using SimulationControl = SubrosaDG::SimulationControl<
    SubrosaDG::SolveControl<SubrosaDG::DimensionEnum::D2, SubrosaDG::PolynomialOrderEnum::P1,
                            SubrosaDG::BoundaryTimeEnum::Steady, SubrosaDG::SourceTermEnum::Boussinesq>,
    SubrosaDG::NumericalControl<SubrosaDG::MeshModelEnum::Quadrangle, SubrosaDG::InitialConditionEnum::Function,
                                SubrosaDG::TimeIntegrationEnum::SSPRK3>,
    SubrosaDG::IncompressibleNSVariable<SubrosaDG::ThermodynamicModelEnum::Constant,
                                        SubrosaDG::EquationOfStateEnum::WeakCompressibleFluid,
                                        SubrosaDG::TransportModelEnum::Constant, SubrosaDG::ConvectiveFluxEnum::Exact,
                                        SubrosaDG::ViscousFluxEnum::BR2>>;

template <typename SimulationControl>
inline void SubrosaDG::InitialCondition<SimulationControl>::computePrimitiveFromCoordinate(
    [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& coordinate,
    Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber>& initial_primitive_variable) {
  initial_primitive_variable = {1.0_r, 0.0_r, 0.0_r, 0.0_r};
}

template <typename SimulationControl>
inline void SubrosaDG::BoundaryCondition<SimulationControl>::computePrimitiveFromCoordinate(
    [[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate,
    Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber>& boundary_primitive_variable,
    const SubrosaDG::Isize gmsh_physical_index) {
  if (gmsh_physical_index == 1) {
    boundary_primitive_variable = {1.0_r, 0.0_r, 0.0_r, 0.0_r};
  } else if (gmsh_physical_index == 2) {
    boundary_primitive_variable = {1.0_r, 0.0_r, 0.0_r, -0.5_r};
  } else if (gmsh_physical_index == 3) {
    boundary_primitive_variable = {1.0_r, 0.0_r, 0.0_r, 0.5_r};
  }
}

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::System<SimulationControl> system;
  system.setMesh(kExampleDirectory / std::format("{}.msh", kExampleName), generateMesh);
  system.template setSourceTerm<SimulationControl::kSourceTerm>(1.0_r, 0.0_r);
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticNonSlipWall>(1);
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::IsoThermalNonSlipWall>(2);
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::IsoThermalNonSlipWall>(3);
  system.setTimeIntegration(1.0_r);
  system.setViewConfig(kExampleDirectory, kExampleName);
  system.addViewVariable({SubrosaDG::ViewVariableEnum::Density, SubrosaDG::ViewVariableEnum::Velocity,
                          SubrosaDG::ViewVariableEnum::Pressure, SubrosaDG::ViewVariableEnum::Temperature,
                          SubrosaDG::ViewVariableEnum::MachNumber, SubrosaDG::ViewVariableEnum::Vorticity});
  system.synchronize();
  system.solve();
  system.view();
  return EXIT_SUCCESS;
}

void generateMesh(const std::filesystem::path& mesh_file_path) {
  gmsh::model::add("unsteadycavity_2d");
  gmsh::model::geo::addPoint(0.0, 0.0, 0.0);
  gmsh::model::geo::addPoint(1.0, 0.0, 0.0);
  gmsh::model::geo::addPoint(1.0, 8.0, 0.0);
  gmsh::model::geo::addPoint(0.0, 8.0, 0.0);
  gmsh::model::geo::addLine(1, 2);
  gmsh::model::geo::addLine(2, 3);
  gmsh::model::geo::addLine(3, 4);
  gmsh::model::geo::addLine(4, 1);
  gmsh::model::geo::addCurveLoop({1, 2, 3, 4});
  gmsh::model::geo::addPlaneSurface({1});
  gmsh::model::geo::mesh::setTransfiniteCurve(1, 41, "Bump", 0.30);
  gmsh::model::geo::mesh::setTransfiniteCurve(2, 201, "Bump", 0.20);
  gmsh::model::geo::mesh::setTransfiniteCurve(3, 41, "Bump", 0.30);
  gmsh::model::geo::mesh::setTransfiniteCurve(4, 201, "Bump", 0.20);
  gmsh::model::geo::mesh::setTransfiniteSurface(1);
  gmsh::model::geo::mesh::setRecombine(2, 1);
  gmsh::model::geo::synchronize();
  gmsh::model::addPhysicalGroup(1, {1, 3}, 1, "bc-1");
  gmsh::model::addPhysicalGroup(1, {2}, 2, "bc-2");
  gmsh::model::addPhysicalGroup(1, {4}, 3, "bc-3");
  gmsh::model::addPhysicalGroup(2, {1}, 4, "vc-1");
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::setOrder(SimulationControl::kPolynomialOrder);
  gmsh::model::mesh::optimize("HighOrder");
  gmsh::write(mesh_file_path);
}
