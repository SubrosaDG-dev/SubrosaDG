/**
 * @file explosion_2d_ceuler.cpp
 * @brief The source file for SubrosaDG example explosion_2d_ceuler.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2024-07-19
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2025 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#include "SubrosaDG.cpp"

inline const std::string kExampleName{"explosion_2d_ceuler"};

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory / "build/out" / kExampleName};

using SimulationControl = SubrosaDG::SimulationControl<
    SubrosaDG::SolveControl<SubrosaDG::DimensionEnum::D2, SubrosaDG::PolynomialOrderEnum::P3,
                            SubrosaDG::BoundaryTimeEnum::Steady, SubrosaDG::SourceTermEnum::None>,
    SubrosaDG::NumericalControl<SubrosaDG::MeshModelEnum::Triangle, SubrosaDG::ShockCapturingEnum::ArtificialViscosity,
                                SubrosaDG::LimiterEnum::None, SubrosaDG::InitialConditionEnum::Function,
                                SubrosaDG::TimeIntegrationEnum::SSPRK3>,
    SubrosaDG::CompresibleEulerVariable<SubrosaDG::ThermodynamicModelEnum::Constant,
                                        SubrosaDG::EquationOfStateEnum::IdealGas, SubrosaDG::ConvectiveFluxEnum::HLLC>>;

template <typename SimulationControl>
inline Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>
SubrosaDG::InitialCondition<SimulationControl>::calculatePrimitiveFromCoordinate(
    const Eigen::Vector<Real, SimulationControl::kDimension>& coordinate) const {
  return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{
      ((coordinate.x() - 1.0_r) * (coordinate.x() - 1.0_r) + (coordinate.y() - 1.0_r) * (coordinate.y() - 1.0_r) <=
       0.4_r * 0.4_r)
          ? 1.0_r
          : 0.125_r,
      0.0_r, 0.0_r,
      ((coordinate.x() - 1.0_r) * (coordinate.x() - 1.0_r) + (coordinate.y() - 1.0_r) * (coordinate.y() - 1.0_r) <=
       0.4_r * 0.4_r)
          ? 1.4_r
          : 0.8_r * 1.4_r};
}

template <typename SimulationControl>
inline Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>
SubrosaDG::BoundaryCondition<SimulationControl>::calculatePrimitiveFromCoordinate(
    [[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate,
    [[maybe_unused]] const SubrosaDG::Isize gmsh_physical_index) const {
  return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>::Zero();
}

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::System<SimulationControl> system;
  system.setMesh(kExampleDirectory / "explosion_2d_ceuler.msh", generateMesh);
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticSlipWall>(1);
  system.setThermodynamicModel<SimulationControl::kThermodynamicModel>(2.5_r, 25.0_r / 14.0_r);
  system.setArtificialViscosity(4.0_r);
  system.setTimeIntegration(0.1_r);
  system.setViewConfig(kExampleDirectory, kExampleName);
  system.addViewVariable({SubrosaDG::ViewVariableEnum::Density, SubrosaDG::ViewVariableEnum::Velocity,
                          SubrosaDG::ViewVariableEnum::Pressure, SubrosaDG::ViewVariableEnum::ArtificialViscosity});
  system.synchronize();
  system.solve();
  system.view();
  return EXIT_SUCCESS;
}

void generateMesh(const std::filesystem::path& mesh_file_path) {
  Eigen::Matrix<double, 4, 3, Eigen::RowMajor> farfield_point_coordinate;
  Eigen::Matrix<double, 4, 3, Eigen::RowMajor> cylinder_point_coordinate;
  // clang-format off
  farfield_point_coordinate << 0.0, 0.0, 0.0,
                               2.0, 0.0, 0.0,
                               2.0, 2.0, 0.0,
                               0.0, 2.0, 0.0;
  cylinder_point_coordinate << 1.0, 0.6, 0.0,
                               1.4, 1.0, 0.0,
                               1.0, 1.4, 0.0,
                               0.6, 1.0, 0.0;
  // clang-format on
  Eigen::Array<int, 4, 1> farfield_point_tag;
  Eigen::Array<int, 4, 1> cylinder_point_tag;
  Eigen::Array<int, 4, 1> farfield_line_tag;
  Eigen::Array<int, 2, 1> connection_line_tag;
  Eigen::Array<int, 4, 1> cylinder_line_tag;
  Eigen::Array<int, 2, 1> curve_loop_tag;
  Eigen::Array<int, 2, 1> plane_surface_tag;
  std::array<std::vector<int>, 2> physical_group_tag;
  gmsh::model::add("explosion_2d");
  const int center_point_tag = gmsh::model::geo::addPoint(1.0, 1.0, 0.0);
  for (int i = 0; i < 4; i++) {
    farfield_point_tag(i) = gmsh::model::geo::addPoint(farfield_point_coordinate(i, 0), farfield_point_coordinate(i, 1),
                                                       farfield_point_coordinate(i, 2), 0.02);
    cylinder_point_tag(i, 1) = gmsh::model::geo::addPoint(
        cylinder_point_coordinate(i, 0), cylinder_point_coordinate(i, 1), cylinder_point_coordinate(i, 2), 0.02);
  }
  for (int i = 0; i < 4; i++) {
    farfield_line_tag(i) = gmsh::model::geo::addLine(farfield_point_tag(i), farfield_point_tag((i + 1) % 4));
    cylinder_line_tag(i) =
        gmsh::model::geo::addCircleArc(cylinder_point_tag(i), center_point_tag, cylinder_point_tag((i + 1) % 4));
  }
  curve_loop_tag(0) = gmsh::model::geo::addCurveLoop(
      {farfield_line_tag(0), farfield_line_tag(1), farfield_line_tag(2), farfield_line_tag(3)});
  curve_loop_tag(1) = gmsh::model::geo::addCurveLoop(
      {cylinder_line_tag(0), cylinder_line_tag(1), cylinder_line_tag(2), cylinder_line_tag(3)});
  plane_surface_tag(0) = gmsh::model::geo::addPlaneSurface({curve_loop_tag(0), -curve_loop_tag(1)});
  plane_surface_tag(1) = gmsh::model::geo::addPlaneSurface({curve_loop_tag(1)});
  gmsh::model::geo::synchronize();
  for (int i = 0; i < 4; i++) {
    physical_group_tag[0].emplace_back(farfield_line_tag(i));
  }
  for (int i = 0; i < 2; i++) {
    physical_group_tag[1].emplace_back(plane_surface_tag(i));
  }
  gmsh::model::addPhysicalGroup(1, physical_group_tag[0], 1, "bc-1");
  gmsh::model::addPhysicalGroup(2, physical_group_tag[1], 2, "vc-1");
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::setOrder(SimulationControl::kPolynomialOrder);
  gmsh::model::mesh::optimize("HighOrder");
  gmsh::write(mesh_file_path);
}
