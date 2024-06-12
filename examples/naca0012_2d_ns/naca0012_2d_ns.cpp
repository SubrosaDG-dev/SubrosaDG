/**
 * @file naca0012_2d_ns.cpp
 * @brief The source file for SubrosaDG example naca0012_2d_ns.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2024-05-08
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#include "SubrosaDG"

inline const std::string kExampleName{"naca0012_2d_ns"};

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory / "build/out" / kExampleName};

using SimulationControl = SubrosaDG::SimulationControlNavierStokes<
    SubrosaDG::DimensionEnum::D2, SubrosaDG::PolynomialOrderEnum::P3, SubrosaDG::MeshModelEnum::Quadrangle,
    SubrosaDG::SourceTermEnum::None, SubrosaDG::InitialConditionEnum::Function,
    SubrosaDG::ThermodynamicModelEnum::ConstantE, SubrosaDG::EquationOfStateEnum::IdealGas,
    SubrosaDG::TransportModelEnum::Sutherland, SubrosaDG::ConvectiveFluxEnum::HLLC, SubrosaDG::ViscousFluxEnum::BR2,
    SubrosaDG::TimeIntegrationEnum::SSPRK3>;

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::System<SimulationControl> system;
  system.setMesh(kExampleDirectory / "naca0012_2d_ns.msh", generateMesh);
  system.addInitialCondition(
      []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
          -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
        return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{
            1.4, 0.2 * std::cos(SubrosaDG::toRadian(30.0)), 0.2 * std::sin(SubrosaDG::toRadian(30.0)), 1.0};
      });
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::RiemannFarfield>(
      "bc-1",
      []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
          -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
        return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{
            1.4, 0.2 * std::cos(SubrosaDG::toRadian(30.0)), 0.2 * std::sin(SubrosaDG::toRadian(30.0)), 1.0};
      });
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticNoSlipWall>("bc-2");
  system.setTransportModel(1.4 * 0.2 / 16000);
  system.setTimeIntegration(0.5);
  system.setViewConfig(kExampleDirectory, kExampleName);
  system.addViewVariable({SubrosaDG::ViewVariableEnum::Density, SubrosaDG::ViewVariableEnum::Velocity,
                          SubrosaDG::ViewVariableEnum::Pressure, SubrosaDG::ViewVariableEnum::Temperature,
                          SubrosaDG::ViewVariableEnum::MachNumber, SubrosaDG::ViewVariableEnum::Vorticity});
  system.synchronize();
  system.solve();
  system.view();
  return EXIT_SUCCESS;
}

inline std::array<double, 64> naca0012_point_x_array{
    0.000584, 0.002334, 0.005247, 0.009315, 0.014529, 0.020877, 0.028344, 0.036913, 0.046563, 0.057272, 0.069015,
    0.081765, 0.095492, 0.110163, 0.125745, 0.142201, 0.159492, 0.177579, 0.196419, 0.215968, 0.236180, 0.257008,
    0.278404, 0.300318, 0.322698, 0.345492, 0.368646, 0.392108, 0.415822, 0.439732, 0.463783, 0.487918, 0.512082,
    0.536217, 0.560268, 0.584179, 0.607892, 0.631354, 0.654509, 0.677303, 0.699682, 0.721596, 0.742992, 0.763820,
    0.784032, 0.803581, 0.822421, 0.840508, 0.857800, 0.874255, 0.889837, 0.904509, 0.918235, 0.930985, 0.942728,
    0.953437, 0.963087, 0.971656, 0.979123, 0.985471, 0.990685, 0.994753, 0.997666, 0.999416};

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
    naca0012_point_tag[i].emplace_back(naca0012_leading_edge_point_tag);
  }
  for (std::ptrdiff_t i = 0; i < 64; i++) {
    naca0012_point_tag[0].emplace_back(gmsh::model::geo::addPoint(naca0012_point(0, i), naca0012_point(1, i), 0.0));
    naca0012_point_tag[1].emplace_back(gmsh::model::geo::addPoint(naca0012_point(0, i), -naca0012_point(1, i), 0.0));
  }
  for (std::size_t i = 0; i < 2; i++) {
    naca0012_point_tag[i].emplace_back(naca0012_trailing_edge_point_tag);
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
      {-connection_line_tag(0), farfield_line_tag(0), connection_line_tag(1), naca0012_line_tag(0)});
  curve_loop_tag(1) = gmsh::model::geo::addCurveLoop(
      {-connection_line_tag(1), farfield_line_tag(1), connection_line_tag(2), -naca0012_line_tag(1)});
  curve_loop_tag(2) = gmsh::model::geo::addCurveLoop(
      {-connection_line_tag(2), farfield_line_tag(2), farfield_line_tag(3), connection_line_tag(3)});
  curve_loop_tag(3) = gmsh::model::geo::addCurveLoop(
      {-connection_line_tag(3), farfield_line_tag(4), farfield_line_tag(5), connection_line_tag(0)});
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    plane_surface_tag(i) = gmsh::model::geo::addPlaneSurface({curve_loop_tag(i)});
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    gmsh::model::geo::mesh::setTransfiniteCurve(naca0012_line_tag(i), 60, "Progression", 1.05);
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag(i), 60);
  }
  gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag(2), 20, "Progression", 1.1);
  gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag(3), 30, "Progression", -1.25);
  gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag(4), 30, "Progression", 1.25);
  gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag(5), 20, "Progression", -1.1);
  gmsh::model::geo::mesh::setTransfiniteCurve(connection_line_tag(0), 30, "Progression", -1.25);
  gmsh::model::geo::mesh::setTransfiniteCurve(connection_line_tag(1), 30, "Progression", -1.25);
  gmsh::model::geo::mesh::setTransfiniteCurve(connection_line_tag(2), 30, "Progression", -1.25);
  gmsh::model::geo::mesh::setTransfiniteCurve(connection_line_tag(3), 20, "Progression", -1.1);
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
  gmsh::model::mesh::setOrder(SimulationControl::kPolynomialOrder);
  gmsh::model::mesh::optimize("HighOrder");
  gmsh::write(mesh_file_path);
}
