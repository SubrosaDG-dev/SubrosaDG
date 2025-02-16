/**
 * @file shuosher_1d_ceuler.cpp
 * @brief The source file for SubrosaDG example shuosher_1d_ceuler.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2024-07-20
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2025 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#include "SubrosaDG.cpp"

inline const std::string kExampleName{"shuosher_1d_ceuler"};

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory / "build/out" / kExampleName};

using SimulationControl = SubrosaDG::SimulationControl<
    SubrosaDG::SolveControl<SubrosaDG::DimensionEnum::D1, SubrosaDG::PolynomialOrderEnum::P3,
                            SubrosaDG::BoundaryTimeEnum::Steady, SubrosaDG::SourceTermEnum::None>,
    SubrosaDG::NumericalControl<SubrosaDG::MeshModelEnum::Line, SubrosaDG::ShockCapturingEnum::ArtificialViscosity,
                                SubrosaDG::LimiterEnum::None, SubrosaDG::InitialConditionEnum::Function,
                                SubrosaDG::TimeIntegrationEnum::SSPRK3>,
    SubrosaDG::CompresibleEulerVariable<SubrosaDG::ThermodynamicModelEnum::Constant,
                                        SubrosaDG::EquationOfStateEnum::IdealGas, SubrosaDG::ConvectiveFluxEnum::HLLC>>;

template <typename SimulationControl>
inline Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>
SubrosaDG::InitialCondition<SimulationControl>::calculatePrimitiveFromCoordinate(
    const Eigen::Vector<Real, SimulationControl::kDimension>& coordinate) const {
  return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{
      coordinate.x() <= -4.0_r ? 3.857143_r : 1.0_r + 0.2_r * std::sin(5.0_r * coordinate.x()),
      coordinate.x() <= -4.0_r ? 2.629369_r : 0.0_r,
      coordinate.x() <= -4.0_r ? 3.750342_r : 1.4_r / (1.0_r + 0.2_r * std::sin(5.0_r * coordinate.x()))};
}

template <typename SimulationControl>
inline Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>
SubrosaDG::BoundaryCondition<SimulationControl>::calculatePrimitiveFromCoordinate(
    [[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate,
    const SubrosaDG::Isize gmsh_physical_index) const {
  if (gmsh_physical_index == 1) {
    return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{3.857143_r, 2.629369_r,
                                                                                       3.750342_r};
  }
  if (gmsh_physical_index == 2) {
    return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{
        1.0_r + 0.2_r * std::sin(25.0_r), 0.0_r, 1.4_r / (1.0_r + 0.2_r * std::sin(25.0_r))};
  }
  return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>::Zero();
}

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::System<SimulationControl> system;
  system.setMesh(kExampleDirectory / "shuosher_1d_ceuler.msh", generateMesh);
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::RiemannFarfield>(1);
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::RiemannFarfield>(2);
  system.setThermodynamicModel<SimulationControl::kThermodynamicModel>(2.5_r, 25.0_r / 14.0_r);
  system.setArtificialViscosity(3.0_r);
  system.setTimeIntegration(0.01_r);
  system.setViewConfig(kExampleDirectory, kExampleName);
  system.addViewVariable({SubrosaDG::ViewVariableEnum::Density, SubrosaDG::ViewVariableEnum::Velocity,
                          SubrosaDG::ViewVariableEnum::Pressure, SubrosaDG::ViewVariableEnum::ArtificialViscosity});
  system.synchronize();
  system.solve();
  system.view();
  return EXIT_SUCCESS;
}

void generateMesh(const std::filesystem::path& mesh_file_path) {
  gmsh::model::add("shuosher_1d");
  gmsh::model::geo::addPoint(-5.0, 0.0, 0.0);
  gmsh::model::geo::addPoint(5.0, 0.0, 0.0);
  gmsh::model::geo::addLine(1, 2);
  gmsh::model::geo::mesh::setTransfiniteCurve(1, 101);
  gmsh::model::geo::synchronize();
  gmsh::model::addPhysicalGroup(0, {1}, 1, "bc-1");
  gmsh::model::addPhysicalGroup(0, {2}, 2, "bc-2");
  gmsh::model::addPhysicalGroup(1, {1}, 3, "vc-1");
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::setOrder(SimulationControl::kPolynomialOrder);
  gmsh::model::mesh::optimize("HighOrder");
  gmsh::write(mesh_file_path);
}
