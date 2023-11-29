/**
 * @file cylinder_2d_euler.cpp
 * @brief The 2D cylinder flow example with Euler equations.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-07-15
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#include <gmsh.h>

#include <Eigen/Core>
#include <cstdlib>
#include <filesystem>
#include <vector>

#include "SubrosaDG"

inline const std::filesystem::path kProjectDirectory{SubrosaDG::kProjectSourceDirectory /
                                                     "build/out/cylinder_2d_euler"};

using SimulationControl =
    SubrosaDG::SimulationControlEuler<2, SubrosaDG::PolynomialOrder::P3, SubrosaDG::MeshModel::TriangleQuadrangle,
                                      SubrosaDG::MeshHighOrderModel::Straight, SubrosaDG::ThermodynamicModel::ConstantE,
                                      SubrosaDG::EquationOfState::IdealGas, SubrosaDG::ConvectiveFlux::Roe,
                                      SubrosaDG::TimeIntegration::SSPRK3, SubrosaDG::ViewModel::Dat>;

void generateMesh() {
  gmsh::option::setNumber("Mesh.SecondOrderLinear", 1);
  Eigen::Matrix<double, 4, 3, Eigen::RowMajor> farfield_point;
  farfield_point << -5, -5, 0, 5, -5, 0, 5, 5, 0, -5, 5, 0;
  Eigen::Matrix<double, 5, 3, Eigen::RowMajor> cylinder_point;
  cylinder_point << 0, 0, 0, -1, 0, 0, 0, -1, 0, 1, 0, 0, 0, 1, 0;
  std::vector<int> farfield_point_tag;
  std::vector<int> cylinder_point_tag;
  std::vector<int> farfield_line_tag;
  std::vector<int> cylinder_line_tag;
  gmsh::model::add("cylinder_2d");
  for (const auto& row : farfield_point.rowwise()) {
    farfield_point_tag.emplace_back(gmsh::model::geo::addPoint(row.x(), row.y(), row.z(), 0.5));
  }
  for (const auto& row : cylinder_point.rowwise()) {
    cylinder_point_tag.emplace_back(gmsh::model::geo::addPoint(row.x(), row.y(), row.z(), 0.05));
  }
  for (std::size_t i = 0; i < farfield_point_tag.size(); i++) {
    farfield_line_tag.emplace_back(
        gmsh::model::geo::addLine(farfield_point_tag[i], farfield_point_tag[(i + 1) % farfield_point_tag.size()]));
  }
  for (std::size_t i = 1; i < cylinder_point_tag.size(); i++) {
    cylinder_line_tag.emplace_back(gmsh::model::geo::addCircleArc(
        cylinder_point_tag[i], cylinder_point_tag[0], cylinder_point_tag[i % (cylinder_point_tag.size() - 1) + 1]));
  }
  const int farfield_line_loop = gmsh::model::geo::addCurveLoop(farfield_line_tag);
  const int cylinder_line_loop = gmsh::model::geo::addCurveLoop(cylinder_line_tag);
  const int cylinder_plane_surface = gmsh::model::geo::addPlaneSurface({farfield_line_loop, cylinder_line_loop});
  gmsh::model::geo::synchronize();
  std::vector<double> cylinder_line_tag_double_cast{cylinder_line_tag.begin(), cylinder_line_tag.end()};
  const int cylinder_boundary_layer = gmsh::model::mesh::field::add("BoundaryLayer");
  gmsh::model::mesh::field::setNumbers(cylinder_boundary_layer, "CurvesList", cylinder_line_tag_double_cast);
  gmsh::model::mesh::field::setNumber(cylinder_boundary_layer, "Size", 0.05);
  gmsh::model::mesh::field::setNumber(cylinder_boundary_layer, "Ratio", 1.05);
  gmsh::model::mesh::field::setNumber(cylinder_boundary_layer, "Quads", 1);
  gmsh::model::mesh::field::setNumber(cylinder_boundary_layer, "Thickness", 0.4);
  gmsh::model::mesh::field::setAsBoundaryLayer(cylinder_boundary_layer);
  gmsh::model::addPhysicalGroup(1, farfield_line_tag, -1, "bc-1");
  gmsh::model::addPhysicalGroup(1, cylinder_line_tag, -1, "bc-2");
  gmsh::model::addPhysicalGroup(2, {cylinder_plane_surface}, -1, "vc-1");
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::setOrder(static_cast<int>(SimulationControl::kPolynomialOrder));
  gmsh::write(kProjectDirectory / "cylinder_2d.msh");
}

void generateMeshShell(){};

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::System<SimulationControl> system(generateMesh, kProjectDirectory / "cylinder_2d.msh");
  system.addInitialCondition("vc-1", []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, 2>& coordinate) {
    return Eigen::Vector<SubrosaDG::Real, 5>{1.4, 0.38, 0.0, 1.0, 1.0};
  });
  system.addBoundaryCondition<SubrosaDG::BoundaryCondition::CharacteristicFarfield>("bc-1", {1.4, 0.38, 0.0, 1.0, 1.0});
  system.addBoundaryCondition<SubrosaDG::BoundaryCondition::NoSlipWall>("bc-2");
  system.setTimeIntegration(false, 1, 1.0, 1e-10);
  system.setViewConfig(-1, kProjectDirectory, "cylinder_2d");
  system.solve();
  return EXIT_SUCCESS;
}
