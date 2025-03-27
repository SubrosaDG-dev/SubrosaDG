/**
 * @file cylinder_2d_ceuler.cpp
 * @brief The 2D cylinder flow example with Euler equations.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-07-15
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2025 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#include "SubrosaDG.cpp"

inline const std::string kExampleName{"cylinder_2d_ceuler"};

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory / "build/out" / kExampleName};

using SimulationControl = SubrosaDG::SimulationControl<
    SubrosaDG::SolveControl<SubrosaDG::DimensionEnum::D2, SubrosaDG::PolynomialOrderEnum::P3,
                            SubrosaDG::BoundaryTimeEnum::Steady, SubrosaDG::SourceTermEnum::None>,
    SubrosaDG::NumericalControl<SubrosaDG::MeshModelEnum::TriangleQuadrangle,
                                SubrosaDG::ShockCapturingEnum::ArtificialViscosity, SubrosaDG::LimiterEnum::None,
                                SubrosaDG::InitialConditionEnum::Function, SubrosaDG::TimeIntegrationEnum::SSPRK3>,
    SubrosaDG::CompresibleEulerVariable<SubrosaDG::ThermodynamicModelEnum::Constant,
                                        SubrosaDG::EquationOfStateEnum::IdealGas, SubrosaDG::ConvectiveFluxEnum::HLLC>>;

template <typename SimulationControl>
inline Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>
SubrosaDG::InitialCondition<SimulationControl>::calculatePrimitiveFromCoordinate(
    [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& coordinate) const {
  return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{1.4_r, 0.38_r, 0.0_r, 1.0_r};
}

template <typename SimulationControl>
inline Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>
SubrosaDG::BoundaryCondition<SimulationControl>::calculatePrimitiveFromCoordinate(
    [[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate,
    const SubrosaDG::Isize gmsh_physical_index) const {
  if (gmsh_physical_index == 1) {
    return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{1.4_r, 0.38_r, 0.0_r, 1.0_r};
  }
  return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>::Zero();
}

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::System<SimulationControl> system;
  system.setMesh(kExampleDirectory / "cylinder_2d_ceuler.msh", generateMesh);
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::RiemannFarfield>(1);
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticSlipWall>(2);
  system.setThermodynamicModel<SimulationControl::kThermodynamicModel>(2.5_r, 25.0_r / 14.0_r);
  system.setArtificialViscosity(0.2_r);
  system.setTimeIntegration(0.1_r);
  system.setViewConfig(kExampleDirectory, kExampleName);
  system.addViewVariable({SubrosaDG::ViewVariableEnum::Density, SubrosaDG::ViewVariableEnum::Velocity,
                          SubrosaDG::ViewVariableEnum::Pressure, SubrosaDG::ViewVariableEnum::Temperature,
                          SubrosaDG::ViewVariableEnum::MachNumber, SubrosaDG::ViewVariableEnum::ArtificialViscosity});
  system.synchronize();
  system.solve();
  system.view();
  return EXIT_SUCCESS;
}

void generateMesh(const std::filesystem::path& mesh_file_path) {
  Eigen::Matrix<double, 4, 3, Eigen::RowMajor> farfield_point_coordinate;
  Eigen::Matrix<double, 4, 3, Eigen::RowMajor> separation_point_coordinate;
  Eigen::Matrix<double, 4, 3, Eigen::RowMajor> cylinder_point_coordinate;
  // clang-format off
  farfield_point_coordinate << -5.0,  0.0, 0.0,
                                0.0, -5.0, 0.0,
                                5.0,  0.0, 0.0,
                                0.0,  5.0, 0.0;
  separation_point_coordinate << -1.5,  0.0, 0.0,
                                  0.0, -1.5, 0.0,
                                  1.5,  0.0, 0.0,
                                  0.0,  1.5, 0.0;
  cylinder_point_coordinate << -0.5,  0.0, 0.0,
                                0.0, -0.5, 0.0,
                                0.5,  0.0, 0.0,
                                0.0,  0.5, 0.0;
  // clang-format on
  Eigen::Array<int, 4, 3, Eigen::RowMajor> point_tag;
  Eigen::Array<int, 4, 5, Eigen::RowMajor> line_tag;
  Eigen::Array<int, 4, 2, Eigen::RowMajor> curve_loop_tag;
  Eigen::Array<int, 4, 2, Eigen::RowMajor> plane_surface_tag;
  std::array<std::vector<int>, 3> physical_group_tag;
  gmsh::model::add("cylinder_2d");
  const int center_point_tag = gmsh::model::geo::addPoint(0.0, 0.0, 0.0);
  for (int i = 0; i < 4; i++) {
    point_tag(i, 0) = gmsh::model::geo::addPoint(farfield_point_coordinate(i, 0), farfield_point_coordinate(i, 1),
                                                 farfield_point_coordinate(i, 2));
    point_tag(i, 1) = gmsh::model::geo::addPoint(separation_point_coordinate(i, 0), separation_point_coordinate(i, 1),
                                                 separation_point_coordinate(i, 2));
    point_tag(i, 2) = gmsh::model::geo::addPoint(cylinder_point_coordinate(i, 0), cylinder_point_coordinate(i, 1),
                                                 cylinder_point_coordinate(i, 2));
  }
  for (int i = 0; i < 4; i++) {
    line_tag(i, 0) = gmsh::model::geo::addCircleArc(point_tag(i, 0), center_point_tag, point_tag((i + 1) % 4, 0));
    line_tag(i, 1) = gmsh::model::geo::addCircleArc(point_tag(i, 1), center_point_tag, point_tag((i + 1) % 4, 1));
    line_tag(i, 2) = gmsh::model::geo::addCircleArc(point_tag(i, 2), center_point_tag, point_tag((i + 1) % 4, 2));
    line_tag(i, 3) = gmsh::model::geo::addLine(point_tag(i, 0), point_tag(i, 1));
    line_tag(i, 4) = gmsh::model::geo::addLine(point_tag(i, 1), point_tag(i, 2));
  }
  for (int i = 0; i < 4; i++) {
    curve_loop_tag(i, 0) =
        gmsh::model::geo::addCurveLoop({-line_tag(i, 3), line_tag(i, 0), line_tag((i + 1) % 4, 3), -line_tag(i, 1)});
    curve_loop_tag(i, 1) =
        gmsh::model::geo::addCurveLoop({-line_tag(i, 4), line_tag(i, 1), line_tag((i + 1) % 4, 4), -line_tag(i, 2)});
  }
  for (int i = 0; i < 4; i++) {
    plane_surface_tag(i, 0) = gmsh::model::geo::addPlaneSurface({curve_loop_tag(i, 0)});
    plane_surface_tag(i, 1) = gmsh::model::geo::addPlaneSurface({curve_loop_tag(i, 1)});
  }
  for (int i = 0; i < 4; i++) {
    gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(i, 0), 16);
    gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(i, 1), 16);
    gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(i, 2), 16);
    gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(i, 3), 8, "Progression", -1.2);
    gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(i, 4), 12, "Progression", -1.2);
  }
  for (int i = 0; i < 4; i++) {
    gmsh::model::geo::mesh::setTransfiniteSurface(plane_surface_tag(i, 0));
    gmsh::model::geo::mesh::setTransfiniteSurface(plane_surface_tag(i, 1));
    gmsh::model::geo::mesh::setRecombine(2, plane_surface_tag(i, 1));
  }
  gmsh::model::geo::synchronize();
  for (int i = 0; i < 4; i++) {
    physical_group_tag[0].emplace_back(line_tag(i, 0));
    physical_group_tag[1].emplace_back(line_tag(i, 2));
    physical_group_tag[2].emplace_back(plane_surface_tag(i, 0));
    physical_group_tag[2].emplace_back(plane_surface_tag(i, 1));
  }
  gmsh::model::addPhysicalGroup(1, physical_group_tag[0], 1, "bc-1");
  gmsh::model::addPhysicalGroup(1, physical_group_tag[1], 2, "bc-2");
  gmsh::model::addPhysicalGroup(2, physical_group_tag[2], 3, "vc-1");
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::setOrder(SimulationControl::kPolynomialOrder);
  gmsh::model::mesh::optimize("HighOrder");
  gmsh::write(mesh_file_path);
}
