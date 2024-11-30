/**
 * @file periodic_3d_ceuler.cpp
 * @brief The source file for SubrosaDG example periodic_3d_ceuler.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2024-06-11
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#include "SubrosaDG"

inline const std::string kExampleName{"periodic_3d_ceuler"};

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory / "build/out" / kExampleName};

using SimulationControl = SubrosaDG::SimulationControl<
    SubrosaDG::SolveControl<SubrosaDG::DimensionEnum::D3, SubrosaDG::PolynomialOrderEnum::P1,
                            SubrosaDG::BoundaryTimeEnum::Steady, SubrosaDG::SourceTermEnum::None>,
    SubrosaDG::NumericalControl<SubrosaDG::MeshModelEnum::Hexahedron, SubrosaDG::ShockCapturingEnum::None,
                                SubrosaDG::LimiterEnum::None, SubrosaDG::InitialConditionEnum::Function,
                                SubrosaDG::TimeIntegrationEnum::SSPRK3>,
    SubrosaDG::CompresibleEulerVariable<SubrosaDG::ThermodynamicModelEnum::Constant,
                                        SubrosaDG::EquationOfStateEnum::IdealGas, SubrosaDG::ConvectiveFluxEnum::HLLC>>;

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::System<SimulationControl> system;
  system.setMesh(kExampleDirectory / "periodic_3d_ceuler.msh", generateMesh);
  // NOTE: https://arxiv.org/pdf/1704.04549
  system.addInitialCondition([](const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
                                 -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
    return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{
        1.0_r + 0.2_r * std::sin(SubrosaDG::kPi * (coordinate.x() + coordinate.y() + coordinate.z())), 0.5_r, 0.3_r,
        0.2_r, 1.4_r / (1.0_r + 0.2_r * std::sin(SubrosaDG::kPi * (coordinate.x() + coordinate.y() + coordinate.z())))};
  });
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::Periodic>("bc-1");
  system.setThermodynamicModel<SimulationControl::kThermodynamicModel>(2.5_r, 25.0_r / 14.0_r);
  system.setTimeIntegration(1.0_r);
  system.setViewConfig(kExampleDirectory, kExampleName);
  system.addViewVariable({SubrosaDG::ViewVariableEnum::Density, SubrosaDG::ViewVariableEnum::Velocity,
                          SubrosaDG::ViewVariableEnum::Pressure});
  system.synchronize();
  system.solve();
  system.view();
  return EXIT_SUCCESS;
}

void generateMesh(const std::filesystem::path& mesh_file_path) {
  gmsh::model::add("periodic_3d");
  Eigen::Vector<double, 2> point_coordinate;
  // clang-format off
  point_coordinate << 0.0, 2.0;
  // clang-format on
  Eigen::Tensor<int, 3> point_tag(2, 2, 2);
  Eigen::Tensor<int, 3> line_tag(2, 2, 3);
  Eigen::Tensor<int, 2> curve_loop_tag(2, 3);
  Eigen::Tensor<int, 2> surface_filling_tag(2, 3);
  std::vector<int> physical_group_tag;
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      for (std::ptrdiff_t k = 0; k < 2; k++) {
        point_tag(k, j, i) =
            gmsh::model::geo::addPoint(point_coordinate(k), point_coordinate(j), point_coordinate(i), 0.2);
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
  Eigen::Matrix<double, 4, 4, Eigen::RowMajor> transform_x =
      (Eigen::Transform<double, 3, Eigen::Affine>::Identity() * Eigen::Translation<double, 3>(2, 0, 0)).matrix();
  Eigen::Matrix<double, 4, 4, Eigen::RowMajor> transform_y =
      (Eigen::Transform<double, 3, Eigen::Affine>::Identity() * Eigen::Translation<double, 3>(0, 2, 0)).matrix();
  Eigen::Matrix<double, 4, 4, Eigen::RowMajor> transform_z =
      (Eigen::Transform<double, 3, Eigen::Affine>::Identity() * Eigen::Translation<double, 3>(0, 0, 2)).matrix();
  gmsh::model::mesh::setPeriodic(2, {surface_filling_tag(1, 1)}, {surface_filling_tag(0, 1)},
                                 {transform_x.data(), transform_x.data() + transform_x.size()});
  gmsh::model::mesh::setPeriodic(2, {surface_filling_tag(1, 2)}, {surface_filling_tag(0, 2)},
                                 {transform_y.data(), transform_y.data() + transform_y.size()});
  gmsh::model::mesh::setPeriodic(2, {surface_filling_tag(1, 0)}, {surface_filling_tag(0, 0)},
                                 {transform_z.data(), transform_z.data() + transform_z.size()});
  for (std::ptrdiff_t i = 0; i < 3; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      physical_group_tag.emplace_back(surface_filling_tag(j, i));
    }
  }
  gmsh::model::addPhysicalGroup(2, physical_group_tag, -1, "bc-1");
  gmsh::model::addPhysicalGroup(3, {volume_tag}, -1, "vc-1");
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::setOrder(SimulationControl::kPolynomialOrder);
  gmsh::model::mesh::optimize("HighOrder");
  gmsh::write(mesh_file_path);
}
