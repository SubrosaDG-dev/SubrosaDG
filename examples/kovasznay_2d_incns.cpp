/**
 * @file kovasznay_2d_incns.cpp
 * @brief The source file for SubrosaDG example kovasznay_2d_incns.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2024-11-25
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2025 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#include "SubrosaDG.cpp"

inline const std::string kExampleName{"kovasznay_2d_incns"};

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory / "build/out" / kExampleName};

using SimulationControl = SubrosaDG::SimulationControl<
    SubrosaDG::SolveControl<SubrosaDG::DimensionEnum::D2, SubrosaDG::PolynomialOrderEnum::P1,
                            SubrosaDG::BoundaryTimeEnum::Steady, SubrosaDG::SourceTermEnum::None>,
    SubrosaDG::NumericalControl<SubrosaDG::MeshModelEnum::TriangleQuadrangle, SubrosaDG::ShockCapturingEnum::None,
                                SubrosaDG::LimiterEnum::None, SubrosaDG::InitialConditionEnum::Function,
                                SubrosaDG::TimeIntegrationEnum::SSPRK3>,
    SubrosaDG::IncompresibleNSVariable<SubrosaDG::ThermodynamicModelEnum::Constant,
                                       SubrosaDG::EquationOfStateEnum::WeakCompressibleFluid,
                                       SubrosaDG::TransportModelEnum::Constant,
                                       SubrosaDG::ConvectiveFluxEnum::LaxFriedrichs, SubrosaDG::ViscousFluxEnum::BR2>>;

template <typename SimulationControl>
inline Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>
SubrosaDG::InitialCondition<SimulationControl>::calculatePrimitiveFromCoordinate(
    const Eigen::Vector<Real, SimulationControl::kDimension>& coordinate) const {
  const SubrosaDG::Real k =
      40.0_r / 2.0_r - std::sqrt(40.0_r * 40.0_r / 4.0_r + 4.0_r * SubrosaDG::kPi * SubrosaDG::kPi);
  return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{
      (1.0_r - 0.5_r * std::exp(2.0_r * k * coordinate.x())) / 100.0_r + 0.99_r * 1.0_r,
      1.0_r - std::exp(k * coordinate.x()) * std::cos(2.0_r * SubrosaDG::kPi * coordinate.y()),
      k * std::exp(k * coordinate.x()) * std::sin(2.0_r * SubrosaDG::kPi * coordinate.y()) / (2.0_r * SubrosaDG::kPi),
      1.0_r};
}

template <typename SimulationControl>
inline Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>
SubrosaDG::BoundaryCondition<SimulationControl>::calculatePrimitiveFromCoordinate(
    const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate,
    const SubrosaDG::Isize gmsh_physical_index) const {
  if (gmsh_physical_index == 1) {
    const SubrosaDG::Real k =
        40.0_r / 2.0_r - std::sqrt(40.0_r * 40.0_r / 4.0_r + 4.0_r * SubrosaDG::kPi * SubrosaDG::kPi);
    return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{
        (1.0_r - 0.5_r * std::exp(2.0_r * k * coordinate.x())) / 100.0_r + 0.99_r * 1.0_r,
        1.0_r - std::exp(k * coordinate.x()) * std::cos(2.0_r * SubrosaDG::kPi * coordinate.y()),
        k * std::exp(k * coordinate.x()) * std::sin(2.0_r * SubrosaDG::kPi * coordinate.y()) / (2.0_r * SubrosaDG::kPi),
        1.0_r};
  }
  return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>::Zero();
}

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::System<SimulationControl> system;
  system.setMesh(kExampleDirectory / "kovasznay_2d_incns.msh", generateMesh);
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::RiemannFarfield>(1);
  system.setThermodynamicModel<SimulationControl::kThermodynamicModel>(1.0_r, 1.0_r);
  system.setEquationOfState<SimulationControl::kEquationOfState>(10.0_r, 1.0_r);
  system.setTransportModel<SimulationControl::kTransportModel>(1.0_r * 0.5_r * 2.0_r / 40.0_r);
  system.setTimeIntegration(1.0_r, {0, 1});
  system.setViewConfig(kExampleDirectory, kExampleName, -1);
  system.addViewVariable({SubrosaDG::ViewVariableEnum::Density, SubrosaDG::ViewVariableEnum::Velocity,
                          SubrosaDG::ViewVariableEnum::Pressure});
  system.synchronize();
  system.solve();
  system.view();
  return EXIT_SUCCESS;
}

void generateMesh(const std::filesystem::path& mesh_file_path) {
  Eigen::Vector<double, 3> point_coordinate;
  Eigen::Matrix<double, 2, 201, Eigen::RowMajor> connection_point_coordinate;
  connection_point_coordinate.row(0) = Eigen::Array<double, 1, 201>::LinSpaced(201, -0.5, 1.5);
  connection_point_coordinate.row(1) =
      Eigen::cos(SubrosaDG::kPi * connection_point_coordinate.row(0).array()) * 0.25 + 0.5;
  // clang-format off
  point_coordinate << -0.5, 0.5, 1.5;
  // clang-format on
  Eigen::Array<int, 3, 3, Eigen::RowMajor> farfield_point_tag;
  std::array<std::vector<int>, 4> connection_point_tag;
  Eigen::Array<int, 2, 3, Eigen::RowMajor> line_x_tag;
  Eigen::Array<int, 2, 3, Eigen::RowMajor> line_y_tag;
  Eigen::Array<int, 4, 1> curve_loop_tag;
  Eigen::Array<int, 4, 1> plane_surface_tag;
  std::array<std::vector<int>, 2> physical_group_tag;
  gmsh::model::add("kovasznay_2d");
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      farfield_point_tag(j, i) = gmsh::model::geo::addPoint(point_coordinate(j), point_coordinate(i), 0.0);
    }
  }
  connection_point_tag[0].emplace_back(farfield_point_tag(0, 1));
  connection_point_tag[1].emplace_back(farfield_point_tag(1, 1));
  connection_point_tag[2].emplace_back(farfield_point_tag(1, 0));
  connection_point_tag[3].emplace_back(farfield_point_tag(1, 1));
  for (int i = 0; i < 100; i++) {
    connection_point_tag[0].emplace_back(
        gmsh::model::geo::addPoint(connection_point_coordinate(0, i + 1), connection_point_coordinate(1, i + 1), 0.0));
    connection_point_tag[1].emplace_back(gmsh::model::geo::addPoint(connection_point_coordinate(0, i + 100),
                                                                    connection_point_coordinate(1, i + 100), 0.0));
    connection_point_tag[2].emplace_back(
        gmsh::model::geo::addPoint(connection_point_coordinate(1, i + 100), connection_point_coordinate(0, i), 0.0));
    connection_point_tag[3].emplace_back(gmsh::model::geo::addPoint(connection_point_coordinate(1, i + 1),
                                                                    connection_point_coordinate(0, i + 101), 0.0));
  }
  connection_point_tag[0].emplace_back(farfield_point_tag(1, 1));
  connection_point_tag[1].emplace_back(farfield_point_tag(2, 1));
  connection_point_tag[2].emplace_back(farfield_point_tag(1, 1));
  connection_point_tag[3].emplace_back(farfield_point_tag(1, 2));
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 2; j++) {
      if (i == 1) {
        line_x_tag(j, i) = gmsh::model::geo::addSpline(connection_point_tag[static_cast<std::size_t>(j)]);
        line_y_tag(j, i) = gmsh::model::geo::addSpline(connection_point_tag[static_cast<std::size_t>(j + 2)]);
      } else {
        line_x_tag(j, i) = gmsh::model::geo::addLine(farfield_point_tag(j, i), farfield_point_tag(j + 1, i));
        line_y_tag(j, i) = gmsh::model::geo::addLine(farfield_point_tag(i, j), farfield_point_tag(i, j + 1));
      }
    }
  }
  curve_loop_tag(0) =
      gmsh::model::geo::addCurveLoop({line_x_tag(0, 0), line_y_tag(0, 1), -line_x_tag(0, 1), -line_y_tag(0, 0)});
  curve_loop_tag(1) =
      gmsh::model::geo::addCurveLoop({line_x_tag(1, 0), line_y_tag(0, 2), -line_x_tag(1, 1), -line_y_tag(0, 1)});
  curve_loop_tag(2) =
      gmsh::model::geo::addCurveLoop({line_x_tag(0, 1), line_y_tag(1, 1), -line_x_tag(0, 2), -line_y_tag(1, 0)});
  curve_loop_tag(3) =
      gmsh::model::geo::addCurveLoop({line_x_tag(1, 1), line_y_tag(1, 2), -line_x_tag(1, 2), -line_y_tag(1, 1)});
  for (int i = 0; i < 4; i++) {
    plane_surface_tag(i) = gmsh::model::geo::addPlaneSurface({curve_loop_tag(i)});
  }
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 2; j++) {
      gmsh::model::geo::mesh::setTransfiniteCurve(line_x_tag(j, i), 8 + 1);
      gmsh::model::geo::mesh::setTransfiniteCurve(line_y_tag(j, i), 8 + 1);
    }
  }
  for (int i = 0; i < 4; i++) {
    gmsh::model::geo::mesh::setTransfiniteSurface(plane_surface_tag(i));
  }
  for (int i = 0; i < 2; i++) {
    gmsh::model::geo::mesh::setRecombine(2, plane_surface_tag(i));
  }
  gmsh::model::geo::synchronize();
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 2; j++) {
      if (i != 1) {
        physical_group_tag[0].emplace_back(line_x_tag(j, i));
        physical_group_tag[0].emplace_back(line_y_tag(j, i));
      }
    }
  }
  for (int i = 0; i < 4; i++) {
    physical_group_tag[1].emplace_back(plane_surface_tag(i));
  }
  gmsh::model::addPhysicalGroup(1, physical_group_tag[0], 1, "bc-1");
  gmsh::model::addPhysicalGroup(2, physical_group_tag[1], 2, "vc-1");
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::setOrder(SimulationControl::kPolynomialOrder);
  gmsh::model::mesh::optimize("HighOrder");
  gmsh::write(mesh_file_path);
}
