/**
 * @file vortex_2d_ceuler.cpp
 * @brief The source file for SubrosaDG example vortex_2d_ceuler.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2026-09-04
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

inline const std::string kExampleName{"vortex_2d_ceuler"};

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory / "build/out" / kExampleName};

using SimulationControl = SubrosaDG::SimulationControl<
    SubrosaDG::SolveControl<SubrosaDG::DimensionEnum::D2, SubrosaDG::PolynomialOrderEnum::P1,
                            SubrosaDG::BoundaryTimeEnum::TimeVarying, SubrosaDG::SourceTermEnum::None>,
    SubrosaDG::NumericalControl<SubrosaDG::MeshModelEnum::TriangleQuadrangle, SubrosaDG::InitialConditionEnum::Function,
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
      coordinate - Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>{kVortexMaximumVelocity * time, 0.0_r};
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
        kVortexReferenceTemperature *
            (1.0_r - (kSpecificHeatRatio - 1.0_r) * std::pow((kVortexStrength * kMachNumber), 2.0_r) / 2.0_r *
                         std::exp(1.0_r - radius_squared / (kVortexRadius * kVortexRadius)))};
  }
}

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::System<SimulationControl> system;
  system.setMesh(kExampleDirectory / std::format("{}.msh", kExampleName), generateMesh);
  // omega = 0, 1, 5, 10, 15, 20
  system.setRotation(Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>{0.0_r, 0.0_r},
                     Eigen::Vector<SubrosaDG::Real, 3>{0.0_r, 0.0_r, 1.0_r});
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::RiemannFarfield>(1);
  system.template addVolumeCondition<SubrosaDG::InteriorConditionEnum::Static>(2);
  system.template addVolumeCondition<SubrosaDG::InteriorConditionEnum::Rotate>(3);
  system.setTimeIntegration(1.0_r, {0, 20000}, 1e-4_r);
  system.setViewConfig(kExampleDirectory, kExampleName, 20000);
  system.addViewVariable({SubrosaDG::ViewVariableEnum::Density, SubrosaDG::ViewVariableEnum::Velocity,
                          SubrosaDG::ViewVariableEnum::Pressure});
  system.synchronize();
  system.solve();
  system.view();
  return EXIT_SUCCESS;
}

// N = 10, 20, 40, 80
inline constexpr int kFarfieldCellsPerSide = 40;
inline constexpr double kFarfieldHalfWidth = 1.5;
inline constexpr double kInterfaceRadius = 0.6;
inline constexpr double kTargetMeshSize = 2.0 * kFarfieldHalfWidth / kFarfieldCellsPerSide;
inline constexpr int kInterfaceCellsPerQuadrant =
    static_cast<int>(SubrosaDG::kPi * kInterfaceRadius / (2.0 * kTargetMeshSize) + 0.5);

void generateMesh(const std::filesystem::path& mesh_file_path) {
  Eigen::Matrix<double, 4, 3, Eigen::RowMajor> farfield_point_coordinate;
  Eigen::Matrix<double, 4, 3, Eigen::RowMajor> interface_point_coordinate;
  // clang-format off
  farfield_point_coordinate << -kFarfieldHalfWidth, -kFarfieldHalfWidth, 0.0,
                                kFarfieldHalfWidth, -kFarfieldHalfWidth, 0.0,
                                kFarfieldHalfWidth,  kFarfieldHalfWidth, 0.0,
                               -kFarfieldHalfWidth,  kFarfieldHalfWidth, 0.0;
  interface_point_coordinate << -kInterfaceRadius,  0.0, 0.0,
                                 0.0, -kInterfaceRadius, 0.0,
                                 kInterfaceRadius,  0.0, 0.0,
                                 0.0,  kInterfaceRadius, 0.0;
  // clang-format on
  Eigen::Array<int, 4, 2, Eigen::RowMajor> point_tag;
  Eigen::Array<int, 4, 2, Eigen::RowMajor> line_tag;
  Eigen::Array<int, 2, 1> curve_loop_tag;
  Eigen::Array<int, 2, 1> plane_surface_tag;
  std::array<std::vector<int>, 3> physical_group_tag;
  gmsh::model::add("vortex_2d");
  const int center_point_tag = gmsh::model::geo::addPoint(0.0, 0.0, 0.0, kTargetMeshSize);
  for (int i = 0; i < 4; i++) {
    point_tag(i, 0) = gmsh::model::geo::addPoint(farfield_point_coordinate(i, 0), farfield_point_coordinate(i, 1),
                                                 farfield_point_coordinate(i, 2), kTargetMeshSize);
    point_tag(i, 1) = gmsh::model::geo::addPoint(interface_point_coordinate(i, 0), interface_point_coordinate(i, 1),
                                                 interface_point_coordinate(i, 2), kTargetMeshSize);
  }
  for (int i = 0; i < 4; i++) {
    line_tag(i, 0) = gmsh::model::geo::addLine(point_tag(i, 0), point_tag((i + 1) % 4, 0));
    line_tag(i, 1) = gmsh::model::geo::addCircleArc(point_tag(i, 1), center_point_tag, point_tag((i + 1) % 4, 1));
  }
  for (int i = 0; i < 2; i++) {
    curve_loop_tag(i, 0) =
        gmsh::model::geo::addCurveLoop({line_tag(0, i), line_tag(1, i), line_tag(2, i), line_tag(3, i)});
  }
  plane_surface_tag(0, 0) = gmsh::model::geo::addPlaneSurface({curve_loop_tag(0, 0), -curve_loop_tag(1, 0)});
  plane_surface_tag(1, 0) = gmsh::model::geo::addPlaneSurface({curve_loop_tag(1, 0)});
  for (int i = 0; i < 4; i++) {
    gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(i, 0), kFarfieldCellsPerSide + 1);
    gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(i, 1), kInterfaceCellsPerQuadrant + 1);
  }
  gmsh::model::geo::mesh::setRecombine(2, plane_surface_tag(1, 0));
  gmsh::model::geo::synchronize();
  for (int i = 0; i < 4; i++) {
    physical_group_tag[0].emplace_back(line_tag(i, 0));
  }
  physical_group_tag[1].emplace_back(plane_surface_tag(0, 0));
  physical_group_tag[2].emplace_back(plane_surface_tag(1, 0));
  gmsh::model::addPhysicalGroup(1, physical_group_tag[0], 1, "bc-1");
  gmsh::model::addPhysicalGroup(2, physical_group_tag[1], 2, "vc-1");
  gmsh::model::addPhysicalGroup(2, physical_group_tag[2], 3, "rc-1");
  gmsh::option::setNumber("Mesh.MeshSizeMin", kTargetMeshSize);
  gmsh::option::setNumber("Mesh.MeshSizeMax", kTargetMeshSize);
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::setOrder(SimulationControl::kPolynomialOrder);
  gmsh::model::mesh::optimize("HighOrder");
  gmsh::write(mesh_file_path);
}
