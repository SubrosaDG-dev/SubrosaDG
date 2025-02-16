/**
 * @file naca0012_2d_cns.cpp
 * @brief The source file for SubrosaDG example naca0012_2d_cns.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2024-05-08
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2025 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#include "SubrosaDG.cpp"

inline const std::string kExampleName{"naca0012_2d_cns"};

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory / "build/out" / kExampleName};

using SimulationControl = SubrosaDG::SimulationControl<
    SubrosaDG::SolveControl<SubrosaDG::DimensionEnum::D2, SubrosaDG::PolynomialOrderEnum::P3,
                            SubrosaDG::BoundaryTimeEnum::Steady, SubrosaDG::SourceTermEnum::None>,
    SubrosaDG::NumericalControl<SubrosaDG::MeshModelEnum::Quadrangle, SubrosaDG::ShockCapturingEnum::None,
                                SubrosaDG::LimiterEnum::None, SubrosaDG::InitialConditionEnum::Function,
                                SubrosaDG::TimeIntegrationEnum::SSPRK3>,
    SubrosaDG::CompresibleNSVariable<SubrosaDG::ThermodynamicModelEnum::Constant,
                                     SubrosaDG::EquationOfStateEnum::IdealGas,
                                     SubrosaDG::TransportModelEnum::Sutherland, SubrosaDG::ConvectiveFluxEnum::HLLC,
                                     SubrosaDG::ViscousFluxEnum::BR2>>;

template <typename SimulationControl>
inline Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>
SubrosaDG::InitialCondition<SimulationControl>::calculatePrimitiveFromCoordinate(
    [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& coordinate) const {
  // NOTE: https://arxiv.org/pdf/1704.04549
  return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{1.4_r, 0.2_r * std::cos(30.0_deg),
                                                                                     0.2_r * std::sin(30.0_deg), 1.0_r};
}

template <typename SimulationControl>
inline Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>
SubrosaDG::BoundaryCondition<SimulationControl>::calculatePrimitiveFromCoordinate(
    [[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate,
    const SubrosaDG::Isize gmsh_physical_index) const {
  if (gmsh_physical_index == 1) {
    return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{
        1.4_r, 0.2_r * std::cos(30.0_deg), 0.2_r * std::sin(30.0_deg), 1.0_r};
  }
  if (gmsh_physical_index == 2) {
    return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{1.4_r, 0.0_r, 0.0_r, 1.0_r};
  }
  return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>::Zero();
}

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::System<SimulationControl> system;
  system.setMesh(kExampleDirectory / "naca0012_2d_cns.msh", generateMesh);
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::RiemannFarfield>(1);
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticNonSlipWall>(2);
  system.setThermodynamicModel<SimulationControl::kThermodynamicModel>(2.5_r, 25.0_r / 14.0_r);
  system.setTransportModel<SimulationControl::kTransportModel>(1.4_r * 0.2_r / 16000.0_r);
  system.setTimeIntegration(0.5_r);
  system.setViewConfig(kExampleDirectory, kExampleName);
  system.addViewVariable({SubrosaDG::ViewVariableEnum::Density, SubrosaDG::ViewVariableEnum::Velocity,
                          SubrosaDG::ViewVariableEnum::Pressure, SubrosaDG::ViewVariableEnum::Temperature,
                          SubrosaDG::ViewVariableEnum::MachNumber, SubrosaDG::ViewVariableEnum::Vorticity});
  system.synchronize();
  system.solve();
  system.view();
  return EXIT_SUCCESS;
}

inline std::array<double, 99> naca0012_point_x_array{
    0.000247, 0.000987, 0.002219, 0.003943, 0.006156, 0.008856, 0.012042, 0.015708, 0.019853, 0.024472, 0.029560,
    0.035112, 0.041123, 0.047586, 0.054497, 0.061847, 0.069629, 0.077836, 0.086460, 0.095492, 0.104922, 0.114743,
    0.124944, 0.135516, 0.146447, 0.157726, 0.169344, 0.181288, 0.193546, 0.206107, 0.218958, 0.232087, 0.245479,
    0.259123, 0.273005, 0.287110, 0.301426, 0.315938, 0.330631, 0.345492, 0.360504, 0.375655, 0.390928, 0.406309,
    0.421783, 0.437333, 0.452946, 0.468605, 0.484295, 0.500000, 0.515705, 0.531395, 0.547054, 0.562667, 0.578217,
    0.593691, 0.609072, 0.624345, 0.639496, 0.654508, 0.669369, 0.684062, 0.698574, 0.712890, 0.726995, 0.740877,
    0.754521, 0.767913, 0.781042, 0.793893, 0.806454, 0.818712, 0.830656, 0.842274, 0.853553, 0.864484, 0.875056,
    0.885257, 0.895078, 0.904508, 0.913540, 0.922164, 0.930371, 0.938153, 0.945503, 0.952414, 0.958877, 0.964888,
    0.970440, 0.975528, 0.980147, 0.984292, 0.987958, 0.991144, 0.993844, 0.996057, 0.997781, 0.999013, 0.999753,
};

inline std::array<double, 99> naca0012_point_y_array{
    0.002779, 0.005521, 0.008223, 0.010884, 0.013503, 0.016078, 0.018607, 0.021088, 0.023517, 0.025893, 0.028213,
    0.030473, 0.032671, 0.034803, 0.036867, 0.038859, 0.040776, 0.042615, 0.044374, 0.046049, 0.047638, 0.049138,
    0.050546, 0.051862, 0.053083, 0.054206, 0.055232, 0.056159, 0.056986, 0.057712, 0.058338, 0.058863, 0.059288,
    0.059614, 0.059841, 0.059971, 0.060006, 0.059947, 0.059797, 0.059557, 0.059230, 0.058819, 0.058326, 0.057755,
    0.057108, 0.056390, 0.055602, 0.054749, 0.053835, 0.052862, 0.051833, 0.050754, 0.049626, 0.048455, 0.047242,
    0.045992, 0.044708, 0.043394, 0.042052, 0.040686, 0.039300, 0.037896, 0.036478, 0.035048, 0.033610, 0.032168,
    0.030723, 0.029279, 0.027838, 0.026405, 0.024981, 0.023569, 0.022173, 0.020795, 0.019438, 0.018106, 0.016800,
    0.015523, 0.014280, 0.013071, 0.011900, 0.010770, 0.009684, 0.008643, 0.007651, 0.006710, 0.005822, 0.004990,
    0.004216, 0.003501, 0.002849, 0.002260, 0.001737, 0.001280, 0.000891, 0.000572, 0.000322, 0.000143, 0.000036,
};

void generateMesh(const std::filesystem::path& mesh_file_path) {
  Eigen::Matrix<double, 2, 99, Eigen::RowMajor> naca0012_point;
  naca0012_point.row(0) = Eigen::Map<Eigen::Matrix<double, 1, 99, Eigen::RowMajor>>(naca0012_point_x_array.data());
  naca0012_point.row(1) = Eigen::Map<Eigen::Matrix<double, 1, 99, Eigen::RowMajor>>(naca0012_point_y_array.data());
  Eigen::Matrix<double, 6, 3, Eigen::RowMajor> farfield_point_coordinate;
  // clang-format off
  farfield_point_coordinate <<  1.0,   10.0, 0.0,
                               -9.0,   0.0,  0.0,
                                1.0,  -10.0, 0.0,
                                10.0, -10.0, 0.0,
                                10.0,  0.0,  0.0,
                                10.0,  10.0, 0.0;
  // clang-format on
  Eigen::Array<int, 6, 1> farfield_point_tag;
  std::array<std::vector<int>, 2> naca0012_point_tag;
  Eigen::Array<int, 6, 1> farfield_line_tag;
  Eigen::Array<int, 2, 1> naca0012_line_tag;
  Eigen::Array<int, 4, 1> connection_line_tag;
  Eigen::Array<int, 4, 1> curve_loop_tag;
  Eigen::Array<int, 4, 1> plane_surface_tag;
  std::array<std::vector<int>, 3> physical_group_tag;
  gmsh::model::add("naca0012_2d");
  const int naca0012_leading_edge_point_tag = gmsh::model::geo::addPoint(0.0, 0.0, 0.0);
  const int naca0012_trailing_edge_point_tag = gmsh::model::geo::addPoint(1.0, 0.0, 0.0);
  for (int i = 0; i < 6; i++) {
    farfield_point_tag(i) = gmsh::model::geo::addPoint(farfield_point_coordinate(i, 0), farfield_point_coordinate(i, 1),
                                                       farfield_point_coordinate(i, 2));
  }
  for (std::size_t i = 0; i < 2; i++) {
    naca0012_point_tag[i].emplace_back(naca0012_leading_edge_point_tag);
  }
  for (int i = 0; i < 99; i++) {
    naca0012_point_tag[0].emplace_back(gmsh::model::geo::addPoint(naca0012_point(0, i), naca0012_point(1, i), 0.0));
    naca0012_point_tag[1].emplace_back(gmsh::model::geo::addPoint(naca0012_point(0, i), -naca0012_point(1, i), 0.0));
  }
  for (std::size_t i = 0; i < 2; i++) {
    naca0012_point_tag[i].emplace_back(naca0012_trailing_edge_point_tag);
  }
  for (int i = 0; i < 6; i++) {
    if (i < 2) {
      farfield_line_tag(i) = gmsh::model::geo::addCircleArc(farfield_point_tag(i), naca0012_trailing_edge_point_tag,
                                                            farfield_point_tag(i + 1));
    } else {
      farfield_line_tag(i) = gmsh::model::geo::addLine(farfield_point_tag(i), farfield_point_tag((i + 1) % 6));
    }
  }
  connection_line_tag(0) = gmsh::model::geo::addLine(farfield_point_tag(0), naca0012_trailing_edge_point_tag);
  connection_line_tag(1) = gmsh::model::geo::addLine(farfield_point_tag(1), naca0012_leading_edge_point_tag);
  connection_line_tag(2) = gmsh::model::geo::addLine(farfield_point_tag(2), naca0012_trailing_edge_point_tag);
  connection_line_tag(3) = gmsh::model::geo::addLine(farfield_point_tag(4), naca0012_trailing_edge_point_tag);
  for (int i = 0; i < 2; i++) {
    naca0012_line_tag(i) = gmsh::model::geo::addSpline(naca0012_point_tag[static_cast<std::size_t>(i)]);
  }
  curve_loop_tag(0) = gmsh::model::geo::addCurveLoop(
      {-connection_line_tag(0), farfield_line_tag(0), connection_line_tag(1), naca0012_line_tag(0)});
  curve_loop_tag(1) = gmsh::model::geo::addCurveLoop(
      {-connection_line_tag(1), farfield_line_tag(1), connection_line_tag(2), -naca0012_line_tag(1)});
  curve_loop_tag(2) = gmsh::model::geo::addCurveLoop(
      {-connection_line_tag(2), farfield_line_tag(2), farfield_line_tag(3), connection_line_tag(3)});
  curve_loop_tag(3) = gmsh::model::geo::addCurveLoop(
      {-connection_line_tag(3), farfield_line_tag(4), farfield_line_tag(5), connection_line_tag(0)});
  for (int i = 0; i < 4; i++) {
    plane_surface_tag(i) = gmsh::model::geo::addPlaneSurface({curve_loop_tag(i)});
  }
  for (int i = 0; i < 2; i++) {
    gmsh::model::geo::mesh::setTransfiniteCurve(naca0012_line_tag(i), 60, "Bump", 0.20);
  }
  for (int i = 0; i < 2; i++) {
    gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag(i), 60);
  }
  gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag(2), 40, "Progression", 1.15);
  gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag(3), 40, "Progression", -1.2);
  gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag(4), 40, "Progression", 1.2);
  gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag(5), 40, "Progression", -1.15);
  gmsh::model::geo::mesh::setTransfiniteCurve(connection_line_tag(0), 40, "Progression", -1.2);
  gmsh::model::geo::mesh::setTransfiniteCurve(connection_line_tag(1), 40, "Progression", -1.2);
  gmsh::model::geo::mesh::setTransfiniteCurve(connection_line_tag(2), 40, "Progression", -1.2);
  gmsh::model::geo::mesh::setTransfiniteCurve(connection_line_tag(3), 40, "Progression", -1.15);
  for (int i = 0; i < 4; i++) {
    gmsh::model::geo::mesh::setTransfiniteSurface(plane_surface_tag(i));
    gmsh::model::geo::mesh::setRecombine(2, plane_surface_tag(i));
  }
  gmsh::model::geo::synchronize();
  for (int i = 0; i < 6; i++) {
    physical_group_tag[0].emplace_back(farfield_line_tag(i));
  }
  for (int i = 0; i < 2; i++) {
    physical_group_tag[1].emplace_back(naca0012_line_tag(i));
  }
  for (int i = 0; i < 4; i++) {
    physical_group_tag[2].emplace_back(plane_surface_tag(i));
  }
  gmsh::model::addPhysicalGroup(1, physical_group_tag[0], 1, "bc-1");
  gmsh::model::addPhysicalGroup(1, physical_group_tag[1], 2, "bc-2");
  gmsh::model::addPhysicalGroup(2, physical_group_tag[2], 3, "vc-1");
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::setOrder(SimulationControl::kPolynomialOrder);
  gmsh::model::mesh::optimize("HighOrder");
  gmsh::write(mesh_file_path);
}
