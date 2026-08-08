/**
 * @file periodic_2d_ceuler.cpp
 * @brief The source file for SubrosaDG example periodic_2d_ceuler.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2024-01-17
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

inline const std::string kExampleName{"periodic_2d_ceuler"};

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory / "build/out" / kExampleName};

using SimulationControl = SubrosaDG::SimulationControl<
    SubrosaDG::SolveControl<SubrosaDG::DimensionEnum::D2, SubrosaDG::PolynomialOrderEnum::P3,
                            SubrosaDG::BoundaryTimeEnum::Steady, SubrosaDG::SourceTermEnum::None>,
    SubrosaDG::NumericalControl<SubrosaDG::MeshModelEnum::TriangleQuadrangle, SubrosaDG::InitialConditionEnum::Function,
                                SubrosaDG::TimeIntegrationEnum::SSPRK3>,
    SubrosaDG::CompressibleEulerVariable<SubrosaDG::ThermodynamicModelEnum::Constant,
                                         SubrosaDG::EquationOfStateEnum::IdealGas,
                                         SubrosaDG::ConvectiveFluxEnum::LaxFriedrichs>>;

template <typename SimulationControl>
inline void SubrosaDG::InitialCondition<SimulationControl>::computePrimitiveFromCoordinate(
    const Eigen::Vector<Real, SimulationControl::kDimension>& coordinate,
    Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber>& initial_primitive_variable) {
  initial_primitive_variable = {
      1.0_r + 0.2_r * std::sin(SubrosaDG::kPi * (coordinate.x() + coordinate.y()) / 5.0_r), 0.7_r, 0.3_r,
      1.4_r / (1.0_r + 0.2_r * std::sin(SubrosaDG::kPi * (coordinate.x() + coordinate.y()) / 5.0_r))};
}

template <typename SimulationControl>
inline void SubrosaDG::BoundaryCondition<SimulationControl>::computePrimitiveFromCoordinate(
    [[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate,
    [[maybe_unused]] Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber>& boundary_primitive_variable,
    [[maybe_unused]] const SubrosaDG::Isize gmsh_physical_index) {}

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::System<SimulationControl> system;
  system.setMesh(kExampleDirectory / std::format("{}.msh", kExampleName), generateMesh);
  system.setRotation(Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>{0.0_r, 0.0_r},
                     Eigen::Vector<SubrosaDG::Real, 3>{0.0_r, 0.0_r, 1.0_r});
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::Periodic>(1);
  system.template addVolumeCondition<SubrosaDG::InteriorConditionEnum::Static>(2);
  system.template addVolumeCondition<SubrosaDG::InteriorConditionEnum::Rotate>(3);
  system.setTimeIntegration(0.2_r, {0, 10});
  system.setViewConfig(kExampleDirectory, kExampleName, 10);
  system.addViewVariable({SubrosaDG::ViewVariableEnum::Density, SubrosaDG::ViewVariableEnum::Velocity,
                          SubrosaDG::ViewVariableEnum::Pressure});
  system.synchronize();
  system.solve();
  system.view();
  return EXIT_SUCCESS;
}

void generateMesh(const std::filesystem::path& mesh_file_path) {
  gmsh::model::add("periodic_2d");
  gmsh::model::geo::addPoint(-5.0, -5.0, 0.0);
  gmsh::model::geo::addPoint(5.0, -5.0, 0.0);
  gmsh::model::geo::addPoint(5.0, 5.0, 0.0);
  gmsh::model::geo::addPoint(-5.0, 5.0, 0.0);
  gmsh::model::geo::addPoint(0.0, 0.0, 0.0);
  gmsh::model::geo::addPoint(2.5, 0.0, 0.0);
  gmsh::model::geo::addPoint(0.0, 2.5, 0.0);
  gmsh::model::geo::addPoint(-2.5, 0.0, 0.0);
  gmsh::model::geo::addPoint(0.0, -2.5, 0.0);
  gmsh::model::geo::addLine(1, 2);
  gmsh::model::geo::addLine(2, 3);
  gmsh::model::geo::addLine(4, 3);
  gmsh::model::geo::addLine(1, 4);
  gmsh::model::geo::addCircleArc(6, 5, 7);
  gmsh::model::geo::addCircleArc(7, 5, 8);
  gmsh::model::geo::addCircleArc(8, 5, 9);
  gmsh::model::geo::addCircleArc(9, 5, 6);
  gmsh::model::geo::addCurveLoop({1, 2, -3, -4});
  gmsh::model::geo::addCurveLoop({5, 6, 7, 8});
  gmsh::model::geo::addPlaneSurface({1, 2});
  gmsh::model::geo::addPlaneSurface({2});
  gmsh::model::geo::mesh::setTransfiniteCurve(1, 31);
  gmsh::model::geo::mesh::setTransfiniteCurve(2, 31);
  gmsh::model::geo::mesh::setTransfiniteCurve(3, 31);
  gmsh::model::geo::mesh::setTransfiniteCurve(4, 31);
  gmsh::model::geo::mesh::setTransfiniteCurve(5, 16);
  gmsh::model::geo::mesh::setTransfiniteCurve(6, 16);
  gmsh::model::geo::mesh::setTransfiniteCurve(7, 16);
  gmsh::model::geo::mesh::setTransfiniteCurve(8, 16);
  gmsh::model::geo::mesh::setRecombine(2, 2);
  gmsh::model::geo::synchronize();
  Eigen::Matrix<double, 4, 4, Eigen::RowMajor> transform_x =
      (Eigen::Transform<double, 3, Eigen::Affine>::Identity() * Eigen::Translation<double, 3>(10, 0, 0)).matrix();
  Eigen::Matrix<double, 4, 4, Eigen::RowMajor> transform_y =
      (Eigen::Transform<double, 3, Eigen::Affine>::Identity() * Eigen::Translation<double, 3>(0, 10, 0)).matrix();
  gmsh::model::mesh::setPeriodic(1, {2}, {4}, {transform_x.data(), transform_x.data() + transform_x.size()});
  gmsh::model::mesh::setPeriodic(1, {3}, {1}, {transform_y.data(), transform_y.data() + transform_y.size()});
  gmsh::model::addPhysicalGroup(1, {1, 2, 3, 4}, 1, "bc-1");
  gmsh::model::addPhysicalGroup(2, {1}, 2, "vc-1");
  gmsh::model::addPhysicalGroup(2, {2}, 3, "rc-1");
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::setOrder(SimulationControl::kPolynomialOrder);
  gmsh::model::mesh::optimize("HighOrder");
  gmsh::write(mesh_file_path);
}
