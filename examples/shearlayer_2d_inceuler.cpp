/**
 * @file shearlayer_2d_inceuler.cpp
 * @brief The source file for SubrosaDG example shearlayer_2d_inceuler.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2024-11-25
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2025 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#include "SubrosaDG.cpp"

inline const std::string kExampleName{"shearlayer_2d_inceuler"};

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory / "build/out" / kExampleName};

using SimulationControl = SubrosaDG::SimulationControl<
    SubrosaDG::SolveControl<SubrosaDG::DimensionEnum::D2, SubrosaDG::PolynomialOrderEnum::P1,
                            SubrosaDG::BoundaryTimeEnum::Steady, SubrosaDG::SourceTermEnum::None>,
    SubrosaDG::NumericalControl<SubrosaDG::MeshModelEnum::Quadrangle, SubrosaDG::ShockCapturingEnum::None,
                                SubrosaDG::LimiterEnum::None, SubrosaDG::InitialConditionEnum::Function,
                                SubrosaDG::TimeIntegrationEnum::SSPRK3>,
    SubrosaDG::IncompresibleEulerVariable<SubrosaDG::ThermodynamicModelEnum::Constant,
                                          SubrosaDG::EquationOfStateEnum::WeakCompressibleFluid,
                                          SubrosaDG::ConvectiveFluxEnum::LaxFriedrichs>>;

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::System<SimulationControl> system;
  system.setMesh(kExampleDirectory / "shearlayer_2d_inceuler.msh", generateMesh);

  system.addInitialCondition([](const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
                                 -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
    const SubrosaDG::Real k = SubrosaDG::kPi / 15.0_r;
    return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{
        1.0_r,
        coordinate.y() <= SubrosaDG::kPi ? std::tanh((2.0 * coordinate.y() - SubrosaDG::kPi) / (2.0_r * k))
                                         : std::tanh((3.0 * SubrosaDG::kPi - 2.0_r * coordinate.y()) / (2.0_r * k)),
        0.05_r * std::sin(coordinate.x()), 1.0_r};
  });
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::Periodic>("bc-1");
  system.setThermodynamicModel<SimulationControl::kThermodynamicModel>(1.0_r, 1.0_r);
  system.setEquationOfState<SimulationControl::kEquationOfState>(10.0_r, 1.0_r);
  system.setTimeIntegration(1.0_r);
  system.setDeltaTime(5e-04_r);
  system.setViewConfig(kExampleDirectory, kExampleName);
  system.addViewVariable({SubrosaDG::ViewVariableEnum::Density, SubrosaDG::ViewVariableEnum::Velocity,
                          SubrosaDG::ViewVariableEnum::Pressure});
  system.synchronize();
  system.solve();
  system.view();
  return EXIT_SUCCESS;
}

void generateMesh(const std::filesystem::path& mesh_file_path) {
  gmsh::model::add("shearlayer_2d");
  gmsh::model::geo::addPoint(0.0, 0.0, 0.0);
  gmsh::model::geo::addPoint(2.0 * SubrosaDG::kPi, 0.0, 0.0);
  gmsh::model::geo::addPoint(2.0 * SubrosaDG::kPi, 2.0 * SubrosaDG::kPi, 0.0);
  gmsh::model::geo::addPoint(0.0, 2.0 * SubrosaDG::kPi, 0.0);
  gmsh::model::geo::addLine(1, 2);
  gmsh::model::geo::addLine(2, 3);
  gmsh::model::geo::addLine(4, 3);
  gmsh::model::geo::addLine(1, 4);
  gmsh::model::geo::addCurveLoop({1, 2, -3, -4});
  gmsh::model::geo::addPlaneSurface({1});
  gmsh::model::geo::mesh::setTransfiniteCurve(1, 41);
  gmsh::model::geo::mesh::setTransfiniteCurve(2, 41);
  gmsh::model::geo::mesh::setTransfiniteCurve(3, 41);
  gmsh::model::geo::mesh::setTransfiniteCurve(4, 41);
  gmsh::model::geo::mesh::setTransfiniteSurface(1);
  gmsh::model::geo::mesh::setRecombine(2, 1);
  gmsh::model::geo::synchronize();
  Eigen::Matrix<double, 4, 4, Eigen::RowMajor> transform_x = (Eigen::Transform<double, 3, Eigen::Affine>::Identity() *
                                                              Eigen::Translation<double, 3>(2.0 * SubrosaDG::kPi, 0, 0))
                                                                 .matrix();
  Eigen::Matrix<double, 4, 4, Eigen::RowMajor> transform_y = (Eigen::Transform<double, 3, Eigen::Affine>::Identity() *
                                                              Eigen::Translation<double, 3>(0, 2.0 * SubrosaDG::kPi, 0))
                                                                 .matrix();
  gmsh::model::mesh::setPeriodic(1, {2}, {4}, {transform_x.data(), transform_x.data() + transform_x.size()});
  gmsh::model::mesh::setPeriodic(1, {3}, {1}, {transform_y.data(), transform_y.data() + transform_y.size()});
  gmsh::model::addPhysicalGroup(1, {1, 2, 3, 4}, -1, "bc-1");
  gmsh::model::addPhysicalGroup(2, {1}, -1, "vc-1");
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::setOrder(SimulationControl::kPolynomialOrder);
  gmsh::model::mesh::optimize("HighOrder");
  gmsh::write(mesh_file_path);
}
