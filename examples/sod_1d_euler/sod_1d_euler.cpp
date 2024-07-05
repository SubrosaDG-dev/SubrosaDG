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

inline const std::string kExampleName{"sod_1d_euler"};

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory / "build/out" / kExampleName};

using SimulationControl = SubrosaDG::SimulationControlEuler<
    SubrosaDG::DimensionEnum::D1, SubrosaDG::PolynomialOrderEnum::P3, SubrosaDG::MeshModelEnum::Line,
    SubrosaDG::SourceTermEnum::None, SubrosaDG::InitialConditionEnum::Function,
    SubrosaDG::ThermodynamicModelEnum::ConstantE, SubrosaDG::EquationOfStateEnum::IdealGas,
    SubrosaDG::ConvectiveFluxEnum::HLLC, SubrosaDG::TimeIntegrationEnum::SSPRK3>;

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::System<SimulationControl> system;
  system.setMesh(kExampleDirectory / "sod_1d_euler.msh", generateMesh);
  system.addInitialCondition([]([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, 1>& coordinate)
                                 -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
    return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{
        coordinate.x() <= 0.5_r ? 1.0_r : 0.125_r, coordinate.x() <= 0.5_r ? 0.75_r : 0.0_r,
        coordinate.x() <= 0.5_r ? 1.4_r : 0.8_r * 1.4_r};
  });
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::RiemannFarfield>(
      "bc-1",
      []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
          -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
        return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{1.0_r, 0.75_r, 1.4_r};
      });
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::RiemannFarfield>(
      "bc-2",
      []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
          -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
        return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{0.125_r, 0.0_r,
                                                                                           0.8_r * 1.4_r};
      });
  system.setArtificialViscosity(2.0_r);
  system.setTimeIntegration(0.1_r);
  system.setViewConfig(kExampleDirectory, kExampleName);
  system.addViewVariable({SubrosaDG::ViewVariableEnum::Density, SubrosaDG::ViewVariableEnum::Velocity,
                          SubrosaDG::ViewVariableEnum::Pressure, SubrosaDG::ViewVariableEnum::MachNumber,
                          SubrosaDG::ViewVariableEnum::ArtificialViscosity});
  system.synchronize();
  system.solve();
  system.view();
  return EXIT_SUCCESS;
}

void generateMesh(const std::filesystem::path& mesh_file_path) {
  gmsh::model::add("sod_1d");
  gmsh::model::geo::addPoint(0.0, 0.0, 0.0, 0.01);
  gmsh::model::geo::addPoint(1.0, 0.0, 0.0, 0.01);
  gmsh::model::geo::addLine(1, 2);
  gmsh::model::geo::synchronize();
  gmsh::model::addPhysicalGroup(0, {1}, -1, "bc-1");
  gmsh::model::addPhysicalGroup(0, {2}, -1, "bc-2");
  gmsh::model::addPhysicalGroup(1, {1}, -1, "vc-1");
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::setOrder(SimulationControl::kPolynomialOrder);
  gmsh::model::mesh::optimize("HighOrder");
  gmsh::write(mesh_file_path);
}
