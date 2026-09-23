/**
 * @file vortex_2d_ceuler.cpp
 * @brief The source file for SubrosaDG example vortex_2d_ceuler.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2026-09-04
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

inline const std::string kExampleName{"vortex_2d_ceuler"};

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory / "build/out" / kExampleName};

using SimulationControl = SubrosaDG::SimulationControl<
    SubrosaDG::SolveControl<SubrosaDG::DimensionEnum::D2, SubrosaDG::PolynomialOrderEnum::P1,
                            SubrosaDG::BoundaryTimeEnum::TimeVarying, SubrosaDG::SourceTermEnum::None>,
    SubrosaDG::NumericalControl<SubrosaDG::MeshModelEnum::TriangleQuadrangle, SubrosaDG::InitialConditionEnum::Function,
                                SubrosaDG::TimeIntegrationEnum::SSPRK3>,
    SubrosaDG::CompressibleEulerVariable<SubrosaDG::ThermodynamicModelEnum::Constant,
                                         SubrosaDG::EquationOfStateEnum::IdealGas,
                                         SubrosaDG::ConvectiveFluxEnum::HLLC>>;

template <typename SimulationControl>
inline void SubrosaDG::InitialCondition<SimulationControl>::computePrimitiveFromCoordinate(
    const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate,
    Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>& initial_primitive_variable) {
  initial_primitive_variable = {
      1.4_r *
          std::pow(1.0_r - (1.4_r - 1.0_r) * std::pow((1.0_r * 0.3_r), 2.0_r) / 2.0_r *
                               std::exp((1.0_r - coordinate.x() * coordinate.x() - coordinate.y() * coordinate.y()) /
                                        (1.0_r * 1.0_r)),
                   1.0_r / (1.4_r - 1.0_r)),
      0.3_r * (1.0_r - 1.0_r * coordinate.y() / 1.0_r *
                           std::exp((1.0_r - coordinate.x() * coordinate.x() - coordinate.y() * coordinate.y()) /
                                    (2.0_r * 1.0_r * 1.0_r))),
      0.3_r * (1.0_r * coordinate.x() / 1.0_r *
               std::exp((1.0_r - coordinate.x() * coordinate.x() - coordinate.y() * coordinate.y()) /
                        (2.0_r * 1.0_r * 1.0_r))),
      std::pow(1.0_r - (1.4_r - 1.0_r) * std::pow((1.0_r * 0.3_r), 2.0_r) / 2.0_r *
                           std::exp((1.0_r - coordinate.x() * coordinate.x() - coordinate.y() * coordinate.y()) /
                                    (1.0_r * 1.0_r)),
               1.4_r)};
}

template <typename SimulationControl>
inline void SubrosaDG::BoundaryCondition<SimulationControl>::computePrimitiveFromCoordinate(
    const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate,
    Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>& boundary_primitive_variable,
    const Real time, const SubrosaDG::Isize gmsh_physical_index) {
  Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension> relative_coordinate;
  relative_coordinate = coordinate - Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>{0.3_r * time, 0.0_r};
  if (gmsh_physical_index == 1) {
    boundary_primitive_variable = {
        1.4_r * std::pow(1.0_r - (1.4_r - 1.0_r) * std::pow((1.0_r * 0.3_r), 2.0_r) / 2.0_r *
                                     std::exp((1.0_r - relative_coordinate.x() * relative_coordinate.x() -
                                               relative_coordinate.y() * relative_coordinate.y()) /
                                              (1.0_r * 1.0_r)),
                         1.0_r / (1.4_r - 1.0_r)),
        0.3_r * (1.0_r - 1.0_r * relative_coordinate.y() / 1.0_r *
                             std::exp((1.0_r - relative_coordinate.x() * relative_coordinate.x() -
                                       relative_coordinate.y() * relative_coordinate.y()) /
                                      (2.0_r * 1.0_r * 1.0_r))),
        0.3_r * (1.0_r * relative_coordinate.x() / 1.0_r *
                 std::exp((1.0_r - relative_coordinate.x() * relative_coordinate.x() -
                           relative_coordinate.y() * relative_coordinate.y()) /
                          (2.0_r * 1.0_r * 1.0_r))),
        std::pow(1.0_r - (1.4_r - 1.0_r) * std::pow((1.0_r * 0.3_r), 2.0_r) / 2.0_r *
                             std::exp((1.0_r - relative_coordinate.x() * relative_coordinate.x() -
                                       relative_coordinate.y() * relative_coordinate.y()) /
                                      (1.0_r * 1.0_r)),
                 1.4_r)};
  }
}

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::System<SimulationControl> system;
  system.setMesh(kExampleDirectory / std::format("{}.msh", kExampleName), generateMesh);
  system.setRotation(Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>{0.0_r, 0.0_r},
                     Eigen::Vector<SubrosaDG::Real, 3>{0.0_r, 0.0_r, 1.0_r});
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::RiemannFarfield>(1);
  system.template addVolumeCondition<SubrosaDG::InteriorConditionEnum::Static>(2);
  system.template addVolumeCondition<SubrosaDG::InteriorConditionEnum::Rotate>(3);
  system.setTimeIntegration(1.0_r, {0, 1000}, 1e-2_r);
  system.setViewConfig(kExampleDirectory, kExampleName, 10);
  system.addViewVariable({SubrosaDG::ViewVariableEnum::Density, SubrosaDG::ViewVariableEnum::Velocity,
                          SubrosaDG::ViewVariableEnum::Pressure});
  system.synchronize();
  system.solve();
  system.view();
  return EXIT_SUCCESS;
}

void generateMesh(const std::filesystem::path& mesh_file_path) {
  Eigen::Matrix<double, 4, 3, Eigen::RowMajor> farfield_point_coordinate;
  Eigen::Matrix<double, 4, 3, Eigen::RowMajor> interface_point_coordinate;
  Eigen::Matrix<double, 4, 3, Eigen::RowMajor> square_point_coordinate;
  // clang-format off
  farfield_point_coordinate << -5.0, -5.0, 0.0,
                                5.0, -5.0, 0.0,
                                5.0,  5.0, 0.0,
                               -5.0,  5.0, 0.0;
  interface_point_coordinate << -2.0,  0.0, 0.0,
                                 0.0, -2.0, 0.0,
                                 2.0,  0.0, 0.0,
                                 0.0,  2.0, 0.0;
  // clang-format on
  Eigen::Array<int, 4, 2, Eigen::RowMajor> point_tag;
  Eigen::Array<int, 4, 2, Eigen::RowMajor> line_tag;
  Eigen::Array<int, 2, 1> curve_loop_tag;
  Eigen::Array<int, 2, 1> plane_surface_tag;
  std::array<std::vector<int>, 3> physical_group_tag;
  gmsh::model::add("square_2d");
  const int center_point_tag = gmsh::model::geo::addPoint(0.0, 0.0, 0.0);
  for (int i = 0; i < 4; i++) {
    point_tag(i, 0) = gmsh::model::geo::addPoint(farfield_point_coordinate(i, 0), farfield_point_coordinate(i, 1),
                                                 farfield_point_coordinate(i, 2), 3.0);
    point_tag(i, 1) = gmsh::model::geo::addPoint(interface_point_coordinate(i, 0), interface_point_coordinate(i, 1),
                                                 interface_point_coordinate(i, 2));
  }
  for (int i = 0; i < 4; i++) {
    line_tag(i, 0) = gmsh::model::geo::addLine(point_tag(i, 0), point_tag((i + 1) % 4, 0));
    line_tag(i, 1) = gmsh::model::geo::addCircleArc(point_tag(i, 1), center_point_tag, point_tag((i + 1) % 4, 1));
  }
  for (int i = 0; i < 2; i++) {
    curve_loop_tag(i, 0) =
        gmsh::model::geo::addCurveLoop({line_tag(0, i), line_tag(1, i), line_tag(2, i), line_tag(3, i)});
  }
  plane_surface_tag(0, 0) = gmsh::model::geo::addPlaneSurface({curve_loop_tag(0, 0), -curve_loop_tag(1, 0)});
  plane_surface_tag(1, 0) = gmsh::model::geo::addPlaneSurface({curve_loop_tag(1, 0)});
  for (int i = 0; i < 4; i++) {
    gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(i, 0), 16);
    gmsh::model::geo::mesh::setTransfiniteCurve(line_tag(i, 1), 16);
  }
  gmsh::model::geo::mesh::setRecombine(2, plane_surface_tag(1, 0));
  gmsh::model::geo::synchronize();
  for (int i = 0; i < 4; i++) {
    physical_group_tag[0].emplace_back(line_tag(i, 0));
  }
  physical_group_tag[1].emplace_back(plane_surface_tag(0, 0));
  physical_group_tag[2].emplace_back(plane_surface_tag(1, 0));
  gmsh::model::addPhysicalGroup(1, physical_group_tag[0], 1, "bc-1");
  gmsh::model::addPhysicalGroup(2, physical_group_tag[1], 2, "vc-1");
  gmsh::model::addPhysicalGroup(2, physical_group_tag[2], 3, "rc-1");
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::setOrder(SimulationControl::kPolynomialOrder);
  gmsh::model::mesh::optimize("HighOrder");
  gmsh::write(mesh_file_path);
}
