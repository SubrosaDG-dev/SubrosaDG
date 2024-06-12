/**
 * @file blasius_3d_ns.cpp
 * @brief The 3D protuberance flow example with Euler equations.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2024-05-20
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#include "SubrosaDG"

inline const std::string kExampleName{"blasius_3d_ns"};

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory / "build/out" / kExampleName};

using SimulationControl = SubrosaDG::SimulationControlNavierStokes<
    SubrosaDG::DimensionEnum::D3, SubrosaDG::PolynomialOrderEnum::P1, SubrosaDG::MeshModelEnum::Hexahedron,
    SubrosaDG::SourceTermEnum::None, SubrosaDG::InitialConditionEnum::Function,
    SubrosaDG::ThermodynamicModelEnum::ConstantE, SubrosaDG::EquationOfStateEnum::IdealGas,
    SubrosaDG::TransportModelEnum::Constant, SubrosaDG::ConvectiveFluxEnum::HLLC, SubrosaDG::ViscousFluxEnum::BR2,
    SubrosaDG::TimeIntegrationEnum::SSPRK3>;

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::System<SimulationControl> system;
  system.setMesh(kExampleDirectory / "blasius_3d_ns.msh", generateMesh);
  system.addInitialCondition(
      []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
          -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
        return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{1.4, 0.0, 0.5, 0.0, 1.0};
      });
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::RiemannFarfield>(
      "bc-1",
      []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
          -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
        return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{1.4, 0.0, 0.5, 0.0, 1.0};
      });
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::PressureOutflow>(
      "bc-2",
      []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
          -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
        return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{1.4, 0.0, 0.5, 0.0, 1.0};
      });
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::Periodic>("bc-3");
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticSlipWall>("bc-4");
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticNoSlipWall>("bc-5");
  system.setTransportModel(1.4 * 0.5 / 100000);
  system.setTimeIntegration(1.0);
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
  Eigen::Matrix<double, 12, 3, Eigen::RowMajor> hexahedron_point_coordinate;
  // clang-format off
  hexahedron_point_coordinate << 0.0, 0.0, 0.0,
                                 0.1, 0.0, 0.0,
                                 0.1, 0.5, 0.0,
                                 0.1, 1.5, 0.0,
                                 0.0, 1.5, 0.0,
                                 0.0, 0.5, 0.0,
                                 0.0, 0.0, 0.5,
                                 0.1, 0.0, 0.5,
                                 0.1, 0.5, 0.5,
                                 0.1, 1.5, 0.5,
                                 0.0, 1.5, 0.5,
                                 0.0, 0.5, 0.5;
  // clang-format on
  Eigen::Array<int, 12, 1> point_tag;
  Eigen::Array<int, 20, 1> line_tag;
  Eigen::Array<int, 11, 1> curve_loop_tag;
  Eigen::Array<int, 11, 1> surface_filling_tag;
  Eigen::Array<int, 2, 1> surface_loop_tag;
  Eigen::Array<int, 2, 1> volume_tag;
  std::array<std::vector<int>, 6> physical_group_tag;
  gmsh::model::add("blasius_3d");
  for (std::ptrdiff_t i = 0; i < 12; i++) {
    point_tag(i) = gmsh::model::geo::addPoint(hexahedron_point_coordinate(i, 0), hexahedron_point_coordinate(i, 1),
                                              hexahedron_point_coordinate(i, 2));
  }
  line_tag(0) = gmsh::model::geo::addLine(point_tag(0), point_tag(1));
  line_tag(1) = gmsh::model::geo::addLine(point_tag(5), point_tag(2));
  line_tag(2) = gmsh::model::geo::addLine(point_tag(4), point_tag(3));
  line_tag(3) = gmsh::model::geo::addLine(point_tag(0), point_tag(5));
  line_tag(4) = gmsh::model::geo::addLine(point_tag(5), point_tag(4));
  line_tag(5) = gmsh::model::geo::addLine(point_tag(1), point_tag(2));
  line_tag(6) = gmsh::model::geo::addLine(point_tag(2), point_tag(3));
  line_tag(7) = gmsh::model::geo::addLine(point_tag(6), point_tag(7));
  line_tag(8) = gmsh::model::geo::addLine(point_tag(11), point_tag(8));
  line_tag(9) = gmsh::model::geo::addLine(point_tag(10), point_tag(9));
  line_tag(10) = gmsh::model::geo::addLine(point_tag(6), point_tag(11));
  line_tag(11) = gmsh::model::geo::addLine(point_tag(11), point_tag(10));
  line_tag(12) = gmsh::model::geo::addLine(point_tag(7), point_tag(8));
  line_tag(13) = gmsh::model::geo::addLine(point_tag(8), point_tag(9));
  line_tag(14) = gmsh::model::geo::addLine(point_tag(0), point_tag(6));
  line_tag(15) = gmsh::model::geo::addLine(point_tag(5), point_tag(11));
  line_tag(16) = gmsh::model::geo::addLine(point_tag(4), point_tag(10));
  line_tag(17) = gmsh::model::geo::addLine(point_tag(1), point_tag(7));
  line_tag(18) = gmsh::model::geo::addLine(point_tag(2), point_tag(8));
  line_tag(19) = gmsh::model::geo::addLine(point_tag(3), point_tag(9));
  curve_loop_tag(0) = gmsh::model::geo::addCurveLoop({line_tag(0), line_tag(5), -line_tag(1), -line_tag(3)});
  curve_loop_tag(1) = gmsh::model::geo::addCurveLoop({line_tag(1), line_tag(6), -line_tag(2), -line_tag(4)});
  curve_loop_tag(2) = gmsh::model::geo::addCurveLoop({line_tag(7), line_tag(12), -line_tag(8), -line_tag(10)});
  curve_loop_tag(3) = gmsh::model::geo::addCurveLoop({line_tag(8), line_tag(13), -line_tag(9), -line_tag(11)});
  curve_loop_tag(4) = gmsh::model::geo::addCurveLoop({line_tag(0), line_tag(17), -line_tag(7), -line_tag(14)});
  curve_loop_tag(5) = gmsh::model::geo::addCurveLoop({line_tag(1), line_tag(18), -line_tag(8), -line_tag(15)});
  curve_loop_tag(6) = gmsh::model::geo::addCurveLoop({line_tag(2), line_tag(19), -line_tag(9), -line_tag(16)});
  curve_loop_tag(7) = gmsh::model::geo::addCurveLoop({-line_tag(3), line_tag(14), line_tag(10), -line_tag(15)});
  curve_loop_tag(8) = gmsh::model::geo::addCurveLoop({-line_tag(4), line_tag(15), line_tag(11), -line_tag(16)});
  curve_loop_tag(9) = gmsh::model::geo::addCurveLoop({line_tag(5), line_tag(18), -line_tag(12), -line_tag(17)});
  curve_loop_tag(10) = gmsh::model::geo::addCurveLoop({line_tag(6), line_tag(19), -line_tag(13), -line_tag(18)});
  for (std::ptrdiff_t i = 0; i < 11; i++) {
    surface_filling_tag(i) = gmsh::model::geo::addSurfaceFilling({curve_loop_tag(i)});
  }
  surface_loop_tag(0) =
      gmsh::model::geo::addSurfaceLoop({surface_filling_tag(0), surface_filling_tag(2), surface_filling_tag(4),
                                        surface_filling_tag(5), surface_filling_tag(7), surface_filling_tag(9)});
  surface_loop_tag(1) =
      gmsh::model::geo::addSurfaceLoop({surface_filling_tag(1), surface_filling_tag(3), surface_filling_tag(5),
                                        surface_filling_tag(6), surface_filling_tag(8), surface_filling_tag(10)});
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    volume_tag(i) = gmsh::model::geo::addVolume({surface_loop_tag(i)});
  }
  gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(0), 4);
  gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(1), 4);
  gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(2), 4);
  gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(3), 20, "Progression", -1.3);
  gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(4), 40, "Progression", 1.15);
  gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(5), 20, "Progression", -1.3);
  gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(6), 40, "Progression", 1.15);
  gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(7), 4);
  gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(8), 4);
  gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(9), 4);
  gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(10), 20, "Progression", -1.3);
  gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(11), 40, "Progression", 1.15);
  gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(12), 20, "Progression", -1.3);
  gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(13), 40, "Progression", 1.15);
  gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(14), 20, "Progression", 1.35);
  gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(15), 20, "Progression", 1.35);
  gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(16), 20, "Progression", 1.35);
  gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(17), 20, "Progression", 1.35);
  gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(18), 20, "Progression", 1.35);
  gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(19), 20, "Progression", 1.35);
  for (std::ptrdiff_t i = 0; i < 11; i++) {
    gmsh::model::geo::mesh::setTransfiniteSurface(surface_filling_tag(i));
    gmsh::model::geo::mesh::setRecombine(2, surface_filling_tag(i));
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    gmsh::model::geo::mesh::setTransfiniteVolume(volume_tag(i));
    gmsh::model::geo::mesh::setRecombine(3, volume_tag(i));
  }
  gmsh::model::geo::synchronize();
  Eigen::Matrix<double, 4, 4, Eigen::RowMajor> transform_x =
      (Eigen::Transform<double, 3, Eigen::Affine>::Identity() *
       Eigen::Translation<double, 3>(hexahedron_point_coordinate(1, 0) - hexahedron_point_coordinate(0, 0), 0, 0))
          .matrix();
  gmsh::model::mesh::setPeriodic(2, {surface_filling_tag(9)}, {surface_filling_tag(7)},
                                 {transform_x.data(), transform_x.data() + transform_x.size()});
  gmsh::model::mesh::setPeriodic(2, {surface_filling_tag(10)}, {surface_filling_tag(8)},
                                 {transform_x.data(), transform_x.data() + transform_x.size()});
  physical_group_tag[3].emplace_back(surface_filling_tag(0));
  physical_group_tag[4].emplace_back(surface_filling_tag(1));
  physical_group_tag[0].emplace_back(surface_filling_tag(2));
  physical_group_tag[0].emplace_back(surface_filling_tag(3));
  physical_group_tag[0].emplace_back(surface_filling_tag(4));
  physical_group_tag[1].emplace_back(surface_filling_tag(6));
  physical_group_tag[2].emplace_back(surface_filling_tag(7));
  physical_group_tag[2].emplace_back(surface_filling_tag(8));
  physical_group_tag[2].emplace_back(surface_filling_tag(9));
  physical_group_tag[2].emplace_back(surface_filling_tag(10));
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    physical_group_tag[5].emplace_back(volume_tag(i));
  }
  gmsh::model::addPhysicalGroup(2, physical_group_tag[0], -1, "bc-1");
  gmsh::model::addPhysicalGroup(2, physical_group_tag[1], -1, "bc-2");
  gmsh::model::addPhysicalGroup(2, physical_group_tag[2], -1, "bc-3");
  gmsh::model::addPhysicalGroup(2, physical_group_tag[3], -1, "bc-4");
  gmsh::model::addPhysicalGroup(2, physical_group_tag[4], -1, "bc-5");
  gmsh::model::addPhysicalGroup(3, physical_group_tag[5], -1, "vc-1");
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::setOrder(SimulationControl::kPolynomialOrder);
  gmsh::model::mesh::optimize("HighOrder");
  gmsh::write(mesh_file_path);
}
