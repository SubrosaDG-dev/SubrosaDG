/**
 * @file vawt_2d_incns.cpp
 * @brief The 2D three-bladed NACA 0018 vertical-axis wind turbine example.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2026-10-22
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

namespace SubrosaDG {

// Illustrative air properties for the laminar solver; the reference paper uses SST URANS.
struct PhysicalModelData {
  // EquationOfState
  static constexpr double kReferenceSoundSpeed = 340.0;
  static constexpr double kReferenceDensity = 1.225;

  // ThermodynamicModel
  static constexpr double kSpecificHeatConstantPressure = 1005.0;
  static constexpr double kSpecificHeatConstantVolume = 718.0;

  // TransportModel
  static constexpr double kDynamicViscosity = 1.81e-5;
  static constexpr double kPrandtlNumber = 0.71;
  static constexpr double kThermalConductivity = kSpecificHeatConstantPressure * kDynamicViscosity / kPrandtlNumber;
};

}  // namespace SubrosaDG

#include "SubrosaDG.cpp"

inline const std::string kExampleName{"vawt_2d_incns"};

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory / "build/out" / kExampleName};

inline constexpr double kRotorRadius = 0.85;
inline constexpr double kBladeChord = 0.246;
// The inflow speed is a configurable example setting, not specified in the paper's geometry table.
inline constexpr SubrosaDG::Real kInflowVelocity = 8.0_r;
inline constexpr SubrosaDG::Real kTipSpeedRatio = 2.36_r;
inline constexpr SubrosaDG::Real kAngularVelocity =
    kTipSpeedRatio * kInflowVelocity / static_cast<SubrosaDG::Real>(kRotorRadius);
inline constexpr SubrosaDG::Real kZeroPressureDensity =
    static_cast<SubrosaDG::Real>(0.99 * SubrosaDG::PhysicalModelData::kReferenceDensity);

using SimulationControl = SubrosaDG::SimulationControl<
    SubrosaDG::SolveControl<SubrosaDG::DimensionEnum::D2, SubrosaDG::PolynomialOrderEnum::P1,
                            SubrosaDG::BoundaryTimeEnum::Steady, SubrosaDG::SourceTermEnum::None>,
    SubrosaDG::NumericalControl<SubrosaDG::MeshModelEnum::TriangleQuadrangle, SubrosaDG::InitialConditionEnum::Function,
                                SubrosaDG::TimeIntegrationEnum::SSPRK3>,
    SubrosaDG::IncompressibleNSVariable<SubrosaDG::ThermodynamicModelEnum::Constant,
                                        SubrosaDG::EquationOfStateEnum::WeakCompressibleFluid,
                                        SubrosaDG::TransportModelEnum::Constant,
                                        SubrosaDG::ConvectiveFluxEnum::LaxFriedrichs, SubrosaDG::ViscousFluxEnum::BR2>>;

template <typename SimulationControl>
inline void SubrosaDG::InitialCondition<SimulationControl>::computePrimitiveFromCoordinate(
    [[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate,
    Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>& initial_primitive_variable) {
  initial_primitive_variable = {kZeroPressureDensity, kInflowVelocity, 0.0_r, 288.15_r};
}

template <typename SimulationControl>
inline void SubrosaDG::BoundaryCondition<SimulationControl>::computePrimitiveFromCoordinate(
    [[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate,
    Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>& boundary_primitive_variable,
    const SubrosaDG::Isize gmsh_physical_index) {
  boundary_primitive_variable = {kZeroPressureDensity, kInflowVelocity, 0.0_r, 288.15_r};
  if (gmsh_physical_index == 4) {
    // The wall condition adds the local mesh velocity to this zero relative velocity.
    boundary_primitive_variable = {kZeroPressureDensity, 0.0_r, 0.0_r, 288.15_r};
  }
}

int main(int argc, char* argv[]) {
  const bool mesh_only = argc > 1 && std::string_view{argv[1]} == "--mesh-only";
  SubrosaDG::System<SimulationControl> system{SubrosaDG::CommandLineEnum::Agent};
  system.setMesh(kExampleDirectory / std::format("{}.msh", kExampleName), generateMesh);
  system.setRotation(Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>{0.0_r, 0.0_r},
                     Eigen::Vector<SubrosaDG::Real, 3>{0.0_r, 0.0_r, kAngularVelocity});
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::VelocityInflow>(1);
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::PressureOutflow>(2);
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticSlipWall>(3);
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticNonSlipWall>(4);
  system.template addVolumeCondition<SubrosaDG::InteriorConditionEnum::Static>(5);
  system.template addVolumeCondition<SubrosaDG::InteriorConditionEnum::Rotate>(6);
  system.synchronize();
  if (mesh_only) {
    return EXIT_SUCCESS;
  }
  system.setTimeIntegration(0.5_r);
  system.setViewConfig(kExampleDirectory, kExampleName);
  system.addViewVariable({SubrosaDG::ViewVariableEnum::Density, SubrosaDG::ViewVariableEnum::Velocity,
                          SubrosaDG::ViewVariableEnum::Pressure, SubrosaDG::ViewVariableEnum::Vorticity});
  system.solve();
  system.view();
  return EXIT_SUCCESS;
}

void generateMesh(const std::filesystem::path& mesh_file_path) {
  // Onel et al. (2025), doi:10.1002/cpe.8353, Table 2, Figure 2 and Mesh-B in Table 3.
  // All lengths are in metres. The shaft and struts are absent from the paper's 2D geometry.
  constexpr double kRotorDiameter = 2.0 * kRotorRadius;
  constexpr double kInterfaceRadius = 1.5 * kRotorRadius;
  constexpr double kInletDistance = 40.0 * kRotorDiameter;
  constexpr double kOutletDistance = 100.0 * kRotorDiameter;
  constexpr double kLateralDistance = 30.0 * kRotorDiameter;
  constexpr int kInterfaceCells = 664;
  constexpr int kAirfoilNodes = 480;
  constexpr int kProfileSamples = 100;
  constexpr double kFirstLayerHeight = 5.0e-5;
  constexpr double kLayerGrowth = 1.2;
  constexpr int kLayerCount = 13;
  // Zero pitch, quarter-chord mounting and zero initial azimuth are explicit geometry assumptions.
  // At zero azimuth blade 1 is above the centre, with its leading edge pointing to the left.
  constexpr double kMountingChordFraction = 0.25;
  constexpr double kInitialAzimuth = 0.0;
  // The paper specifies wake sizes but not its exact extent; expose the latter as mesh choices.
  constexpr double kWakeLength = 10.0 * kRotorDiameter;
  constexpr double kWakeEndHalfWidth = 1.5 * kRotorDiameter;
  constexpr double kWakeEndSize = 0.1205;
  constexpr double kFarfieldSize = 2.0 * kRotorDiameter;
  constexpr double kSizeGrowth = 0.15;
  const double interface_size = 2.0 * std::numbers::pi_v<double> * kInterfaceRadius / kInterfaceCells;
  const double layer_thickness = kFirstLayerHeight * (std::pow(kLayerGrowth, kLayerCount) - 1.0) / (kLayerGrowth - 1.0);

  Eigen::Matrix<double, 4, 2, Eigen::RowMajor> farfield_point_coordinate;
  // clang-format off
  farfield_point_coordinate << -kInletDistance, -kLateralDistance,
                               kOutletDistance, -kLateralDistance,
                               kOutletDistance,  kLateralDistance,
                              -kInletDistance,  kLateralDistance;
  // clang-format on
  Eigen::Array<int, 4, 1> farfield_point_tag;
  Eigen::Array<int, 4, 1> farfield_curve_tag;
  Eigen::Array<int, 4, 1> interface_point_tag;
  Eigen::Array<int, 4, 1> interface_curve_tag;
  std::array<std::vector<int>, 6> physical_group_tag;
  std::vector<int> blade_loop_tag;
  std::vector<double> blade_curve_tag;
  std::vector<double> trailing_edge_point_tag;
  gmsh::model::add("vawt_2d");
  const int center_point_tag = gmsh::model::geo::addPoint(0.0, 0.0, 0.0);
  for (int i = 0; i < 4; i++) {
    farfield_point_tag(i) = gmsh::model::geo::addPoint(farfield_point_coordinate(i, 0), farfield_point_coordinate(i, 1),
                                                       0.0, kFarfieldSize);
    const double angle = static_cast<double>(i) * std::numbers::pi_v<double> / 2.0;
    interface_point_tag(i) = gmsh::model::geo::addPoint(kInterfaceRadius * std::cos(angle),
                                                        kInterfaceRadius * std::sin(angle), 0.0, interface_size);
  }
  for (int i = 0; i < 4; i++) {
    farfield_curve_tag(i) = gmsh::model::geo::addLine(farfield_point_tag(i), farfield_point_tag((i + 1) % 4));
    interface_curve_tag(i) =
        gmsh::model::geo::addCircleArc(interface_point_tag(i), center_point_tag, interface_point_tag((i + 1) % 4));
    gmsh::model::geo::mesh::setTransfiniteCurve(interface_curve_tag(i), kInterfaceCells / 4 + 1);
  }
  for (int i = 0; i < 3; i++) {
    const double angle = kInitialAzimuth + static_cast<double>(i) * 2.0 * std::numbers::pi_v<double> / 3.0;
    const double cosine = std::cos(angle);
    const double sine = std::sin(angle);
    const int leading_edge_point_tag =
        gmsh::model::geo::addPoint(-kMountingChordFraction * kBladeChord * cosine - kRotorRadius * sine,
                                   -kMountingChordFraction * kBladeChord * sine + kRotorRadius * cosine, 0.0);
    std::array<std::vector<int>, 2> airfoil_point_tag;
    for (int j = 0; j < 2; j++) {
      airfoil_point_tag[j].emplace_back(leading_edge_point_tag);
      for (int k = 1; k <= kProfileSamples; k++) {
        const double x = 0.5 * (1.0 - std::cos(static_cast<double>(k) * std::numbers::pi_v<double> / kProfileSamples));
        // Standard NACA 0018 thickness with a finite trailing edge, as illustrated in Figure 1.
        const double thickness =
            5.0 * 0.18 *
            (0.2969 * std::sqrt(x) - 0.1260 * x - 0.3516 * x * x + 0.2843 * x * x * x - 0.1015 * x * x * x * x);
        const double local_x = kBladeChord * (x - kMountingChordFraction);
        const double local_y = kRotorRadius + (j == 0 ? 1.0 : -1.0) * kBladeChord * thickness;
        airfoil_point_tag[j].emplace_back(
            gmsh::model::geo::addPoint(local_x * cosine - local_y * sine, local_x * sine + local_y * cosine, 0.0));
      }
      trailing_edge_point_tag.emplace_back(airfoil_point_tag[j].back());
    }
    const int upper_curve_tag = gmsh::model::geo::addSpline(airfoil_point_tag[0]);
    const int lower_curve_tag = gmsh::model::geo::addSpline(airfoil_point_tag[1]);
    const int trailing_curve_tag = gmsh::model::geo::addLine(airfoil_point_tag[0].back(), airfoil_point_tag[1].back());
    blade_loop_tag.emplace_back(
        gmsh::model::geo::addCurveLoop({upper_curve_tag, trailing_curve_tag, -lower_curve_tag}));
    for (const int curve_tag : {upper_curve_tag, lower_curve_tag, trailing_curve_tag}) {
      physical_group_tag[3].emplace_back(curve_tag);
      blade_curve_tag.emplace_back(curve_tag);
    }
    // 239 + 239 + 2 segments give 480 distinct surface nodes per blade at P1.
    gmsh::model::geo::mesh::setTransfiniteCurve(upper_curve_tag, kAirfoilNodes / 2, "Bump", 0.2);
    gmsh::model::geo::mesh::setTransfiniteCurve(lower_curve_tag, kAirfoilNodes / 2, "Bump", 0.2);
    gmsh::model::geo::mesh::setTransfiniteCurve(trailing_curve_tag, 3);
  }
  const int farfield_loop_tag = gmsh::model::geo::addCurveLoop(
      {farfield_curve_tag(0), farfield_curve_tag(1), farfield_curve_tag(2), farfield_curve_tag(3)});
  const int interface_loop_tag = gmsh::model::geo::addCurveLoop(
      {interface_curve_tag(0), interface_curve_tag(1), interface_curve_tag(2), interface_curve_tag(3)});
  physical_group_tag[4].emplace_back(gmsh::model::geo::addPlaneSurface({farfield_loop_tag, interface_loop_tag}));
  std::vector<int> rotating_loop_tag{interface_loop_tag};
  rotating_loop_tag.insert(rotating_loop_tag.end(), blade_loop_tag.begin(), blade_loop_tag.end());
  physical_group_tag[5].emplace_back(gmsh::model::geo::addPlaneSurface(rotating_loop_tag));
  gmsh::model::geo::synchronize();
  physical_group_tag[0] = {farfield_curve_tag(3)};
  physical_group_tag[1] = {farfield_curve_tag(1)};
  physical_group_tag[2] = {farfield_curve_tag(0), farfield_curve_tag(2)};
  for (int i = 0; i < 4; i++) {
    gmsh::model::addPhysicalGroup(1, physical_group_tag[i], i + 1, std::format("bc-{}", i + 1));
  }
  gmsh::model::addPhysicalGroup(2, physical_group_tag[4], 5, "vc-1");
  gmsh::model::addPhysicalGroup(2, physical_group_tag[5], 6, "rc-1");
  // The shared circular interface is internal, not a boundary physical group. The reader splits it for rotation.
  // A C++ size callback also works with Gmsh builds that disable MathEval support.
  gmsh::model::mesh::setSizeCallback([=]([[maybe_unused]] const int dimension, [[maybe_unused]] const int tag,
                                         const double x, const double y, [[maybe_unused]] const double z,
                                         [[maybe_unused]] const double size) {
    const double rotor_size = interface_size + kSizeGrowth * std::max(std::hypot(x, y) - kInterfaceRadius, 0.0);
    const double wake_coordinate = std::clamp(x, 0.0, kWakeLength);
    const double wake_half_width =
        kInterfaceRadius + (kWakeEndHalfWidth - kInterfaceRadius) * wake_coordinate / kWakeLength;
    const double wake_size = interface_size + (kWakeEndSize - interface_size) * wake_coordinate / kWakeLength +
                             kSizeGrowth * (std::max(std::abs(y) - wake_half_width, 0.0) + std::max(-x, 0.0) +
                                            std::max(x - kWakeLength, 0.0));
    return std::min({rotor_size, wake_size, kFarfieldSize});
  });
  const int boundary_layer_field_tag = gmsh::model::mesh::field::add("BoundaryLayer");
  gmsh::model::mesh::field::setNumbers(boundary_layer_field_tag, "CurvesList", blade_curve_tag);
  gmsh::model::mesh::field::setNumber(boundary_layer_field_tag, "Size", kFirstLayerHeight);
  gmsh::model::mesh::field::setNumber(boundary_layer_field_tag, "Ratio", kLayerGrowth);
  // Include the last layer despite roundoff in Gmsh's strict thickness cutoff.
  gmsh::model::mesh::field::setNumber(boundary_layer_field_tag, "Thickness", layer_thickness * (1.0 + 1.0e-8));
  gmsh::model::mesh::field::setNumber(boundary_layer_field_tag, "Quads", 1);
  gmsh::model::mesh::field::setNumbers(boundary_layer_field_tag, "FanPointsList", trailing_edge_point_tag);
  gmsh::model::mesh::field::setNumbers(boundary_layer_field_tag, "FanPointsSizesList", {5, 5, 5, 5, 5, 5});
  gmsh::model::mesh::field::setAsBoundaryLayer(boundary_layer_field_tag);
  gmsh::option::setNumber("Mesh.MeshSizeExtendFromBoundary", 0);
  gmsh::option::setNumber("Mesh.MeshSizeFromPoints", 0);
  gmsh::option::setNumber("Mesh.MeshSizeFromCurvature", 0);
  gmsh::option::setNumber("Mesh.MeshSizeMax", kFarfieldSize);
  gmsh::option::setNumber("Mesh.Algorithm", 6);
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::removeSizeCallback();
  gmsh::model::mesh::setOrder(SimulationControl::kPolynomialOrder);
  gmsh::model::mesh::optimize("HighOrder");
  gmsh::write(mesh_file_path);
}
