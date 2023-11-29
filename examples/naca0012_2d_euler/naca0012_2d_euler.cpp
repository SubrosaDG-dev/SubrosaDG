/**
 * @file naca0012_2d_euler.cpp
 * @brief The source file for SubrosaDG example naca0012_2d_euler.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2022-11-02
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#include <gmsh.h>

#include <Eigen/Core>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <vector>

#include "SubrosaDG"

inline const std::filesystem::path kProjectDirectory{SubrosaDG::kProjectSourceDirectory /
                                                     "build/out/naca0012_2d_euler"};

using SimulationControl =
    SubrosaDG::SimulationControlEuler<2, SubrosaDG::PolynomialOrder::P3, SubrosaDG::MeshModel::TriangleQuadrangle,
                                      SubrosaDG::MeshHighOrderModel::Straight, SubrosaDG::ThermodynamicModel::ConstantE,
                                      SubrosaDG::EquationOfState::IdealGas, SubrosaDG::ConvectiveFlux::Roe,
                                      SubrosaDG::TimeIntegration::SSPRK3, SubrosaDG::ViewModel::Dat>;

inline std::array<double, 130> naca0012_point_flattened{
    0.9987518, 0.0014399, 0.9976658, 0.0015870, 0.9947532, 0.0019938, 0.9906850, 0.0025595, 0.9854709, 0.0032804,
    0.9791229, 0.0041519, 0.9716559, 0.0051685, 0.9630873, 0.0063238, 0.9534372, 0.0076108, 0.9427280, 0.0090217,
    0.9309849, 0.0105485, 0.9182351, 0.0121823, 0.9045085, 0.0139143, 0.8898372, 0.0157351, 0.8742554, 0.0176353,
    0.8577995, 0.0196051, 0.8405079, 0.0216347, 0.8224211, 0.0237142, 0.8035813, 0.0258337, 0.7840324, 0.0279828,
    0.7638202, 0.0301515, 0.7429917, 0.0323294, 0.7215958, 0.0345058, 0.6996823, 0.0366700, 0.6773025, 0.0388109,
    0.6545085, 0.0409174, 0.6313537, 0.0429778, 0.6078921, 0.0449802, 0.5841786, 0.0469124, 0.5602683, 0.0487619,
    0.5362174, 0.0505161, 0.5120819, 0.0521620, 0.4879181, 0.0536866, 0.4637826, 0.0550769, 0.4397317, 0.0563200,
    0.4158215, 0.0574033, 0.3921079, 0.0583145, 0.3686463, 0.0590419, 0.3454915, 0.0595747, 0.3226976, 0.0599028,
    0.3003177, 0.0600172, 0.2784042, 0.0599102, 0.2570083, 0.0595755, 0.2361799, 0.0590081, 0.2159676, 0.0582048,
    0.1964187, 0.0571640, 0.1775789, 0.0558856, 0.1594921, 0.0543715, 0.1422005, 0.0526251, 0.1257446, 0.0506513,
    0.1101628, 0.0484567, 0.0954915, 0.0460489, 0.0817649, 0.0434371, 0.0690152, 0.0406310, 0.0572720, 0.0376414,
    0.0465628, 0.0344792, 0.0369127, 0.0311559, 0.0283441, 0.0276827, 0.0208771, 0.0240706, 0.0145291, 0.0203300,
    0.0093149, 0.0164706, 0.0052468, 0.0125011, 0.0023342, 0.0084289, 0.0005839, 0.0042603, 0.0000000, 0.0000000};

void generateMesh() {
  gmsh::option::setNumber("Mesh.SecondOrderLinear", 1);
  Eigen::Map<Eigen::Matrix<double, 2, 65>> naca0012_point{naca0012_point_flattened.data()};
  Eigen::Matrix<double, 4, 3, Eigen::RowMajor> farfield_point;
  farfield_point << -10, -10, 0, 10, -10, 0, 10, 10, 0, -10, 10, 0;
  std::vector<int> farfield_point_tag;
  std::vector<int> naca0012_point_tag;
  std::vector<int> farfield_line_tag;
  std::vector<int> naca0012_line_tag;
  gmsh::model::add("naca0012");
  for (const auto& row : farfield_point.rowwise()) {
    farfield_point_tag.emplace_back(gmsh::model::occ::addPoint(row.x(), row.y(), row.z(), 1.0));
  }
  for (const auto& col : naca0012_point.colwise()) {
    naca0012_point_tag.emplace_back(gmsh::model::occ::addPoint(col.x(), col.y(), 0.0, 0.01));
  }
  for (const auto& col : naca0012_point.rowwise().reverse()(Eigen::all, Eigen::seq(1, Eigen::last)).colwise()) {
    naca0012_point_tag.emplace_back(gmsh::model::occ::addPoint(col.x(), -col.y(), 0.0, 0.01));
  }
  for (std::size_t i = 0; i < farfield_point_tag.size(); i++) {
    farfield_line_tag.emplace_back(
        gmsh::model::occ::addLine(farfield_point_tag[i], farfield_point_tag[(i + 1) % farfield_point_tag.size()]));
  }
  naca0012_line_tag.emplace_back(gmsh::model::occ::addSpline(naca0012_point_tag));
  const int center_tag = gmsh::model::occ::addPoint(0.9985510, 0.0000000, 0, 0.01);
  naca0012_line_tag.emplace_back(
      gmsh::model::occ::addCircleArc(naca0012_point_tag.back(), center_tag, naca0012_point_tag.front()));
  const int farfield_line_loop = gmsh::model::occ::addCurveLoop(farfield_line_tag);
  const int naca0012_line_loop = gmsh::model::occ::addCurveLoop(naca0012_line_tag);
  const int naca0012_plane_surface = gmsh::model::occ::addPlaneSurface({farfield_line_loop, naca0012_line_loop});
  gmsh::model::occ::synchronize();
  std::vector<double> naca0012_line_tag_double_cast{naca0012_line_tag.begin(), naca0012_line_tag.end()};
  const int naca0012_boundary_layer = gmsh::model::mesh::field::add("BoundaryLayer");
  gmsh::model::mesh::field::setNumbers(naca0012_boundary_layer, "CurvesList", naca0012_line_tag_double_cast);
  gmsh::model::mesh::field::setNumber(naca0012_boundary_layer, "Size", 0.001);
  gmsh::model::mesh::field::setNumber(naca0012_boundary_layer, "Ratio", 1.2);
  gmsh::model::mesh::field::setNumber(naca0012_boundary_layer, "Quads", 1);
  gmsh::model::mesh::field::setNumber(naca0012_boundary_layer, "Thickness", 0.03);
  gmsh::model::mesh::field::setAsBoundaryLayer(naca0012_boundary_layer);
  gmsh::model::addPhysicalGroup(1, farfield_line_tag, -1, "bc-1");
  gmsh::model::addPhysicalGroup(1, naca0012_line_tag, -1, "bc-2");
  gmsh::model::addPhysicalGroup(2, {naca0012_plane_surface}, -1, "vc-1");
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::setOrder(static_cast<int>(SimulationControl::kPolynomialOrder));
  gmsh::write(kProjectDirectory / "naca0012_2d.msh");
}

void generateMeshShell(){};

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::System<SimulationControl> system(generateMesh, kProjectDirectory / "naca0012_2d.msh");
  system.addInitialCondition("vc-1", []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, 2>& coordinate) {
    return Eigen::Vector<SubrosaDG::Real, 5>{1.4, 0.8, 0.0, 1.0, 1.0};
  });
  system.addBoundaryCondition<SubrosaDG::BoundaryCondition::NormalFarfield>("bc-1", {1.4, 0.8, 0.0, 1.0, 1.0});
  system.addBoundaryCondition<SubrosaDG::BoundaryCondition::NoSlipWall>("bc-2");
  system.setTimeIntegration(false, 1, 1.0, 1e-10);
  system.setViewConfig(-1, kProjectDirectory, "naca0012_2d");
  system.solve();
  return EXIT_SUCCESS;
}
