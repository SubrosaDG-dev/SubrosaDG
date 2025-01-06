/**
 * @file unsteadycavity_2d_incns.cpp
 * @brief The source file for SubrosaDG example unsteadycavity_2d_incns.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2024-11-30
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2025 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#include "SubrosaDG.cpp"

inline const std::string kExampleName{"unsteadycavity_2d_incns"};

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory / "build/out" / kExampleName};

using SimulationControl = SubrosaDG::SimulationControl<
    SubrosaDG::SolveControl<SubrosaDG::DimensionEnum::D2, SubrosaDG::PolynomialOrderEnum::P3,
                            SubrosaDG::BoundaryTimeEnum::Steady, SubrosaDG::SourceTermEnum::Boussinesq>,
    SubrosaDG::NumericalControl<SubrosaDG::MeshModelEnum::Quadrangle, SubrosaDG::ShockCapturingEnum::None,
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
  system.setMesh(kExampleDirectory / "unsteadycavity_2d_incns.msh", generateMesh);
  system.setSourceTerm<SimulationControl::kSourceTerm>(1.0_r, 0.0_r);
  // system.addInitialCondition(
  //     []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
  //         -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
  //       return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{1.0_r, 0.0_r, 0.0_r,
  //       0.0_r};
  //     });
  system.addInitialCondition(kExampleDirectory / "unsteadycavity_2d_incns_5000000.raw");
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticNonSlipWall>(
      "bc-1",
      []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
          -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
        return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{1.0_r, 0.0_r, 0.0_r, 0.0_r};
      });
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::IsoThermalNonSlipWall>(
      "bc-2",
      []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
          -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
        return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{1.0_r, 0.0_r, 0.0_r, -0.5_r};
      });
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::IsoThermalNonSlipWall>(
      "bc-3",
      []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
          -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
        return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{1.0_r, 0.0_r, 0.0_r, 0.5_r};
      });
  system.setThermodynamicModel<SimulationControl::kThermodynamicModel>(1.0_r, 1.0_r);
  system.setEquationOfState<SimulationControl::kEquationOfState>(10.0_r, 1.0_r);
  system.setTransportModel<SimulationControl::kTransportModel>(std::sqrt(0.71_r / 3.4e5_r));
  system.setTimeIntegration(1.0_r, {0, 10000000});
  system.setDeltaTime(5e-05_r);
  system.setViewConfig(kExampleDirectory, kExampleName, 1000);
  system.addViewVariable({SubrosaDG::ViewVariableEnum::Density, SubrosaDG::ViewVariableEnum::Velocity,
                          SubrosaDG::ViewVariableEnum::Pressure, SubrosaDG::ViewVariableEnum::Temperature,
                          SubrosaDG::ViewVariableEnum::MachNumber, SubrosaDG::ViewVariableEnum::Vorticity});
  system.synchronize();
  system.solve();
  system.view();
  return EXIT_SUCCESS;
}

void generateMesh(const std::filesystem::path& mesh_file_path) {
  gmsh::model::add("unsteadycavity_2d");
  gmsh::model::geo::addPoint(0.0, 0.0, 0.0);
  gmsh::model::geo::addPoint(1.0, 0.0, 0.0);
  gmsh::model::geo::addPoint(1.0, 8.0, 0.0);
  gmsh::model::geo::addPoint(0.0, 8.0, 0.0);
  gmsh::model::geo::addLine(1, 2);
  gmsh::model::geo::addLine(2, 3);
  gmsh::model::geo::addLine(3, 4);
  gmsh::model::geo::addLine(4, 1);
  gmsh::model::geo::addCurveLoop({1, 2, 3, 4});
  gmsh::model::geo::addPlaneSurface({1});
  gmsh::model::geo::mesh::setTransfiniteCurve(1, 41, "Bump", 0.30);
  gmsh::model::geo::mesh::setTransfiniteCurve(2, 201, "Bump", 0.20);
  gmsh::model::geo::mesh::setTransfiniteCurve(3, 41, "Bump", 0.30);
  gmsh::model::geo::mesh::setTransfiniteCurve(4, 201, "Bump", 0.20);
  gmsh::model::geo::mesh::setTransfiniteSurface(1);
  gmsh::model::geo::mesh::setRecombine(2, 1);
  gmsh::model::geo::synchronize();
  gmsh::model::addPhysicalGroup(1, {1, 3}, -1, "bc-1");
  gmsh::model::addPhysicalGroup(1, {2}, -1, "bc-2");
  gmsh::model::addPhysicalGroup(1, {4}, -1, "bc-3");
  gmsh::model::addPhysicalGroup(2, {1}, -1, "vc-1");
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::setOrder(SimulationControl::kPolynomialOrder);
  gmsh::model::mesh::optimize("HighOrder");
  gmsh::write(mesh_file_path);
}
