/**
 * @file periodic_2d_euler.cpp
 * @brief The source file for SubrosaDG example periodic_2d_euler.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2024-01-17
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#include "SubrosaDG"

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory /
                                                     "build/out/periodic_2d_euler"};

using SimulationControl =
    SubrosaDG::SimulationControlEuler<2, SubrosaDG::PolynomialOrderEnum::P2, SubrosaDG::MeshModelEnum::Quadrangle,
                                      SubrosaDG::ThermodynamicModelEnum::ConstantE,
                                      SubrosaDG::EquationOfStateEnum::IdealGas, SubrosaDG::ConvectiveFluxEnum::HLLC,
                                      SubrosaDG::TimeIntegrationEnum::SSPRK3, SubrosaDG::ViewModelEnum::Vtu>;

void generateMesh(const std::filesystem::path& mesh_file_path) {
  gmsh::model::add("periodic_2d");
  gmsh::model::geo::addPoint(0.0, 0.0, 0.0, 0.01);
  gmsh::model::geo::addPoint(2.0, 0.0, 0.0, 0.01);
  gmsh::model::geo::addPoint(2.0, 2.0, 0.0, 0.01);
  gmsh::model::geo::addPoint(0.0, 2.0, 0.0, 0.01);
  gmsh::model::geo::addLine(1, 2);
  gmsh::model::geo::addLine(2, 3);
  gmsh::model::geo::addLine(4, 3);
  gmsh::model::geo::addLine(1, 4);
  gmsh::model::geo::addCurveLoop({1, 2, -3, -4});
  gmsh::model::geo::addPlaneSurface({1});
  gmsh::model::geo::synchronize();
  gmsh::model::addPhysicalGroup(1, {2, 4}, -1, "bc-1");
  gmsh::model::addPhysicalGroup(1, {1, 3}, -1, "bc-2");
  gmsh::model::addPhysicalGroup(2, {1}, -1, "vc-1");
  Eigen::Matrix<double, 4, 4, Eigen::RowMajor> transform_x =
      (Eigen::Transform<double, 3, Eigen::Affine>::Identity() * Eigen::Translation<double, 3>(2, 0, 0)).matrix();
  Eigen::Matrix<double, 4, 4, Eigen::RowMajor> transform_y =
      (Eigen::Transform<double, 3, Eigen::Affine>::Identity() * Eigen::Translation<double, 3>(0, 2, 0)).matrix();
  gmsh::model::mesh::setPeriodic(1, {2}, {4}, {transform_x.data(), transform_x.data() + transform_x.size()});
  gmsh::model::mesh::setPeriodic(1, {3}, {1}, {transform_y.data(), transform_y.data() + transform_y.size()});
  gmsh::model::mesh::setTransfiniteAutomatic();
  gmsh::model::mesh::setRecombine(2, 1);
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::setOrder(static_cast<int>(SimulationControl::kPolynomialOrder));
  gmsh::write(mesh_file_path);
}

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::System<SimulationControl> system{};
  system.setMesh(kExampleDirectory / "periodic_2d.msh", generateMesh);
  system.addInitialCondition("vc-1", [](const Eigen::Vector<SubrosaDG::Real, 2>& coordinate) {
    return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{
        1.0 + 0.2 * std::sin(SubrosaDG::kPi * (coordinate.x() + coordinate.y())), 0.7, 0.3,
        1.4 / (1.0 + 0.2 * std::sin(SubrosaDG::kPi * (coordinate.x() + coordinate.y())))};
  });
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::Periodic>("bc-1");
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::Periodic>("bc-2");
  system.synchronize();
  system.setTimeIntegration(false, 100, 1.0, 1e-10);
  system.setViewConfig(-1, kExampleDirectory, "periodic_2d", SubrosaDG::ViewConfigEnum::SolverSmoothness);
  system.setViewVariable({SubrosaDG::ViewVariableEnum::Density, SubrosaDG::ViewVariableEnum::Velocity,
                          SubrosaDG::ViewVariableEnum::Pressure});
  system.solve();
  system.view();
}
