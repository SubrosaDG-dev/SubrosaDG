/**
 * @file periodic_1d_ceuler.cpp
 * @brief The source file for SubrosaDG example periodic_1d_ceuler.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-12-15
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2025 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#include "SubrosaDG.cpp"

inline const std::string kExampleName{"periodic_1d_ceuler"};

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory / "build/out" / kExampleName};

using SimulationControl = SubrosaDG::SimulationControl<
    SubrosaDG::SolveControl<SubrosaDG::DimensionEnum::D1, SubrosaDG::PolynomialOrderEnum::P3,
                            SubrosaDG::BoundaryTimeEnum::Steady, SubrosaDG::SourceTermEnum::None>,
    SubrosaDG::NumericalControl<SubrosaDG::MeshModelEnum::Line, SubrosaDG::ShockCapturingEnum::None,
                                SubrosaDG::LimiterEnum::None, SubrosaDG::InitialConditionEnum::Function,
                                SubrosaDG::TimeIntegrationEnum::ForwardEuler>,
    SubrosaDG::CompresibleEulerVariable<SubrosaDG::ThermodynamicModelEnum::Constant,
                                        SubrosaDG::EquationOfStateEnum::IdealGas, SubrosaDG::ConvectiveFluxEnum::HLLC>>;

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::System<SimulationControl> system;
  system.setMesh(kExampleDirectory / "periodic_1d_ceuler.msh", generateMesh);
  system.addInitialCondition([](const Eigen::Vector<SubrosaDG::Real, 1>& coordinate)
                                 -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
    return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{
        1.0_r + 0.2_r * std::sin(SubrosaDG::kPi * coordinate.x()), 1.0_r,
        1.4_r / (1.0_r + 0.2_r * std::sin(SubrosaDG::kPi * coordinate.x()))};
  });
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::Periodic>("bc-1");
  system.setThermodynamicModel<SimulationControl::kThermodynamicModel>(2.5_r, 25.0_r / 14.0_r);
  system.setTimeIntegration(0.01_r, {0, 1});
  system.setDeltaTime(1.0e-04_r);
  system.setViewConfig(kExampleDirectory, kExampleName, 200);
  system.addViewVariable({SubrosaDG::ViewVariableEnum::Density, SubrosaDG::ViewVariableEnum::Velocity,
                          SubrosaDG::ViewVariableEnum::Pressure});
  system.synchronize();
  system.solve();
  system.view();
  return EXIT_SUCCESS;
}

void generateMesh(const std::filesystem::path& mesh_file_path) {
  gmsh::model::add("periodic_1d");
  gmsh::model::geo::addPoint(0.0, 0.0, 0.0);
  gmsh::model::geo::addPoint(2.0, 0.0, 0.0);
  gmsh::model::geo::addLine(1, 2);
  gmsh::model::geo::mesh::setTransfiniteCurve(1, 41);
  gmsh::model::geo::synchronize();
  gmsh::model::addPhysicalGroup(0, {1, 2}, -1, "bc-1");
  gmsh::model::addPhysicalGroup(1, {1}, -1, "vc-1");
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::setOrder(SimulationControl::kPolynomialOrder);
  gmsh::model::mesh::optimize("HighOrder");
  gmsh::write(mesh_file_path);
}
