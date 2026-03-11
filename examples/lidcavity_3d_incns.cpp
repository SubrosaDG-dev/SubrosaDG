/**
 * @file lidcavity_3d_incns.cpp
 * @brief The source file for SubrosaDG example lidcavity_3d_incns.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2024-07-21
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
  static constexpr double kReynoldsNumber = 1000.0;
  static constexpr double kDynamicViscosity = 1.0 * 1.0 * 1.0 / kReynoldsNumber;
  static constexpr double kPrandtlNumber = 0.71;
  static constexpr double kThermalConductivity = kSpecificHeatConstantPressure * kDynamicViscosity / kPrandtlNumber;
};

}  // namespace SubrosaDG

#include "SubrosaDG.cpp"

inline const std::string kExampleName{"lidcavity_3d_incns"};

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory / "build/out" / kExampleName};

using SimulationControl = SubrosaDG::SimulationControl<
    SubrosaDG::SolveControl<SubrosaDG::DimensionEnum::D3, SubrosaDG::PolynomialOrderEnum::P1,
                            SubrosaDG::BoundaryTimeEnum::Steady, SubrosaDG::SourceTermEnum::None>,
    SubrosaDG::NumericalControl<SubrosaDG::MeshModelEnum::Hexahedron, SubrosaDG::InitialConditionEnum::Function,
                                SubrosaDG::TimeIntegrationEnum::SSPRK3>,
    SubrosaDG::IncompressibleNSVariable<SubrosaDG::ThermodynamicModelEnum::Constant,
                                        SubrosaDG::EquationOfStateEnum::WeakCompressibleFluid,
                                        SubrosaDG::TransportModelEnum::Constant,
                                        SubrosaDG::ConvectiveFluxEnum::LaxFriedrichs, SubrosaDG::ViscousFluxEnum::BR2>>;

template <typename SimulationControl>
inline void SubrosaDG::InitialCondition<SimulationControl>::computePrimitiveFromCoordinate(
    [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& coordinate,
    Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber>& initial_primitive_variable) {
  initial_primitive_variable = {1.0_r, 0.0_r, 0.0_r, 0.0_r, 1.0_r};
}

template <typename SimulationControl>
inline void SubrosaDG::BoundaryCondition<SimulationControl>::computePrimitiveFromCoordinate(
    [[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate,
    Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber>& boundary_primitive_variable,
    const SubrosaDG::Isize gmsh_physical_index) {
  if (gmsh_physical_index == 1) {
    boundary_primitive_variable = {1.0_r, 0.0_r, 0.0_r, 0.0_r, 1.0_r};
  } else if (gmsh_physical_index == 2) {
    boundary_primitive_variable = {1.0_r, 0.0_r, 1.0_r, 0.0_r, 1.0_r};
  }
}

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::System<SimulationControl> system;
  system.setMesh(kExampleDirectory / std::format("{}.msh", kExampleName), generateMesh);
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticNonSlipWall>(1);
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticNonSlipWall>(2);
  system.setTimeIntegration(1.0_r);
  system.setViewConfig(kExampleDirectory, kExampleName);
  system.addViewVariable({SubrosaDG::ViewVariableEnum::Density, SubrosaDG::ViewVariableEnum::Velocity,
                          SubrosaDG::ViewVariableEnum::Pressure, SubrosaDG::ViewVariableEnum::Temperature,
                          SubrosaDG::ViewVariableEnum::MachNumber, SubrosaDG::ViewVariableEnum::Vorticity});
  system.synchronize();
  system.solve();
  system.view();
  return EXIT_SUCCESS;
}

void generateMesh(const std::filesystem::path& mesh_file_path) {
  gmsh::model::add("lidcavity_3d");
  Eigen::Vector<double, 2> point_coordinate;
  // clang-format off
  point_coordinate << 0.0, 1.0;
  // clang-format on
  Eigen::Tensor<int, 3> point_tag(2, 2, 2);
  Eigen::Tensor<int, 3> line_tag(2, 2, 3);
  Eigen::Tensor<int, 2> curve_loop_tag(2, 3);
  Eigen::Tensor<int, 2> surface_filling_tag(2, 3);
  std::array<std::vector<int>, 2> physical_group_tag;
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      for (int k = 0; k < 2; k++) {
        point_tag(k, j, i) =
            gmsh::model::geo::addPoint(point_coordinate(k), point_coordinate(j), point_coordinate(i), 0.04);
      }
    }
  }
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      line_tag(j, i, 0) = gmsh::model::geo::addLine(point_tag(0, j, i), point_tag(1, j, i));
      line_tag(j, i, 1) = gmsh::model::geo::addLine(point_tag(j, 0, i), point_tag(j, 1, i));
      line_tag(j, i, 2) = gmsh::model::geo::addLine(point_tag(j, i, 0), point_tag(j, i, 1));
    }
  }
  for (int i = 0; i < 2; i++) {
    curve_loop_tag(i, 0) =
        gmsh::model::geo::addCurveLoop({line_tag(0, i, 0), line_tag(1, i, 1), -line_tag(1, i, 0), -line_tag(0, i, 1)});
    curve_loop_tag(i, 1) =
        gmsh::model::geo::addCurveLoop({line_tag(i, 0, 1), line_tag(i, 1, 2), -line_tag(i, 1, 1), -line_tag(i, 0, 2)});
    curve_loop_tag(i, 2) =
        gmsh::model::geo::addCurveLoop({line_tag(0, i, 2), line_tag(i, 1, 0), -line_tag(1, i, 2), -line_tag(i, 0, 0)});
  }
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 2; j++) {
      surface_filling_tag(j, i) = gmsh::model::geo::addSurfaceFilling({curve_loop_tag(j, i)});
    }
  }
  int surface_loop_tag = gmsh::model::geo::addSurfaceLoop({surface_filling_tag(0, 0), surface_filling_tag(0, 1),
                                                           surface_filling_tag(0, 2), surface_filling_tag(1, 0),
                                                           surface_filling_tag(1, 1), surface_filling_tag(1, 2)});
  int volume_tag = gmsh::model::geo::addVolume({surface_loop_tag});
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 2; j++) {
      gmsh::model::geo::mesh::setRecombine(2, surface_filling_tag(j, i));
    }
  }
  gmsh::model::geo::mesh::setRecombine(3, volume_tag);
  gmsh::model::geo::synchronize();
  gmsh::model::mesh::setTransfiniteAutomatic();
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 2; j++) {
      if (i == 0 && j == 1) {
        physical_group_tag[1].emplace_back(surface_filling_tag(j, i));
      } else {
        physical_group_tag[0].emplace_back(surface_filling_tag(j, i));
      }
    }
  }
  gmsh::model::addPhysicalGroup(2, physical_group_tag[0], 1, "bc-1");
  gmsh::model::addPhysicalGroup(2, physical_group_tag[1], 2, "bc-2");
  gmsh::model::addPhysicalGroup(3, {volume_tag}, 3, "vc-1");
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::setOrder(SimulationControl::kPolynomialOrder);
  gmsh::model::mesh::optimize("HighOrder");
  gmsh::write(mesh_file_path);
}
