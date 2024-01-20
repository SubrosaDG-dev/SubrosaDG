/**
 * @file naca0012_2d_euler.cpp
 * @brief The source file for SubrosaDG example naca0012_2d_euler.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2022-11-02
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#include <gmsh.h>

#include <Eigen/Cholesky>
#include <Eigen/Core>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <vector>

#include "SubrosaDG"

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory /
                                                     "build/out/naca0012_2d_euler"};

using SimulationControl =
    SubrosaDG::SimulationControlEuler<2, SubrosaDG::PolynomialOrderEnum::P1, SubrosaDG::MeshModelEnum::Quadrangle,
                                      SubrosaDG::ThermodynamicModelEnum::ConstantE,
                                      SubrosaDG::EquationOfStateEnum::IdealGas, SubrosaDG::ConvectiveFluxEnum::HLLC,
                                      SubrosaDG::TimeIntegrationEnum::SSPRK3, SubrosaDG::ViewModelEnum::Vtu>;

inline std::array<double, 64> naca0012_point_x_array{
    0.9994160, 0.9976658, 0.9947532, 0.9906850, 0.9854709, 0.9791229, 0.9716559, 0.9630873, 0.9534372, 0.9427280,
    0.9309849, 0.9182351, 0.9045085, 0.8898372, 0.8742554, 0.8577995, 0.8405079, 0.8224211, 0.8035813, 0.7840324,
    0.7638202, 0.7429917, 0.7215958, 0.6996823, 0.6773025, 0.6545085, 0.6313537, 0.6078921, 0.5841786, 0.5602683,
    0.5362174, 0.5120819, 0.4879181, 0.4637826, 0.4397317, 0.4158215, 0.3921079, 0.3686463, 0.3454915, 0.3226976,
    0.3003177, 0.2784042, 0.2570083, 0.2361799, 0.2159676, 0.1964187, 0.1775789, 0.1594921, 0.1422005, 0.1257446,
    0.1101628, 0.0954915, 0.0817649, 0.0690152, 0.0572720, 0.0465628, 0.0369127, 0.0283441, 0.0208771, 0.0145291,
    0.0093149, 0.0052468, 0.0023342, 0.0005839};

void generateMesh(const std::filesystem::path& mesh_file_path) {
  Eigen::Map<Eigen::RowVector<double, 64>> naca0012_point_x{naca0012_point_x_array.data()};
  Eigen::Matrix<double, 2, 64, Eigen::RowMajor> naca0012_point;
  naca0012_point.row(0) = naca0012_point_x;
  // NOTE: Here we use the alternative form of NACA0012 airfoil which is closed at the trailing edge.
  // y= +- 0.594689181*[0.298222773*sqrt(x) - 0.127125232*x - 0.357907906*x2 + 0.291984971*x3 - 0.105174606*x4]
  // More details can be found at https://turbmodels.larc.nasa.gov/naca0012_val.html.
  naca0012_point.row(1) =
      0.594689181 * (0.298222773 * naca0012_point_x.array().sqrt() - 0.127125232 * naca0012_point_x.array() -
                     0.357907906 * naca0012_point_x.array().pow(2) + 0.291984971 * naca0012_point_x.array().pow(3) -
                     0.105174606 * naca0012_point_x.array().pow(4));
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
  std::array<std::vector<int>, 2> naca0012_point_tag;
  Eigen::Array<int, 6, 1> farfield_line_tag;
  Eigen::Array<int, 2, 1> naca0012_line_tag;
  Eigen::Array<int, 4, 1> connection_line_tag;
  Eigen::Array<int, 4, 1> curve_loop_tag;
  Eigen::Array<int, 4, 1> plane_surface_tag;
  std::array<std::vector<int>, 3> physical_group_tag;
  gmsh::model::add("naca0012");
  const int naca0012_leading_edge_point_tag = gmsh::model::geo::addPoint(0.0, 0.0, 0.0);
  const int naca0012_trailing_edge_point_tag = gmsh::model::geo::addPoint(1.0, 0.0, 0.0);
  for (std::ptrdiff_t i = 0; i < 6; i++) {
    farfield_point_tag(i) =
        gmsh::model::geo::addPoint(farfield_point(i, 0), farfield_point(i, 1), farfield_point(i, 2));
  }
  for (std::size_t i = 0; i < 2; i++) {
    naca0012_point_tag[i].emplace_back(naca0012_trailing_edge_point_tag);
  }
  for (std::ptrdiff_t i = 0; i < 64; i++) {
    naca0012_point_tag[0].emplace_back(gmsh::model::geo::addPoint(naca0012_point(0, i), naca0012_point(1, i), 0.0));
    naca0012_point_tag[1].emplace_back(gmsh::model::geo::addPoint(naca0012_point(0, i), -naca0012_point(1, i), 0.0));
  }
  for (std::size_t i = 0; i < 2; i++) {
    naca0012_point_tag[i].emplace_back(naca0012_leading_edge_point_tag);
  }
  for (std::ptrdiff_t i = 0; i < 6; i++) {
    if (i < 2) {
      farfield_line_tag(i) = gmsh::model::geo::addCircleArc(farfield_point_tag(i), naca0012_trailing_edge_point_tag,
                                                            farfield_point_tag(i + 1));
    } else {
      farfield_line_tag(i) = gmsh::model::geo::addLine(farfield_point_tag(i), farfield_point_tag((i + 1) % 6));
    }
  }
  connection_line_tag(0) = gmsh::model::geo::addLine(farfield_point_tag(0), naca0012_trailing_edge_point_tag);
  connection_line_tag(1) = gmsh::model::geo::addLine(farfield_point_tag(1), naca0012_leading_edge_point_tag);
  connection_line_tag(2) = gmsh::model::geo::addLine(farfield_point_tag(2), naca0012_trailing_edge_point_tag);
  connection_line_tag(3) = gmsh::model::geo::addLine(farfield_point_tag(4), naca0012_trailing_edge_point_tag);
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    naca0012_line_tag(i) = gmsh::model::geo::addSpline(naca0012_point_tag[static_cast<std::size_t>(i)]);
  }
  curve_loop_tag(0) = gmsh::model::geo::addCurveLoop(
      {-connection_line_tag(0), farfield_line_tag(0), connection_line_tag(1), -naca0012_line_tag(0)});
  curve_loop_tag(1) = gmsh::model::geo::addCurveLoop(
      {-connection_line_tag(1), farfield_line_tag(1), connection_line_tag(2), naca0012_line_tag(1)});
  curve_loop_tag(2) = gmsh::model::geo::addCurveLoop(
      {-connection_line_tag(2), farfield_line_tag(2), farfield_line_tag(3), connection_line_tag(3)});
  curve_loop_tag(3) = gmsh::model::geo::addCurveLoop(
      {-connection_line_tag(3), farfield_line_tag(4), farfield_line_tag(5), connection_line_tag(0)});
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    plane_surface_tag(i) = gmsh::model::geo::addPlaneSurface({curve_loop_tag(i)});
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    gmsh::model::geo::mesh::setTransfiniteCurve(naca0012_line_tag(i), 40, "Progression", -1.08);
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag(i), 40);
  }
  gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag(2), 20, "Progression", 1.25);
  gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag(3), 20, "Progression", -1.25);
  gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag(4), 20, "Progression", 1.25);
  gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag(5), 20, "Progression", -1.25);
  gmsh::model::geo::mesh::setTransfiniteCurve(connection_line_tag(0), 20, "Progression", -1.25);
  gmsh::model::geo::mesh::setTransfiniteCurve(connection_line_tag(1), 20, "Progression", -1.2);
  gmsh::model::geo::mesh::setTransfiniteCurve(connection_line_tag(2), 20, "Progression", -1.25);
  gmsh::model::geo::mesh::setTransfiniteCurve(connection_line_tag(3), 20, "Progression", -1.25);
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    gmsh::model::geo::mesh::setTransfiniteSurface(plane_surface_tag(i));
    gmsh::model::geo::mesh::setRecombine(2, plane_surface_tag(i));
  }
  gmsh::model::geo::synchronize();
  for (std::ptrdiff_t i = 0; i < 6; i++) {
    physical_group_tag[0].emplace_back(farfield_line_tag(i));
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    physical_group_tag[1].emplace_back(naca0012_line_tag(i));
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
  SubrosaDG::System<SimulationControl> system{};
  system.setMesh(kExampleDirectory / "naca0012_2d.msh", generateMesh);
  system.addInitialCondition("vc-1", []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, 2>& coordinate) {
    return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{
        1.4, 0.63 * std::cos(SubrosaDG::toRadian(2.0)), 0.63 * std::sin(SubrosaDG::toRadian(2.0)), 1.0};
  });
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::RiemannFarfield>(
      "bc-1", {1.4, 0.63 * std::cos(SubrosaDG::toRadian(2.0)), 0.63 * std::sin(SubrosaDG::toRadian(2.0)), 1.0});
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticWall>("bc-2");
  system.synchronize();
  system.setTimeIntegration(false, 1, 1.5, 1e-10);
  system.setViewConfig(-1, kExampleDirectory, "naca0012_2d", SubrosaDG::ViewConfigEnum::SolverSmoothness);
  system.setViewVariable({SubrosaDG::ViewVariableEnum::Density, SubrosaDG::ViewVariableEnum::Velocity,
                          SubrosaDG::ViewVariableEnum::Pressure, SubrosaDG::ViewVariableEnum::Temperature,
                          SubrosaDG::ViewVariableEnum::MachNumber});
  system.solve();
  system.view();
  return EXIT_SUCCESS;
}
