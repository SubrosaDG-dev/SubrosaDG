/**
 * @file taylorvortex_2d_incns.cpp
 * @brief The source file for SubrosaDG example taylorvortex_2d_incns.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2024-12-01
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#include "SubrosaDG"

inline const std::string kExampleName{"taylorvortex_2d_incns"};

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory / "build/out" / kExampleName};

using SimulationControl = SubrosaDG::SimulationControl<
    SubrosaDG::SolveControl<SubrosaDG::DimensionEnum::D2, SubrosaDG::PolynomialOrderEnum::P3,
                            SubrosaDG::BoundaryTimeEnum::Steady, SubrosaDG::SourceTermEnum::None>,
    SubrosaDG::NumericalControl<SubrosaDG::MeshModelEnum::Quadrangle, SubrosaDG::ShockCapturingEnum::None,
                                SubrosaDG::LimiterEnum::None, SubrosaDG::InitialConditionEnum::Function,
                                SubrosaDG::TimeIntegrationEnum::SSPRK3>,
    SubrosaDG::IncompresibleNSVariable<SubrosaDG::ThermodynamicModelEnum::Constant,
                                       SubrosaDG::EquationOfStateEnum::WeakCompressibleFluid,
                                       SubrosaDG::TransportModelEnum::Constant,
                                       SubrosaDG::ConvectiveFluxEnum::LaxFriedrichs, SubrosaDG::ViscousFluxEnum::BR2>>;

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::System<SimulationControl> system;
  system.setMesh(kExampleDirectory / "taylorvortex_2d_incns.msh", generateMesh);
  system.addInitialCondition([](const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
                                 -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
    return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{
        ((std::cos(4.0_r * SubrosaDG::kPi * coordinate.x()) + std::cos(4.0_r * SubrosaDG::kPi * coordinate.y())) /
         4.0_r) /
                10000.0_r +
            0.99_r * 1.0_r,
        std::sin(2.0_r * SubrosaDG::kPi * coordinate.x()) * std::cos(2.0_r * SubrosaDG::kPi * coordinate.y()),
        -std::cos(2.0_r * SubrosaDG::kPi * coordinate.x()) * std::sin(2.0_r * SubrosaDG::kPi * coordinate.y()), 1.0_r};
  });
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::Periodic>("bc-1");
  system.setThermodynamicModel<SimulationControl::kThermodynamicModel>(1.0_r, 1.0_r);
  system.setEquationOfState<SimulationControl::kEquationOfState>(100.0_r, 1.0_r);
  system.setTransportModel<SimulationControl::kTransportModel>(1.0_r * 1.0_r * 1.0_r / 1000.0_r);
  system.setTimeIntegration(1.0_r);
  // system.setDeltaTime(1e-5_r);
  system.setViewConfig(kExampleDirectory, kExampleName);
  system.addViewVariable({SubrosaDG::ViewVariableEnum::Density, SubrosaDG::ViewVariableEnum::Velocity,
                          SubrosaDG::ViewVariableEnum::Pressure, SubrosaDG::ViewVariableEnum::MachNumber});
  system.synchronize();
  system.solve();
  system.view();
  return EXIT_SUCCESS;
}

void generateMesh(const std::filesystem::path& mesh_file_path) {
  gmsh::model::add("taylorvortex_2d");
  gmsh::model::geo::addPoint(0.0, 0.0, 0.0);
  gmsh::model::geo::addPoint(1.0, 0.0, 0.0);
  gmsh::model::geo::addPoint(1.0, 1.0, 0.0);
  gmsh::model::geo::addPoint(0.0, 1.0, 0.0);
  gmsh::model::geo::addLine(1, 2);
  gmsh::model::geo::addLine(2, 3);
  gmsh::model::geo::addLine(4, 3);
  gmsh::model::geo::addLine(1, 4);
  gmsh::model::geo::addCurveLoop({1, 2, -3, -4});
  gmsh::model::geo::addPlaneSurface({1});
  gmsh::model::geo::mesh::setTransfiniteCurve(1, 81);
  gmsh::model::geo::mesh::setTransfiniteCurve(2, 81);
  gmsh::model::geo::mesh::setTransfiniteCurve(3, 81);
  gmsh::model::geo::mesh::setTransfiniteCurve(4, 81);
  gmsh::model::geo::mesh::setTransfiniteSurface(1);
  gmsh::model::geo::mesh::setRecombine(2, 1);
  gmsh::model::geo::synchronize();
  Eigen::Matrix<double, 4, 4, Eigen::RowMajor> transform_x =
      (Eigen::Transform<double, 3, Eigen::Affine>::Identity() * Eigen::Translation<double, 3>(1.0, 0, 0)).matrix();
  Eigen::Matrix<double, 4, 4, Eigen::RowMajor> transform_y =
      (Eigen::Transform<double, 3, Eigen::Affine>::Identity() * Eigen::Translation<double, 3>(0, 1.0, 0)).matrix();
  gmsh::model::mesh::setPeriodic(1, {2}, {4}, {transform_x.data(), transform_x.data() + transform_x.size()});
  gmsh::model::mesh::setPeriodic(1, {3}, {1}, {transform_y.data(), transform_y.data() + transform_y.size()});
  gmsh::model::addPhysicalGroup(1, {1, 2, 3, 4}, -1, "bc-1");
  gmsh::model::addPhysicalGroup(2, {1}, -1, "vc-1");
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::setOrder(SimulationControl::kPolynomialOrder);
  gmsh::model::mesh::optimize("HighOrder");
  gmsh::write(mesh_file_path);
}
