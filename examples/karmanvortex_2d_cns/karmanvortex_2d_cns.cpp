/**
 * @file karmanvortex_2d_cns.cpp
 * @brief The main file of SubrosaDG karmanvortex_2d_cns.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2024-04-29
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#include "SubrosaDG"

inline const std::string kExampleName{"karmanvortex_2d_cns"};

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory / "build/out" / kExampleName};

using SimulationControl = SubrosaDG::SimulationControl<
    SubrosaDG::SolveControl<SubrosaDG::DimensionEnum::D2, SubrosaDG::PolynomialOrderEnum::P3,
                            SubrosaDG::BoundaryTimeEnum::Steady, SubrosaDG::SourceTermEnum::None>,
    SubrosaDG::NumericalControl<SubrosaDG::MeshModelEnum::TriangleQuadrangle, SubrosaDG::ShockCapturingEnum::None,
                                SubrosaDG::LimiterEnum::None, SubrosaDG::InitialConditionEnum::Function,
                                SubrosaDG::TimeIntegrationEnum::SSPRK3>,
    SubrosaDG::CompresibleNSVariable<SubrosaDG::ThermodynamicModelEnum::Constant,
                                     SubrosaDG::EquationOfStateEnum::IdealGas,
                                     SubrosaDG::TransportModelEnum::Sutherland, SubrosaDG::ConvectiveFluxEnum::HLLC,
                                     SubrosaDG::ViscousFluxEnum::BR2>>;

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::System<SimulationControl> system;
  system.setMesh(kExampleDirectory / std::format("{}.msh", kExampleName), generateMesh);
  system.addInitialCondition(
      []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
          -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
        return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{1.4_r, 0.2_r, 0.0_r, 1.0_r};
      });
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::RiemannFarfield>(
      "bc-1",
      []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
          -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
        return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{1.4_r, 0.2_r, 0.0_r, 1.0_r};
      });
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticNonSlipWall>(
      "bc-2",
      []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
          -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
        return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{1.4_r, 0.0_r, 0.0_r, 1.0_r};
      });
  system.setThermodynamicModel<SimulationControl::kThermodynamicModel>(2.5_r, 25.0_r / 14.0_r);
  system.setTransportModel<SimulationControl::kTransportModel>(1.4_r * 0.2_r / 200.0_r);
  system.setTimeIntegration(1.0_r);
  system.setViewConfig(kExampleDirectory, kExampleName);
  system.addViewVariable({SubrosaDG::ViewVariableEnum::Density, SubrosaDG::ViewVariableEnum::Velocity,
                          SubrosaDG::ViewVariableEnum::Pressure, SubrosaDG::ViewVariableEnum::Temperature,
                          SubrosaDG::ViewVariableEnum::MachNumber, SubrosaDG::ViewVariableEnum::Vorticity});
  system.synchronize();
  system.solve();
  system.view();
  return EXIT_SUCCESS;
}

void generateMesh(const std::filesystem::path& mesh_file_path) {
  Eigen::Matrix<double, 6, 3, Eigen::RowMajor> farfield_point_coordinate;
  Eigen::Matrix<double, 4, 3, Eigen::RowMajor> separation_point_coordinate;
  Eigen::Matrix<double, 4, 3, Eigen::RowMajor> cylinder_point_coordinate;
  Eigen::Matrix<double, 6, 1> farfield_point_size;
  // clang-format off
  farfield_point_coordinate << -5.0,   0.0,  0.0,
                               -5.0,  -10.0, 0.0,
                                20.0, -10.0, 0.0,
                                20.0,  0.0,  0.0,
                                20.0,  10.0, 0.0,
                               -5.0,   10.0, 0.0;
  separation_point_coordinate << -1.0,  0.0, 0.0,
                                  0.0, -1.0, 0.0,
                                  1.0,  0.0, 0.0,
                                  0.0,  1.0, 0.0;
  cylinder_point_coordinate << -0.5,  0.0, 0.0,
                                0.0, -0.5, 0.0,
                                0.5,  0.0, 0.0,
                                0.0,  0.5, 0.0;
  farfield_point_size << 0.5, 2.0, 2.0, 0.5, 2.0, 2.0;
  // clang-format on
  Eigen::Array<int, 6, 1> farfield_point_tag;
  Eigen::Array<int, 4, 2, Eigen::RowMajor> cylinder_point_tag;
  Eigen::Array<int, 6, 1> farfield_line_tag;
  Eigen::Array<int, 2, 1> connection_line_tag;
  Eigen::Array<int, 4, 3, Eigen::RowMajor> cylinder_line_tag;
  Eigen::Array<int, 6, 1> curve_loop_tag;
  Eigen::Array<int, 6, 1> plane_surface_tag;
  std::array<std::vector<int>, 3> physical_group_tag;
  gmsh::model::add("karmanvortex_2d");
  const int center_point_tag = gmsh::model::geo::addPoint(0.0, 0.0, 0.0);
  for (std::ptrdiff_t i = 0; i < 6; i++) {
    farfield_point_tag(i) = gmsh::model::geo::addPoint(farfield_point_coordinate(i, 0), farfield_point_coordinate(i, 1),
                                                       farfield_point_coordinate(i, 2), farfield_point_size(i));
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    cylinder_point_tag(i, 0) = gmsh::model::geo::addPoint(
        separation_point_coordinate(i, 0), separation_point_coordinate(i, 1), separation_point_coordinate(i, 2), 0.1);
    cylinder_point_tag(i, 1) = gmsh::model::geo::addPoint(
        cylinder_point_coordinate(i, 0), cylinder_point_coordinate(i, 1), cylinder_point_coordinate(i, 2));
  }
  for (std::ptrdiff_t i = 0; i < 6; i++) {
    farfield_line_tag(i) = gmsh::model::geo::addLine(farfield_point_tag(i), farfield_point_tag((i + 1) % 6));
  }
  connection_line_tag(0) = gmsh::model::geo::addLine(farfield_point_tag(0), cylinder_point_tag(0, 0));
  connection_line_tag(1) = gmsh::model::geo::addLine(farfield_point_tag(3), cylinder_point_tag(2, 0));
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    cylinder_line_tag(i, 0) =
        gmsh::model::geo::addCircleArc(cylinder_point_tag(i, 0), center_point_tag, cylinder_point_tag((i + 1) % 4, 0));
    cylinder_line_tag(i, 1) =
        gmsh::model::geo::addCircleArc(cylinder_point_tag(i, 1), center_point_tag, cylinder_point_tag((i + 1) % 4, 1));
    cylinder_line_tag(i, 2) = gmsh::model::geo::addLine(cylinder_point_tag(i, 0), cylinder_point_tag(i, 1));
  }
  curve_loop_tag(0) = gmsh::model::geo::addCurveLoop({farfield_line_tag(0), farfield_line_tag(1), farfield_line_tag(2),
                                                      connection_line_tag(1), -cylinder_line_tag(1, 0),
                                                      -cylinder_line_tag(0, 0), -connection_line_tag(0)});
  curve_loop_tag(1) = gmsh::model::geo::addCurveLoop(
      {farfield_line_tag(5), connection_line_tag(0), -cylinder_line_tag(3, 0), -cylinder_line_tag(2, 0),
       -connection_line_tag(1), farfield_line_tag(3), farfield_line_tag(4)});
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    curve_loop_tag(i + 2, 0) =
        gmsh::model::geo::addCurveLoop({-cylinder_line_tag(i, 2), cylinder_line_tag(i, 0),
                                        cylinder_line_tag((i + 1) % 4, 2), -cylinder_line_tag(i, 1)});
  }
  for (std::ptrdiff_t i = 0; i < 6; i++) {
    plane_surface_tag(i) = gmsh::model::geo::addPlaneSurface({curve_loop_tag(i)});
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    gmsh::model::geo::mesh::setTransfiniteCurve(cylinder_line_tag(i, 0), 16);
    gmsh::model::geo::mesh::setTransfiniteCurve(cylinder_line_tag(i, 1), 16);
    gmsh::model::geo::mesh::setTransfiniteCurve(cylinder_line_tag(i, 2), 12, "Progression", -1.2);
  }
  for (std::ptrdiff_t i = 2; i < 6; i++) {
    gmsh::model::geo::mesh::setTransfiniteSurface(plane_surface_tag(i));
    gmsh::model::geo::mesh::setRecombine(2, plane_surface_tag(i));
  }
  gmsh::model::geo::synchronize();
  for (std::ptrdiff_t i = 0; i < 6; i++) {
    physical_group_tag[0].emplace_back(farfield_line_tag(i));
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    physical_group_tag[1].emplace_back(cylinder_line_tag(i, 1));
  }
  for (std::ptrdiff_t i = 0; i < 6; i++) {
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
