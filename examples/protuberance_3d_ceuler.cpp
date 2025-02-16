/**
 * @file protuberance_3d_ceuler.cpp
 * @brief The 3D protuberance flow example with Euler equations.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2024-05-20
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2025 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#include "SubrosaDG.cpp"

inline const std::string kExampleName{"protuberance_3d_ceuler"};

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory / "build/out" / kExampleName};

using SimulationControl = SubrosaDG::SimulationControl<
    SubrosaDG::SolveControl<SubrosaDG::DimensionEnum::D3, SubrosaDG::PolynomialOrderEnum::P1,
                            SubrosaDG::BoundaryTimeEnum::Steady, SubrosaDG::SourceTermEnum::None>,
    SubrosaDG::NumericalControl<SubrosaDG::MeshModelEnum::Hexahedron, SubrosaDG::ShockCapturingEnum::None,
                                SubrosaDG::LimiterEnum::None, SubrosaDG::InitialConditionEnum::Function,
                                SubrosaDG::TimeIntegrationEnum::SSPRK3>,
    SubrosaDG::CompresibleEulerVariable<SubrosaDG::ThermodynamicModelEnum::Constant,
                                        SubrosaDG::EquationOfStateEnum::IdealGas, SubrosaDG::ConvectiveFluxEnum::HLLC>>;

template <typename SimulationControl>
inline Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>
SubrosaDG::InitialCondition<SimulationControl>::calculatePrimitiveFromCoordinate(
    [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& coordinate) const {
  return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{1.4_r, 0.0_r, 0.5_r, 0.0_r, 1.0_r};
}

template <typename SimulationControl>
inline Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>
SubrosaDG::BoundaryCondition<SimulationControl>::calculatePrimitiveFromCoordinate(
    [[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate,
    const SubrosaDG::Isize gmsh_physical_index) const {
  if (gmsh_physical_index == 1) {
    return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{1.4_r, 0.0_r, 0.5_r, 0.0_r,
                                                                                       1.0_r};
  }
  return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>::Zero();
}

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::System<SimulationControl> system;
  system.setMesh(kExampleDirectory / "protuberance_3d_ceuler.msh", generateMesh);
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::RiemannFarfield>(1);
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::Periodic>(2);
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticSlipWall>(3);
  system.setThermodynamicModel<SimulationControl::kThermodynamicModel>(2.5_r, 25.0_r / 14.0_r);
  system.setTimeIntegration(1.0_r);
  system.setViewConfig(kExampleDirectory, kExampleName);
  system.addViewVariable({SubrosaDG::ViewVariableEnum::Density, SubrosaDG::ViewVariableEnum::Velocity,
                          SubrosaDG::ViewVariableEnum::Pressure, SubrosaDG::ViewVariableEnum::Temperature,
                          SubrosaDG::ViewVariableEnum::MachNumber});
  system.synchronize();
  system.solve();
  system.view();
  return EXIT_SUCCESS;
}

void generateMesh(const std::filesystem::path& mesh_file_path) {
  Eigen::Matrix<double, 8, 3, Eigen::RowMajor> hexahedron_point_coordinate;
  Eigen::Matrix<double, 2, 101, Eigen::RowMajor> protuberance_point_coordinate;
  // clang-format off
  hexahedron_point_coordinate << 0.0, 0.0, 0.0,
                                 0.5, 0.0, 0.0,
                                 0.5, 3.0, 0.0,
                                 0.0, 3.0, 0.0,
                                 0.0, 0.0, 0.8,
                                 0.5, 0.0, 0.8,
                                 0.5, 3.0, 0.8,
                                 0.0, 3.0, 0.8;
  // clang-format on
  protuberance_point_coordinate.row(0) = Eigen::Vector<double, 101>::LinSpaced(101, 0.0, 3.0).transpose();
  protuberance_point_coordinate.row(1) =
      0.0625 * (-25 * (protuberance_point_coordinate.row(0).array() - 1.5).pow(2)).exp();
  Eigen::Array<int, 8, 1> point_tag;
  std::array<std::vector<int>, 2> protuberance_point_tag;
  Eigen::Array<int, 12, 1> line_tag;
  Eigen::Array<int, 6, 1> curve_loop_tag;
  Eigen::Array<int, 6, 1> surface_filling_tag;
  std::array<std::vector<int>, 3> physical_group_tag;
  gmsh::model::add("protuberance_3d");
  for (int i = 0; i < 8; i++) {
    point_tag(i) = gmsh::model::geo::addPoint(hexahedron_point_coordinate(i, 0), hexahedron_point_coordinate(i, 1),
                                              hexahedron_point_coordinate(i, 2));
  }
  protuberance_point_tag[0].emplace_back(point_tag(0));
  protuberance_point_tag[1].emplace_back(point_tag(1));
  for (int i = 0; i < 99; i++) {
    protuberance_point_tag[0].emplace_back(gmsh::model::geo::addPoint(0.0, protuberance_point_coordinate(0, i + 1),
                                                                      protuberance_point_coordinate(1, i + 1)));
    protuberance_point_tag[1].emplace_back(gmsh::model::geo::addPoint(0.5, protuberance_point_coordinate(0, i + 1),
                                                                      protuberance_point_coordinate(1, i + 1)));
  }
  protuberance_point_tag[0].emplace_back(point_tag(3));
  protuberance_point_tag[1].emplace_back(point_tag(2));
  line_tag(0) = gmsh::model::geo::addLine(point_tag(0), point_tag(1));
  line_tag(1) = gmsh::model::geo::addLine(point_tag(3), point_tag(2));
  line_tag(2) = gmsh::model::geo::addSpline(protuberance_point_tag[0]);
  line_tag(3) = gmsh::model::geo::addSpline(protuberance_point_tag[1]);
  line_tag(4) = gmsh::model::geo::addLine(point_tag(4), point_tag(5));
  line_tag(5) = gmsh::model::geo::addLine(point_tag(7), point_tag(6));
  line_tag(6) = gmsh::model::geo::addLine(point_tag(4), point_tag(7));
  line_tag(7) = gmsh::model::geo::addLine(point_tag(5), point_tag(6));
  line_tag(8) = gmsh::model::geo::addLine(point_tag(0), point_tag(4));
  line_tag(9) = gmsh::model::geo::addLine(point_tag(1), point_tag(5));
  line_tag(10) = gmsh::model::geo::addLine(point_tag(2), point_tag(6));
  line_tag(11) = gmsh::model::geo::addLine(point_tag(3), point_tag(7));
  curve_loop_tag(0) = gmsh::model::geo::addCurveLoop({line_tag(0), line_tag(3), -line_tag(1), -line_tag(2)});
  curve_loop_tag(1) = gmsh::model::geo::addCurveLoop({line_tag(4), line_tag(7), -line_tag(5), -line_tag(6)});
  curve_loop_tag(2) = gmsh::model::geo::addCurveLoop({line_tag(0), line_tag(9), -line_tag(4), -line_tag(8)});
  curve_loop_tag(3) = gmsh::model::geo::addCurveLoop({line_tag(1), line_tag(10), -line_tag(5), -line_tag(11)});
  curve_loop_tag(4) = gmsh::model::geo::addCurveLoop({-line_tag(2), line_tag(8), line_tag(6), -line_tag(11)});
  curve_loop_tag(5) = gmsh::model::geo::addCurveLoop({line_tag(3), line_tag(10), -line_tag(7), -line_tag(9)});
  for (int i = 0; i < 6; i++) {
    surface_filling_tag(i) = gmsh::model::geo::addSurfaceFilling({curve_loop_tag(i)});
  }
  int surface_loop_tag =
      gmsh::model::geo::addSurfaceLoop({surface_filling_tag(0), surface_filling_tag(1), surface_filling_tag(2),
                                        surface_filling_tag(3), surface_filling_tag(4), surface_filling_tag(5)});
  int volume_tag = gmsh::model::geo::addVolume({surface_loop_tag});
  gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(0), 5);
  gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(1), 5);
  gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(2), 80, "Bump", 10.0);
  gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(3), 80, "Bump", 10.0);
  gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(4), 5);
  gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(5), 5);
  gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(6), 80, "Bump", 10.0);
  gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(7), 80, "Bump", 10.0);
  gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(8), 10, "Progression", 1.3);
  gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(9), 10, "Progression", 1.3);
  gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(10), 10, "Progression", 1.3);
  gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(11), 10, "Progression", 1.3);
  for (int i = 0; i < 6; i++) {
    gmsh::model::geo::mesh::setTransfiniteSurface(surface_filling_tag(i));
    gmsh::model::geo::mesh::setRecombine(2, surface_filling_tag(i));
  }
  gmsh::model::geo::mesh::setTransfiniteVolume(volume_tag);
  gmsh::model::geo::mesh::setRecombine(3, volume_tag);
  gmsh::model::geo::synchronize();
  Eigen::Matrix<double, 4, 4, Eigen::RowMajor> transform_x =
      (Eigen::Transform<double, 3, Eigen::Affine>::Identity() *
       Eigen::Translation<double, 3>(hexahedron_point_coordinate(1, 0) - hexahedron_point_coordinate(0, 0), 0, 0))
          .matrix();
  gmsh::model::mesh::setPeriodic(2, {surface_filling_tag(5)}, {surface_filling_tag(4)},
                                 {transform_x.data(), transform_x.data() + transform_x.size()});
  physical_group_tag[2].emplace_back(surface_filling_tag(0));
  physical_group_tag[0].emplace_back(surface_filling_tag(1));
  physical_group_tag[0].emplace_back(surface_filling_tag(2));
  physical_group_tag[0].emplace_back(surface_filling_tag(3));
  physical_group_tag[1].emplace_back(surface_filling_tag(4));
  physical_group_tag[1].emplace_back(surface_filling_tag(5));
  gmsh::model::addPhysicalGroup(2, physical_group_tag[0], 1, "bc-1");
  gmsh::model::addPhysicalGroup(2, physical_group_tag[1], 2, "bc-2");
  gmsh::model::addPhysicalGroup(2, physical_group_tag[2], 3, "bc-3");
  gmsh::model::addPhysicalGroup(3, {volume_tag}, 4, "vc-1");
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::setOrder(SimulationControl::kPolynomialOrder);
  gmsh::model::mesh::optimize("HighOrder");
  gmsh::write(mesh_file_path);
}
