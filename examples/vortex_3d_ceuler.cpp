/**
 * @file vortex_3d_ceuler.cpp
 * @brief The source file for SubrosaDG example vortex_3d_ceuler.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2026-10-04
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
};

}  // namespace SubrosaDG

#include "SubrosaDG.cpp"

inline const std::string kExampleName{"vortex_3d_ceuler"};

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory / "build/out" / kExampleName};

using SimulationControl = SubrosaDG::SimulationControl<
    SubrosaDG::SolveControl<SubrosaDG::DimensionEnum::D3, SubrosaDG::PolynomialOrderEnum::P1,
                            SubrosaDG::BoundaryTimeEnum::TimeVarying, SubrosaDG::SourceTermEnum::None>,
    SubrosaDG::NumericalControl<SubrosaDG::MeshModelEnum::Hexahedron, SubrosaDG::InitialConditionEnum::Function,
                                SubrosaDG::TimeIntegrationEnum::SSPRK3>,
    SubrosaDG::CompressibleEulerVariable<SubrosaDG::ThermodynamicModelEnum::Constant,
                                         SubrosaDG::EquationOfStateEnum::IdealGas,
                                         SubrosaDG::ConvectiveFluxEnum::HLLC>>;

inline constexpr SubrosaDG::Real kSpecificHeatRatio = SubrosaDG::PhysicalModelData::kSpecificHeatRatio;
inline constexpr SubrosaDG::Real kMachNumber = 0.3_r;
inline constexpr SubrosaDG::Real kVortexRadius = 0.15_r;
inline constexpr SubrosaDG::Real kVortexStrength = 1.0_r;
inline constexpr SubrosaDG::Real kVortexMaximumVelocity = 0.3_r;
inline constexpr SubrosaDG::Real kVortexReferenceDensity = 1.4_r;
inline constexpr SubrosaDG::Real kVortexReferenceTemperature = 1.0_r;

template <typename SimulationControl>
inline void SubrosaDG::InitialCondition<SimulationControl>::computePrimitiveFromCoordinate(
    const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate,
    Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>& initial_primitive_variable) {
  const SubrosaDG::Real radius_squared = coordinate.x() * coordinate.x() + coordinate.y() * coordinate.y();
  initial_primitive_variable = {
      kVortexReferenceDensity *
          std::pow(1.0_r - (kSpecificHeatRatio - 1.0_r) * std::pow((kVortexStrength * kMachNumber), 2.0_r) / 2.0_r *
                               std::exp(1.0_r - radius_squared / (kVortexRadius * kVortexRadius)),
                   1.0_r / (kSpecificHeatRatio - 1.0_r)),
      kVortexMaximumVelocity * (1.0_r - kVortexStrength * coordinate.y() / kVortexRadius *
                                            std::exp(0.5_r - 0.5_r * radius_squared / (kVortexRadius * kVortexRadius))),
      kVortexMaximumVelocity * (kVortexStrength * coordinate.x() / kVortexRadius *
                                std::exp(0.5_r - 0.5_r * radius_squared / (kVortexRadius * kVortexRadius))),
      0.0_r,
      kVortexReferenceTemperature *
          (1.0_r - (kSpecificHeatRatio - 1.0_r) * std::pow((kVortexStrength * kMachNumber), 2.0_r) / 2.0_r *
                       std::exp(1.0_r - radius_squared / (kVortexRadius * kVortexRadius)))};
}

template <typename SimulationControl>
inline void SubrosaDG::BoundaryCondition<SimulationControl>::computePrimitiveFromCoordinate(
    const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate,
    Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>& boundary_primitive_variable,
    const Real time, const SubrosaDG::Isize gmsh_physical_index) {
  const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension> relative_coordinate =
      coordinate -
      Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>{kVortexMaximumVelocity * time, 0.0_r, 0.0_r};
  const SubrosaDG::Real radius_squared =
      relative_coordinate.x() * relative_coordinate.x() + relative_coordinate.y() * relative_coordinate.y();
  if (gmsh_physical_index == 1) {
    boundary_primitive_variable = {
        kVortexReferenceDensity *
            std::pow(1.0_r - (kSpecificHeatRatio - 1.0_r) * std::pow((kVortexStrength * kMachNumber), 2.0_r) / 2.0_r *
                                 std::exp(1.0_r - radius_squared / (kVortexRadius * kVortexRadius)),
                     1.0_r / (kSpecificHeatRatio - 1.0_r)),
        kVortexMaximumVelocity *
            (1.0_r - kVortexStrength * relative_coordinate.y() / kVortexRadius *
                         std::exp(0.5_r - 0.5_r * radius_squared / (kVortexRadius * kVortexRadius))),
        kVortexMaximumVelocity * (kVortexStrength * relative_coordinate.x() / kVortexRadius *
                                  std::exp(0.5_r - 0.5_r * radius_squared / (kVortexRadius * kVortexRadius))),
        0.0_r,
        kVortexReferenceTemperature *
            (1.0_r - (kSpecificHeatRatio - 1.0_r) * std::pow((kVortexStrength * kMachNumber), 2.0_r) / 2.0_r *
                         std::exp(1.0_r - radius_squared / (kVortexRadius * kVortexRadius)))};
  }
}

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::System<SimulationControl> system{SubrosaDG::CommandLineEnum::Close};
  system.setMesh(kExampleDirectory / std::format("{}.msh", kExampleName), generateMesh);
  // omega = 0, 1, 5, 10, 15, 20
  system.setRotation(Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>{0.0_r, 0.0_r, 0.0_r},
                     Eigen::Vector<SubrosaDG::Real, 3>{0.0_r, 0.0_r, 1.0_r});
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::RiemannFarfield>(1);
  system.template addVolumeCondition<SubrosaDG::InteriorConditionEnum::Static>(2);
  system.template addVolumeCondition<SubrosaDG::InteriorConditionEnum::Rotate>(3);
  system.setTimeIntegration(0.5_r, {0, 2000}, 1e-3_r);
  system.setViewConfig(kExampleDirectory, kExampleName, 2000);
  system.addViewVariable({SubrosaDG::ViewVariableEnum::Density, SubrosaDG::ViewVariableEnum::Velocity,
                          SubrosaDG::ViewVariableEnum::Pressure});
  system.synchronize();
  system.solve();
  system.view();
  return EXIT_SUCCESS;
}

// The short rotating cylinder is embedded in a longer static farfield cylinder.
inline constexpr double kFarfieldRadius = 1.5;
inline constexpr double kFarfieldHalfLength = 1.5;
inline constexpr double kInterfaceRadius = 0.6;
inline constexpr double kInterfaceHalfLength = 0.3;
inline constexpr double kCoreHalfWidth = 0.15;
inline constexpr int kCircumferentialCellsPerQuadrant = 8;
inline constexpr int kInnerRadialCells = 4;
inline constexpr int kOuterRadialCells = 8;
inline constexpr int kStaticAxialCells = 8;
inline constexpr int kRotatingAxialCells = 4;
inline constexpr double kTargetMeshSize = 2.0 * kCoreHalfWidth / kCircumferentialCellsPerQuadrant;

void generateMesh(const std::filesystem::path& mesh_file_path) {
  Eigen::Matrix<double, 4, 2, Eigen::RowMajor> point_direction;
  // clang-format off
  point_direction << -1.0, -1.0,
                      1.0, -1.0,
                      1.0,  1.0,
                     -1.0,  1.0;
  // clang-format on
  const Eigen::Vector<double, 3> ring_coordinate{kCoreHalfWidth, kInterfaceRadius / std::numbers::sqrt2_v<double>,
                                                 kFarfieldRadius / std::numbers::sqrt2_v<double>};
  const Eigen::Vector<double, 4> axial_coordinate{-kFarfieldHalfLength, -kInterfaceHalfLength, kInterfaceHalfLength,
                                                  kFarfieldHalfLength};
  const Eigen::Vector<int, 3> axial_cells{kStaticAxialCells, kRotatingAxialCells, kStaticAxialCells};
  Eigen::Array<int, 4, 3, Eigen::RowMajor> point_tag;
  Eigen::Array<int, 4, 3, Eigen::RowMajor> circumferential_line_tag;
  Eigen::Array<int, 4, 2, Eigen::RowMajor> radial_line_tag;
  Eigen::Array<int, 9, 1> curve_loop_tag;
  Eigen::Array<int, 9, 1> plane_surface_tag;
  std::array<std::vector<int>, 3> physical_group_tag;
  gmsh::vectorpair volume_tag;
  gmsh::model::add("vortex_3d");
  const int center_point_tag = gmsh::model::geo::addPoint(0.0, 0.0, axial_coordinate(0), kTargetMeshSize);
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 3; j++) {
      point_tag(i, j) =
          gmsh::model::geo::addPoint(point_direction(i, 0) * ring_coordinate(j),
                                     point_direction(i, 1) * ring_coordinate(j), axial_coordinate(0), kTargetMeshSize);
    }
  }
  for (int i = 0; i < 4; i++) {
    circumferential_line_tag(i, 0) = gmsh::model::geo::addLine(point_tag(i, 0), point_tag((i + 1) % 4, 0));
    for (int j = 1; j < 3; j++) {
      circumferential_line_tag(i, j) =
          gmsh::model::geo::addCircleArc(point_tag(i, j), center_point_tag, point_tag((i + 1) % 4, j));
    }
    for (int j = 0; j < 3; j++) {
      gmsh::model::geo::mesh::setTransfiniteCurve(circumferential_line_tag(i, j), kCircumferentialCellsPerQuadrant + 1);
    }
    for (int j = 0; j < 2; j++) {
      radial_line_tag(i, j) = gmsh::model::geo::addLine(point_tag(i, j), point_tag(i, j + 1));
      gmsh::model::geo::mesh::setTransfiniteCurve(radial_line_tag(i, j),
                                                  (j == 0 ? kInnerRadialCells : kOuterRadialCells) + 1);
    }
  }
  // A central square avoids degenerate hexahedra on the rotation axis.
  curve_loop_tag(0) = gmsh::model::geo::addCurveLoop({circumferential_line_tag(0, 0), circumferential_line_tag(1, 0),
                                                      circumferential_line_tag(2, 0), circumferential_line_tag(3, 0)});
  for (int j = 0; j < 2; j++) {
    for (int i = 0; i < 4; i++) {
      curve_loop_tag(1 + 4 * j + i) =
          gmsh::model::geo::addCurveLoop({circumferential_line_tag(i, j + 1), -radial_line_tag((i + 1) % 4, j),
                                          -circumferential_line_tag(i, j), radial_line_tag(i, j)});
    }
  }
  for (int i = 0; i < 9; i++) {
    plane_surface_tag(i) = gmsh::model::geo::addPlaneSurface({curve_loop_tag(i)});
    gmsh::model::geo::mesh::setTransfiniteSurface(plane_surface_tag(i));
    gmsh::model::geo::mesh::setRecombine(2, plane_surface_tag(i));
  }
  // Shared initial faces let the mesh reader identify both cylindrical and endcap sliding interfaces.
  for (int layer = 0; layer < 3; layer++) {
    gmsh::vectorpair surface_tag;
    gmsh::vectorpair extruded_tag;
    for (int i = 0; i < 9; i++) {
      surface_tag.emplace_back(2, plane_surface_tag(i));
    }
    gmsh::model::geo::extrude(surface_tag, 0.0, 0.0, axial_coordinate(layer + 1) - axial_coordinate(layer),
                              extruded_tag, {axial_cells(layer)}, {}, true);
    for (int i = 0; i < 9; i++) {
      // Each four-sided surface produces its top face, volume and four lateral faces.
      plane_surface_tag(i) = extruded_tag[static_cast<std::size_t>(6 * i)].second;
      const int tag = extruded_tag[static_cast<std::size_t>(6 * i + 1)].second;
      volume_tag.emplace_back(3, tag);
      physical_group_tag[layer == 1 && i < 5 ? 2 : 1].emplace_back(tag);
    }
  }
  gmsh::model::geo::synchronize();
  gmsh::vectorpair boundary_surface_tag;
  gmsh::model::getBoundary(volume_tag, boundary_surface_tag, true, false, false);
  for (const auto& [dimension, tag] : boundary_surface_tag) {
    static_cast<void>(dimension);
    physical_group_tag[0].emplace_back(tag);
  }
  gmsh::model::addPhysicalGroup(2, physical_group_tag[0], 1, "bc-1");
  gmsh::model::addPhysicalGroup(3, physical_group_tag[1], 2, "vc-1");
  gmsh::model::addPhysicalGroup(3, physical_group_tag[2], 3, "rc-1");
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::setOrder(SimulationControl::kPolynomialOrder);
  gmsh::model::mesh::optimize("HighOrder");
  gmsh::write(mesh_file_path);
}
