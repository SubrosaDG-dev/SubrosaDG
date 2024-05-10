/**
 * @file rae2822_2d_ns.cpp
 * @brief The main file of SubrosaDG RAE2822 2D Navier-Stokes simulation.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2024-04-20
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#include "SubrosaDG"

inline const std::string kExampleName{"rae2822_2d_ns"};

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory / "build/out" / kExampleName};

using SimulationControl = SubrosaDG::SimulationControlNavierStokes<
    2, SubrosaDG::PolynomialOrderEnum::P3, SubrosaDG::MeshModelEnum::Quadrangle,
    SubrosaDG::ThermodynamicModelEnum::ConstantE, SubrosaDG::EquationOfStateEnum::IdealGas,
    SubrosaDG::TransportModelEnum::Constant, SubrosaDG::ConvectiveFluxEnum::HLLC, SubrosaDG::ViscousFluxEnum::BR2,
    SubrosaDG::TimeIntegrationEnum::SSPRK3, SubrosaDG::PolynomialOrderEnum::P1, SubrosaDG::ViewModelEnum::Vtu>;

inline std::array<double, 63> rae2822_point_x_array{
    0.000602, 0.002408, 0.005412, 0.009607, 0.014984, 0.021530, 0.029228, 0.038060, 0.048005, 0.059039, 0.071136,
    0.084265, 0.098396, 0.113495, 0.129524, 0.146447, 0.164221, 0.182803, 0.202150, 0.222215, 0.242949, 0.264302,
    0.286222, 0.308658, 0.331555, 0.354858, 0.378510, 0.402455, 0.426635, 0.450991, 0.475466, 0.500000, 0.524534,
    0.549009, 0.573365, 0.597545, 0.621490, 0.645142, 0.668445, 0.691342, 0.713778, 0.735698, 0.757051, 0.777785,
    0.797850, 0.817197, 0.835779, 0.853553, 0.870476, 0.886505, 0.901604, 0.915735, 0.928864, 0.940961, 0.951995,
    0.961940, 0.970772, 0.978470, 0.985016, 0.990393, 0.994588, 0.997592, 0.999398};

inline std::array<double, 63> rae2822_upper_point_y_array{
    0.003165, 0.006306, 0.009416, 0.012480, 0.015489, 0.018441, 0.021348, 0.024219, 0.027062, 0.029874, 0.032644,
    0.035360, 0.038011, 0.040585, 0.043071, 0.045457, 0.047729, 0.049874, 0.051885, 0.053753, 0.055470, 0.057026,
    0.058414, 0.059629, 0.060660, 0.061497, 0.062133, 0.062562, 0.062779, 0.062774, 0.062530, 0.062029, 0.061254,
    0.060194, 0.058845, 0.057218, 0.055344, 0.053258, 0.050993, 0.048575, 0.046029, 0.043377, 0.040641, 0.037847,
    0.035017, 0.032176, 0.029347, 0.026554, 0.023817, 0.021153, 0.018580, 0.016113, 0.013769, 0.011562, 0.009508,
    0.007622, 0.005915, 0.004401, 0.003092, 0.002001, 0.001137, 0.000510, 0.000128};

inline std::array<double, 63> rae2822_lower_point_y_array{
    -0.003160, -0.006308, -0.009443, -0.012559, -0.015649, -0.018707, -0.021722, -0.024685, -0.027586,
    -0.030416, -0.033170, -0.035843, -0.038431, -0.040929, -0.043326, -0.045610, -0.047773, -0.049805,
    -0.051694, -0.053427, -0.054994, -0.056376, -0.057547, -0.058459, -0.059046, -0.059236, -0.058974,
    -0.058224, -0.056979, -0.055257, -0.053099, -0.050563, -0.047719, -0.044642, -0.041397, -0.038043,
    -0.034631, -0.031207, -0.027814, -0.024495, -0.021289, -0.018232, -0.015357, -0.012690, -0.010244,
    -0.008027, -0.006048, -0.004314, -0.002829, -0.001592, -0.000600, 0.000157,  0.000694,  0.001033,
    0.001197,  0.001212,  0.001112,  0.000935,  0.000719,  0.000497,  0.000296,  0.000137,  0.000035};

void generateMesh(const std::filesystem::path& mesh_file_path) {
  Eigen::Matrix<double, 3, 63, Eigen::RowMajor> rae2822_point;
  rae2822_point.row(0) = Eigen::Map<Eigen::RowVector<double, 63>>{rae2822_point_x_array.data()};
  rae2822_point.row(1) = Eigen::Map<Eigen::RowVector<double, 63>>{rae2822_upper_point_y_array.data()};
  rae2822_point.row(2) = Eigen::Map<Eigen::RowVector<double, 63>>{rae2822_lower_point_y_array.data()};
  Eigen::Matrix<double, 6, 3, Eigen::RowMajor> farfield_point;
  // clang-format off
  farfield_point <<  1.0,  3.0, 0.0,
                    -2.0,  0.0, 0.0,
                     1.0, -3.0, 0.0,
                     3.0, -3.0, 0.0,
                     3.0,  0.0, 0.0,
                     3.0,  3.0, 0.0;
  // clang-format on
  Eigen::Array<int, 6, 1> farfield_point_tag;
  std::array<std::vector<int>, 2> rae2822_point_tag;
  Eigen::Array<int, 6, 1> farfield_line_tag;
  Eigen::Array<int, 2, 1> rae2822_line_tag;
  Eigen::Array<int, 4, 1> connection_line_tag;
  Eigen::Array<int, 4, 1> curve_loop_tag;
  Eigen::Array<int, 3, 1> plane_surface_tag;
  std::array<std::vector<int>, 4> physical_group_tag;
  gmsh::model::add("rae2822");
  const int rae2822_leading_edge_point_tag = gmsh::model::geo::addPoint(0.0, 0.0, 0.0);
  const int rae2822_trailing_edge_point_tag = gmsh::model::geo::addPoint(1.0, 0.0, 0.0);
  for (std::ptrdiff_t i = 0; i < 6; i++) {
    farfield_point_tag(i) =
        gmsh::model::geo::addPoint(farfield_point(i, 0), farfield_point(i, 1), farfield_point(i, 2));
  }
  for (std::size_t i = 0; i < 2; i++) {
    rae2822_point_tag[i].emplace_back(rae2822_leading_edge_point_tag);
  }
  for (std::ptrdiff_t i = 0; i < 63; i++) {
    rae2822_point_tag[0].emplace_back(gmsh::model::geo::addPoint(rae2822_point(0, i), rae2822_point(1, i), 0.0));
    rae2822_point_tag[1].emplace_back(gmsh::model::geo::addPoint(rae2822_point(0, i), rae2822_point(2, i), 0.0));
  }
  for (std::size_t i = 0; i < 2; i++) {
    rae2822_point_tag[i].emplace_back(rae2822_trailing_edge_point_tag);
  }
  for (std::ptrdiff_t i = 0; i < 6; i++) {
    if (i < 2) {
      farfield_line_tag(i) = gmsh::model::geo::addCircleArc(farfield_point_tag(i), rae2822_trailing_edge_point_tag,
                                                            farfield_point_tag(i + 1));
    } else {
      farfield_line_tag(i) = gmsh::model::geo::addLine(farfield_point_tag(i), farfield_point_tag((i + 1) % 6));
    }
  }
  connection_line_tag(0) = gmsh::model::geo::addLine(farfield_point_tag(0), rae2822_trailing_edge_point_tag);
  connection_line_tag(1) = gmsh::model::geo::addLine(farfield_point_tag(1), rae2822_leading_edge_point_tag);
  connection_line_tag(2) = gmsh::model::geo::addLine(farfield_point_tag(2), rae2822_trailing_edge_point_tag);
  connection_line_tag(3) = gmsh::model::geo::addLine(farfield_point_tag(4), rae2822_trailing_edge_point_tag);
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    rae2822_line_tag(i) = gmsh::model::geo::addSpline(rae2822_point_tag[static_cast<std::size_t>(i)]);
  }
  curve_loop_tag(0) = gmsh::model::geo::addCurveLoop(
      {-connection_line_tag(0), farfield_line_tag(0), connection_line_tag(1), rae2822_line_tag(0)});
  curve_loop_tag(1) = gmsh::model::geo::addCurveLoop(
      {-connection_line_tag(1), farfield_line_tag(1), connection_line_tag(2), -rae2822_line_tag(1)});
  curve_loop_tag(2) = gmsh::model::geo::addCurveLoop(
      {-connection_line_tag(2), farfield_line_tag(2), farfield_line_tag(3), connection_line_tag(3)});
  curve_loop_tag(3) = gmsh::model::geo::addCurveLoop(
      {-connection_line_tag(3), farfield_line_tag(4), farfield_line_tag(5), connection_line_tag(0)});
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    plane_surface_tag(i) = gmsh::model::geo::addPlaneSurface({curve_loop_tag(i)});
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    gmsh::model::geo::mesh::setTransfiniteCurve(rae2822_line_tag(i), 40, "Progression", 1.08);
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag(i), 40);
  }
  gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag(2), 20);
  gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag(3), 20, "Progression", -1.4);
  gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag(4), 20, "Progression", 1.4);
  gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag(5), 20);
  gmsh::model::geo::mesh::setTransfiniteCurve(connection_line_tag(0), 20, "Progression", -1.4);
  gmsh::model::geo::mesh::setTransfiniteCurve(connection_line_tag(1), 20, "Progression", -1.35);
  gmsh::model::geo::mesh::setTransfiniteCurve(connection_line_tag(2), 20, "Progression", -1.4);
  gmsh::model::geo::mesh::setTransfiniteCurve(connection_line_tag(3), 20);
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    gmsh::model::geo::mesh::setTransfiniteSurface(plane_surface_tag(i));
    gmsh::model::geo::mesh::setRecombine(2, plane_surface_tag(i));
  }
  gmsh::model::geo::synchronize();
  for (std::ptrdiff_t i = 0; i < 6; i++) {
    physical_group_tag[0].emplace_back(farfield_line_tag(i));
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    physical_group_tag[1].emplace_back(rae2822_line_tag(i));
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    physical_group_tag[2].emplace_back(plane_surface_tag(i));
  }
  gmsh::model::addPhysicalGroup(1, physical_group_tag[0], -1, "bc-1");
  gmsh::model::addPhysicalGroup(1, physical_group_tag[1], -1, "bc-2");
  gmsh::model::addPhysicalGroup(2, physical_group_tag[2], -1, "vc-1");
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::setOrder(static_cast<int>(SimulationControl::kPolynomialOrder));
  gmsh::write(mesh_file_path);
}

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::System<SimulationControl> system;
  system.setMesh(kExampleDirectory / "rae2822_2d.msh", generateMesh);
  system.addInitialCondition("vc-1", []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, 2>& coordinate) {
    return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{
        1.4, 0.4 * std::cos(SubrosaDG::toRadian(2.79)), 0.4 * std::sin(SubrosaDG::toRadian(2.79)), 1.0};
  });
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::RiemannFarfield>(
      "bc-1", {1.4, 0.4 * std::cos(SubrosaDG::toRadian(2.79)), 0.4 * std::sin(SubrosaDG::toRadian(2.79)), 1.0});
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticNoSlipWall>("bc-2");
  system.setTransportModel(1.4 * 0.4 / 6.5e6);
  system.setTimeIntegration(1.0);
  system.setViewConfig(kExampleDirectory, kExampleName);
  system.setViewVariable({SubrosaDG::ViewVariableEnum::Density, SubrosaDG::ViewVariableEnum::Velocity,
                          SubrosaDG::ViewVariableEnum::Pressure, SubrosaDG::ViewVariableEnum::Temperature,
                          SubrosaDG::ViewVariableEnum::MachNumber, SubrosaDG::ViewVariableEnum::Vorticity});
  system.synchronize();
  system.solve();
  system.view();
  return EXIT_SUCCESS;
}
