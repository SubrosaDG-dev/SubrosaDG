/**
 * @file naca0010_2d_incns.cpp
 * @brief The source file for SubrosaDG example naca0010_2d_incns.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2024-11-24
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#include "SubrosaDG"

inline const std::string kExampleName{"naca0010_2d_incns"};

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
  system.setMesh(kExampleDirectory / "naca0010_2d_incns.msh", generateMesh);
  system.addInitialCondition(
      []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
          -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
        return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{
            1.0_r, 0.2_r * std::cos(30.0_deg), 0.2_r * std::sin(30.0_deg), 1.0_r};
      });
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::RiemannFarfield>(
      "bc-1",
      []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
          -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
        return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{
            1.0_r, 0.2_r * std::cos(30.0_deg), 0.2_r * std::sin(30.0_deg), 1.0_r};
      });
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticNonSlipWall>(
      "bc-2",
      []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
          -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
        return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{1.0_r, 0.0_r, 0.0_r, 1.0_r};
      });
  system.setThermodynamicModel<SimulationControl::kThermodynamicModel>(1.0_r, 1.0_r);
  system.setEquationOfState<SimulationControl::kEquationOfState>(10.0_r, 1.0_r);
  system.setTransportModel<SimulationControl::kTransportModel>(1.0_r * 0.2_r / 1400.0_r);
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

inline std::array<double, 99> naca0010_point_x_array{
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

inline std::array<double, 99> naca0010_point_y_array{
    0.002316, 0.004601, 0.006852, 0.009070, 0.011253, 0.013399, 0.015506, 0.017573, 0.019598, 0.021578, 0.023511,
    0.025394, 0.027225, 0.029002, 0.030722, 0.032382, 0.033980, 0.035513, 0.036978, 0.038374, 0.039698, 0.040948,
    0.042122, 0.043218, 0.044236, 0.045172, 0.046027, 0.046799, 0.047488, 0.048093, 0.048615, 0.049052, 0.049407,
    0.049678, 0.049868, 0.049976, 0.050005, 0.049956, 0.049831, 0.049631, 0.049358, 0.049015, 0.048605, 0.048129,
    0.047590, 0.046991, 0.046335, 0.045625, 0.044862, 0.044051, 0.043194, 0.042295, 0.041355, 0.040379, 0.039368,
    0.038327, 0.037257, 0.036161, 0.035043, 0.033905, 0.032750, 0.031580, 0.030398, 0.029207, 0.028009, 0.026806,
    0.025602, 0.024399, 0.023199, 0.022004, 0.020817, 0.019641, 0.018478, 0.017329, 0.016199, 0.015088, 0.014000,
    0.012936, 0.011900, 0.010892, 0.009917, 0.008975, 0.008070, 0.007202, 0.006376, 0.005591, 0.004852, 0.004158,
    0.003513, 0.002918, 0.002374, 0.001883, 0.001447, 0.001067, 0.000743, 0.000476, 0.000268, 0.000119, 0.000030,
};

void generateMesh(const std::filesystem::path& mesh_file_path) {
  Eigen::Matrix<double, 2, 99, Eigen::RowMajor> naca0010_point;
  naca0010_point.row(0) = Eigen::Map<Eigen::Matrix<double, 1, 99, Eigen::RowMajor>>(naca0010_point_x_array.data());
  naca0010_point.row(1) = Eigen::Map<Eigen::Matrix<double, 1, 99, Eigen::RowMajor>>(naca0010_point_y_array.data());
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
  std::array<std::vector<int>, 2> naca0010_point_tag;
  Eigen::Array<int, 6, 1> farfield_line_tag;
  Eigen::Array<int, 2, 1> naca0010_line_tag;
  Eigen::Array<int, 4, 1> connection_line_tag;
  Eigen::Array<int, 4, 1> curve_loop_tag;
  Eigen::Array<int, 4, 1> plane_surface_tag;
  std::array<std::vector<int>, 3> physical_group_tag;
  gmsh::model::add("naca0010_2d");
  const int naca0010_leading_edge_point_tag = gmsh::model::geo::addPoint(0.0, 0.0, 0.0);
  const int naca0010_trailing_edge_point_tag = gmsh::model::geo::addPoint(1.0, 0.0, 0.0);
  for (std::ptrdiff_t i = 0; i < 6; i++) {
    farfield_point_tag(i) = gmsh::model::geo::addPoint(farfield_point_coordinate(i, 0), farfield_point_coordinate(i, 1),
                                                       farfield_point_coordinate(i, 2));
  }
  for (std::size_t i = 0; i < 2; i++) {
    naca0010_point_tag[i].emplace_back(naca0010_leading_edge_point_tag);
  }
  for (std::ptrdiff_t i = 0; i < 99; i++) {
    naca0010_point_tag[0].emplace_back(gmsh::model::geo::addPoint(naca0010_point(0, i), naca0010_point(1, i), 0.0));
    naca0010_point_tag[1].emplace_back(gmsh::model::geo::addPoint(naca0010_point(0, i), -naca0010_point(1, i), 0.0));
  }
  for (std::size_t i = 0; i < 2; i++) {
    naca0010_point_tag[i].emplace_back(naca0010_trailing_edge_point_tag);
  }
  for (std::ptrdiff_t i = 0; i < 6; i++) {
    if (i < 2) {
      farfield_line_tag(i) = gmsh::model::geo::addCircleArc(farfield_point_tag(i), naca0010_trailing_edge_point_tag,
                                                            farfield_point_tag(i + 1));
    } else {
      farfield_line_tag(i) = gmsh::model::geo::addLine(farfield_point_tag(i), farfield_point_tag((i + 1) % 6));
    }
  }
  connection_line_tag(0) = gmsh::model::geo::addLine(farfield_point_tag(0), naca0010_trailing_edge_point_tag);
  connection_line_tag(1) = gmsh::model::geo::addLine(farfield_point_tag(1), naca0010_leading_edge_point_tag);
  connection_line_tag(2) = gmsh::model::geo::addLine(farfield_point_tag(2), naca0010_trailing_edge_point_tag);
  connection_line_tag(3) = gmsh::model::geo::addLine(farfield_point_tag(4), naca0010_trailing_edge_point_tag);
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    naca0010_line_tag(i) = gmsh::model::geo::addSpline(naca0010_point_tag[static_cast<std::size_t>(i)]);
  }
  curve_loop_tag(0) = gmsh::model::geo::addCurveLoop(
      {-connection_line_tag(0), farfield_line_tag(0), connection_line_tag(1), naca0010_line_tag(0)});
  curve_loop_tag(1) = gmsh::model::geo::addCurveLoop(
      {-connection_line_tag(1), farfield_line_tag(1), connection_line_tag(2), -naca0010_line_tag(1)});
  curve_loop_tag(2) = gmsh::model::geo::addCurveLoop(
      {-connection_line_tag(2), farfield_line_tag(2), farfield_line_tag(3), connection_line_tag(3)});
  curve_loop_tag(3) = gmsh::model::geo::addCurveLoop(
      {-connection_line_tag(3), farfield_line_tag(4), farfield_line_tag(5), connection_line_tag(0)});
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    plane_surface_tag(i) = gmsh::model::geo::addPlaneSurface({curve_loop_tag(i)});
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    gmsh::model::geo::mesh::setTransfiniteCurve(naca0010_line_tag(i), 60, "Bump", 0.20);
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
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
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    gmsh::model::geo::mesh::setTransfiniteSurface(plane_surface_tag(i));
    gmsh::model::geo::mesh::setRecombine(2, plane_surface_tag(i));
  }
  gmsh::model::geo::synchronize();
  for (std::ptrdiff_t i = 0; i < 6; i++) {
    physical_group_tag[0].emplace_back(farfield_line_tag(i));
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    physical_group_tag[1].emplace_back(naca0010_line_tag(i));
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    physical_group_tag[2].emplace_back(plane_surface_tag(i));
  }
  gmsh::model::addPhysicalGroup(1, physical_group_tag[0], -1, "bc-1");
  gmsh::model::addPhysicalGroup(1, physical_group_tag[1], -1, "bc-2");
  gmsh::model::addPhysicalGroup(2, physical_group_tag[2], -1, "vc-1");
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::setOrder(SimulationControl::kPolynomialOrder);
  gmsh::model::mesh::optimize("HighOrder");
  gmsh::write(mesh_file_path);
}
