/**
 * @file bladerotor_2d_incns.cpp
 * @brief The 2D incompressible flow example with Navier-Stokes equations.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2026-09-26
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

namespace SubrosaDG {

struct PhysicalModelData {
  // EquationOfState
  static constexpr double kReferenceSoundSpeed = 10.0;
  static constexpr double kReferenceDensity = 1000.0;

  // ThermodynamicModel
  static constexpr double kSpecificHeatConstantPressure = 1.0;
  static constexpr double kSpecificHeatConstantVolume = 1.0;

  // TransportModel
  static constexpr double kDynamicViscosity = 0.001;
  static constexpr double kPrandtlNumber = 0.71;
  static constexpr double kThermalConductivity = kSpecificHeatConstantPressure * kDynamicViscosity / kPrandtlNumber;
};

}  // namespace SubrosaDG

#include "SubrosaDG.cpp"

inline const std::string kExampleName{"bladerotor_2d_incns"};

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory / "build/out" / kExampleName};

using SimulationControl = SubrosaDG::SimulationControl<
    SubrosaDG::SolveControl<SubrosaDG::DimensionEnum::D2, SubrosaDG::PolynomialOrderEnum::P1,
                            SubrosaDG::BoundaryTimeEnum::Steady, SubrosaDG::SourceTermEnum::None>,
    SubrosaDG::NumericalControl<SubrosaDG::MeshModelEnum::Triangle, SubrosaDG::InitialConditionEnum::Function,
                                SubrosaDG::TimeIntegrationEnum::SSPRK3>,
    SubrosaDG::IncompressibleNSVariable<SubrosaDG::ThermodynamicModelEnum::Constant,
                                        SubrosaDG::EquationOfStateEnum::WeakCompressibleFluid,
                                        SubrosaDG::TransportModelEnum::Constant,
                                        SubrosaDG::ConvectiveFluxEnum::LaxFriedrichs, SubrosaDG::ViscousFluxEnum::BR2>>;

template <typename SimulationControl>
inline void SubrosaDG::InitialCondition<SimulationControl>::computePrimitiveFromCoordinate(
    [[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate,
    Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>& initial_primitive_variable) {
  initial_primitive_variable = {1.0_r, 0.0_r, 0.0_r, 1.0_r};
}

template <typename SimulationControl>
inline void SubrosaDG::BoundaryCondition<SimulationControl>::computePrimitiveFromCoordinate(
    [[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate,
    Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>& boundary_primitive_variable,
    const SubrosaDG::Isize gmsh_physical_index) {
  if (gmsh_physical_index == 1) {
    boundary_primitive_variable = {1.0_r, 0.0_r, 0.0_r, 1.0_r};
  } else if (gmsh_physical_index == 2) {
    boundary_primitive_variable = {1.0_r, 0.0_r, 0.0_r, 1.0_r};
  }
}

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::System<SimulationControl> system;
  system.setMesh(kExampleDirectory / std::format("{}.msh", kExampleName), generateMesh);
  system.setRotation(Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>{0.0_r, 0.0_r},
                     Eigen::Vector<SubrosaDG::Real, 3>{0.0_r, 0.0_r, 0.125_r});
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::PressureOutflow>(1);
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticNonSlipWall>(2);
  system.template addVolumeCondition<SubrosaDG::InteriorConditionEnum::Static>(3);
  system.template addVolumeCondition<SubrosaDG::InteriorConditionEnum::Rotate>(4);
  system.setTimeIntegration(0.2_r);
  system.setViewConfig(kExampleDirectory, kExampleName);
  system.addViewVariable({SubrosaDG::ViewVariableEnum::Density, SubrosaDG::ViewVariableEnum::Velocity,
                          SubrosaDG::ViewVariableEnum::Pressure, SubrosaDG::ViewVariableEnum::Vorticity});
  system.synchronize();
  system.solve();
  system.view();
  return EXIT_SUCCESS;
}

void generateMesh(const std::filesystem::path& mesh_file_path) {
  // Geometry is in metres; the dimensions in the figure are in millimetres.
  constexpr double kOuterRadius = 0.100;
  constexpr double kInterfaceRadius = 0.090;
  constexpr double kHubRadius = 0.040;
  constexpr double kBladeLength = 0.045;
  constexpr double kBladeWidth = 0.010;
  constexpr double kDuctEnd = 0.130;
  constexpr double kDegree = std::numbers::pi_v<double> / 180.0;
  const double blade_root = std::sqrt(kHubRadius * kHubRadius - kBladeWidth * kBladeWidth / 4.0);
  const double blade_tip = kHubRadius + kBladeLength;
  Eigen::Matrix<double, 8, 2, Eigen::RowMajor> outer_point_coordinate;
  Eigen::Matrix<double, 4, 2, Eigen::RowMajor> blade_point_coordinate;
  // clang-format off
  outer_point_coordinate << kOuterRadius * std::cos( 72.0 * kDegree), kOuterRadius * std::sin( 72.0 * kDegree),
                            kOuterRadius * std::cos(205.0 * kDegree), kOuterRadius * std::sin(205.0 * kDegree),
                            kOuterRadius * std::cos(205.0 * kDegree), -kDuctEnd,
                            kOuterRadius * std::cos(250.0 * kDegree), -kDuctEnd,
                            kOuterRadius * std::cos(250.0 * kDegree), kOuterRadius * std::sin(250.0 * kDegree),
                            kOuterRadius * std::cos( 25.0 * kDegree), kOuterRadius * std::sin( 25.0 * kDegree),
                            kDuctEnd, kOuterRadius * std::sin(25.0 * kDegree),
                            kDuctEnd, kOuterRadius * std::sin(72.0 * kDegree);
  blade_point_coordinate << blade_root, -kBladeWidth / 2.0,
                            blade_tip,  -kBladeWidth / 2.0,
                            blade_tip,   kBladeWidth / 2.0,
                            blade_root,  kBladeWidth / 2.0;
  // clang-format on
  Eigen::Array<int, 8, 1> outer_point_tag;
  Eigen::Array<int, 8, 1> outer_curve_tag;
  Eigen::Array<int, 4, 1> interface_point_tag;
  Eigen::Array<int, 4, 1> interface_curve_tag;
  Eigen::Array<int, 4, 4, Eigen::RowMajor> rotor_point_tag;
  Eigen::Array<int, 4, 4, Eigen::RowMajor> rotor_curve_tag;
  std::array<std::vector<int>, 4> physical_group_tag;
  gmsh::model::add("bladerotor_2d");
  const int center_point_tag = gmsh::model::geo::addPoint(0.0, 0.0, 0.0);
  for (int i = 0; i < 8; i++) {
    outer_point_tag(i) =
        gmsh::model::geo::addPoint(outer_point_coordinate(i, 0), outer_point_coordinate(i, 1), 0.0, 0.004);
  }
  for (int i = 0; i < 4; i++) {
    const double angle = static_cast<double>(i) * std::numbers::pi_v<double> / 2.0;
    const double cosine = std::cos(angle);
    const double sine = std::sin(angle);
    interface_point_tag(i) = gmsh::model::geo::addPoint(kInterfaceRadius * cosine, kInterfaceRadius * sine, 0.0, 0.002);
    for (int j = 0; j < 4; j++) {
      const double x = blade_point_coordinate(j, 0);
      const double y = blade_point_coordinate(j, 1);
      rotor_point_tag(i, j) = gmsh::model::geo::addPoint(x * cosine - y * sine, x * sine + y * cosine, 0.0, 0.0015);
    }
  }
  for (int i = 0; i < 8; i++) {
    if (i == 0 || i == 4) {
      outer_curve_tag(i) =
          gmsh::model::geo::addCircleArc(outer_point_tag(i), center_point_tag, outer_point_tag((i + 1) % 8));
    } else {
      outer_curve_tag(i) = gmsh::model::geo::addLine(outer_point_tag(i), outer_point_tag((i + 1) % 8));
    }
  }
  for (int i = 0; i < 4; i++) {
    interface_curve_tag(i) =
        gmsh::model::geo::addCircleArc(interface_point_tag(i), center_point_tag, interface_point_tag((i + 1) % 4));
    for (int j = 0; j < 3; j++) {
      rotor_curve_tag(i, j) = gmsh::model::geo::addLine(rotor_point_tag(i, j), rotor_point_tag(i, j + 1));
    }
    rotor_curve_tag(i, 3) =
        gmsh::model::geo::addCircleArc(rotor_point_tag(i, 3), center_point_tag, rotor_point_tag((i + 1) % 4, 0));
  }
  std::vector<int> outer_loop_curves;
  std::vector<int> interface_loop_curves;
  std::vector<int> rotor_loop_curves;
  for (int i = 0; i < 8; i++) {
    outer_loop_curves.emplace_back(outer_curve_tag(i));
    if (i == 2 || i == 6) {
      physical_group_tag[0].emplace_back(outer_curve_tag(i));
    } else {
      physical_group_tag[1].emplace_back(outer_curve_tag(i));
    }
  }
  for (int i = 0; i < 4; i++) {
    interface_loop_curves.emplace_back(interface_curve_tag(i));
    for (int j = 0; j < 4; j++) {
      rotor_loop_curves.emplace_back(rotor_curve_tag(i, j));
      physical_group_tag[1].emplace_back(rotor_curve_tag(i, j));
    }
    gmsh::model::geo::mesh::setTransfiniteCurve(interface_curve_tag(i), 72);
  }
  const int outer_loop_tag = gmsh::model::geo::addCurveLoop(outer_loop_curves);
  const int interface_loop_tag = gmsh::model::geo::addCurveLoop(interface_loop_curves);
  const int rotor_loop_tag = gmsh::model::geo::addCurveLoop(rotor_loop_curves);
  const int outer_surface_tag = gmsh::model::geo::addPlaneSurface({outer_loop_tag, -interface_loop_tag});
  const int inner_surface_tag = gmsh::model::geo::addPlaneSurface({interface_loop_tag, -rotor_loop_tag});
  gmsh::model::geo::synchronize();
  physical_group_tag[2].emplace_back(outer_surface_tag);
  physical_group_tag[3].emplace_back(inner_surface_tag);
  gmsh::model::addPhysicalGroup(1, physical_group_tag[0], 1, "bc-1");
  gmsh::model::addPhysicalGroup(1, physical_group_tag[1], 2, "bc-2");
  gmsh::model::addPhysicalGroup(2, physical_group_tag[2], 3, "vc-1");
  gmsh::model::addPhysicalGroup(2, physical_group_tag[3], 4, "rc-1");
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::setOrder(SimulationControl::kPolynomialOrder);
  gmsh::model::mesh::optimize("HighOrder");
  gmsh::write(mesh_file_path);
}
