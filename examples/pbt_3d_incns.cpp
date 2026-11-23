/**
 * @file pbt_3d_incns.cpp
 * @brief The three-dimensional pitched-blade mixer of Campos et al., arXiv:2608.27423, Section 4.5.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2026-10-24
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

namespace SubrosaDG {

struct PhysicalModelData {
  // EquationOfState: unit initial density at zero gauge pressure.
  static constexpr double kReferenceSoundSpeed = 100.0;
  static constexpr double kReferenceDensity = 1.0 / 0.99;

  // ThermodynamicModel
  static constexpr double kSpecificHeatConstantPressure = 1.0;
  static constexpr double kSpecificHeatConstantVolume = 1.0;

  // TransportModel: Re = N Di^2 / nu, with N in revolutions per second.
  static constexpr double kTankDiameter = 0.365;
  static constexpr double kImpellerDiameter = kTankDiameter / 3.0;
  static constexpr double kRotationFrequency = 16.0;
  static constexpr double kReynoldsNumber = 200.0;
  static constexpr double kDynamicViscosity =
      kRotationFrequency * kImpellerDiameter * kImpellerDiameter / kReynoldsNumber;
  static constexpr double kPrandtlNumber = 0.71;
  static constexpr double kThermalConductivity = kSpecificHeatConstantPressure * kDynamicViscosity / kPrandtlNumber;
};

}  // namespace SubrosaDG

#include "SubrosaDG.cpp"

inline const std::string kExampleName{"pbt_3d_incns"};

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory / "build/out" / kExampleName};

inline constexpr double kTankDiameter = SubrosaDG::PhysicalModelData::kTankDiameter;
inline constexpr double kTankHeight = kTankDiameter;
inline constexpr double kImpellerDiameter = SubrosaDG::PhysicalModelData::kImpellerDiameter;
inline constexpr double kImpellerClearance = kTankDiameter / 4.0;
inline constexpr double kBladeWidth = kImpellerDiameter / 5.0;
inline constexpr double kBladeThickness = kBladeWidth / 10.0;
inline constexpr double kBladePitch = std::numbers::pi_v<double> / 4.0;
inline constexpr double kShaftDiameter = kBladeWidth * std::numbers::sqrt2_v<double> / 2.0;
inline constexpr double kHubDiameter = 1.4 * kShaftDiameter;
inline constexpr double kHubHeight = kBladeWidth;
inline constexpr double kInterfaceRadius = 1.2 * kImpellerDiameter / 2.0;
inline constexpr SubrosaDG::Real kAngularVelocity =
    2.0_r * std::numbers::pi_v<SubrosaDG::Real> * SubrosaDG::PhysicalModelData::kRotationFrequency;

using SimulationControl = SubrosaDG::SimulationControl<
    SubrosaDG::SolveControl<SubrosaDG::DimensionEnum::D3, SubrosaDG::PolynomialOrderEnum::P1,
                            SubrosaDG::BoundaryTimeEnum::TimeVarying, SubrosaDG::SourceTermEnum::None>,
    SubrosaDG::NumericalControl<SubrosaDG::MeshModelEnum::TetrahedronPyramidHexahedron,
                                SubrosaDG::InitialConditionEnum::Function, SubrosaDG::TimeIntegrationEnum::SSPRK3>,
    SubrosaDG::IncompressibleNSVariable<SubrosaDG::ThermodynamicModelEnum::Constant,
                                        SubrosaDG::EquationOfStateEnum::WeakCompressibleFluid,
                                        SubrosaDG::TransportModelEnum::Constant,
                                        SubrosaDG::ConvectiveFluxEnum::LaxFriedrichs, SubrosaDG::ViscousFluxEnum::BR2>>;

template <typename SimulationControl>
inline void SubrosaDG::InitialCondition<SimulationControl>::computePrimitiveFromCoordinate(
    [[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate,
    Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>& initial_primitive_variable) {
  initial_primitive_variable = {1.0_r, 0.0_r, 0.0_r, 0.0_r, 1.0_r};
}

template <typename SimulationControl>
inline void SubrosaDG::BoundaryCondition<SimulationControl>::computePrimitiveFromCoordinate(
    const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate,
    Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>& boundary_primitive_variable,
    [[maybe_unused]] const SubrosaDG::Real time, const SubrosaDG::Isize gmsh_physical_index) {
  boundary_primitive_variable = {1.0_r, 0.0_r, 0.0_r, 0.0_r, 1.0_r};
  if (gmsh_physical_index == 1 && coordinate.template head<2>().squaredNorm() < kInterfaceRadius * kInterfaceRadius) {
    // The full-height rotating grid touches the stationary bottom. Cancel the
    // mesh velocity there; zero relative velocity on group 3 moves the impeller.
    boundary_primitive_variable(1) = kAngularVelocity * coordinate.y();
    boundary_primitive_variable(2) = -kAngularVelocity * coordinate.x();
  }
}

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::System<SimulationControl> system;
  system.setMesh(kExampleDirectory / std::format("{}.msh", kExampleName), generateMesh);
  system.setRotation(Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>{0.0_r, 0.0_r, 0.0_r},
                     Eigen::Vector<SubrosaDG::Real, 3>{0.0_r, 0.0_r, kAngularVelocity});
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticNonSlipWall>(1);
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticSlipWall>(2);
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticNonSlipWall>(3);
  system.template addVolumeCondition<SubrosaDG::InteriorConditionEnum::Static>(4);
  system.template addVolumeCondition<SubrosaDG::InteriorConditionEnum::Rotate>(5);
  system.setTimeIntegration(0.1_r, {0, 2000});
  system.setViewConfig(kExampleDirectory, kExampleName, -1);
  system.addViewVariable({SubrosaDG::ViewVariableEnum::Density, SubrosaDG::ViewVariableEnum::Velocity,
                          SubrosaDG::ViewVariableEnum::Pressure, SubrosaDG::ViewVariableEnum::Vorticity});
  system.synchronize();
  system.solve();
  system.view();
  return EXIT_SUCCESS;
}

void generateMesh(const std::filesystem::path& mesh_file_path) {
  // Campos et al., Table 4 and Figures 17-18. The shaft is the z-axis, the
  // bottom is z = 0, and Ci locates the bottom of the hub. All lengths are metres.
  constexpr int kBladeNumber = 4;
  constexpr int kBaffleNumber = 4;
  constexpr int kCircumferentialBlocks = 3 * kBaffleNumber;
  constexpr double kBaffleClearance = kImpellerClearance / 3.0;
  constexpr double kBaffleWidth = kTankDiameter / 10.0;
  constexpr double kBaffleThickness = kBaffleWidth / 3.0;
  // Sb in the source geometry is the span between opposite outer baffle edges.
  constexpr double kBaffleOuterRadius = 19.0 * kTankDiameter / 40.0;
  constexpr double kBaffleInnerRadius = kBaffleOuterRadius - kBaffleWidth;
  constexpr double kBaffleHalfThickness = kBaffleThickness / 2.0;
  constexpr double kTankRadius = kTankDiameter / 2.0;
  constexpr double kImpellerRadius = kImpellerDiameter / 2.0;
  constexpr double kShaftRadius = kShaftDiameter / 2.0;
  constexpr double kHubRadius = kHubDiameter / 2.0;
  constexpr double kHubBottom = kImpellerClearance;
  constexpr double kHubTop = kHubBottom + kHubHeight;
  constexpr double kBladeCenterHeight = (kHubBottom + kHubTop) / 2.0;
  constexpr double kBulkSize = 0.025;
  constexpr double kBladeSize = 0.003;
  constexpr double kShaftSize = 0.006;
  constexpr double kSizeGrowth = 0.35;
  constexpr double kPyramidHeightRatio = 0.15;
  constexpr int kCircumferentialCellsPerBlock = 4;  // Three blocks per quadrant.
  constexpr int kInterfaceAxialCells = 36;
  constexpr int kBottomAxialCells = kInterfaceAxialCells / 12;
  constexpr double kPi = std::numbers::pi_v<double>;
  const double blade_pitch_cosine = std::cos(kBladePitch);
  const double blade_pitch_sine = std::sin(kBladePitch);

  const Eigen::Vector<double, 4> stator_radius{kInterfaceRadius, std::hypot(kBaffleInnerRadius, kBaffleHalfThickness),
                                               std::hypot(kBaffleOuterRadius, kBaffleHalfThickness), kTankRadius};
  const Eigen::Vector<double, 4> stator_narrow_angle{kPi / 12.0, std::atan2(kBaffleHalfThickness, kBaffleInnerRadius),
                                                     std::atan2(kBaffleHalfThickness, kBaffleOuterRadius),
                                                     std::atan2(kBaffleHalfThickness, kBaffleOuterRadius)};
  const Eigen::Vector<double, 3> stator_height{0.0, kBaffleClearance, kTankHeight};
  const Eigen::Vector<int, 3> stator_radial_cells{8, 6, 3};
  const Eigen::Vector<int, 2> stator_axial_cells{kBottomAxialCells, kInterfaceAxialCells - kBottomAxialCells};
  const Eigen::Vector<double, 2> shaft_height{kHubTop, kTankHeight};
  const Eigen::Vector<double, 2> hub_height{kHubBottom, kHubTop};
  Eigen::Matrix<int, 4, 2, Eigen::RowMajor> blade_corner_sign;
  // clang-format off
  blade_corner_sign << -1, -1,
                        1, -1,
                        1,  1,
                       -1,  1;
  // clang-format on
  Eigen::Array<int, 3, 1> stator_axis_point_tag;
  Eigen::Tensor<int, 3> stator_point_tag(kCircumferentialBlocks, 4, 3);
  Eigen::Tensor<int, 3> stator_circumferential_line_tag(kCircumferentialBlocks, 4, 3);
  Eigen::Tensor<int, 3> stator_radial_line_tag(kCircumferentialBlocks, 3, 3);
  Eigen::Tensor<int, 3> stator_axial_line_tag(kCircumferentialBlocks, 4, 2);
  Eigen::Tensor<int, 3> stator_plane_surface_tag(kCircumferentialBlocks, 3, 3);
  Eigen::Tensor<int, 3> stator_circumferential_surface_tag(kCircumferentialBlocks, 4, 2);
  Eigen::Tensor<int, 3> stator_radial_surface_tag(kCircumferentialBlocks, 3, 2);
  Eigen::Array<int, 2, 1> shaft_axis_point_tag;
  Eigen::Array<int, 4, 2> shaft_point_tag;
  Eigen::Array<int, 4, 2> shaft_arc_tag;
  Eigen::Array<int, 4, 1> shaft_axial_line_tag;
  Eigen::Array<int, 4, 1> shaft_surface_tag;
  Eigen::Array<int, 2, 1> hub_axis_point_tag;
  Eigen::Array<int, 4, 2> hub_point_tag;
  Eigen::Array<int, 4, 2> hub_arc_tag;
  std::array<std::vector<int>, 5> physical_group_tag;
  std::vector<int> interface_surface_tag;

  gmsh::model::add("pbt_3d");

  // Each 90-degree sector has a 3-by-3 quadrilateral layout. The middle block
  // is the rectangular baffle above Cb and fluid below it; the other eight
  // blocks retain the wall gap and connect the circular interface to the tank.
  for (int layer = 0; layer < 3; layer++) {
    stator_axis_point_tag(layer) = gmsh::model::geo::addPoint(0.0, 0.0, stator_height(layer));
    for (int ring = 0; ring < 4; ring++) {
      const std::array<double, 3> sector_angle{-kPi / 4.0, -stator_narrow_angle(ring), stator_narrow_angle(ring)};
      for (int sector = 0; sector < kBaffleNumber; sector++) {
        const double azimuth = static_cast<double>(sector) * kPi / 2.0;
        for (int local = 0; local < 3; local++) {
          const int index = 3 * sector + local;
          const double angle = azimuth + sector_angle[static_cast<std::size_t>(local)];
          stator_point_tag(index, ring, layer) = gmsh::model::geo::addPoint(
              stator_radius(ring) * std::cos(angle), stator_radius(ring) * std::sin(angle), stator_height(layer));
        }
      }
    }
  }
  for (int layer = 0; layer < 3; layer++) {
    for (int ring = 0; ring < 4; ring++) {
      for (int i = 0; i < kCircumferentialBlocks; i++) {
        const int next = (i + 1) % kCircumferentialBlocks;
        // The two short straight edges bound the exact rectangular baffle.
        if ((ring == 1 || ring == 2) && (i % 3 == 1)) {
          stator_circumferential_line_tag(i, ring, layer) =
              gmsh::model::geo::addLine(stator_point_tag(i, ring, layer), stator_point_tag(next, ring, layer));
        } else {
          stator_circumferential_line_tag(i, ring, layer) = gmsh::model::geo::addCircleArc(
              stator_point_tag(i, ring, layer), stator_axis_point_tag(layer), stator_point_tag(next, ring, layer));
        }
        gmsh::model::geo::mesh::setTransfiniteCurve(stator_circumferential_line_tag(i, ring, layer),
                                                    kCircumferentialCellsPerBlock + 1);
      }
    }
    for (int ring = 0; ring < 3; ring++) {
      for (int i = 0; i < kCircumferentialBlocks; i++) {
        stator_radial_line_tag(i, ring, layer) =
            gmsh::model::geo::addLine(stator_point_tag(i, ring, layer), stator_point_tag(i, ring + 1, layer));
        gmsh::model::geo::mesh::setTransfiniteCurve(stator_radial_line_tag(i, ring, layer),
                                                    stator_radial_cells(ring) + 1);
      }
    }
    for (int ring = 0; ring < 3; ring++) {
      for (int i = 0; i < kCircumferentialBlocks; i++) {
        if (layer == 2 && ring == 1 && i % 3 == 1) {
          continue;  // Top of the solid baffle.
        }
        const int next = (i + 1) % kCircumferentialBlocks;
        const int curve_loop_tag = gmsh::model::geo::addCurveLoop(
            {stator_radial_line_tag(i, ring, layer), stator_circumferential_line_tag(i, ring + 1, layer),
             -stator_radial_line_tag(next, ring, layer), -stator_circumferential_line_tag(i, ring, layer)});
        stator_plane_surface_tag(i, ring, layer) = gmsh::model::geo::addPlaneSurface({curve_loop_tag});
        gmsh::model::geo::mesh::setTransfiniteSurface(stator_plane_surface_tag(i, ring, layer));
        gmsh::model::geo::mesh::setRecombine(2, stator_plane_surface_tag(i, ring, layer));
      }
    }
  }
  for (int layer = 0; layer < 2; layer++) {
    for (int ring = 0; ring < 4; ring++) {
      for (int i = 0; i < kCircumferentialBlocks; i++) {
        stator_axial_line_tag(i, ring, layer) =
            gmsh::model::geo::addLine(stator_point_tag(i, ring, layer), stator_point_tag(i, ring, layer + 1));
        gmsh::model::geo::mesh::setTransfiniteCurve(stator_axial_line_tag(i, ring, layer),
                                                    stator_axial_cells(layer) + 1);
      }
    }
    for (int ring = 0; ring < 4; ring++) {
      for (int i = 0; i < kCircumferentialBlocks; i++) {
        const int next = (i + 1) % kCircumferentialBlocks;
        const int curve_loop_tag = gmsh::model::geo::addCurveLoop(
            {stator_circumferential_line_tag(i, ring, layer), stator_axial_line_tag(next, ring, layer),
             -stator_circumferential_line_tag(i, ring, layer + 1), -stator_axial_line_tag(i, ring, layer)});
        stator_circumferential_surface_tag(i, ring, layer) = gmsh::model::geo::addSurfaceFilling({curve_loop_tag});
        gmsh::model::geo::mesh::setTransfiniteSurface(stator_circumferential_surface_tag(i, ring, layer));
        gmsh::model::geo::mesh::setRecombine(2, stator_circumferential_surface_tag(i, ring, layer));
      }
    }
    for (int ring = 0; ring < 3; ring++) {
      for (int i = 0; i < kCircumferentialBlocks; i++) {
        const int curve_loop_tag = gmsh::model::geo::addCurveLoop(
            {stator_radial_line_tag(i, ring, layer), stator_axial_line_tag(i, ring + 1, layer),
             -stator_radial_line_tag(i, ring, layer + 1), -stator_axial_line_tag(i, ring, layer)});
        stator_radial_surface_tag(i, ring, layer) = gmsh::model::geo::addSurfaceFilling({curve_loop_tag});
        gmsh::model::geo::mesh::setTransfiniteSurface(stator_radial_surface_tag(i, ring, layer));
        gmsh::model::geo::mesh::setRecombine(2, stator_radial_surface_tag(i, ring, layer));
      }
    }
    for (int ring = 0; ring < 3; ring++) {
      for (int i = 0; i < kCircumferentialBlocks; i++) {
        if (layer == 1 && ring == 1 && i % 3 == 1) {
          continue;  // Solid baffle.
        }
        const int next = (i + 1) % kCircumferentialBlocks;
        const int surface_loop_tag = gmsh::model::geo::addSurfaceLoop(
            {stator_plane_surface_tag(i, ring, layer), stator_plane_surface_tag(i, ring, layer + 1),
             stator_radial_surface_tag(i, ring, layer), stator_circumferential_surface_tag(i, ring + 1, layer),
             stator_radial_surface_tag(next, ring, layer), stator_circumferential_surface_tag(i, ring, layer)});
        const int volume_tag = gmsh::model::geo::addVolume({surface_loop_tag});
        gmsh::model::geo::mesh::setTransfiniteVolume(volume_tag);
        physical_group_tag[3].emplace_back(volume_tag);
        if (ring == 0) {
          interface_surface_tag.emplace_back(stator_circumferential_surface_tag(i, ring, layer));
        }
        if (ring == 2) {
          physical_group_tag[0].emplace_back(stator_circumferential_surface_tag(i, ring + 1, layer));
        }
        if (layer == 0) {
          physical_group_tag[0].emplace_back(stator_plane_surface_tag(i, ring, layer));
        }
        if (layer == 1) {
          physical_group_tag[1].emplace_back(stator_plane_surface_tag(i, ring, layer + 1));
        }
        if (layer == 0 && ring == 1 && i % 3 == 1) {
          physical_group_tag[0].emplace_back(stator_plane_surface_tag(i, ring, layer + 1));
        }
        if (layer == 1) {
          if (ring == 0 && i % 3 == 1) {
            physical_group_tag[0].emplace_back(stator_circumferential_surface_tag(i, ring + 1, layer));
          }
          if (ring == 2 && i % 3 == 1) {
            physical_group_tag[0].emplace_back(stator_circumferential_surface_tag(i, ring, layer));
          }
          if (ring == 1 && i % 3 == 0) {
            physical_group_tag[0].emplace_back(stator_radial_surface_tag(next, ring, layer));
          }
          if (ring == 1 && i % 3 == 2) {
            physical_group_tag[0].emplace_back(stator_radial_surface_tag(i, ring, layer));
          }
        }
      }
    }
  }

  // Build the cylindrical shaft, hub endcaps and four pitched blades.
  // Retain only the true cylindrical hub/blade intersections. The hub skin
  // is meshed in its developed (R theta, z) plane and wrapped back onto the
  // cylinder; artificial angular strips and Coons triangle fans are unnecessary.
  struct HubBoundaryCurve {
    int curve_tag_;
    double start_angle_;
    double end_angle_;
  };
  std::vector<HubBoundaryCurve> hub_boundary_curve;
  std::array<std::array<std::vector<int>, 4>, kBladeNumber> blade_root_curve_tag;

  for (int layer = 0; layer < 2; layer++) {
    hub_axis_point_tag(layer) = gmsh::model::geo::addPoint(0.0, 0.0, hub_height(layer));
    for (int i = 0; i < 4; i++) {
      const double angle = static_cast<double>(i) * kPi / 2.0;
      hub_point_tag(i, layer) =
          gmsh::model::geo::addPoint(kHubRadius * std::cos(angle), kHubRadius * std::sin(angle), hub_height(layer));
    }
    for (int i = 0; i < 4; i++) {
      const int next = (i + 1) % 4;
      const double start_angle = static_cast<double>(i) * kPi / 2.0;
      const double end_angle = static_cast<double>(i + 1) * kPi / 2.0;
      hub_arc_tag(i, layer) = gmsh::model::geo::addCircleArc(hub_point_tag(i, layer), hub_axis_point_tag(layer),
                                                             hub_point_tag(next, layer));
      hub_boundary_curve.push_back({hub_arc_tag(i, layer), start_angle, end_angle});
    }
  }
  shaft_axis_point_tag(0) = hub_axis_point_tag(1);
  shaft_axis_point_tag(1) = stator_axis_point_tag(2);
  for (int layer = 0; layer < 2; layer++) {
    for (int i = 0; i < 4; i++) {
      const double angle = static_cast<double>(i) * kPi / 2.0;
      shaft_point_tag(i, layer) = gmsh::model::geo::addPoint(kShaftRadius * std::cos(angle),
                                                             kShaftRadius * std::sin(angle), shaft_height(layer));
    }
    for (int i = 0; i < 4; i++) {
      const int next = (i + 1) % 4;
      shaft_arc_tag(i, layer) = gmsh::model::geo::addCircleArc(shaft_point_tag(i, layer), shaft_axis_point_tag(layer),
                                                               shaft_point_tag(next, layer));
    }
  }
  for (int i = 0; i < 4; i++) {
    shaft_axial_line_tag(i) = gmsh::model::geo::addLine(shaft_point_tag(i, 0), shaft_point_tag(i, 1));
  }
  for (int i = 0; i < 4; i++) {
    const int next = (i + 1) % 4;
    const int curve_loop_tag = gmsh::model::geo::addCurveLoop(
        {shaft_arc_tag(i, 0), shaft_axial_line_tag(next), -shaft_arc_tag(i, 1), -shaft_axial_line_tag(i)});
    shaft_surface_tag(i) = gmsh::model::geo::addSurfaceFilling({curve_loop_tag});
    physical_group_tag[2].emplace_back(shaft_surface_tag(i));
  }
  const int hub_bottom_loop_tag =
      gmsh::model::geo::addCurveLoop({hub_arc_tag(0, 0), hub_arc_tag(1, 0), hub_arc_tag(2, 0), hub_arc_tag(3, 0)});
  const int hub_top_loop_tag =
      gmsh::model::geo::addCurveLoop({hub_arc_tag(0, 1), hub_arc_tag(1, 1), hub_arc_tag(2, 1), hub_arc_tag(3, 1)});
  const int shaft_bottom_loop_tag = gmsh::model::geo::addCurveLoop(
      {shaft_arc_tag(0, 0), shaft_arc_tag(1, 0), shaft_arc_tag(2, 0), shaft_arc_tag(3, 0)});
  const int shaft_top_loop_tag = gmsh::model::geo::addCurveLoop(
      {shaft_arc_tag(0, 1), shaft_arc_tag(1, 1), shaft_arc_tag(2, 1), shaft_arc_tag(3, 1)});
  physical_group_tag[2].emplace_back(gmsh::model::geo::addPlaneSurface({hub_bottom_loop_tag}));
  physical_group_tag[2].emplace_back(gmsh::model::geo::addPlaneSurface({hub_top_loop_tag, shaft_bottom_loop_tag}));

  for (int blade = 0; blade < kBladeNumber; blade++) {
    const double azimuth = static_cast<double>(blade) * kPi / 2.0;
    const Eigen::Vector3d radial{std::cos(azimuth), std::sin(azimuth), 0.0};
    const Eigen::Vector3d tangent{-std::sin(azimuth), std::cos(azimuth), 0.0};
    Eigen::Array<int, 4, 1> blade_root_point_tag;
    Eigen::Array<int, 4, 1> blade_tip_point_tag;
    Eigen::Array<int, 4, 1> blade_span_line_tag;
    Eigen::Array<int, 4, 1> blade_tip_line_tag;
    Eigen::Vector<double, 4> blade_root_angle;
    for (int corner = 0; corner < 4; corner++) {
      const double width = static_cast<double>(blade_corner_sign(corner, 0)) * kBladeWidth / 2.0;
      const double thickness = static_cast<double>(blade_corner_sign(corner, 1)) * kBladeThickness / 2.0;
      const double tangential = blade_pitch_cosine * width + blade_pitch_sine * thickness;
      const double height = kBladeCenterHeight - blade_pitch_sine * width + blade_pitch_cosine * thickness;
      const double root_angle = azimuth + std::asin(tangential / kHubRadius);
      const Eigen::Vector3d tip_coordinate =
          kImpellerRadius * radial + tangential * tangent + Eigen::Vector3d{0.0, 0.0, height};
      blade_root_angle(corner) = root_angle;
      blade_root_point_tag(corner) =
          gmsh::model::geo::addPoint(kHubRadius * std::cos(root_angle), kHubRadius * std::sin(root_angle), height);
      blade_tip_point_tag(corner) =
          gmsh::model::geo::addPoint(tip_coordinate.x(), tip_coordinate.y(), tip_coordinate.z());
      blade_span_line_tag(corner) =
          gmsh::model::geo::addLine(blade_root_point_tag(corner), blade_tip_point_tag(corner));
    }
    for (int face = 0; face < 4; face++) {
      const int next = (face + 1) % 4;
      const double slope =
          face % 2 == 0 ? -blade_pitch_sine / blade_pitch_cosine : blade_pitch_cosine / blade_pitch_sine;
      const double offset = face % 2 == 0 ? (face == 0 ? -1.0 : 1.0) * kBladeThickness / (2.0 * blade_pitch_cosine)
                                          : (face == 1 ? -1.0 : 1.0) * kBladeWidth / (2.0 * blade_pitch_sine);
      const Eigen::Vector3d center_coordinate{0.0, 0.0, kBladeCenterHeight + offset};
      const Eigen::Vector3d major_coordinate =
          center_coordinate + kHubRadius * (tangent + Eigen::Vector3d{0.0, 0.0, slope});
      const int center_point_tag =
          gmsh::model::geo::addPoint(center_coordinate.x(), center_coordinate.y(), center_coordinate.z());
      const int major_point_tag =
          gmsh::model::geo::addPoint(major_coordinate.x(), major_coordinate.y(), major_coordinate.z());
      std::vector<int> segment_point_tag{blade_root_point_tag(face)};
      std::vector<double> segment_angle{blade_root_angle(face)};
      if (blade == 0 && face % 2 == 0) {
        segment_point_tag.emplace_back(gmsh::model::geo::addPoint(kHubRadius, 0.0, center_coordinate.z()));
        segment_angle.emplace_back(0.0);
      }
      segment_point_tag.emplace_back(blade_root_point_tag(next));
      segment_angle.emplace_back(blade_root_angle(next));
      std::vector<int>& root_curves =
          blade_root_curve_tag[static_cast<std::size_t>(blade)][static_cast<std::size_t>(face)];
      for (std::size_t segment = 0; segment + 1 < segment_point_tag.size(); segment++) {
        const int curve_tag = gmsh::model::geo::addEllipseArc(
            segment_point_tag[segment], center_point_tag, major_point_tag, segment_point_tag[segment + 1], -1,
            slope * std::sin(azimuth), -slope * std::cos(azimuth), 1.0);
        root_curves.emplace_back(curve_tag);
        double start_angle = segment_angle[segment];
        double end_angle = segment_angle[segment + 1];
        // Only blade 0 crosses the periodic cut of the developed cylinder.
        if ((start_angle + end_angle) / 2.0 < 0.0) {
          start_angle += 2.0 * kPi;
          end_angle += 2.0 * kPi;
        }
        hub_boundary_curve.push_back({curve_tag, start_angle, end_angle});
      }
      blade_tip_line_tag(face) = gmsh::model::geo::addLine(blade_tip_point_tag(face), blade_tip_point_tag(next));
      std::vector<int> face_curve_tag = root_curves;
      face_curve_tag.emplace_back(blade_span_line_tag(next));
      face_curve_tag.emplace_back(-blade_tip_line_tag(face));
      face_curve_tag.emplace_back(-blade_span_line_tag(face));
      const int face_curve_loop_tag = gmsh::model::geo::addCurveLoop(face_curve_tag);
      physical_group_tag[2].emplace_back(gmsh::model::geo::addPlaneSurface({face_curve_loop_tag}));
    }
    const int tip_curve_loop_tag = gmsh::model::geo::addCurveLoop(
        {blade_tip_line_tag(0), blade_tip_line_tag(1), blade_tip_line_tag(2), blade_tip_line_tag(3)});
    physical_group_tag[2].emplace_back(gmsh::model::geo::addPlaneSurface({tip_curve_loop_tag}));
  }

  std::vector<int> rotor_bottom_curve_tag;
  std::vector<int> rotor_top_curve_tag;
  rotor_bottom_curve_tag.reserve(kCircumferentialBlocks);
  rotor_top_curve_tag.reserve(kCircumferentialBlocks);
  for (int i = 0; i < kCircumferentialBlocks; i++) {
    rotor_bottom_curve_tag.emplace_back(stator_circumferential_line_tag(i, 0, 0));
    rotor_top_curve_tag.emplace_back(stator_circumferential_line_tag(i, 0, 2));
  }
  const int rotor_bottom_loop_tag = gmsh::model::geo::addCurveLoop(rotor_bottom_curve_tag);
  const int rotor_top_loop_tag = gmsh::model::geo::addCurveLoop(rotor_top_curve_tag);
  const int rotor_bottom_surface_tag = gmsh::model::geo::addPlaneSurface({rotor_bottom_loop_tag});
  const int rotor_top_surface_tag = gmsh::model::geo::addPlaneSurface({rotor_top_loop_tag, shaft_top_loop_tag});
  gmsh::model::geo::synchronize();

  std::vector<int> hub_boundary_curve_tag;
  hub_boundary_curve_tag.reserve(hub_boundary_curve.size());
  for (const HubBoundaryCurve& curve : hub_boundary_curve) {
    hub_boundary_curve_tag.emplace_back(curve.curve_tag_);
  }
  const int hub_surface_tag = gmsh::model::addDiscreteEntity(2, -1, hub_boundary_curve_tag);
  physical_group_tag[2].emplace_back(hub_surface_tag);
  std::vector<int> rotor_surface_tag = physical_group_tag[2];
  rotor_surface_tag.emplace_back(rotor_bottom_surface_tag);
  rotor_surface_tag.emplace_back(rotor_top_surface_tag);
  rotor_surface_tag.insert(rotor_surface_tag.end(), interface_surface_tag.begin(), interface_surface_tag.end());
  const int rotor_surface_loop_tag = gmsh::model::geo::addSurfaceLoop(rotor_surface_tag);
  const int rotor_volume_tag = gmsh::model::geo::addVolume({rotor_surface_loop_tag});
  physical_group_tag[0].emplace_back(rotor_bottom_surface_tag);
  physical_group_tag[1].emplace_back(rotor_top_surface_tag);
  physical_group_tag[4].emplace_back(rotor_volume_tag);
  gmsh::model::geo::synchronize();

  for (int group = 0; group < 3; group++) {
    gmsh::model::addPhysicalGroup(2, physical_group_tag[static_cast<std::size_t>(group)], group + 1,
                                  std::format("bc-{}", group + 1));
  }
  gmsh::model::addPhysicalGroup(3, physical_group_tag[3], 4, "vc-1");
  gmsh::model::addPhysicalGroup(3, physical_group_tag[4], 5, "rc-1");

  gmsh::model::mesh::setSizeCallback([=]([[maybe_unused]] const int dimension, [[maybe_unused]] const int tag,
                                         const double x, const double y, const double z,
                                         [[maybe_unused]] const double size) {
    double mesh_size = kBulkSize;
    const double radius = std::hypot(x, y);
    const double shaft_distance = std::hypot(std::max(radius - kShaftRadius, 0.0), std::max(kHubTop - z, 0.0));
    mesh_size = std::min(mesh_size, kShaftSize + kSizeGrowth * shaft_distance);
    const double hub_distance =
        std::hypot(std::max(radius - kHubRadius, 0.0), std::max({kHubBottom - z, z - kHubTop, 0.0}));
    mesh_size = std::min(mesh_size, kBladeSize + kSizeGrowth * hub_distance);
    for (int i = 0; i < kBladeNumber; i++) {
      const double angle = static_cast<double>(i) * kPi / 2.0;
      const double radial = x * std::cos(angle) + y * std::sin(angle);
      const double tangential = -x * std::sin(angle) + y * std::cos(angle);
      const double width = blade_pitch_cosine * tangential - blade_pitch_sine * (z - kBladeCenterHeight);
      const double thickness = blade_pitch_sine * tangential + blade_pitch_cosine * (z - kBladeCenterHeight);
      const double blade_distance = std::sqrt(std::pow(std::max({-radial, radial - kImpellerRadius, 0.0}), 2) +
                                              std::pow(std::max(std::abs(width) - kBladeWidth / 2.0, 0.0), 2) +
                                              std::pow(std::max(std::abs(thickness) - kBladeThickness / 2.0, 0.0), 2));
      mesh_size = std::min(mesh_size, kBladeSize + kSizeGrowth * blade_distance);
    }
    return mesh_size;
  });
  gmsh::option::setNumber("Mesh.MeshSizeExtendFromBoundary", 0);
  gmsh::option::setNumber("Mesh.MeshSizeFromPoints", 0);
  gmsh::option::setNumber("Mesh.MeshSizeFromCurvature", 0);
  gmsh::option::setNumber("Mesh.Algorithm", 6);
  gmsh::option::setNumber("Mesh.Algorithm3D", 1);
  gmsh::option::setNumber("Mesh.Optimize", 1);
  gmsh::option::setNumber("Mesh.OptimizeThreshold", 0.3);
  gmsh::option::setNumber("Mesh.Renumber", 0);
  gmsh::model::mesh::generate(1);

  // Mesh the developed hub with the exact boundary nodes already shared by
  // the blade faces and hub caps. The temporary periodic cut is welded during
  // wrapping and leaves no curve or patch boundary on the final hub surface.
  std::size_t hub_max_node_tag = 0;
  gmsh::model::mesh::getMaxNodeTag(hub_max_node_tag);
  std::vector<std::size_t> boundary_node_tag;
  std::vector<double> boundary_node_coordinate;
  std::vector<double> boundary_node_parametric;
  gmsh::model::mesh::getNodes(boundary_node_tag, boundary_node_coordinate, boundary_node_parametric);
  std::vector<Eigen::Vector3d> boundary_node_position(hub_max_node_tag + 1);
  for (std::size_t i = 0; i < boundary_node_tag.size(); i++) {
    boundary_node_position[boundary_node_tag[i]] = {
        boundary_node_coordinate[3 * i], boundary_node_coordinate[3 * i + 1], boundary_node_coordinate[3 * i + 2]};
  }
  std::map<int, std::vector<std::size_t>> boundary_segment_node_tag;
  for (const HubBoundaryCurve& curve : hub_boundary_curve) {
    std::vector<std::size_t> element_tag;
    gmsh::model::mesh::getElementsByType(1, element_tag, boundary_segment_node_tag[curve.curve_tag_], curve.curve_tag_);
  }

  gmsh::model::add("pbt_3d_developed_hub");
  std::map<std::array<long long, 2>, int> developed_point_tag;
  std::map<std::array<long long, 2>, std::size_t> shared_node_tag;
  std::map<int, std::vector<int>> developed_curve_tag;
  for (const HubBoundaryCurve& curve : hub_boundary_curve) {
    const double middle_angle = (curve.start_angle_ + curve.end_angle_) / 2.0;
    const std::vector<std::size_t>& segment_nodes = boundary_segment_node_tag.at(curve.curve_tag_);
    for (std::size_t i = 0; i < segment_nodes.size(); i += 2) {
      std::array<int, 2> endpoint_tag{};
      for (int end = 0; end < 2; end++) {
        const std::size_t node_tag = segment_nodes[i + static_cast<std::size_t>(end)];
        const Eigen::Vector3d& position = boundary_node_position[node_tag];
        const double angle =
            middle_angle + std::remainder(std::atan2(position.y(), position.x()) - middle_angle, 2.0 * kPi);
        const double developed_x = kHubRadius * angle;
        const std::array<long long, 2> key{std::llround(developed_x * 1e12), std::llround(position.z() * 1e12)};
        if (!developed_point_tag.contains(key)) {
          developed_point_tag[key] = gmsh::model::geo::addPoint(developed_x, position.z(), 0.0, kBladeSize);
        }
        endpoint_tag[static_cast<std::size_t>(end)] = developed_point_tag.at(key);
        shared_node_tag[key] = node_tag;
      }
      const int line_tag = gmsh::model::geo::addLine(endpoint_tag[0], endpoint_tag[1]);
      gmsh::model::geo::mesh::setTransfiniteCurve(line_tag, 2);
      developed_curve_tag[curve.curve_tag_].emplace_back(line_tag);
    }
  }
  std::vector<int> developed_outer_curve_tag;
  for (int i = 0; i < 4; i++) {
    const std::vector<int>& bottom_edges = developed_curve_tag.at(hub_arc_tag(i, 0));
    const std::vector<int>& top_edges = developed_curve_tag.at(hub_arc_tag(i, 1));
    developed_outer_curve_tag.insert(developed_outer_curve_tag.end(), bottom_edges.begin(), bottom_edges.end());
    developed_outer_curve_tag.insert(developed_outer_curve_tag.end(), top_edges.begin(), top_edges.end());
  }
  for (int face = 0; face < 4; face++) {
    for (const int curve_tag : blade_root_curve_tag[0][static_cast<std::size_t>(face)]) {
      const std::vector<int>& root_edges = developed_curve_tag.at(curve_tag);
      developed_outer_curve_tag.insert(developed_outer_curve_tag.end(), root_edges.begin(), root_edges.end());
    }
  }
  const double root_lower_height = kBladeCenterHeight - kBladeThickness / (2.0 * blade_pitch_cosine);
  const double root_upper_height = kBladeCenterHeight + kBladeThickness / (2.0 * blade_pitch_cosine);
  const std::array<double, 2> seam_x{0.0, 2.0 * kPi * kHubRadius};
  const std::array<std::array<double, 2>, 2> seam_z{{{kHubBottom, root_lower_height}, {root_upper_height, kHubTop}}};
  for (const double x : seam_x) {
    for (const std::array<double, 2>& z_interval : seam_z) {
      const std::array<long long, 2> lower_key{std::llround(x * 1e12), std::llround(z_interval[0] * 1e12)};
      const std::array<long long, 2> upper_key{std::llround(x * 1e12), std::llround(z_interval[1] * 1e12)};
      if (!developed_point_tag.contains(lower_key)) {
        developed_point_tag[lower_key] = gmsh::model::geo::addPoint(x, z_interval[0], 0.0, kBladeSize);
      }
      if (!developed_point_tag.contains(upper_key)) {
        developed_point_tag[upper_key] = gmsh::model::geo::addPoint(x, z_interval[1], 0.0, kBladeSize);
      }
      const int seam_line_tag =
          gmsh::model::geo::addLine(developed_point_tag.at(lower_key), developed_point_tag.at(upper_key));
      gmsh::model::geo::mesh::setTransfiniteCurve(
          seam_line_tag, static_cast<int>(std::ceil((z_interval[1] - z_interval[0]) / kBladeSize)) + 1);
      developed_outer_curve_tag.emplace_back(seam_line_tag);
    }
  }
  std::vector<int> developed_curve_loop_tag{gmsh::model::geo::addCurveLoop(developed_outer_curve_tag, -1, true)};
  for (int blade = 1; blade < kBladeNumber; blade++) {
    std::vector<int> hole_curve_tag;
    for (int face = 0; face < 4; face++) {
      for (const int curve_tag :
           blade_root_curve_tag[static_cast<std::size_t>(blade)][static_cast<std::size_t>(face)]) {
        const std::vector<int>& root_edges = developed_curve_tag.at(curve_tag);
        hole_curve_tag.insert(hole_curve_tag.end(), root_edges.begin(), root_edges.end());
      }
    }
    developed_curve_loop_tag.emplace_back(gmsh::model::geo::addCurveLoop(hole_curve_tag, -1, true));
  }
  gmsh::model::geo::addPlaneSurface(developed_curve_loop_tag);
  gmsh::model::geo::synchronize();
  gmsh::model::mesh::setSizeCallback([]([[maybe_unused]] const int dimension, [[maybe_unused]] const int tag,
                                        [[maybe_unused]] const double x, [[maybe_unused]] const double y,
                                        [[maybe_unused]] const double z,
                                        [[maybe_unused]] const double size) { return kBladeSize; });
  gmsh::model::mesh::generate(2);
  std::size_t developed_max_node_tag = 0;
  gmsh::model::mesh::getMaxNodeTag(developed_max_node_tag);
  std::vector<std::size_t> developed_node_tag;
  std::vector<double> developed_node_coordinate;
  std::vector<double> developed_node_parametric;
  std::vector<std::size_t> developed_triangle_tag;
  std::vector<std::size_t> developed_triangle_node_tag;
  gmsh::model::mesh::getNodes(developed_node_tag, developed_node_coordinate, developed_node_parametric);
  gmsh::model::mesh::getElementsByType(2, developed_triangle_tag, developed_triangle_node_tag);
  gmsh::model::remove();
  gmsh::model::setCurrent("pbt_3d");

  std::vector<std::size_t> wrapped_node_tag(developed_max_node_tag + 1, 0);
  std::map<std::array<long long, 3>, std::size_t> periodic_node_tag;
  std::vector<std::size_t> hub_node_tag;
  std::vector<double> hub_node_coordinate;
  std::vector<std::size_t> hub_triangle_node_tag;
  hub_triangle_node_tag.reserve(developed_triangle_node_tag.size());
  for (std::size_t i = 0; i < developed_node_tag.size(); i++) {
    const double x = developed_node_coordinate[3 * i];
    const double z = developed_node_coordinate[3 * i + 1];
    const std::array<long long, 2> key{std::llround(x * 1e12), std::llround(z * 1e12)};
    if (shared_node_tag.contains(key)) {
      wrapped_node_tag[developed_node_tag[i]] = shared_node_tag.at(key);
    } else {
      const double angle = x / kHubRadius;
      const Eigen::Vector3d position{kHubRadius * std::cos(angle), kHubRadius * std::sin(angle), z};
      const std::array<long long, 3> periodic_key{std::llround(position.x() * 1e12), std::llround(position.y() * 1e12),
                                                  std::llround(z * 1e12)};
      if (!periodic_node_tag.contains(periodic_key)) {
        periodic_node_tag[periodic_key] = ++hub_max_node_tag;
        hub_node_tag.emplace_back(hub_max_node_tag);
        hub_node_coordinate.emplace_back(position.x());
        hub_node_coordinate.emplace_back(position.y());
        hub_node_coordinate.emplace_back(position.z());
      }
      wrapped_node_tag[developed_node_tag[i]] = periodic_node_tag.at(periodic_key);
    }
  }
  for (const std::size_t node_tag : developed_triangle_node_tag) {
    hub_triangle_node_tag.emplace_back(wrapped_node_tag[node_tag]);
  }
  gmsh::model::mesh::addNodes(2, hub_surface_tag, hub_node_tag, hub_node_coordinate);
  gmsh::model::mesh::addElementsByType(hub_surface_tag, 2, {}, hub_triangle_node_tag);
  gmsh::option::setNumber("Mesh.MeshOnlyEmpty", 1);
  gmsh::model::mesh::generate(2);

  // Prescribed rotor-side pyramids join the structured stator's quadrilaterals
  // to the tetrahedral rotor. No transitions are needed inside the stator.
  std::vector<std::vector<std::size_t>> interface_quad_node_tag(interface_surface_tag.size());
  for (std::size_t i = 0; i < interface_surface_tag.size(); i++) {
    std::vector<std::size_t> element_tag;
    gmsh::model::mesh::getElementsByType(3, element_tag, interface_quad_node_tag[i], interface_surface_tag[i]);
  }
  gmsh::vectorpair volume_entity{{3, rotor_volume_tag}};
  for (const int volume_tag : physical_group_tag[3]) {
    volume_entity.emplace_back(3, volume_tag);
  }
  gmsh::option::setNumber("Mesh.MeshOnlyVisible", 1);
  gmsh::option::setNumber("Mesh.MeshOnlyEmpty", 1);
  gmsh::model::setVisibility(volume_entity, 0);
  gmsh::model::setVisibility({{3, rotor_volume_tag}}, 1);
  std::vector<std::size_t> mesh_node_tag;
  std::vector<double> mesh_node_coordinate;
  std::vector<double> mesh_node_parametric;
  gmsh::model::mesh::getNodes(mesh_node_tag, mesh_node_coordinate, mesh_node_parametric);
  std::size_t max_node_tag = 0;
  gmsh::model::mesh::getMaxNodeTag(max_node_tag);
  std::vector<Eigen::Vector3d> node_position(max_node_tag + 1);
  for (std::size_t i = 0; i < mesh_node_tag.size(); i++) {
    node_position[mesh_node_tag[i]] = {mesh_node_coordinate[3 * i], mesh_node_coordinate[3 * i + 1],
                                       mesh_node_coordinate[3 * i + 2]};
  }
  std::vector<std::size_t> pyramid_node_tag;
  for (std::size_t surface_index = 0; surface_index < interface_surface_tag.size(); surface_index++) {
    const int surface_tag = interface_surface_tag[surface_index];
    const std::vector<std::size_t>& quad_nodes = interface_quad_node_tag[surface_index];
    std::vector<std::size_t> apex_node_tag;
    std::vector<double> apex_coordinate;
    std::vector<std::size_t> skin_triangle_node_tag;
    for (std::size_t i = 0; i < quad_nodes.size(); i += 4) {
      std::array<std::size_t, 4> base_node_tag{quad_nodes[i], quad_nodes[i + 1], quad_nodes[i + 2], quad_nodes[i + 3]};
      Eigen::Vector3d center = Eigen::Vector3d::Zero();
      double shortest_edge = std::numeric_limits<double>::max();
      for (int corner = 0; corner < 4; corner++) {
        const std::size_t current_node = base_node_tag[static_cast<std::size_t>(corner)];
        const std::size_t next_node = base_node_tag[static_cast<std::size_t>((corner + 1) % 4)];
        center += node_position[current_node] / 4.0;
        shortest_edge = std::min(shortest_edge, (node_position[current_node] - node_position[next_node]).norm());
      }
      const Eigen::Vector3d inward = -Eigen::Vector3d{center.x(), center.y(), 0.0}.normalized();
      const Eigen::Vector3d apex = center + kPyramidHeightRatio * shortest_edge * inward;
      const std::size_t apex_tag = ++max_node_tag;
      apex_node_tag.emplace_back(apex_tag);
      apex_coordinate.emplace_back(apex.x());
      apex_coordinate.emplace_back(apex.y());
      apex_coordinate.emplace_back(apex.z());
      const Eigen::Vector3d normal = (node_position[base_node_tag[1]] - node_position[base_node_tag[0]])
                                         .cross(node_position[base_node_tag[3]] - node_position[base_node_tag[0]]);
      if (normal.dot(inward) < 0.0) {
        std::swap(base_node_tag[1], base_node_tag[3]);
      }
      pyramid_node_tag.insert(pyramid_node_tag.end(), base_node_tag.begin(), base_node_tag.end());
      pyramid_node_tag.emplace_back(apex_tag);
      for (int edge = 0; edge < 4; edge++) {
        skin_triangle_node_tag.emplace_back(base_node_tag[static_cast<std::size_t>(edge)]);
        skin_triangle_node_tag.emplace_back(base_node_tag[static_cast<std::size_t>((edge + 1) % 4)]);
        skin_triangle_node_tag.emplace_back(apex_tag);
      }
    }
    gmsh::model::mesh::removeElements(2, surface_tag);
    gmsh::model::mesh::addNodes(2, surface_tag, apex_node_tag, apex_coordinate);
    gmsh::model::mesh::addElementsByType(surface_tag, 2, {}, skin_triangle_node_tag);
  }
  gmsh::model::mesh::generate(3);
  for (std::size_t surface_index = 0; surface_index < interface_surface_tag.size(); surface_index++) {
    const int surface_tag = interface_surface_tag[surface_index];
    gmsh::model::mesh::removeElements(2, surface_tag);
    gmsh::model::mesh::addElementsByType(surface_tag, 3, {}, interface_quad_node_tag[surface_index]);
  }
  gmsh::model::mesh::addElementsByType(rotor_volume_tag, 7, {}, pyramid_node_tag);
  gmsh::model::mesh::reclassifyNodes();
  gmsh::model::setVisibility(volume_entity, 0);
  for (const int volume_tag : physical_group_tag[3]) {
    gmsh::model::setVisibility({{3, volume_tag}}, 1);
  }
  gmsh::option::setNumber("Mesh.Optimize", 0);
  gmsh::model::mesh::generate(3);
  gmsh::option::setNumber("Mesh.Optimize", 1);
  gmsh::model::setVisibility(volume_entity, 1);
  gmsh::option::setNumber("Mesh.MeshOnlyVisible", 0);
  gmsh::option::setNumber("Mesh.MeshOnlyEmpty", 0);
  gmsh::option::setNumber("Mesh.Renumber", 1);
  gmsh::model::mesh::renumberNodes();
  gmsh::model::mesh::renumberElements();
  gmsh::model::mesh::removeSizeCallback();
  gmsh::model::mesh::setOrder(SimulationControl::kPolynomialOrder);
  gmsh::model::mesh::optimize("HighOrder");
  gmsh::write(mesh_file_path);
}
