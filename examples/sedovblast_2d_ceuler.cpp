/**
 * @file sedovblast_2d_ceuler.cpp
 * @brief The source file for SubrosaDG example sedovblast_2d_ceuler.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2024-01-17
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2025 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#include "SubrosaDG.cpp"

inline const std::string kExampleName{"sedovblast_2d_ceuler"};

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory / "build/out" / kExampleName};

using SimulationControl = SubrosaDG::SimulationControl<
    SubrosaDG::SolveControl<SubrosaDG::DimensionEnum::D2, SubrosaDG::PolynomialOrderEnum::P3,
                            SubrosaDG::BoundaryTimeEnum::Steady, SubrosaDG::SourceTermEnum::None>,
    SubrosaDG::NumericalControl<SubrosaDG::MeshModelEnum::Quadrangle,
                                SubrosaDG::ShockCapturingEnum::ArtificialViscosity, SubrosaDG::LimiterEnum::None,
                                SubrosaDG::InitialConditionEnum::Function, SubrosaDG::TimeIntegrationEnum::SSPRK3>,
    SubrosaDG::CompresibleEulerVariable<SubrosaDG::ThermodynamicModelEnum::Constant,
                                        SubrosaDG::EquationOfStateEnum::IdealGas, SubrosaDG::ConvectiveFluxEnum::HLLC>>;

template <typename SimulationControl>
inline Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>
SubrosaDG::InitialCondition<SimulationControl>::calculatePrimitiveFromCoordinate(
    const Eigen::Vector<Real, SimulationControl::kDimension>& coordinate) const {
  // NOTE: https://arxiv.org/pdf/2102.06017
  return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{
      1.0_r +
          std::exp(-(coordinate.x() * coordinate.x() + coordinate.y() * coordinate.y()) / (2.0_r * 0.25_r * 0.25_r)) /
              (4.0_r * SubrosaDG::kPi * 0.25_r * 0.25_r),
      0.0_r, 0.0_r,
      1.4_r *
          (1e-5_r + (1.4_r - 1.0_r) *
                        std::exp(-(coordinate.x() * coordinate.x() + coordinate.y() * coordinate.y()) /
                                 (2.0_r * 0.15_r * 0.15_r)) /
                        (4.0_r * SubrosaDG::kPi * 0.15_r * 0.15_r)) /
          (1.0_r +
           std::exp(-(coordinate.x() * coordinate.x() + coordinate.y() * coordinate.y()) / (2.0_r * 0.25_r * 0.25_r)) /
               (4.0_r * SubrosaDG::kPi * 0.25_r * 0.25_r))};
}

template <typename SimulationControl>
inline Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>
SubrosaDG::BoundaryCondition<SimulationControl>::calculatePrimitiveFromCoordinate(
    [[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate,
    [[maybe_unused]] const SubrosaDG::Isize gmsh_physical_index) const {
  return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>::Zero();
}

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::System<SimulationControl> system;
  system.setMesh(kExampleDirectory / "sedovblast_2d_ceuler.msh", generateMesh);
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::Periodic>(1);
  system.setThermodynamicModel<SimulationControl::kThermodynamicModel>(2.5_r, 25.0_r / 14.0_r);
  system.setArtificialViscosity(5.0_r);
  system.setTimeIntegration(0.1_r);
  system.setViewConfig(kExampleDirectory, kExampleName);
  system.addViewVariable({SubrosaDG::ViewVariableEnum::Density, SubrosaDG::ViewVariableEnum::Velocity,
                          SubrosaDG::ViewVariableEnum::Pressure, SubrosaDG::ViewVariableEnum::ArtificialViscosity});
  system.synchronize();
  system.solve();
  system.view();
  return EXIT_SUCCESS;
}

void generateMesh(const std::filesystem::path& mesh_file_path) {
  gmsh::model::add("sedovblast_2d");
  gmsh::model::geo::addPoint(-1.5, -1.5, 0.0);
  gmsh::model::geo::addPoint(1.5, -1.5, 0.0);
  gmsh::model::geo::addPoint(1.5, 1.5, 0.0);
  gmsh::model::geo::addPoint(-1.5, 1.5, 0.0);
  gmsh::model::geo::addLine(1, 2);
  gmsh::model::geo::addLine(2, 3);
  gmsh::model::geo::addLine(4, 3);
  gmsh::model::geo::addLine(1, 4);
  gmsh::model::geo::addCurveLoop({1, 2, -3, -4});
  gmsh::model::geo::addPlaneSurface({1});
  gmsh::model::geo::mesh::setTransfiniteCurve(1, 101);
  gmsh::model::geo::mesh::setTransfiniteCurve(2, 101);
  gmsh::model::geo::mesh::setTransfiniteCurve(3, 101);
  gmsh::model::geo::mesh::setTransfiniteCurve(4, 101);
  gmsh::model::geo::mesh::setTransfiniteSurface(1);
  gmsh::model::geo::mesh::setRecombine(2, 1);
  gmsh::model::geo::synchronize();
  gmsh::model::mesh::setTransfiniteAutomatic();
  Eigen::Matrix<double, 4, 4, Eigen::RowMajor> transform_x =
      (Eigen::Transform<double, 3, Eigen::Affine>::Identity() * Eigen::Translation<double, 3>(3, 0, 0)).matrix();
  Eigen::Matrix<double, 4, 4, Eigen::RowMajor> transform_y =
      (Eigen::Transform<double, 3, Eigen::Affine>::Identity() * Eigen::Translation<double, 3>(0, 3, 0)).matrix();
  gmsh::model::mesh::setPeriodic(1, {2}, {4}, {transform_x.data(), transform_x.data() + transform_x.size()});
  gmsh::model::mesh::setPeriodic(1, {3}, {1}, {transform_y.data(), transform_y.data() + transform_y.size()});
  gmsh::model::addPhysicalGroup(1, {1, 2, 3, 4}, 1, "bc-1");
  gmsh::model::addPhysicalGroup(2, {1}, 2, "vc-1");
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::setOrder(SimulationControl::kPolynomialOrder);
  gmsh::model::mesh::optimize("HighOrder");
  gmsh::write(mesh_file_path);
}
