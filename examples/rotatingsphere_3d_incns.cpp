/**
 * @file rotatingsphere_3d_incns.cpp
 * @brief The main file of SubrosaDG rotatingsphere_3d_incns.
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
  static constexpr double kReferenceSoundSpeed = 10.0;
  static constexpr double kReferenceDensity = 1.0;

  // ThermodynamicModel
  static constexpr double kSpecificHeatConstantPressure = 1.0;
  static constexpr double kSpecificHeatConstantVolume = 1.0;

  // TransportModel
  static constexpr double kReynoldsNumber = 300.0;
  static constexpr double kDynamicViscosity = 1.0 * 1.0 * 1.0 / kReynoldsNumber;
  static constexpr double kPrandtlNumber = 0.71;
  static constexpr double kThermalConductivity = kSpecificHeatConstantPressure * kDynamicViscosity / kPrandtlNumber;
};

}  // namespace SubrosaDG

#include "SubrosaDG.cpp"

inline const std::string kExampleName{"rotatingsphere_3d_incns"};

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory / "build/out" / kExampleName};

inline constexpr double kSphereRadius = 0.5;
inline constexpr SubrosaDG::Real kInflowVelocity = 1.0_r;
// Kim and Choi (2002) define the rotation rate as the equatorial wall speed divided by the inflow speed.
inline constexpr SubrosaDG::Real kRotationRate = 0.6_r;
inline constexpr SubrosaDG::Real kAngularVelocity =
    kRotationRate * kInflowVelocity / static_cast<SubrosaDG::Real>(kSphereRadius);

using SimulationControl = SubrosaDG::SimulationControl<
    SubrosaDG::SolveControl<SubrosaDG::DimensionEnum::D3, SubrosaDG::PolynomialOrderEnum::P3,
                            SubrosaDG::BoundaryTimeEnum::Steady, SubrosaDG::SourceTermEnum::None>,
    SubrosaDG::NumericalControl<SubrosaDG::MeshModelEnum::Hexahedron, SubrosaDG::InitialConditionEnum::Function,
                                SubrosaDG::TimeIntegrationEnum::SSPRK3>,
    SubrosaDG::IncompressibleNSVariable<SubrosaDG::ThermodynamicModelEnum::Constant,
                                        SubrosaDG::EquationOfStateEnum::WeakCompressibleFluid,
                                        SubrosaDG::TransportModelEnum::Constant,
                                        SubrosaDG::ConvectiveFluxEnum::LaxFriedrichs, SubrosaDG::ViscousFluxEnum::BR2>>;

template <typename SimulationControl>
inline void SubrosaDG::InitialCondition<SimulationControl>::computePrimitiveFromCoordinate(
    [[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate,
    Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>& initial_primitive_variable) {
  initial_primitive_variable = {1.0_r, 0.0_r, 0.0_r, kInflowVelocity, 1.0_r};
}

template <typename SimulationControl>
inline void SubrosaDG::BoundaryCondition<SimulationControl>::computePrimitiveFromCoordinate(
    [[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate,
    Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>& boundary_primitive_variable,
    const SubrosaDG::Isize gmsh_physical_index) {
  if (gmsh_physical_index == 1) {
    boundary_primitive_variable = {1.0_r, 0.0_r, 0.0_r, kInflowVelocity, 1.0_r};
  } else if (gmsh_physical_index == 2) {
    boundary_primitive_variable = {1.0_r, 0.0_r, 0.0_r, 0.0_r, 1.0_r};
  }
}

int main(int argc, char* argv[]) {
  SubrosaDG::System<SimulationControl> system;
  system.setMesh(kExampleDirectory / std::format("{}.msh", kExampleName), generateMesh);
  system.setRotation(Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>{0.0_r, 0.0_r, 0.0_r},
                     Eigen::Vector<SubrosaDG::Real, 3>{0.0_r, 0.0_r, kAngularVelocity});
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::RiemannFarfield>(1);
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticNonSlipWall>(2);
  system.template addVolumeCondition<SubrosaDG::InteriorConditionEnum::Static>(3);
  system.template addVolumeCondition<SubrosaDG::InteriorConditionEnum::Rotate>(4);
  system.setTimeIntegration(0.6_r);
  system.setViewConfig(kExampleDirectory, kExampleName);
  system.addViewVariable({SubrosaDG::ViewVariableEnum::Density, SubrosaDG::ViewVariableEnum::Velocity,
                          SubrosaDG::ViewVariableEnum::Pressure, SubrosaDG::ViewVariableEnum::Temperature,
                          SubrosaDG::ViewVariableEnum::MachNumber, SubrosaDG::ViewVariableEnum::HeatFlux});
  system.synchronize();
  system.solve();
  system.view();
  return EXIT_SUCCESS;
}

void generateMesh(const std::filesystem::path& mesh_file_path) {
  constexpr double kInterfaceRadius = 1.2;
  constexpr double kInterfaceHalfLength = 0.75;
  constexpr double kFarfieldHalfWidth = 5.0;
  // The square encloses the sphere and its corners stay inside the sliding cylinder.
  constexpr double kCoreHalfWidth = 0.6;
  constexpr int kCircumferentialCellsPerQuadrant = 11;
  constexpr int kInnerRadialCells = 4;
  constexpr int kOuterRadialCells = 11;
  constexpr int kAxialCells = 11;
  constexpr int kSphereRadialCells = 7;
  constexpr double kFarfieldProgression = 1.3;
  constexpr double kSphereRadialProgression = 1.2;
  Eigen::Matrix<int, 4, 2, Eigen::RowMajor> point_direction;
  // clang-format off
  point_direction << -1, -1,
                      1, -1,
                      1,  1,
                     -1,  1;
  // clang-format on
  const Eigen::Vector<double, 3> ring_coordinate{kCoreHalfWidth, kInterfaceRadius / std::numbers::sqrt2_v<double>,
                                                 kFarfieldHalfWidth};
  const Eigen::Vector<double, 4> axial_coordinate{-kFarfieldHalfWidth, -kInterfaceHalfLength, kInterfaceHalfLength,
                                                  kFarfieldHalfWidth};
  const Eigen::Vector<int, 2> radial_cells{kInnerRadialCells, kOuterRadialCells};
  Eigen::Array<int, 4, 1> axis_point_tag;
  Eigen::Tensor<int, 3> point_tag(4, 3, 4);
  Eigen::Tensor<int, 3> circumferential_line_tag(4, 3, 4);
  Eigen::Tensor<int, 3> radial_line_tag(4, 2, 4);
  Eigen::Tensor<int, 3> axial_line_tag(4, 3, 3);
  Eigen::Array<int, 9, 4> plane_surface_tag;
  Eigen::Tensor<int, 3> circumferential_surface_tag(4, 3, 3);
  Eigen::Tensor<int, 3> radial_surface_tag(4, 2, 3);
  Eigen::Array<int, 4, 2> sphere_point_tag;
  Eigen::Array<int, 4, 2> sphere_circumferential_line_tag;
  Eigen::Array<int, 4, 1> sphere_axial_line_tag;
  Eigen::Array<int, 4, 2> sphere_connection_line_tag;
  Eigen::Array<int, 4, 2> sphere_connection_surface_tag;
  Eigen::Array<int, 4, 1> sphere_axial_connection_surface_tag;
  Eigen::Array<int, 6, 1> sphere_surface_tag;
  std::array<std::vector<int>, 4> physical_group_tag;
  gmsh::model::add("rotatingsphere_3d");
  const int center_point_tag = gmsh::model::geo::addPoint(0.0, 0.0, 0.0);
  for (int layer = 0; layer < 4; layer++) {
    axis_point_tag(layer) = gmsh::model::geo::addPoint(0.0, 0.0, axial_coordinate(layer));
    for (int ring = 0; ring < 3; ring++) {
      for (int i = 0; i < 4; i++) {
        point_tag(i, ring, layer) =
            gmsh::model::geo::addPoint(point_direction(i, 0) * ring_coordinate(ring),
                                       point_direction(i, 1) * ring_coordinate(ring), axial_coordinate(layer));
      }
    }
  }
  // Each endcap has a Cartesian square and four surrounding quadrangular ring patches.
  for (int layer = 0; layer < 4; layer++) {
    for (int i = 0; i < 4; i++) {
      const int next = (i + 1) % 4;
      for (int ring = 0; ring < 3; ring++) {
        if (ring == 1) {
          circumferential_line_tag(i, ring, layer) = gmsh::model::geo::addCircleArc(
              point_tag(i, ring, layer), axis_point_tag(layer), point_tag(next, ring, layer));
        } else {
          circumferential_line_tag(i, ring, layer) =
              gmsh::model::geo::addLine(point_tag(i, ring, layer), point_tag(next, ring, layer));
        }
        gmsh::model::geo::mesh::setTransfiniteCurve(circumferential_line_tag(i, ring, layer),
                                                    kCircumferentialCellsPerQuadrant + 1);
      }
      for (int ring = 0; ring < 2; ring++) {
        radial_line_tag(i, ring, layer) =
            gmsh::model::geo::addLine(point_tag(i, ring, layer), point_tag(i, ring + 1, layer));
        gmsh::model::geo::mesh::setTransfiniteCurve(radial_line_tag(i, ring, layer), radial_cells(ring) + 1,
                                                    "Progression", ring == 0 ? 1.0 : kFarfieldProgression);
      }
    }
    const int center_curve_loop_tag =
        gmsh::model::geo::addCurveLoop({circumferential_line_tag(0, 0, layer), circumferential_line_tag(1, 0, layer),
                                        circumferential_line_tag(2, 0, layer), circumferential_line_tag(3, 0, layer)});
    plane_surface_tag(0, layer) = gmsh::model::geo::addPlaneSurface({center_curve_loop_tag});
    for (int ring = 0; ring < 2; ring++) {
      for (int i = 0; i < 4; i++) {
        const int next = (i + 1) % 4;
        const int curve_loop_tag = gmsh::model::geo::addCurveLoop(
            {circumferential_line_tag(i, ring + 1, layer), -radial_line_tag(next, ring, layer),
             -circumferential_line_tag(i, ring, layer), radial_line_tag(i, ring, layer)});
        plane_surface_tag(1 + 4 * ring + i, layer) = gmsh::model::geo::addPlaneSurface({curve_loop_tag});
      }
    }
  }
  double axial_progression = kFarfieldProgression;
  for (int layer = 0; layer < 3; layer++) {
    for (int ring = 0; ring < 3; ring++) {
      for (int i = 0; i < 4; i++) {
        axial_line_tag(i, ring, layer) =
            gmsh::model::geo::addLine(point_tag(i, ring, layer), point_tag(i, ring, layer + 1));
        if (layer == 0) {
          axial_progression = 1.0 / kFarfieldProgression;
        } else if (layer == 1) {
          axial_progression = 1.0;
        }
        gmsh::model::geo::mesh::setTransfiniteCurve(axial_line_tag(i, ring, layer), kAxialCells + 1, "Progression",
                                                    axial_progression);
      }
    }
    for (int ring = 0; ring < 3; ring++) {
      for (int i = 0; i < 4; i++) {
        const int next = (i + 1) % 4;
        const int curve_loop_tag = gmsh::model::geo::addCurveLoop(
            {circumferential_line_tag(i, ring, layer), axial_line_tag(next, ring, layer),
             -circumferential_line_tag(i, ring, layer + 1), -axial_line_tag(i, ring, layer)});
        circumferential_surface_tag(i, ring, layer) = gmsh::model::geo::addSurfaceFilling({curve_loop_tag});
      }
    }
    for (int ring = 0; ring < 2; ring++) {
      for (int i = 0; i < 4; i++) {
        const int curve_loop_tag =
            gmsh::model::geo::addCurveLoop({radial_line_tag(i, ring, layer), axial_line_tag(i, ring + 1, layer),
                                            -radial_line_tag(i, ring, layer + 1), -axial_line_tag(i, ring, layer)});
        radial_surface_tag(i, ring, layer) = gmsh::model::geo::addPlaneSurface({curve_loop_tag});
      }
    }
    if (layer != 1) {
      const int surface_loop_tag = gmsh::model::geo::addSurfaceLoop(
          {plane_surface_tag(0, layer), plane_surface_tag(0, layer + 1), circumferential_surface_tag(0, 0, layer),
           circumferential_surface_tag(1, 0, layer), circumferential_surface_tag(2, 0, layer),
           circumferential_surface_tag(3, 0, layer)});
      physical_group_tag[2].emplace_back(gmsh::model::geo::addVolume({surface_loop_tag}));
    }
    for (int ring = 0; ring < 2; ring++) {
      for (int i = 0; i < 4; i++) {
        const int next = (i + 1) % 4;
        const int surface_loop_tag = gmsh::model::geo::addSurfaceLoop(
            {plane_surface_tag(1 + 4 * ring + i, layer), plane_surface_tag(1 + 4 * ring + i, layer + 1),
             circumferential_surface_tag(i, ring, layer), circumferential_surface_tag(i, ring + 1, layer),
             radial_surface_tag(i, ring, layer), radial_surface_tag(next, ring, layer)});
        physical_group_tag[layer == 1 && ring == 0 ? 3 : 2].emplace_back(
            gmsh::model::geo::addVolume({surface_loop_tag}));
      }
    }
  }
  // Replace the middle square prism with six nondegenerate blocks surrounding the sphere.
  const double sphere_coordinate = kSphereRadius / std::numbers::sqrt3_v<double>;
  for (int layer = 0; layer < 2; layer++) {
    for (int i = 0; i < 4; i++) {
      sphere_point_tag(i, layer) =
          gmsh::model::geo::addPoint(point_direction(i, 0) * sphere_coordinate,
                                     point_direction(i, 1) * sphere_coordinate, (2 * layer - 1) * sphere_coordinate);
    }
    for (int i = 0; i < 4; i++) {
      const int next = (i + 1) % 4;
      sphere_circumferential_line_tag(i, layer) =
          gmsh::model::geo::addCircleArc(sphere_point_tag(i, layer), center_point_tag, sphere_point_tag(next, layer));
      gmsh::model::geo::mesh::setTransfiniteCurve(sphere_circumferential_line_tag(i, layer),
                                                  kCircumferentialCellsPerQuadrant + 1);
      sphere_connection_line_tag(i, layer) =
          gmsh::model::geo::addLine(sphere_point_tag(i, layer), point_tag(i, 0, layer + 1));
      gmsh::model::geo::mesh::setTransfiniteCurve(sphere_connection_line_tag(i, layer), kSphereRadialCells + 1,
                                                  "Progression", kSphereRadialProgression);
    }
    const int curve_loop_tag = gmsh::model::geo::addCurveLoop(
        {sphere_circumferential_line_tag(0, layer), sphere_circumferential_line_tag(1, layer),
         sphere_circumferential_line_tag(2, layer), sphere_circumferential_line_tag(3, layer)});
    sphere_surface_tag(4 + layer) = gmsh::model::geo::addSurfaceFilling({curve_loop_tag}, -1, center_point_tag);
    for (int i = 0; i < 4; i++) {
      const int next = (i + 1) % 4;
      const int connection_curve_loop_tag = gmsh::model::geo::addCurveLoop(
          {sphere_circumferential_line_tag(i, layer), sphere_connection_line_tag(next, layer),
           -circumferential_line_tag(i, 0, layer + 1), -sphere_connection_line_tag(i, layer)});
      sphere_connection_surface_tag(i, layer) = gmsh::model::geo::addSurfaceFilling({connection_curve_loop_tag});
    }
  }
  for (int i = 0; i < 4; i++) {
    sphere_axial_line_tag(i) =
        gmsh::model::geo::addCircleArc(sphere_point_tag(i, 0), center_point_tag, sphere_point_tag(i, 1));
    gmsh::model::geo::mesh::setTransfiniteCurve(sphere_axial_line_tag(i), kAxialCells + 1);
    const int curve_loop_tag =
        gmsh::model::geo::addCurveLoop({sphere_axial_line_tag(i), sphere_connection_line_tag(i, 1),
                                        -axial_line_tag(i, 0, 1), -sphere_connection_line_tag(i, 0)});
    sphere_axial_connection_surface_tag(i) = gmsh::model::geo::addPlaneSurface({curve_loop_tag});
  }
  for (int i = 0; i < 4; i++) {
    const int next = (i + 1) % 4;
    const int curve_loop_tag =
        gmsh::model::geo::addCurveLoop({sphere_circumferential_line_tag(i, 0), sphere_axial_line_tag(next),
                                        -sphere_circumferential_line_tag(i, 1), -sphere_axial_line_tag(i)});
    sphere_surface_tag(i) = gmsh::model::geo::addSurfaceFilling({curve_loop_tag}, -1, center_point_tag);
    const int surface_loop_tag = gmsh::model::geo::addSurfaceLoop(
        {sphere_surface_tag(i), circumferential_surface_tag(i, 0, 1), sphere_connection_surface_tag(i, 0),
         sphere_connection_surface_tag(i, 1), sphere_axial_connection_surface_tag(i),
         sphere_axial_connection_surface_tag(next)});
    physical_group_tag[3].emplace_back(gmsh::model::geo::addVolume({surface_loop_tag}));
  }
  for (int layer = 0; layer < 2; layer++) {
    const int surface_loop_tag = gmsh::model::geo::addSurfaceLoop(
        {sphere_surface_tag(4 + layer), plane_surface_tag(0, layer + 1), sphere_connection_surface_tag(0, layer),
         sphere_connection_surface_tag(1, layer), sphere_connection_surface_tag(2, layer),
         sphere_connection_surface_tag(3, layer)});
    physical_group_tag[3].emplace_back(gmsh::model::geo::addVolume({surface_loop_tag}));
  }
  gmsh::vectorpair surface_tag;
  gmsh::model::geo::synchronize();
  gmsh::model::getEntities(surface_tag, 2);
  for (const auto& [dimension, tag] : surface_tag) {
    static_cast<void>(dimension);
    gmsh::model::mesh::setTransfiniteSurface(tag);
    gmsh::model::mesh::setRecombine(2, tag);
  }
  for (int i = 0; i < 6; i++) {
    physical_group_tag[1].emplace_back(sphere_surface_tag(i));
  }
  for (int i = 0; i < 9; i++) {
    physical_group_tag[0].emplace_back(plane_surface_tag(i, 0));
    physical_group_tag[0].emplace_back(plane_surface_tag(i, 3));
  }
  for (int layer = 0; layer < 3; layer++) {
    for (int i = 0; i < 4; i++) {
      physical_group_tag[0].emplace_back(circumferential_surface_tag(i, 2, layer));
    }
  }
  for (int group = 2; group < 4; group++) {
    for (const int tag : physical_group_tag[static_cast<std::size_t>(group)]) {
      gmsh::model::mesh::setTransfiniteVolume(tag);
    }
  }
  // Shared cylinder and five-patch endcaps remain internal sliding interfaces.
  gmsh::model::addPhysicalGroup(2, physical_group_tag[0], 1, "bc-1");
  gmsh::model::addPhysicalGroup(2, physical_group_tag[1], 2, "bc-2");
  gmsh::model::addPhysicalGroup(3, physical_group_tag[2], 3, "vc-1");
  gmsh::model::addPhysicalGroup(3, physical_group_tag[3], 4, "rc-1");
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::setOrder(SimulationControl::kPolynomialOrder);
  gmsh::model::mesh::optimize("HighOrder");
  gmsh::write(mesh_file_path);
}
