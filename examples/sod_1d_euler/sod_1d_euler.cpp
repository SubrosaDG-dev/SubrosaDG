/**
 * @file sod_1d_euler.cpp
 * @brief The source file for SubrosaDG example sod_1d_euler.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-12-15
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#include "SubrosaDG"

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory / "build/out/sod_1d_euler"};

using SimulationControl = SubrosaDG::SimulationControlEuler<
    1, SubrosaDG::PolynomialOrderEnum::P2, SubrosaDG::MeshModelEnum::Line, SubrosaDG::ThermodynamicModelEnum::ConstantE,
    SubrosaDG::EquationOfStateEnum::IdealGas, SubrosaDG::ConvectiveFluxEnum::LaxFriedrichs,
    SubrosaDG::TimeIntegrationEnum::SSPRK3, SubrosaDG::ViewModelEnum::Vtu>;

void generateMesh(const std::filesystem::path& mesh_file_path) {
  gmsh::option::setNumber("Mesh.SecondOrderLinear", 1);
  gmsh::model::add("sod_1d");
  gmsh::model::geo::addPoint(0.0, 0.0, 0.0, 0.01);
  gmsh::model::geo::addPoint(1.0, 0.0, 0.0, 0.01);
  gmsh::model::geo::addLine(1, 2);
  gmsh::model::geo::synchronize();
  gmsh::model::addPhysicalGroup(0, {1}, -1, "bc-1");
  gmsh::model::addPhysicalGroup(0, {2}, -1, "bc-2");
  gmsh::model::addPhysicalGroup(1, {1}, -1, "vc-1");
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::setOrder(static_cast<int>(SimulationControl::kPolynomialOrder));
  gmsh::write(mesh_file_path);
}

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::System<SimulationControl> system(kExampleDirectory / "sod_1d.msh", generateMesh);
  // NOTE: This example is a modified version of Sod’s problem in 1D from Toro’s book; the solution has a right shock
  // wave, a right travelling contact wave and a left sonic rarefaction wave; this test is useful for assessing the
  // entropy satisfaction property of numerical methods.
  system.addInitialCondition("vc-1", []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, 1>& coordinate) {
    return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{
        (coordinate.x() < 0.5) ? 1.0 : 0.125, (coordinate.x() < 0.5) ? 0.75 : 0.0, (coordinate.x() < 0.5) ? 1.4 : 1.12};
  });
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::RiemannFarfield>("bc-1", {1.0, 0.75, 1.4});
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::RiemannFarfield>("bc-2", {0.125, 0.0, 1.12});
  system.setTimeIntegration(false, 1, 0.5, 1e-10);
  system.setViewConfig(-1, kExampleDirectory, "sod_1d", SubrosaDG::ViewConfigEnum::SolverSmoothness);
  system.setViewVariable({SubrosaDG::ViewVariableEnum::Density, SubrosaDG::ViewVariableEnum::Velocity,
                          SubrosaDG::ViewVariableEnum::Pressure});
  system.solve();
  system.view();
  return EXIT_SUCCESS;
}
