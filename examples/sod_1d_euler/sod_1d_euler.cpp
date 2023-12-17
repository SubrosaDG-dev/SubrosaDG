/**
 * @file sod_1d_euler.cpp
 * @brief The source file for SubrosaDG example sod_1d_euler.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-12-15
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#include <gmsh.h>

#include <Eigen/Cholesky>
#include <Eigen/Core>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <vector>

#include "SubrosaDG"

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory / "build/out/sod_1d_euler"};

using SimulationControl =
    SubrosaDG::SimulationControlEuler<1, SubrosaDG::PolynomialOrder::P2, SubrosaDG::MeshModel::Line,
                                      SubrosaDG::ThermodynamicModel::ConstantE, SubrosaDG::EquationOfState::IdealGas,
                                      SubrosaDG::ConvectiveFlux::LaxFriedrichs, SubrosaDG::TimeIntegration::SSPRK3,
                                      SubrosaDG::ViewModel::Vtu>;

void generateMesh() {
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
  gmsh::write(kExampleDirectory / "sod_1d.msh");
}

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::System<SimulationControl> system(generateMesh, kExampleDirectory / "sod_1d.msh");
  system.addInitialCondition("vc-1", []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, 1>& coordinate) {
    return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{
        1.4, (coordinate.x() < 0.5) ? 0.63 : 0.0, 1.0};
  });
  system.addBoundaryCondition<SubrosaDG::BoundaryCondition::RiemannFarfield>("bc-1", {1.4, 0.63, 1.0});
  system.addBoundaryCondition<SubrosaDG::BoundaryCondition::RiemannFarfield>("bc-2", {1.4, 0.63, 1.0});
  // system.addInitialCondition("vc-1", []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, 1>& coordinate) {
  //   return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{
  //       (coordinate.x() < 0.5) ? 1.0 : 0.125, 0.0, (coordinate.x() < 0.5) ? 1.4 : 1.12};
  // });
  // system.addBoundaryCondition<SubrosaDG::BoundaryCondition::RiemannFarfield>("bc-1", {1.0, 0.0, 1.4});
  // system.addBoundaryCondition<SubrosaDG::BoundaryCondition::RiemannFarfield>("bc-2", {0.125, 0.0, 1.12});
  system.setTimeIntegration(false, 500, 0.6, 1e-10);
  system.setViewConfig(-1, kExampleDirectory, "sod_1d",
                       {SubrosaDG::ViewConfig::HighOrderReconstruction, SubrosaDG::ViewConfig::SolverSmoothness});
  system.addViewVariable(
      {SubrosaDG::ViewVariable::Density, SubrosaDG::ViewVariable::Velocity, SubrosaDG::ViewVariable::Pressure});
  system.solve();
  system.view();
  return EXIT_SUCCESS;
}
