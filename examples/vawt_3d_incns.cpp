/**
 * @file vawt_3d_incns.cpp
 * @brief The Elsakka (2021) half-span two-bladed NACA 0015 vertical-axis wind turbine example.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2026-10-22
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

namespace SubrosaDG {

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

inline const std::string kExampleName{"vawt_3d_incns"};

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory / "build/out" / kExampleName};

inline constexpr int kBladeNumber = 2;
inline constexpr double kRotorRadius = 0.85;
inline constexpr double kBladeChord = 0.225;
// Elsakka's thesis, Chapters 4-5, supplies the span omitted from the journal geometry description.
inline constexpr double kBladeHeight = 1.02;
inline constexpr double kBladeThicknessRatio = 0.15;
inline constexpr double kBladePitch = 6.0 * std::numbers::pi_v<double> / 180.0;
inline constexpr double kInterfaceRadius = 1.5 * kRotorRadius;
// The thesis confirms 7 m/s; the journal's isolated 10 m/s statement is inconsistent with TSR = 2.29.
inline constexpr SubrosaDG::Real kInflowVelocity = 7.0_r;
inline constexpr SubrosaDG::Real kTipSpeedRatio = 2.29_r;
// Figure 1: clockwise rotation, phi = 0 at the bottom and phi = 90 degrees upstream.
inline constexpr SubrosaDG::Real kAngularVelocity =
    -kTipSpeedRatio * kInflowVelocity / static_cast<SubrosaDG::Real>(kRotorRadius);

using SimulationControl = SubrosaDG::SimulationControl<
    SubrosaDG::SolveControl<SubrosaDG::DimensionEnum::D3, SubrosaDG::PolynomialOrderEnum::P1,
                            SubrosaDG::BoundaryTimeEnum::Steady, SubrosaDG::SourceTermEnum::None>,
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
  initial_primitive_variable = {1.225_r, kInflowVelocity, 0.0_r, 0.0_r, 288.15_r};
}

template <typename SimulationControl>
inline void SubrosaDG::BoundaryCondition<SimulationControl>::computePrimitiveFromCoordinate(
    [[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate,
    Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>& boundary_primitive_variable,
    const SubrosaDG::Isize gmsh_physical_index) {
  boundary_primitive_variable = {1.225_r, kInflowVelocity, 0.0_r, 0.0_r, 288.15_r};
  if (gmsh_physical_index == 3) {
    // The wall condition adds the local mesh velocity to this zero relative velocity.
    boundary_primitive_variable = {1.225_r, 0.0_r, 0.0_r, 0.0_r, 288.15_r};
  }
}

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  std::filesystem::create_directories(kExampleDirectory);
  SubrosaDG::System<SimulationControl> system;
  system.setMesh(kExampleDirectory / std::format("{}.msh", kExampleName), generateMesh);
  system.setRotation(Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>{0.0_r, 0.0_r, 0.0_r},
                     Eigen::Vector<SubrosaDG::Real, 3>{0.0_r, 0.0_r, kAngularVelocity});
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::RiemannFarfield>(1);
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticSlipWall>(2);
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticNonSlipWall>(3);
  system.template addVolumeCondition<SubrosaDG::InteriorConditionEnum::Static>(4);
  system.template addVolumeCondition<SubrosaDG::InteriorConditionEnum::Rotate>(5);
  system.setTimeIntegration(0.8_r);
  system.setViewConfig(kExampleDirectory, kExampleName);
  system.addViewVariable({SubrosaDG::ViewVariableEnum::Density, SubrosaDG::ViewVariableEnum::Velocity,
                          SubrosaDG::ViewVariableEnum::Pressure, SubrosaDG::ViewVariableEnum::Vorticity});
  system.synchronize();
  system.solve();
  system.view();
  return EXIT_SUCCESS;
}

void generateMesh(const std::filesystem::path& mesh_file_path) {
  // Elsakka et al. (2021), IJRER 11(1), pp. 276-293, Figures 1, 2, 14 and 15.
  // Upper half of the rotor; z = 0 is the mid-span symmetry plane. Lengths are in metres.
  constexpr double kRotorDiameter = 2.0 * kRotorRadius;
  constexpr double kInletDistance = 3.0 * kRotorDiameter;
  constexpr double kOutletDistance = 5.0 * kRotorDiameter;
  constexpr double kFarfieldHalfWidth = 3.0 * kRotorDiameter;
  constexpr double kFarfieldHeight = 3.0 * kRotorDiameter;
  // The cylinder height and support dimensions are not dimensioned in the journal paper.
  constexpr double kInterfaceHeight = kBladeHeight;
  constexpr double kCoreHalfWidth = 0.6;
  constexpr double kRotorBulkSize = 0.22;
  constexpr double kBladeSize = 0.0125;
  constexpr double kSupportSize = 0.023;
  constexpr double kSizeGrowth = 0.35;
  constexpr double kMountingChordFraction = 0.25;
  constexpr double kShaftRadius = 0.04;
  constexpr double kShaftHeight = 0.6;
  constexpr double kArmSpan = 0.35;
  constexpr double kArmWidth = 0.04;
  constexpr double kArmThickness = 0.02;
  constexpr int kProfileSamples = 90;
  constexpr int kBladeSideCells = 40;
  constexpr double kProfileBump = 0.4;
  constexpr double kPyramidHeightRatio = 0.15;
  // Block counts control the structured mesh independently of the body-size callback.
  constexpr int kCircumferentialCells = 12;  // Per quadrant.
  constexpr int kCoreRadialCells = 4;
  constexpr int kRotorAxialCells = 8;
  constexpr int kFarfieldRadialCells = 8;
  constexpr int kUpperAxialCells = 12;
  constexpr double kFarfieldGrowth = 1.4;
  constexpr double kPi = std::numbers::pi_v<double>;
  const double blade_pitch_cosine = std::cos(kBladePitch);
  const double blade_pitch_sine = std::sin(kBladePitch);
  const double shaft_arm_y = std::sqrt(kShaftRadius * kShaftRadius - kArmWidth * kArmWidth / 4.0);

  Eigen::Matrix<int, 4, 2, Eigen::RowMajor> point_direction;
  Eigen::Matrix<double, 4, 2, Eigen::RowMajor> farfield_point_coordinate;
  Eigen::Matrix<double, 4, 2, Eigen::RowMajor> shaft_point_coordinate;
  // clang-format off
  point_direction << -1, -1,
                      1, -1,
                      1,  1,
                     -1,  1;
  farfield_point_coordinate << -kInletDistance,  -kFarfieldHalfWidth,
                                kOutletDistance, -kFarfieldHalfWidth,
                                kOutletDistance,  kFarfieldHalfWidth,
                               -kInletDistance,   kFarfieldHalfWidth;
  shaft_point_coordinate << -kArmWidth / 2.0, -shaft_arm_y,
                             kArmWidth / 2.0, -shaft_arm_y,
                             kArmWidth / 2.0,  shaft_arm_y,
                            -kArmWidth / 2.0,  shaft_arm_y;
  // clang-format on
  const Eigen::Vector<double, 2> ring_coordinate{kCoreHalfWidth, kInterfaceRadius / std::numbers::sqrt2_v<double>};
  const Eigen::Vector<double, 3> farfield_height{0.0, kInterfaceHeight, kFarfieldHeight};
  const Eigen::Vector<double, 4> blade_height{0.0, kArmSpan - kArmThickness / 2.0, kArmSpan + kArmThickness / 2.0,
                                              kBladeHeight / 2.0};
  const Eigen::Vector<double, 4> shaft_height{0.0, blade_height(1), blade_height(2), kShaftHeight};

  Eigen::Array<int, 3, 1> axis_point_tag;
  Eigen::Tensor<int, 3> point_tag(4, 3, 3);
  Eigen::Tensor<int, 3> circumferential_line_tag(4, 3, 3);
  Eigen::Tensor<int, 3> radial_line_tag(4, 2, 3);
  Eigen::Tensor<int, 3> axial_line_tag(4, 3, 2);
  Eigen::Array<int, 9, 3> plane_surface_tag;
  Eigen::Tensor<int, 3> circumferential_surface_tag(4, 3, 2);
  Eigen::Tensor<int, 3> radial_surface_tag(4, 2, 2);
  Eigen::Array<int, 4, 1> shaft_axis_point_tag;
  Eigen::Array<int, 4, 4> shaft_point_tag;
  Eigen::Array<int, 4, 4> shaft_arc_tag;
  Eigen::Array<int, 4, 3> shaft_axial_line_tag;
  Eigen::Tensor<int, 3> blade_section_point_tag(5, 4, kBladeNumber);
  Eigen::Tensor<int, 3> blade_section_curve_tag(5, 4, kBladeNumber);
  Eigen::Tensor<int, 3> blade_axial_line_tag(5, 3, kBladeNumber);
  Eigen::Tensor<int, 3> arm_line_tag(2, 2, kBladeNumber);
  std::array<std::vector<int>, 5> physical_group_tag;
  std::vector<int> interface_surface_tag;

  gmsh::model::add("vawt_3d");

  // An O-grid joins the circular sliding interface to the rectangular far field.
  // Its nine-patch cross-section continues above the rotor, giving 4 lower + 9 upper blocks.
  for (int layer = 0; layer < 3; layer++) {
    axis_point_tag(layer) = gmsh::model::geo::addPoint(0.0, 0.0, farfield_height(layer));
    for (int ring = layer == 0 ? 1 : 0; ring < 3; ring++) {
      for (int i = 0; i < 4; i++) {
        if (ring < 2) {
          point_tag(i, ring, layer) =
              gmsh::model::geo::addPoint(point_direction(i, 0) * ring_coordinate(ring),
                                         point_direction(i, 1) * ring_coordinate(ring), farfield_height(layer));
        } else {
          point_tag(i, ring, layer) = gmsh::model::geo::addPoint(
              farfield_point_coordinate(i, 0), farfield_point_coordinate(i, 1), farfield_height(layer));
        }
      }
    }
  }
  for (int layer = 0; layer < 3; layer++) {
    for (int ring = layer == 0 ? 1 : 0; ring < 3; ring++) {
      for (int i = 0; i < 4; i++) {
        const int next = (i + 1) % 4;
        if (ring == 1) {
          circumferential_line_tag(i, ring, layer) = gmsh::model::geo::addCircleArc(
              point_tag(i, ring, layer), axis_point_tag(layer), point_tag(next, ring, layer));
        } else {
          circumferential_line_tag(i, ring, layer) =
              gmsh::model::geo::addLine(point_tag(i, ring, layer), point_tag(next, ring, layer));
        }
        gmsh::model::geo::mesh::setTransfiniteCurve(circumferential_line_tag(i, ring, layer),
                                                    kCircumferentialCells + 1);
      }
    }
    for (int ring = layer == 0 ? 1 : 0; ring < 2; ring++) {
      for (int i = 0; i < 4; i++) {
        radial_line_tag(i, ring, layer) =
            gmsh::model::geo::addLine(point_tag(i, ring, layer), point_tag(i, ring + 1, layer));
        if (ring == 0) {
          gmsh::model::geo::mesh::setTransfiniteCurve(radial_line_tag(i, ring, layer), kCoreRadialCells + 1);
        } else {
          gmsh::model::geo::mesh::setTransfiniteCurve(radial_line_tag(i, ring, layer), kFarfieldRadialCells + 1,
                                                      "Progression", kFarfieldGrowth);
        }
      }
    }
    if (layer > 0) {
      const int center_curve_loop_tag = gmsh::model::geo::addCurveLoop(
          {circumferential_line_tag(0, 0, layer), circumferential_line_tag(1, 0, layer),
           circumferential_line_tag(2, 0, layer), circumferential_line_tag(3, 0, layer)});
      plane_surface_tag(0, layer) = gmsh::model::geo::addPlaneSurface({center_curve_loop_tag});
      gmsh::model::geo::mesh::setTransfiniteSurface(plane_surface_tag(0, layer));
      gmsh::model::geo::mesh::setRecombine(2, plane_surface_tag(0, layer));
      if (layer == 2) {
        physical_group_tag[0].emplace_back(plane_surface_tag(0, layer));
      }
    }
    for (int ring = layer == 0 ? 1 : 0; ring < 2; ring++) {
      for (int i = 0; i < 4; i++) {
        const int next = (i + 1) % 4;
        const int patch = 1 + 4 * ring + i;
        const int curve_loop_tag = gmsh::model::geo::addCurveLoop(
            {circumferential_line_tag(i, ring + 1, layer), -radial_line_tag(next, ring, layer),
             -circumferential_line_tag(i, ring, layer), radial_line_tag(i, ring, layer)});
        plane_surface_tag(patch, layer) = gmsh::model::geo::addPlaneSurface({curve_loop_tag});
        gmsh::model::geo::mesh::setTransfiniteSurface(plane_surface_tag(patch, layer));
        gmsh::model::geo::mesh::setRecombine(2, plane_surface_tag(patch, layer));
        if (layer == 0 && ring == 1) {
          physical_group_tag[1].emplace_back(plane_surface_tag(patch, layer));
        } else if (layer == 2) {
          physical_group_tag[0].emplace_back(plane_surface_tag(patch, layer));
        }
      }
    }
  }
  for (int layer = 0; layer < 2; layer++) {
    for (int ring = layer == 0 ? 1 : 0; ring < 3; ring++) {
      for (int i = 0; i < 4; i++) {
        axial_line_tag(i, ring, layer) =
            gmsh::model::geo::addLine(point_tag(i, ring, layer), point_tag(i, ring, layer + 1));
        gmsh::model::geo::mesh::setTransfiniteCurve(axial_line_tag(i, ring, layer),
                                                    (layer == 0 ? kRotorAxialCells : kUpperAxialCells) + 1,
                                                    "Progression", layer == 0 ? 1.0 : kFarfieldGrowth);
      }
    }
    for (int ring = layer == 0 ? 1 : 0; ring < 3; ring++) {
      for (int i = 0; i < 4; i++) {
        const int next = (i + 1) % 4;
        const int curve_loop_tag = gmsh::model::geo::addCurveLoop(
            {circumferential_line_tag(i, ring, layer), axial_line_tag(next, ring, layer),
             -circumferential_line_tag(i, ring, layer + 1), -axial_line_tag(i, ring, layer)});
        circumferential_surface_tag(i, ring, layer) = gmsh::model::geo::addSurfaceFilling({curve_loop_tag});
        gmsh::model::geo::mesh::setTransfiniteSurface(circumferential_surface_tag(i, ring, layer));
        gmsh::model::geo::mesh::setRecombine(2, circumferential_surface_tag(i, ring, layer));
        if (ring == 2) {
          physical_group_tag[0].emplace_back(circumferential_surface_tag(i, ring, layer));
        }
      }
    }
    for (int ring = layer == 0 ? 1 : 0; ring < 2; ring++) {
      for (int i = 0; i < 4; i++) {
        const int curve_loop_tag =
            gmsh::model::geo::addCurveLoop({radial_line_tag(i, ring, layer), axial_line_tag(i, ring + 1, layer),
                                            -radial_line_tag(i, ring, layer + 1), -axial_line_tag(i, ring, layer)});
        radial_surface_tag(i, ring, layer) = gmsh::model::geo::addSurfaceFilling({curve_loop_tag});
        gmsh::model::geo::mesh::setTransfiniteSurface(radial_surface_tag(i, ring, layer));
        gmsh::model::geo::mesh::setRecombine(2, radial_surface_tag(i, ring, layer));
      }
    }
    if (layer == 1) {
      const int surface_loop_tag = gmsh::model::geo::addSurfaceLoop(
          {plane_surface_tag(0, layer), plane_surface_tag(0, layer + 1), circumferential_surface_tag(0, 0, layer),
           circumferential_surface_tag(1, 0, layer), circumferential_surface_tag(2, 0, layer),
           circumferential_surface_tag(3, 0, layer)});
      const int volume_tag = gmsh::model::geo::addVolume({surface_loop_tag});
      gmsh::model::geo::mesh::setTransfiniteVolume(volume_tag);
      physical_group_tag[3].emplace_back(volume_tag);
    }
    for (int ring = layer == 0 ? 1 : 0; ring < 2; ring++) {
      for (int i = 0; i < 4; i++) {
        const int next = (i + 1) % 4;
        const int patch = 1 + 4 * ring + i;
        const int surface_loop_tag = gmsh::model::geo::addSurfaceLoop(
            {plane_surface_tag(patch, layer), plane_surface_tag(patch, layer + 1),
             circumferential_surface_tag(i, ring, layer), circumferential_surface_tag(i, ring + 1, layer),
             radial_surface_tag(i, ring, layer), radial_surface_tag(next, ring, layer)});
        const int volume_tag = gmsh::model::geo::addVolume({surface_loop_tag});
        gmsh::model::geo::mesh::setTransfiniteVolume(volume_tag);
        physical_group_tag[3].emplace_back(volume_tag);
      }
    }
  }
  // Five top-cap patches followed by four cylindrical side patches form the sliding interface.
  for (int patch = 0; patch < 5; patch++) {
    interface_surface_tag.emplace_back(plane_surface_tag(patch, 1));
  }
  for (int i = 0; i < 4; i++) {
    interface_surface_tag.emplace_back(circumferential_surface_tag(i, 1, 0));
  }

  // Split the inward-facing blade skin at the two arm edges. Reusing these curves
  // at each span station joins blades, arms and shaft without intersecting solid skins.
  std::array<std::vector<Eigen::Vector2d>, 2> blade_profile;
  for (int side = 0; side < 2; side++) {
    const std::size_t side_index = static_cast<std::size_t>(side);
    blade_profile[side_index].reserve(static_cast<std::size_t>(kProfileSamples + 3));
    for (int i = 0; i <= kProfileSamples; i++) {
      const double x = 0.5 * (1.0 - std::cos(static_cast<double>(i) * kPi / static_cast<double>(kProfileSamples)));
      const double thickness =
          5.0 * kBladeThicknessRatio *
          (0.2969 * std::sqrt(x) - 0.1260 * x - 0.3516 * x * x + 0.2843 * x * x * x - 0.1015 * x * x * x * x);
      const double chord_x = kBladeChord * (x - kMountingChordFraction);
      const double chord_y = (side == 0 ? 1.0 : -1.0) * kBladeChord * thickness;
      blade_profile[side_index].emplace_back(chord_x * blade_pitch_cosine - chord_y * blade_pitch_sine,
                                             -kRotorRadius + chord_x * blade_pitch_sine + chord_y * blade_pitch_cosine);
    }
  }
  std::array<std::size_t, 2> arm_profile_index{};
  for (std::size_t edge = 0; edge < 2; edge++) {
    const double target_x = (edge == 0 ? -0.5 : 0.5) * kArmWidth;
    for (std::size_t i = 1; i < blade_profile[0].size(); i++) {
      if (blade_profile[0][i - 1].x() < target_x && blade_profile[0][i].x() >= target_x) {
        const double weight =
            (target_x - blade_profile[0][i - 1].x()) / (blade_profile[0][i].x() - blade_profile[0][i - 1].x());
        const double target_y = std::lerp(blade_profile[0][i - 1].y(), blade_profile[0][i].y(), weight);
        blade_profile[0].insert(blade_profile[0].begin() + static_cast<std::ptrdiff_t>(i),
                                Eigen::Vector2d{target_x, target_y});
        arm_profile_index[edge] = i;
        break;
      }
    }
  }
  // Geometry samples define the spline; these independent counts prescribe actual mesh edges.
  const std::array<std::size_t, 4> upper_profile_split{0, arm_profile_index[0], arm_profile_index[1],
                                                       blade_profile[0].size() - 1};
  Eigen::Vector<double, 3> upper_profile_length = Eigen::Vector<double, 3>::Zero();
  for (int segment = 0; segment < 3; segment++) {
    const std::size_t begin = upper_profile_split[static_cast<std::size_t>(segment)];
    const std::size_t end = upper_profile_split[static_cast<std::size_t>(segment + 1)];
    for (std::size_t i = begin; i < end; i++) {
      upper_profile_length(segment) += (blade_profile[0][i + 1] - blade_profile[0][i]).norm();
    }
  }
  const double upper_length = upper_profile_length.sum();
  Eigen::Vector<int, 3> upper_profile_cells;
  for (int segment = 0; segment < 2; segment++) {
    upper_profile_cells(segment) = static_cast<int>(
        std::round(static_cast<double>(kBladeSideCells) * upper_profile_length(segment) / upper_length));
  }
  upper_profile_cells(2) = kBladeSideCells - upper_profile_cells(0) - upper_profile_cells(1);

  for (int level = 0; level < 4; level++) {
    shaft_axis_point_tag(level) = gmsh::model::geo::addPoint(0.0, 0.0, shaft_height(level));
    for (int edge = 0; edge < 4; edge++) {
      shaft_point_tag(edge, level) = gmsh::model::geo::addPoint(
          shaft_point_coordinate(edge, 0), shaft_point_coordinate(edge, 1), shaft_height(level), kSupportSize);
    }
    for (int edge = 0; edge < 4; edge++) {
      const int next = (edge + 1) % 4;
      shaft_arc_tag(edge, level) = gmsh::model::geo::addCircleArc(
          shaft_point_tag(edge, level), shaft_axis_point_tag(level), shaft_point_tag(next, level));
    }
    if (level == 3) {
      const int shaft_top_loop_tag = gmsh::model::geo::addCurveLoop(
          {shaft_arc_tag(0, level), shaft_arc_tag(1, level), shaft_arc_tag(2, level), shaft_arc_tag(3, level)});
      physical_group_tag[2].emplace_back(gmsh::model::geo::addPlaneSurface({shaft_top_loop_tag}));
    }
    for (int blade = 0; blade < kBladeNumber; blade++) {
      const double direction = blade == 0 ? 1.0 : -1.0;
      std::array<std::vector<int>, 2> profile_point_tag;
      for (std::size_t side = 0; side < 2; side++) {
        profile_point_tag[side].reserve(blade_profile[side].size());
        for (std::size_t i = 0; i < blade_profile[side].size(); i++) {
          if (side == 1 && i == 0) {
            profile_point_tag[side].emplace_back(profile_point_tag[0][0]);
          } else {
            profile_point_tag[side].emplace_back(gmsh::model::geo::addPoint(direction * blade_profile[side][i].x(),
                                                                            direction * blade_profile[side][i].y(),
                                                                            blade_height(level), kBladeSize));
          }
        }
      }
      for (int segment = 0; segment < 3; segment++) {
        const std::size_t begin = upper_profile_split[static_cast<std::size_t>(segment)];
        const std::size_t end = upper_profile_split[static_cast<std::size_t>(segment + 1)];
        const std::vector<int> spline_point_tag(profile_point_tag[0].begin() + static_cast<std::ptrdiff_t>(begin),
                                                profile_point_tag[0].begin() + static_cast<std::ptrdiff_t>(end + 1));
        blade_section_point_tag(segment, level, blade) = profile_point_tag[0][begin];
        const int curve_tag = gmsh::model::geo::addSpline(spline_point_tag);
        gmsh::model::geo::mesh::setTransfiniteCurve(curve_tag, upper_profile_cells(segment) + 1, "Bump", kProfileBump);
        blade_section_curve_tag(segment, level, blade) = curve_tag;
      }
      blade_section_point_tag(3, level, blade) = profile_point_tag[0].back();
      blade_section_point_tag(4, level, blade) = profile_point_tag[1].back();
      blade_section_curve_tag(3, level, blade) =
          gmsh::model::geo::addLine(profile_point_tag[0].back(), profile_point_tag[1].back());
      const int lower_curve_tag = gmsh::model::geo::addSpline(profile_point_tag[1]);
      gmsh::model::geo::mesh::setTransfiniteCurve(lower_curve_tag, kBladeSideCells + 1, "Bump", kProfileBump);
      blade_section_curve_tag(4, level, blade) = -lower_curve_tag;
      if (level == 3) {
        const int blade_tip_loop_tag = gmsh::model::geo::addCurveLoop(
            {blade_section_curve_tag(0, level, blade), blade_section_curve_tag(1, level, blade),
             blade_section_curve_tag(2, level, blade), blade_section_curve_tag(3, level, blade),
             blade_section_curve_tag(4, level, blade)});
        physical_group_tag[2].emplace_back(gmsh::model::geo::addPlaneSurface({blade_tip_loop_tag}));
      }
    }
  }
  for (int level = 0; level < 3; level++) {
    for (int edge = 0; edge < 4; edge++) {
      shaft_axial_line_tag(edge, level) =
          gmsh::model::geo::addLine(shaft_point_tag(edge, level), shaft_point_tag(edge, level + 1));
    }
    for (int edge = 0; edge < 4; edge++) {
      // The two opposite shaft arcs connect to the arms in the middle span layer.
      if (level == 1 && edge % 2 == 0) {
        continue;
      }
      const int next = (edge + 1) % 4;
      const int curve_loop_tag =
          gmsh::model::geo::addCurveLoop({shaft_arc_tag(edge, level), shaft_axial_line_tag(next, level),
                                          -shaft_arc_tag(edge, level + 1), -shaft_axial_line_tag(edge, level)});
      physical_group_tag[2].emplace_back(gmsh::model::geo::addSurfaceFilling({curve_loop_tag}));
    }
    for (int blade = 0; blade < kBladeNumber; blade++) {
      for (int edge = 0; edge < 5; edge++) {
        blade_axial_line_tag(edge, level, blade) = gmsh::model::geo::addLine(
            blade_section_point_tag(edge, level, blade), blade_section_point_tag(edge, level + 1, blade));
      }
      for (int edge = 0; edge < 5; edge++) {
        if (level == 1 && edge == 1) {
          continue;
        }
        const int next = (edge + 1) % 5;
        const int curve_loop_tag = gmsh::model::geo::addCurveLoop(
            {blade_section_curve_tag(edge, level, blade), blade_axial_line_tag(next, level, blade),
             -blade_section_curve_tag(edge, level + 1, blade), -blade_axial_line_tag(edge, level, blade)});
        physical_group_tag[2].emplace_back(gmsh::model::geo::addSurfaceFilling({curve_loop_tag}));
      }
    }
  }
  for (int blade = 0; blade < kBladeNumber; blade++) {
    const int shaft_left = 2 * blade;
    for (int arm_level = 0; arm_level < 2; arm_level++) {
      const int level = 1 + arm_level;
      for (int edge = 0; edge < 2; edge++) {
        arm_line_tag(edge, arm_level, blade) = gmsh::model::geo::addLine(
            blade_section_point_tag(1 + edge, level, blade), shaft_point_tag(shaft_left + edge, level));
      }
      const int curve_loop_tag =
          gmsh::model::geo::addCurveLoop({blade_section_curve_tag(1, level, blade), arm_line_tag(1, arm_level, blade),
                                          -shaft_arc_tag(shaft_left, level), -arm_line_tag(0, arm_level, blade)});
      physical_group_tag[2].emplace_back(gmsh::model::geo::addPlaneSurface({curve_loop_tag}));
    }
    for (int edge = 0; edge < 2; edge++) {
      const int curve_loop_tag =
          gmsh::model::geo::addCurveLoop({arm_line_tag(edge, 0, blade), shaft_axial_line_tag(shaft_left + edge, 1),
                                          -arm_line_tag(edge, 1, blade), -blade_axial_line_tag(1 + edge, 1, blade)});
      physical_group_tag[2].emplace_back(gmsh::model::geo::addPlaneSurface({curve_loop_tag}));
    }
  }

  const int rotor_bottom_outer_loop_tag =
      gmsh::model::geo::addCurveLoop({circumferential_line_tag(0, 1, 0), circumferential_line_tag(1, 1, 0),
                                      circumferential_line_tag(2, 1, 0), circumferential_line_tag(3, 1, 0)});
  const int rotor_bottom_shaft_loop_tag = gmsh::model::geo::addCurveLoop(
      {shaft_arc_tag(0, 0), shaft_arc_tag(1, 0), shaft_arc_tag(2, 0), shaft_arc_tag(3, 0)});
  std::vector<int> rotor_bottom_loop_tag{rotor_bottom_outer_loop_tag, rotor_bottom_shaft_loop_tag};
  for (int blade = 0; blade < kBladeNumber; blade++) {
    rotor_bottom_loop_tag.emplace_back(
        gmsh::model::geo::addCurveLoop({blade_section_curve_tag(0, 0, blade), blade_section_curve_tag(1, 0, blade),
                                        blade_section_curve_tag(2, 0, blade), blade_section_curve_tag(3, 0, blade),
                                        blade_section_curve_tag(4, 0, blade)}));
  }
  const int rotor_bottom_surface_tag = gmsh::model::geo::addPlaneSurface(rotor_bottom_loop_tag);
  physical_group_tag[1].emplace_back(rotor_bottom_surface_tag);
  std::vector<int> rotor_surface_tag(interface_surface_tag.begin(), interface_surface_tag.end());
  rotor_surface_tag.emplace_back(rotor_bottom_surface_tag);
  rotor_surface_tag.insert(rotor_surface_tag.end(), physical_group_tag[2].begin(), physical_group_tag[2].end());
  const int rotor_surface_loop_tag = gmsh::model::geo::addSurfaceLoop(rotor_surface_tag);
  const int rotor_volume_tag = gmsh::model::geo::addVolume({rotor_surface_loop_tag});
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
    double mesh_size = kRotorBulkSize;
    const double shaft_distance =
        std::hypot(std::max(std::hypot(x, y) - kShaftRadius, 0.0), std::max(z - kShaftHeight, 0.0));
    mesh_size = std::min(mesh_size, kSupportSize + kSizeGrowth * shaft_distance);
    for (int blade = 0; blade < kBladeNumber; blade++) {
      const double direction = blade == 0 ? 1.0 : -1.0;
      const double pitched_x = direction * x;
      const double pitched_y = direction * y + kRotorRadius;
      const double local_x =
          pitched_x * blade_pitch_cosine + pitched_y * blade_pitch_sine + kMountingChordFraction * kBladeChord;
      const double local_y = -pitched_x * blade_pitch_sine + pitched_y * blade_pitch_cosine;
      const double chord_coordinate = std::clamp(local_x / kBladeChord, 0.0, 1.0);
      const double thickness =
          5.0 * kBladeThicknessRatio * kBladeChord *
          (0.2969 * std::sqrt(chord_coordinate) - 0.1260 * chord_coordinate - 0.3516 * std::pow(chord_coordinate, 2) +
           0.2843 * std::pow(chord_coordinate, 3) - 0.1015 * std::pow(chord_coordinate, 4));
      const double blade_distance =
          std::hypot(std::hypot(local_x - kBladeChord * chord_coordinate, std::max(std::abs(local_y) - thickness, 0.0)),
                     std::max(z - kBladeHeight / 2.0, 0.0));
      mesh_size = std::min(mesh_size, kBladeSize + kSizeGrowth * blade_distance);
      const double arm_distance = std::hypot(std::hypot(std::max(std::abs(x) - kArmWidth / 2.0, 0.0),
                                                        std::max({-direction * y - kRotorRadius, direction * y, 0.0})),
                                             std::max(std::abs(z - kArmSpan) - kArmThickness / 2.0, 0.0));
      mesh_size = std::min(mesh_size, kSupportSize + kSizeGrowth * arm_distance);
    }
    return mesh_size;
  });
  gmsh::option::setNumber("Mesh.MeshSizeExtendFromBoundary", 0);
  gmsh::option::setNumber("Mesh.MeshSizeFromPoints", 0);
  gmsh::option::setNumber("Mesh.MeshSizeFromCurvature", 0);
  gmsh::option::setNumber("Mesh.Algorithm", 6);
  gmsh::option::setNumber("Mesh.Algorithm3D", 1);
  gmsh::option::setNumber("Mesh.Optimize", 1);
  // Give the tetrahedral optimizer more scope near the thin trailing edges that limit the explicit step.
  gmsh::option::setNumber("Mesh.OptimizeThreshold", 0.95);
  // Fix the pyramid apices before tetrahedralization: Gmsh 4.15's automatic
  // hybrid smoother can flip neighboring tetrahedra and then hide the flips by
  // reorienting them. A prescribed skin lets the ordinary tetrahedron mesher
  // fill only the remaining fluid. The stationary hexahedra reuse the quad bases.
  // Keep saved quad/apex node tags stable across both 3D generation passes.
  gmsh::option::setNumber("Mesh.Renumber", 0);
  gmsh::model::mesh::generate(2);

  gmsh::vectorpair volume_entity;
  gmsh::model::getEntities(volume_entity, 3);
  gmsh::model::setVisibility(volume_entity, 0);
  gmsh::model::setVisibility({{3, rotor_volume_tag}}, 1);
  gmsh::option::setNumber("Mesh.MeshOnlyVisible", 1);
  gmsh::option::setNumber("Mesh.MeshOnlyEmpty", 1);
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
  std::vector<std::vector<std::size_t>> interface_quad_node_tag(interface_surface_tag.size());
  std::vector<std::size_t> pyramid_node_tag;
  for (std::size_t surface_index = 0; surface_index < interface_surface_tag.size(); surface_index++) {
    const int surface_tag = interface_surface_tag[surface_index];
    std::vector<std::size_t> element_tag;
    gmsh::model::mesh::getElementsByType(3, element_tag, interface_quad_node_tag[surface_index], surface_tag);
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
      const Eigen::Vector3d inward = surface_index < 5 ? Eigen::Vector3d{0.0, 0.0, -1.0}
                                                       : Eigen::Vector3d{-center.x(), -center.y(), 0.0}.normalized();
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
  // Replace the temporary triangular skin by its original CAD-surface quads;
  // the tetrahedron faces now join the prescribed pyramids inside the rotor.
  for (std::size_t surface_index = 0; surface_index < interface_surface_tag.size(); surface_index++) {
    const int surface_tag = interface_surface_tag[surface_index];
    gmsh::model::mesh::removeElements(2, surface_tag);
    gmsh::model::mesh::addElementsByType(surface_tag, 3, {}, interface_quad_node_tag[surface_index]);
  }
  gmsh::model::mesh::addElementsByType(rotor_volume_tag, 7, {}, pyramid_node_tag);
  gmsh::model::mesh::reclassifyNodes();
  gmsh::model::setVisibility(volume_entity, 1);
  gmsh::option::setNumber("Mesh.MeshOnlyVisible", 0);
  // Only the empty transfinite blocks remain; do not reoptimize the filled rotor.
  gmsh::option::setNumber("Mesh.Optimize", 0);
  gmsh::model::mesh::generate(3);
  gmsh::option::setNumber("Mesh.Optimize", 1);
  gmsh::option::setNumber("Mesh.MeshOnlyEmpty", 0);
  gmsh::option::setNumber("Mesh.Renumber", 1);
  gmsh::model::mesh::renumberNodes();
  gmsh::model::mesh::renumberElements();
  gmsh::model::mesh::removeSizeCallback();
  gmsh::model::mesh::setOrder(SimulationControl::kPolynomialOrder);
  if constexpr (SimulationControl::kPolynomialOrder > 1) {
    gmsh::model::mesh::optimize("HighOrder");
  }
  gmsh::write(mesh_file_path);
}
