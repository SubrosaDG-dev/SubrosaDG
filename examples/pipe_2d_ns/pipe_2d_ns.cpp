/**
 * @file pipe_2d_ns.cpp
 * @brief The main file of SubrosaDG pipe_2d_ns.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2024-05-28
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#include "SubrosaDG"

inline const std::string kExampleName{"pipe_2d_ns"};

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory / "build/out" / kExampleName};

using SimulationControl = SubrosaDG::SimulationControlNavierStokes<
    SubrosaDG::DimensionEnum::D2, SubrosaDG::PolynomialOrderEnum::P1, SubrosaDG::MeshModelEnum::Quadrangle,
    SubrosaDG::SourceTermEnum::None, SubrosaDG::InitialConditionEnum::Function, SubrosaDG::PolynomialOrderEnum::P1,
    SubrosaDG::ThermodynamicModelEnum::ConstantE, SubrosaDG::EquationOfStateEnum::IdealGas,
    SubrosaDG::TransportModelEnum::Constant, SubrosaDG::ConvectiveFluxEnum::HLLC, SubrosaDG::ViscousFluxEnum::BR1,
    SubrosaDG::TimeIntegrationEnum::SSPRK3>;

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::System<SimulationControl> system;
  system.setMesh(kExampleDirectory / std::format("{}.msh", kExampleName), generateMesh);
  system.addInitialCondition(
      []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate) {
        return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{
            1.4, 4.0 * 0.3 * coordinate.y() * (0.4 - coordinate.y()) / (0.4 * 0.4), 0.0, 1.0};
      });
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::VelocityInflow>(
      "bc-1", []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate) {
        return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{
            1.4, 4.0 * 0.3 * coordinate.y() * (0.4 - coordinate.y()) / (0.4 * 0.4), 0.0, 1.0};
      });
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::PressureOutflow>(
      "bc-2", []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate) {
        return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{
            1.4, 4.0 * 0.3 * coordinate.y() * (0.4 - coordinate.y()) / (0.4 * 0.4), 0.0, 1.0};
      });
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticNoSlipWall>("bc-3");
  system.setTransportModel(1.4 * 0.2 * 0.1 / 200);
  system.setTimeIntegration(1.0);
  system.setViewConfig(kExampleDirectory, kExampleName);
  system.addViewVariable({SubrosaDG::ViewVariableEnum::Density, SubrosaDG::ViewVariableEnum::Velocity,
                          SubrosaDG::ViewVariableEnum::Pressure, SubrosaDG::ViewVariableEnum::Temperature,
                          SubrosaDG::ViewVariableEnum::MachNumber, SubrosaDG::ViewVariableEnum::Vorticity});
  system.synchronize();
  system.solve();
  system.view();
  return EXIT_SUCCESS;
}

void generateMesh(const std::filesystem::path& mesh_file_path) {
  const double x = 0.05 / std::sqrt(2.0);
  Eigen::Matrix<double, 2, 4, Eigen::RowMajor> farfield_point_coordinate;
  Eigen::Vector<double, 2> cylinder_point_coordinate;
  // clang-format off
  farfield_point_coordinate << 0.0, 0.2 - 2 * x, 0.2 + 2 * x, 2.2,
                               0.0, 0.2 - 2 * x, 0.2 + 2 * x, 0.4;
  cylinder_point_coordinate << 0.2 - x, 0.2 + x;
  // clang-format on
  Eigen::Tensor<int, 2> farfield_point_tag(4, 4);
  Eigen::Tensor<int, 2> cylinder_point_tag(2, 2);
  Eigen::Tensor<int, 3> farfield_line_tag(3, 4, 2);
  Eigen::Tensor<int, 3> cylinder_line_tag(1, 2, 2);
  Eigen::Tensor<int, 2> connection_line_tag(2, 2);
  Eigen::Tensor<int, 2> farfield_curve_loop_tag(3, 3);
  Eigen::Tensor<int, 2> cylinder_curve_loop_tag(2, 2);
  Eigen::Tensor<int, 2> farfield_plane_surface_tag(3, 3);
  Eigen::Tensor<int, 2> cylinder_plane_surface_tag(2, 2);
  std::array<std::vector<int>, 4> physical_group_tag;
  gmsh::model::add("pipe_2d");
  const int center_point_tag = gmsh::model::geo::addPoint(0.2, 0.2, 0.0);
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 4; j++) {
      farfield_point_tag(j, i) =
          gmsh::model::geo::addPoint(farfield_point_coordinate(0, j), farfield_point_coordinate(1, i), 0.0);
    }
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      cylinder_point_tag(j, i) =
          gmsh::model::geo::addPoint(cylinder_point_coordinate(j), cylinder_point_coordinate(i), 0.0);
    }
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 3; j++) {
      if ((i == 1 || i == 2) && (j == 1)) {
        farfield_line_tag(j, i, 0) =
            gmsh::model::geo::addCircleArc(farfield_point_tag(j, i), center_point_tag, farfield_point_tag(j + 1, i));
        farfield_line_tag(j, i, 1) =
            gmsh::model::geo::addCircleArc(farfield_point_tag(i, j), center_point_tag, farfield_point_tag(i, j + 1));
      } else {
        farfield_line_tag(j, i, 0) = gmsh::model::geo::addLine(farfield_point_tag(j, i), farfield_point_tag(j + 1, i));
        farfield_line_tag(j, i, 1) = gmsh::model::geo::addLine(farfield_point_tag(i, j), farfield_point_tag(i, j + 1));
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    for (std::ptrdiff_t j = 0; j < 1; j++) {
      cylinder_line_tag(j, i, 0) =
          gmsh::model::geo::addCircleArc(cylinder_point_tag(j, i), center_point_tag, cylinder_point_tag(j + 1, i));
      cylinder_line_tag(j, i, 1) =
          gmsh::model::geo::addCircleArc(cylinder_point_tag(i, j), center_point_tag, cylinder_point_tag(i, j + 1));
    }
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      connection_line_tag(j, i) = gmsh::model::geo::addLine(cylinder_point_tag(j, i), farfield_point_tag(j + 1, i + 1));
    }
  }
  for (std::ptrdiff_t i = 0; i < 3; i++) {
    for (std::ptrdiff_t j = 0; j < 3; j++) {
      if (i == 1 && j == 1) {
        continue;
      }
      farfield_curve_loop_tag(j, i) =
          gmsh::model::geo::addCurveLoop({farfield_line_tag(j, i, 0), farfield_line_tag(i, j + 1, 1),
                                          -farfield_line_tag(j, i + 1, 0), -farfield_line_tag(i, j, 1)});
    }
  }
  cylinder_curve_loop_tag(0, 0) = gmsh::model::geo::addCurveLoop(
      {connection_line_tag(0, 0), farfield_line_tag(1, 1, 0), -connection_line_tag(1, 0), -cylinder_line_tag(0, 0, 0)});
  cylinder_curve_loop_tag(1, 0) = gmsh::model::geo::addCurveLoop(
      {-connection_line_tag(0, 1), cylinder_line_tag(0, 1, 0), connection_line_tag(1, 1), -farfield_line_tag(1, 2, 0)});
  cylinder_curve_loop_tag(0, 1) = gmsh::model::geo::addCurveLoop(
      {-connection_line_tag(0, 0), cylinder_line_tag(0, 0, 1), connection_line_tag(0, 1), -farfield_line_tag(1, 1, 1)});
  cylinder_curve_loop_tag(1, 1) = gmsh::model::geo::addCurveLoop(
      {connection_line_tag(1, 0), farfield_line_tag(1, 2, 1), -connection_line_tag(1, 1), -cylinder_line_tag(0, 1, 1)});
  for (std::ptrdiff_t i = 0; i < 3; i++) {
    for (std::ptrdiff_t j = 0; j < 3; j++) {
      if (i == 1 && j == 1) {
        continue;
      }
      farfield_plane_surface_tag(j, i) = gmsh::model::geo::addPlaneSurface({farfield_curve_loop_tag(j, i)});
    }
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      cylinder_plane_surface_tag(j, i) = gmsh::model::geo::addPlaneSurface({cylinder_curve_loop_tag(j, i)});
    }
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 3; j++) {
      switch (j) {
      case 0:
        gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag(j, i, 0), 10);
        gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag(j, i, 1), 12, "Progression", 1.1);
        break;
      case 1:
        gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag(j, i, 0), 12);
        gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag(j, i, 1), 12);
        break;
      case 2:
        gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag(j, i, 0), 50, "Progression", 1.04);
        gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag(j, i, 1), 12, "Progression", -1.1);
        break;
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      for (std::ptrdiff_t k = 0; k < 1; k++) {
        gmsh::model::geo::mesh::setTransfiniteCurve(cylinder_line_tag(k, j, i), 12);
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      gmsh::model::geo::mesh::setTransfiniteCurve(connection_line_tag(j, i), 6, "Progression", 1.1);
    }
  }
  for (std::ptrdiff_t i = 0; i < 3; i++) {
    for (std::ptrdiff_t j = 0; j < 3; j++) {
      if (i == 1 && j == 1) {
        continue;
      }
      gmsh::model::geo::mesh::setTransfiniteSurface(farfield_plane_surface_tag(j, i));
      gmsh::model::geo::mesh::setRecombine(2, farfield_plane_surface_tag(j, i));
    }
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      gmsh::model::geo::mesh::setTransfiniteSurface(cylinder_plane_surface_tag(j, i));
      gmsh::model::geo::mesh::setRecombine(2, cylinder_plane_surface_tag(j, i));
    }
  }
  gmsh::model::geo::synchronize();
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    for (std::ptrdiff_t j = 0; j < 4; j++) {
      for (std::ptrdiff_t k = 0; k < 3; k++) {
        if (i == 1 && j == 0) {
          physical_group_tag[0].emplace_back(farfield_line_tag(k, j, i));
        }
        if (i == 1 && j == 3) {
          physical_group_tag[1].emplace_back(farfield_line_tag(k, j, i));
        }
        if ((i == 0) && (j == 0 || j == 3)) {
          physical_group_tag[2].emplace_back(farfield_line_tag(k, j, i));
        }
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      for (std::ptrdiff_t k = 0; k < 1; k++) {
        physical_group_tag[2].emplace_back(cylinder_line_tag(k, j, i));
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 3; i++) {
    for (std::ptrdiff_t j = 0; j < 3; j++) {
      if (i == 1 && j == 1) {
        continue;
      }
      physical_group_tag[3].emplace_back(farfield_plane_surface_tag(j, i));
    }
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      physical_group_tag[3].emplace_back(cylinder_plane_surface_tag(j, i));
    }
  }
  gmsh::model::addPhysicalGroup(1, physical_group_tag[0], -1, "bc-1");
  gmsh::model::addPhysicalGroup(1, physical_group_tag[1], -1, "bc-2");
  gmsh::model::addPhysicalGroup(1, physical_group_tag[2], -1, "bc-3");
  gmsh::model::addPhysicalGroup(2, physical_group_tag[3], -1, "vc-1");
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::setOrder(SimulationControl::kPolynomialOrder);
  gmsh::model::mesh::optimize("HighOrder");
  gmsh::write(mesh_file_path);
}
