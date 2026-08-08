/**
 * @file cylinder_2d_cns.cpp
 * @brief The 2D cylinder flow example with compressible Navier-Stokes equations.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2026-05-31
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

namespace SubrosaDG {

struct PhysicalModelData {
  // EquationOfState
  static constexpr double kSpecificHeatRatio = 1.4;

  // ThermodynamicModel
  static constexpr double kSpecificHeatConstantPressure = 2.5;
  static constexpr double kSpecificHeatConstantVolume = kSpecificHeatConstantPressure / kSpecificHeatRatio;

  // TransportModel
  static constexpr double kReynoldsNumber = 100.0;
  static constexpr double kDynamicViscosity = 1.4 * 0.1 * 1.0 / kReynoldsNumber;
  static constexpr double kPrandtlNumber = 0.71;
  static constexpr double kThermalConductivity = kSpecificHeatConstantPressure * kDynamicViscosity / kPrandtlNumber;
};

}  // namespace SubrosaDG

#include "SubrosaDG.cpp"

inline const std::string kExampleName{"cylinder_2d_cns"};

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory / "build/out" / kExampleName};

using SimulationControl = SubrosaDG::SimulationControl<
    SubrosaDG::SolveControl<SubrosaDG::DimensionEnum::D2, SubrosaDG::PolynomialOrderEnum::P1,
                            SubrosaDG::BoundaryTimeEnum::Steady, SubrosaDG::SourceTermEnum::None>,
    SubrosaDG::NumericalControl<SubrosaDG::MeshModelEnum::TriangleQuadrangle, SubrosaDG::InitialConditionEnum::Function,
                                SubrosaDG::TimeIntegrationEnum::SSPRK3>,
    SubrosaDG::CompressibleNSVariable<SubrosaDG::ThermodynamicModelEnum::Constant,
                                      SubrosaDG::EquationOfStateEnum::IdealGas, SubrosaDG::TransportModelEnum::Constant,
                                      SubrosaDG::ConvectiveFluxEnum::HLLC, SubrosaDG::ViscousFluxEnum::BR2>>;

template <typename SimulationControl>
inline void SubrosaDG::InitialCondition<SimulationControl>::computePrimitiveFromCoordinate(
    [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& coordinate,
    Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber>& initial_primitive_variable) {
  initial_primitive_variable = {1.4_r, 0.1_r, 0.0_r, 1.0_r};
}

template <typename SimulationControl>
inline void SubrosaDG::BoundaryCondition<SimulationControl>::computePrimitiveFromCoordinate(
    [[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate,
    Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber>& boundary_primitive_variable,
    const SubrosaDG::Isize gmsh_physical_index) {
  if (gmsh_physical_index == 1) {
    boundary_primitive_variable = {1.4_r, 0.1_r, 0.0_r, 1.0_r};
  } else if (gmsh_physical_index == 2) {
    boundary_primitive_variable = {1.4_r, 0.0_r, 0.0_r, 1.0_r};
    // boundary_primitive_variable = {1.4_r, -(0.1_r * SubrosaDG::kPi / 2.0_r) * coordinate(1),  (0.1_r * SubrosaDG::kPi
    // / 2.0_r) * coordinate(0), 1.0_r};
  }
}

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::System<SimulationControl> system{SubrosaDG::CommandLineEnum::Close};
  system.setMesh(kExampleDirectory / std::format("{}.msh", kExampleName), generateMesh);
  // system.setMesh(kExampleDirectory / std::format("{}.msh", kExampleName));
  system.setRotation(Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>{0.0_r, 0.0_r},
                     Eigen::Vector<SubrosaDG::Real, 3>{0.0_r, 0.0_r, 0.1_r * SubrosaDG::kPi / 2.0_r});
  //  Eigen::Vector<SubrosaDG::Real, 3>{0.0_r, 0.0_r, 0.0_r});
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::RiemannFarfield>(1);
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticNonSlipWall>(2);
  system.template addVolumeCondition<SubrosaDG::InteriorConditionEnum::Static>(3);
  system.template addVolumeCondition<SubrosaDG::InteriorConditionEnum::Rotate>(4);
  system.setTimeIntegration(1.0_r);
  system.setViewConfig(kExampleDirectory, kExampleName, 1);
  system.addViewVariable({SubrosaDG::ViewVariableEnum::Density, SubrosaDG::ViewVariableEnum::Velocity,
                          SubrosaDG::ViewVariableEnum::Pressure, SubrosaDG::ViewVariableEnum::Temperature,
                          SubrosaDG::ViewVariableEnum::MachNumber, SubrosaDG::ViewVariableEnum::Vorticity});
  system.synchronize();
  system.solve();
  system.view();
  return EXIT_SUCCESS;
}

void generateMesh(const std::filesystem::path& mesh_file_path) {
  Eigen::Matrix<double, 4, 3, Eigen::RowMajor> farfield_point_coordinate;
  Eigen::Matrix<double, 4, 3, Eigen::RowMajor> interface_point_coordinate;
  Eigen::Matrix<double, 4, 3, Eigen::RowMajor> cylinder_point_coordinate;
  // clang-format off
  farfield_point_coordinate << -15.0, -15.0, 0.0,
                                30.0, -15.0, 0.0,
                                30.0,  15.0, 0.0,
                               -15.0,  15.0, 0.0;
  interface_point_coordinate << -1.0,  0.0, 0.0,
                                 0.0, -1.0, 0.0,
                                 1.0,  0.0, 0.0,
                                 0.0,  1.0, 0.0;
  cylinder_point_coordinate << -0.5,  0.0, 0.0,
                                0.0, -0.5, 0.0,
                                0.5,  0.0, 0.0,
                                0.0,  0.5, 0.0;
  // clang-format on
  Eigen::Array<int, 4, 3, Eigen::RowMajor> point_tag;
  Eigen::Array<int, 4, 4, Eigen::RowMajor> line_tag;
  Eigen::Array<int, 6, 1> curve_loop_tag;
  Eigen::Array<int, 5, 1> plane_surface_tag;
  std::array<std::vector<int>, 4> physical_group_tag;
  gmsh::model::add("cylinder_2d");
  const int center_point_tag = gmsh::model::geo::addPoint(0.0, 0.0, 0.0);
  for (int i = 0; i < 4; i++) {
    point_tag(i, 0) = gmsh::model::geo::addPoint(farfield_point_coordinate(i, 0), farfield_point_coordinate(i, 1),
                                                 farfield_point_coordinate(i, 2), 3.0);
    point_tag(i, 1) = gmsh::model::geo::addPoint(interface_point_coordinate(i, 0), interface_point_coordinate(i, 1),
                                                 interface_point_coordinate(i, 2));
    point_tag(i, 2) = gmsh::model::geo::addPoint(cylinder_point_coordinate(i, 0), cylinder_point_coordinate(i, 1),
                                                 cylinder_point_coordinate(i, 2));
  }
  for (int i = 0; i < 4; i++) {
    line_tag(i, 0) = gmsh::model::geo::addLine(point_tag(i, 0), point_tag((i + 1) % 4, 0));
    line_tag(i, 1) = gmsh::model::geo::addCircleArc(point_tag(i, 1), center_point_tag, point_tag((i + 1) % 4, 1));
    line_tag(i, 2) = gmsh::model::geo::addCircleArc(point_tag(i, 2), center_point_tag, point_tag((i + 1) % 4, 2));
    line_tag(i, 3) = gmsh::model::geo::addLine(point_tag(i, 1), point_tag(i, 2));
  }
  curve_loop_tag(0, 0) =
      gmsh::model::geo::addCurveLoop({line_tag(0, 0), line_tag(1, 0), line_tag(2, 0), line_tag(3, 0)});
  curve_loop_tag(1, 0) =
      gmsh::model::geo::addCurveLoop({line_tag(0, 1), line_tag(1, 1), line_tag(2, 1), line_tag(3, 1)});
  for (int i = 0; i < 4; i++) {
    curve_loop_tag(i + 2, 0) =
        gmsh::model::geo::addCurveLoop({-line_tag(i, 3), line_tag(i, 1), line_tag((i + 1) % 4, 3), -line_tag(i, 2)});
  }
  plane_surface_tag(0, 0) = gmsh::model::geo::addPlaneSurface({curve_loop_tag(0, 0), -curve_loop_tag(1, 0)});
  for (int i = 0; i < 4; i++) {
    plane_surface_tag(i + 1, 0) = gmsh::model::geo::addPlaneSurface({curve_loop_tag(i + 2, 0)});
  }
  for (int i = 0; i < 4; i++) {
    gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(i, 1), 16);
    gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(i, 2), 16);
    gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(i, 3), 12, "Progression", -1.2);
  }
  for (int i = 0; i < 4; i++) {
    gmsh::model::geo::mesh::setTransfiniteSurface(plane_surface_tag(i + 1, 0));
    gmsh::model::geo::mesh::setRecombine(2, plane_surface_tag(i + 1, 0));
  }
  int field_tag = gmsh::model::mesh::field::add("Box");
  gmsh::model::mesh::field::setNumber(field_tag, "VIn", 0.3);
  gmsh::model::mesh::field::setNumber(field_tag, "VOut", 3.0);
  gmsh::model::mesh::field::setNumber(field_tag, "XMin", -3.0);
  gmsh::model::mesh::field::setNumber(field_tag, "XMax", 12.0);
  gmsh::model::mesh::field::setNumber(field_tag, "YMin", -4.0);
  gmsh::model::mesh::field::setNumber(field_tag, "YMax", 4.0);
  gmsh::model::mesh::field::setNumber(field_tag, "Thickness", 5.0);
  gmsh::model::mesh::field::setAsBackgroundMesh(field_tag);
  gmsh::model::geo::synchronize();
  for (int i = 0; i < 4; i++) {
    physical_group_tag[0].emplace_back(line_tag(i, 0));
    physical_group_tag[1].emplace_back(line_tag(i, 2));
  }
  physical_group_tag[2].emplace_back(plane_surface_tag(0, 0));
  for (int i = 0; i < 4; i++) {
    physical_group_tag[3].emplace_back(plane_surface_tag(i + 1, 0));
    // physical_group_tag[2].emplace_back(plane_surface_tag(i + 1, 0));
  }
  gmsh::model::addPhysicalGroup(1, physical_group_tag[0], 1, "bc-1");
  gmsh::model::addPhysicalGroup(1, physical_group_tag[1], 2, "bc-2");
  gmsh::model::addPhysicalGroup(2, physical_group_tag[2], 3, "vc-1");
  gmsh::model::addPhysicalGroup(2, physical_group_tag[3], 4, "rc-1");
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::setOrder(SimulationControl::kPolynomialOrder);
  gmsh::model::mesh::optimize("HighOrder");
  gmsh::write(mesh_file_path);
}
