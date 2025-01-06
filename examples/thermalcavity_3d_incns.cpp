/**
 * @file thermalcavity_3d_incns.cpp
 * @brief The source file for SubrosaDG example thermalcavity_3d_incns.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2024-11-25
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2025 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#include "SubrosaDG.cpp"

inline const std::string kExampleName{"thermalcavity_3d_incns"};

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory / "build/out" / kExampleName};

using SimulationControl = SubrosaDG::SimulationControl<
    SubrosaDG::SolveControl<SubrosaDG::DimensionEnum::D3, SubrosaDG::PolynomialOrderEnum::P3,
                            SubrosaDG::BoundaryTimeEnum::Steady, SubrosaDG::SourceTermEnum::Boussinesq>,
    SubrosaDG::NumericalControl<SubrosaDG::MeshModelEnum::Hexahedron, SubrosaDG::ShockCapturingEnum::None,
                                SubrosaDG::LimiterEnum::None, SubrosaDG::InitialConditionEnum::SpecificFile,
                                SubrosaDG::TimeIntegrationEnum::SSPRK3>,
    SubrosaDG::IncompresibleNSVariable<SubrosaDG::ThermodynamicModelEnum::Constant,
                                       SubrosaDG::EquationOfStateEnum::WeakCompressibleFluid,
                                       SubrosaDG::TransportModelEnum::Constant, SubrosaDG::ConvectiveFluxEnum::Exact,
                                       SubrosaDG::ViscousFluxEnum::BR2>>;

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::System<SimulationControl> system;
  system.setMesh(kExampleDirectory / "thermalcavity_3d_incns.msh", generateMesh);
  system.setSourceTerm<SimulationControl::kSourceTerm>(1.0_r, 0.5_r);
  // system.addInitialCondition(
  //     []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
  //         -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
  //       return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{1.0_r, 0.0_r, 0.0_r,
  //       0.0_r,
  //                                                                                          0.5_r};
  //     });
  system.addInitialCondition(kExampleDirectory / "thermalcavity_3d_incns_500000.raw");
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticNonSlipWall>(
      "bc-1",
      []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
          -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
        return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{1.0_r, 0.0_r, 0.0_r, 0.0_r,
                                                                                           0.5_r};
      });
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::IsoThermalNonSlipWall>(
      "bc-2",
      []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
          -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
        return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{1.0_r, 0.0_r, 1.0_r, 0.0_r,
                                                                                           1.0_r};
      });
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::IsoThermalNonSlipWall>(
      "bc-3",
      []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
          -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
        return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{1.0_r, 0.0_r, 0.0_r, 0.0_r,
                                                                                           0.0_r};
      });
  system.setThermodynamicModel<SimulationControl::kThermodynamicModel>(1.0_r, 1.0_r);
  system.setEquationOfState<SimulationControl::kEquationOfState>(10.0_r, 1.0_r);
  system.setTransportModel<SimulationControl::kTransportModel>(1.0_r * 1.0_r * 1.0_r / 1000.0_r);
  system.setTimeIntegration(0.5_r);
  system.setDeltaTime(1e-4_r);
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
  gmsh::model::add("thermalcavity_3d");
  Eigen::Vector<double, 2> point_coordinate;
  // clang-format off
  point_coordinate << 0.0, 1.0;
  // clang-format on
  Eigen::Tensor<int, 3> point_tag(2, 2, 2);
  Eigen::Tensor<int, 3> line_tag(2, 2, 3);
  Eigen::Tensor<int, 2> curve_loop_tag(2, 3);
  Eigen::Tensor<int, 2> surface_filling_tag(2, 3);
  std::array<std::vector<int>, 3> physical_group_tag;
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      for (std::ptrdiff_t k = 0; k < 2; k++) {
        point_tag(k, j, i) =
            gmsh::model::geo::addPoint(point_coordinate(k), point_coordinate(j), point_coordinate(i), 0.04);
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      line_tag(j, i, 0) = gmsh::model::geo::addLine(point_tag(0, j, i), point_tag(1, j, i));
      line_tag(j, i, 1) = gmsh::model::geo::addLine(point_tag(j, 0, i), point_tag(j, 1, i));
      line_tag(j, i, 2) = gmsh::model::geo::addLine(point_tag(j, i, 0), point_tag(j, i, 1));
    }
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    curve_loop_tag(i, 0) =
        gmsh::model::geo::addCurveLoop({line_tag(0, i, 0), line_tag(1, i, 1), -line_tag(1, i, 0), -line_tag(0, i, 1)});
    curve_loop_tag(i, 1) =
        gmsh::model::geo::addCurveLoop({line_tag(i, 0, 1), line_tag(i, 1, 2), -line_tag(i, 1, 1), -line_tag(i, 0, 2)});
    curve_loop_tag(i, 2) =
        gmsh::model::geo::addCurveLoop({line_tag(0, i, 2), line_tag(i, 1, 0), -line_tag(1, i, 2), -line_tag(i, 0, 0)});
  }
  for (std::ptrdiff_t i = 0; i < 3; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      surface_filling_tag(j, i) = gmsh::model::geo::addSurfaceFilling({curve_loop_tag(j, i)});
    }
  }
  int surface_loop_tag = gmsh::model::geo::addSurfaceLoop({surface_filling_tag(0, 0), surface_filling_tag(0, 1),
                                                           surface_filling_tag(0, 2), surface_filling_tag(1, 0),
                                                           surface_filling_tag(1, 1), surface_filling_tag(1, 2)});
  int volume_tag = gmsh::model::geo::addVolume({surface_loop_tag});
  for (std::ptrdiff_t i = 0; i < 3; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      gmsh::model::geo::mesh::setRecombine(2, surface_filling_tag(j, i));
    }
  }
  gmsh::model::geo::mesh::setRecombine(3, volume_tag);
  gmsh::model::geo::synchronize();
  gmsh::model::mesh::setTransfiniteAutomatic();
  for (std::ptrdiff_t i = 0; i < 3; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      if (i == 0 && j == 0) {
        physical_group_tag[2].emplace_back(surface_filling_tag(j, i));
      } else if (i == 0 && j == 1) {
        physical_group_tag[1].emplace_back(surface_filling_tag(j, i));
      } else {
        physical_group_tag[0].emplace_back(surface_filling_tag(j, i));
      }
    }
  }
  gmsh::model::addPhysicalGroup(2, physical_group_tag[0], -1, "bc-1");
  gmsh::model::addPhysicalGroup(2, physical_group_tag[1], -1, "bc-2");
  gmsh::model::addPhysicalGroup(2, physical_group_tag[2], -1, "bc-3");
  gmsh::model::addPhysicalGroup(3, {volume_tag}, -1, "vc-1");
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::setOrder(SimulationControl::kPolynomialOrder);
  gmsh::model::mesh::optimize("HighOrder");
  gmsh::write(mesh_file_path);
}
