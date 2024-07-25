/**
 * @file delta_3d_ns.cpp
 * @brief The main file of SubrosaDG delta_2d_ns.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2024-06-13
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#include "SubrosaDG"

inline const std::string kExampleName{"delta_3d_ns"};

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory / "build/out" / kExampleName};

// inline const std::filesystem::path kExampleDirectory{"/mnt/data/" + kExampleName};

using SimulationControl = SubrosaDG::SimulationControlNavierStokes<
    SubrosaDG::DimensionEnum::D3, SubrosaDG::PolynomialOrderEnum::P1, SubrosaDG::MeshModelEnum::Hexahedron,
    SubrosaDG::SourceTermEnum::None, SubrosaDG::InitialConditionEnum::Function,
    SubrosaDG::ThermodynamicModelEnum::ConstantE, SubrosaDG::EquationOfStateEnum::IdealGas,
    SubrosaDG::TransportModelEnum::Constant, SubrosaDG::ConvectiveFluxEnum::HLLC, SubrosaDG::ViscousFluxEnum::BR2,
    SubrosaDG::TimeIntegrationEnum::SSPRK3>;

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::System<SimulationControl> system;
  system.setMesh(kExampleDirectory / "delta_3d_ns.msh", generateMesh);
  // NOTE: Phd Thesis: Yuchen.Yang, Research on Adaptive Mesh Method for Compressible Flow Simulation, 2023.
  system.addInitialCondition(
      []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
          -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
        return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{
            1.4_r, 0.0_r, 0.3_r * std::cos(12.5_deg), 0.3_r * std::sin(12.5_deg), 1.0_r};
      });
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::RiemannFarfield>(
      "bc-1",
      []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
          -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
        return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{
            1.4_r, 0.0_r, 0.3_r * std::cos(12.5_deg), 0.3_r * std::sin(12.5_deg), 1.0_r};
      });
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::IsothermalNoSlipWall>(
      "bc-2",
      []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
          -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
        return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{1.4_r, 0.0_r, 0.0_r, 0.0_r,
                                                                                           1.0_r};
      });
  system.setTransportModel(1.4_r * 0.3_r / 4000.0_r);
  system.setTimeIntegration(1.0_r);
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
  constexpr double kZ = 0.024416137;
  const double tan15 = std::tan(15 * SubrosaDG::kPi / 180);
  const double sqrt3 = std::sqrt(3);
  Eigen::Matrix<double, 5, 5, Eigen::RowMajor> coordinate_x;
  Eigen::Matrix<double, 4, 5, Eigen::RowMajor> coordinate_y;
  Eigen::Vector<double, 4> coordinate_z;
  // clang-format off
  coordinate_x << -5.0, 0.0,                       0.0, 0.0,                      5.0,
                  -5.0, -tan15 / 2.0 + sqrt3 * kZ, 0.0, tan15 / 2.0 - sqrt3 * kZ, 5.0,
                  -5.0, -tan15 + sqrt3 * kZ,       0.0, tan15 - sqrt3 * kZ,       5.0,
                  -5.0, -tan15 / 2.0,              0.0, tan15 / 2.0,              5.0,
                  -5.0, -tan15,                    0.0, tan15,                    5.0;
  coordinate_y << -5.0, sqrt3 * kZ / tan15, 0.5, 1.0, 10.0,
                  -5.0, sqrt3 * kZ / tan15, 0.6, 1.0, 10.0,
                  -5.0, 0.0,                0.5, 1.0, 10.0,
                  -5.0, 0.0,                0.6, 1.0, 10.0;
  coordinate_z << -5.0, -kZ, 0.0, 5.0;
  // clang-format on
  Eigen::Tensor<int, 3> point_tag(5, 5, 4);
  Eigen::Tensor<int, 3> line_x_tag(4, 5, 4);
  Eigen::Tensor<int, 3> line_y_tag(4, 5, 4);
  Eigen::Tensor<int, 3> line_z_tag(3, 5, 5);
  Eigen::Tensor<int, 3> curve_loop_x_tag(5, 4, 4);
  Eigen::Tensor<int, 3> curve_loop_y_tag(4, 3, 5);
  Eigen::Tensor<int, 3> curve_loop_z_tag(3, 4, 5);
  Eigen::Tensor<int, 3> surface_filling_x_tag(5, 4, 4);
  Eigen::Tensor<int, 3> surface_filling_y_tag(4, 3, 5);
  Eigen::Tensor<int, 3> surface_filling_z_tag(3, 4, 5);
  Eigen::Tensor<int, 3> surface_loop_tag(5, 4, 3);
  Eigen::Tensor<int, 3> volumn_tag(5, 4, 3);
  std::array<std::vector<int>, 3> physical_group_tag;
  gmsh::model::add("delta_3d");
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    if (i < 2) {
      for (std::ptrdiff_t j = 0; j < 5; j++) {
        if (j < 2) {
          for (std::ptrdiff_t k = 0; k < 5; k++) {
            if (k == 1 || k == 3) {
              continue;
            }
            point_tag(k, j, i) = gmsh::model::geo::addPoint(coordinate_x(0, k), coordinate_y(0, j), coordinate_z(i));
          }
          point_tag(1, j, i) = point_tag(2, j, i);
          point_tag(3, j, i) = point_tag(2, j, i);
        } else if (j == 2) {
          for (std::ptrdiff_t k = 0; k < 5; k++) {
            if (k == 2) {
              point_tag(k, j, i) = gmsh::model::geo::addPoint(coordinate_x(1, k), coordinate_y(1, j), coordinate_z(i));
            } else {
              point_tag(k, j, i) = gmsh::model::geo::addPoint(coordinate_x(1, k), coordinate_y(0, j), coordinate_z(i));
            }
          }
        } else {
          for (std::ptrdiff_t k = 0; k < 5; k++) {
            point_tag(k, j, i) = gmsh::model::geo::addPoint(coordinate_x(2, k), coordinate_y(0, j), coordinate_z(i));
          }
        }
      }
    } else {
      for (std::ptrdiff_t j = 0; j < 5; j++) {
        if (j < 2) {
          for (std::ptrdiff_t k = 0; k < 5; k++) {
            if (k == 1 || k == 3) {
              continue;
            }
            point_tag(k, j, i) = gmsh::model::geo::addPoint(coordinate_x(0, k), coordinate_y(2, j), coordinate_z(i));
          }
          point_tag(1, j, i) = point_tag(2, j, i);
          point_tag(3, j, i) = point_tag(2, j, i);
        } else if (j == 2) {
          for (std::ptrdiff_t k = 0; k < 5; k++) {
            if (k == 2) {
              point_tag(k, j, i) = gmsh::model::geo::addPoint(coordinate_x(3, k), coordinate_y(3, j), coordinate_z(i));
            } else {
              point_tag(k, j, i) = gmsh::model::geo::addPoint(coordinate_x(3, k), coordinate_y(2, j), coordinate_z(i));
            }
          }
        } else {
          for (std::ptrdiff_t k = 0; k < 5; k++) {
            point_tag(k, j, i) = gmsh::model::geo::addPoint(coordinate_x(4, k), coordinate_y(2, j), coordinate_z(i));
          }
        }
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 5; j++) {
      if (j < 2) {
        for (std::ptrdiff_t k = 0; k < 4; k++) {
          if (k == 1 || k == 2) {
            continue;
          }
          line_x_tag(k, j, i) = gmsh::model::geo::addLine(point_tag(k, j, i), point_tag(k + 1, j, i));
        }
      } else {
        for (std::ptrdiff_t k = 0; k < 4; k++) {
          line_x_tag(k, j, i) = gmsh::model::geo::addLine(point_tag(k, j, i), point_tag(k + 1, j, i));
        }
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 5; j++) {
      if (j == 2) {
        for (std::ptrdiff_t k = 0; k < 4; k++) {
          if (k == 1) {
            continue;
          }
          line_y_tag(k, j, i) = gmsh::model::geo::addLine(point_tag(j, k, i), point_tag(j, k + 1, i));
        }
      } else if (j == 1 || j == 3) {
        for (std::ptrdiff_t k = 0; k < 4; k++) {
          if (k == 0) {
            continue;
          }
          line_y_tag(k, j, i) = gmsh::model::geo::addLine(point_tag(j, k, i), point_tag(j, k + 1, i));
        }
      } else {
        for (std::ptrdiff_t k = 0; k < 4; k++) {
          line_y_tag(k, j, i) = gmsh::model::geo::addLine(point_tag(j, k, i), point_tag(j, k + 1, i));
        }
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 5; i++) {
    if (i == 0 || i == 1) {
      for (std::ptrdiff_t j = 0; j < 5; j++) {
        if (j == 1 || j == 3) {
          continue;
        }
        for (std::ptrdiff_t k = 0; k < 3; k++) {
          line_z_tag(k, j, i) = gmsh::model::geo::addLine(point_tag(j, i, k), point_tag(j, i, k + 1));
        }
      }
    } else if (i == 2) {
      for (std::ptrdiff_t j = 0; j < 5; j++) {
        if (j == 2) {
          for (std::ptrdiff_t k = 0; k < 3; k++) {
            if (k == 1) {
              continue;
            }
            line_z_tag(k, j, i) = gmsh::model::geo::addLine(point_tag(j, i, k), point_tag(j, i, k + 1));
          }
        } else {
          for (std::ptrdiff_t k = 0; k < 3; k++) {
            line_z_tag(k, j, i) = gmsh::model::geo::addLine(point_tag(j, i, k), point_tag(j, i, k + 1));
          }
        }
      }
    } else {
      for (std::ptrdiff_t j = 0; j < 5; j++) {
        for (std::ptrdiff_t k = 0; k < 3; k++) {
          line_z_tag(k, j, i) = gmsh::model::geo::addLine(point_tag(j, i, k), point_tag(j, i, k + 1));
        }
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 4; j++) {
      if (j == 0) {
        curve_loop_x_tag(0, j, i) = gmsh::model::geo::addCurveLoop(
            {line_x_tag(0, j, i), line_y_tag(j, 2, i), -line_x_tag(0, j + 1, i), -line_y_tag(j, 0, i)});
        curve_loop_x_tag(4, j, i) = gmsh::model::geo::addCurveLoop(
            {line_x_tag(3, j, i), line_y_tag(j, 4, i), -line_x_tag(3, j + 1, i), -line_y_tag(j, 2, i)});
      } else if (j == 1) {
        curve_loop_x_tag(0, j, i) = gmsh::model::geo::addCurveLoop(
            {line_x_tag(0, j, i), line_y_tag(j, 1, i), -line_x_tag(0, j + 1, i), -line_y_tag(j, 0, i)});
        curve_loop_x_tag(2, j, i) = gmsh::model::geo::addCurveLoop(
            {line_x_tag(1, j + 1, i), line_x_tag(2, j + 1, i), -line_y_tag(j, 3, i), line_y_tag(j, 1, i)});
        curve_loop_x_tag(4, j, i) = gmsh::model::geo::addCurveLoop(
            {line_x_tag(3, j, i), line_y_tag(j, 4, i), -line_x_tag(3, j + 1, i), -line_y_tag(j, 3, i)});
      } else {
        curve_loop_x_tag(0, j, i) = gmsh::model::geo::addCurveLoop(
            {line_x_tag(0, j, i), line_y_tag(j, 1, i), -line_x_tag(0, j + 1, i), -line_y_tag(j, 0, i)});
        curve_loop_x_tag(1, j, i) = gmsh::model::geo::addCurveLoop(
            {line_x_tag(1, j, i), line_y_tag(j, 2, i), -line_x_tag(1, j + 1, i), -line_y_tag(j, 1, i)});
        curve_loop_x_tag(3, j, i) = gmsh::model::geo::addCurveLoop(
            {line_x_tag(2, j, i), line_y_tag(j, 3, i), -line_x_tag(2, j + 1, i), -line_y_tag(j, 2, i)});
        curve_loop_x_tag(4, j, i) = gmsh::model::geo::addCurveLoop(
            {line_x_tag(3, j, i), line_y_tag(j, 4, i), -line_x_tag(3, j + 1, i), -line_y_tag(j, 3, i)});
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 4; j++) {
      if (j == 0) {
        for (std::ptrdiff_t k = 0; k < 5; k++) {
          if (k == 0 || k == 4) {
            surface_filling_x_tag(k, j, i) = gmsh::model::geo::addSurfaceFilling({curve_loop_x_tag(k, j, i)});
          } else {
            continue;
          }
        }
      } else if (j == 1) {
        for (std::ptrdiff_t k = 0; k < 5; k++) {
          if (k == 1 || k == 3) {
            continue;
          }
          surface_filling_x_tag(k, j, i) = gmsh::model::geo::addSurfaceFilling({curve_loop_x_tag(k, j, i)});
        }
      } else {
        for (std::ptrdiff_t k = 0; k < 5; k++) {
          if (k == 2) {
            continue;
          }
          surface_filling_x_tag(k, j, i) = gmsh::model::geo::addSurfaceFilling({curve_loop_x_tag(k, j, i)});
        }
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 5; i++) {
    for (std::ptrdiff_t j = 0; j < 3; j++) {
      if (i == 1 || i == 3) {
        for (std::ptrdiff_t k = 0; k < 4; k++) {
          if (k == 0) {
            continue;
          }
          if (k == 1) {
            curve_loop_y_tag(k, j, i) = gmsh::model::geo::addCurveLoop(
                {line_y_tag(k, i, j), line_z_tag(j, i, k + 1), -line_y_tag(k, i, j + 1), -line_z_tag(j, 2, k)});
          } else {
            curve_loop_y_tag(k, j, i) = gmsh::model::geo::addCurveLoop(
                {line_y_tag(k, i, j), line_z_tag(j, i, k + 1), -line_y_tag(k, i, j + 1), -line_z_tag(j, i, k)});
          }
        }
      } else if (i == 2) {
        if (j == 1) {
          for (std::ptrdiff_t k = 0; k < 4; k++) {
            if (k == 1 || k == 2) {
              continue;
            }
            curve_loop_y_tag(k, j, i) = gmsh::model::geo::addCurveLoop(
                {line_y_tag(k, i, j), line_z_tag(j, i, k + 1), -line_y_tag(k, i, j + 1), -line_z_tag(j, i, k)});
          }
        } else {
          for (std::ptrdiff_t k = 0; k < 4; k++) {
            if (k == 1) {
              continue;
            }
            curve_loop_y_tag(k, j, i) = gmsh::model::geo::addCurveLoop(
                {line_y_tag(k, i, j), line_z_tag(j, i, k + 1), -line_y_tag(k, i, j + 1), -line_z_tag(j, i, k)});
          }
        }
      } else {
        for (std::ptrdiff_t k = 0; k < 4; k++) {
          curve_loop_y_tag(k, j, i) = gmsh::model::geo::addCurveLoop(
              {line_y_tag(k, i, j), line_z_tag(j, i, k + 1), -line_y_tag(k, i, j + 1), -line_z_tag(j, i, k)});
        }
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 5; i++) {
    for (std::ptrdiff_t j = 0; j < 3; j++) {
      if (i == 1 || i == 3) {
        for (std::ptrdiff_t k = 0; k < 4; k++) {
          if (k == 0) {
            continue;
          }
          surface_filling_y_tag(k, j, i) = gmsh::model::geo::addSurfaceFilling({curve_loop_y_tag(k, j, i)});
        }
      } else if (i == 2) {
        if (j == 1) {
          for (std::ptrdiff_t k = 0; k < 4; k++) {
            if (k == 1 || k == 2) {
              continue;
            }
            surface_filling_y_tag(k, j, i) = gmsh::model::geo::addSurfaceFilling({curve_loop_y_tag(k, j, i)});
          }
        } else {
          for (std::ptrdiff_t k = 0; k < 4; k++) {
            if (k == 1) {
              continue;
            }
            surface_filling_y_tag(k, j, i) = gmsh::model::geo::addSurfaceFilling({curve_loop_y_tag(k, j, i)});
          }
        }
      } else {
        for (std::ptrdiff_t k = 0; k < 4; k++) {
          surface_filling_y_tag(k, j, i) = gmsh::model::geo::addSurfaceFilling({curve_loop_y_tag(k, j, i)});
        }
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 5; i++) {
    for (std::ptrdiff_t j = 0; j < 4; j++) {
      if (i == 0 || i == 1) {
        if (j == 0) {
          for (std::ptrdiff_t k = 0; k < 3; k++) {
            curve_loop_z_tag(k, j, i) = gmsh::model::geo::addCurveLoop(
                {line_z_tag(k, j, i), line_x_tag(j, i, k + 1), -line_z_tag(k, 2, i), -line_x_tag(j, i, k)});
          }
        } else if (j == 3) {
          for (std::ptrdiff_t k = 0; k < 3; k++) {
            curve_loop_z_tag(k, j, i) = gmsh::model::geo::addCurveLoop(
                {line_z_tag(k, 2, i), line_x_tag(j, i, k + 1), -line_z_tag(k, j + 1, i), -line_x_tag(j, i, k)});
          }
        } else {
          continue;
        }
      } else if (i == 2) {
        if (j == 1 || j == 2) {
          for (std::ptrdiff_t k = 0; k < 3; k++) {
            if (k == 1) {
              continue;
            }
            curve_loop_z_tag(k, j, i) = gmsh::model::geo::addCurveLoop(
                {line_z_tag(k, j, i), line_x_tag(j, i, k + 1), -line_z_tag(k, j + 1, i), -line_x_tag(j, i, k)});
          }
        } else {
          for (std::ptrdiff_t k = 0; k < 3; k++) {
            curve_loop_z_tag(k, j, i) = gmsh::model::geo::addCurveLoop(
                {line_z_tag(k, j, i), line_x_tag(j, i, k + 1), -line_z_tag(k, j + 1, i), -line_x_tag(j, i, k)});
          }
        }
      } else {
        for (std::ptrdiff_t k = 0; k < 3; k++) {
          curve_loop_z_tag(k, j, i) = gmsh::model::geo::addCurveLoop(
              {line_z_tag(k, j, i), line_x_tag(j, i, k + 1), -line_z_tag(k, j + 1, i), -line_x_tag(j, i, k)});
        }
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 5; i++) {
    for (std::ptrdiff_t j = 0; j < 4; j++) {
      if (i == 0 || i == 1) {
        if (j == 0 || j == 3) {
          for (std::ptrdiff_t k = 0; k < 3; k++) {
            surface_filling_z_tag(k, j, i) = gmsh::model::geo::addSurfaceFilling({curve_loop_z_tag(k, j, i)});
          }
        } else {
          continue;
        }
      } else if (i == 2) {
        if (j == 1 || j == 2) {
          for (std::ptrdiff_t k = 0; k < 3; k++) {
            if (k == 1) {
              continue;
            }
            surface_filling_z_tag(k, j, i) = gmsh::model::geo::addSurfaceFilling({curve_loop_z_tag(k, j, i)});
          }
        } else {
          for (std::ptrdiff_t k = 0; k < 3; k++) {
            surface_filling_z_tag(k, j, i) = gmsh::model::geo::addSurfaceFilling({curve_loop_z_tag(k, j, i)});
          }
        }
      } else {
        for (std::ptrdiff_t k = 0; k < 3; k++) {
          surface_filling_z_tag(k, j, i) = gmsh::model::geo::addSurfaceFilling({curve_loop_z_tag(k, j, i)});
        }
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 3; i++) {
    for (std::ptrdiff_t j = 0; j < 4; j++) {
      if (j == 0) {
        surface_loop_tag(0, j, i) = gmsh::model::geo::addSurfaceLoop(
            {surface_filling_x_tag(0, j, i), surface_filling_x_tag(0, j, i + 1), surface_filling_y_tag(j, i, 0),
             surface_filling_y_tag(j, i, 2), surface_filling_z_tag(i, 0, j), surface_filling_z_tag(i, 0, j + 1)});
        surface_loop_tag(4, j, i) = gmsh::model::geo::addSurfaceLoop(
            {surface_filling_x_tag(4, j, i), surface_filling_x_tag(4, j, i + 1), surface_filling_y_tag(j, i, 2),
             surface_filling_y_tag(j, i, 4), surface_filling_z_tag(i, 3, j), surface_filling_z_tag(i, 3, j + 1)});
      } else if (j == 1) {
        surface_loop_tag(0, j, i) = gmsh::model::geo::addSurfaceLoop(
            {surface_filling_x_tag(0, j, i), surface_filling_x_tag(0, j, i + 1), surface_filling_y_tag(j, i, 0),
             surface_filling_y_tag(j, i, 1), surface_filling_z_tag(i, 0, j), surface_filling_z_tag(i, 0, j + 1)});
        if (i != 1) {
          surface_loop_tag(2, j, i) = gmsh::model::geo::addSurfaceLoop(
              {surface_filling_x_tag(2, j, i), surface_filling_x_tag(2, j, i + 1), surface_filling_y_tag(j, i, 1),
               surface_filling_y_tag(j, i, 3), surface_filling_z_tag(i, 1, 2), surface_filling_z_tag(i, 2, 2)});
        }
        surface_loop_tag(4, j, i) = gmsh::model::geo::addSurfaceLoop(
            {surface_filling_x_tag(4, j, i), surface_filling_x_tag(4, j, i + 1), surface_filling_y_tag(j, i, 3),
             surface_filling_y_tag(j, i, 4), surface_filling_z_tag(i, 3, j), surface_filling_z_tag(i, 3, j + 1)});
      } else if (j == 2) {
        if (i == 1) {
          surface_loop_tag(0, j, i) = gmsh::model::geo::addSurfaceLoop(
              {surface_filling_x_tag(0, j, i), surface_filling_x_tag(0, j, i + 1), surface_filling_y_tag(j, i, 0),
               surface_filling_y_tag(j, i, 1), surface_filling_z_tag(i, 0, j), surface_filling_z_tag(i, 0, j + 1)});
          surface_loop_tag(4, j, i) = gmsh::model::geo::addSurfaceLoop(
              {surface_filling_x_tag(4, j, i), surface_filling_x_tag(4, j, i + 1), surface_filling_y_tag(j, i, 3),
               surface_filling_y_tag(j, i, 4), surface_filling_z_tag(i, 3, j), surface_filling_z_tag(i, 3, j + 1)});
        } else {
          surface_loop_tag(0, j, i) = gmsh::model::geo::addSurfaceLoop(
              {surface_filling_x_tag(0, j, i), surface_filling_x_tag(0, j, i + 1), surface_filling_y_tag(j, i, 0),
               surface_filling_y_tag(j, i, 1), surface_filling_z_tag(i, 0, j), surface_filling_z_tag(i, 0, j + 1)});
          surface_loop_tag(1, j, i) = gmsh::model::geo::addSurfaceLoop(
              {surface_filling_x_tag(1, j, i), surface_filling_x_tag(1, j, i + 1), surface_filling_y_tag(j, i, 1),
               surface_filling_y_tag(j, i, 2), surface_filling_z_tag(i, 1, j), surface_filling_z_tag(i, 1, j + 1)});
          surface_loop_tag(3, j, i) = gmsh::model::geo::addSurfaceLoop(
              {surface_filling_x_tag(3, j, i), surface_filling_x_tag(3, j, i + 1), surface_filling_y_tag(j, i, 2),
               surface_filling_y_tag(j, i, 3), surface_filling_z_tag(i, 2, j), surface_filling_z_tag(i, 2, j + 1)});
          surface_loop_tag(4, j, i) = gmsh::model::geo::addSurfaceLoop(
              {surface_filling_x_tag(4, j, i), surface_filling_x_tag(4, j, i + 1), surface_filling_y_tag(j, i, 3),
               surface_filling_y_tag(j, i, 4), surface_filling_z_tag(i, 3, j), surface_filling_z_tag(i, 3, j + 1)});
        }
      } else {
        surface_loop_tag(0, j, i) = gmsh::model::geo::addSurfaceLoop(
            {surface_filling_x_tag(0, j, i), surface_filling_x_tag(0, j, i + 1), surface_filling_y_tag(j, i, 0),
             surface_filling_y_tag(j, i, 1), surface_filling_z_tag(i, 0, j), surface_filling_z_tag(i, 0, j + 1)});
        surface_loop_tag(1, j, i) = gmsh::model::geo::addSurfaceLoop(
            {surface_filling_x_tag(1, j, i), surface_filling_x_tag(1, j, i + 1), surface_filling_y_tag(j, i, 1),
             surface_filling_y_tag(j, i, 2), surface_filling_z_tag(i, 1, j), surface_filling_z_tag(i, 1, j + 1)});
        surface_loop_tag(3, j, i) = gmsh::model::geo::addSurfaceLoop(
            {surface_filling_x_tag(3, j, i), surface_filling_x_tag(3, j, i + 1), surface_filling_y_tag(j, i, 2),
             surface_filling_y_tag(j, i, 3), surface_filling_z_tag(i, 2, j), surface_filling_z_tag(i, 2, j + 1)});
        surface_loop_tag(4, j, i) = gmsh::model::geo::addSurfaceLoop(
            {surface_filling_x_tag(4, j, i), surface_filling_x_tag(4, j, i + 1), surface_filling_y_tag(j, i, 3),
             surface_filling_y_tag(j, i, 4), surface_filling_z_tag(i, 3, j), surface_filling_z_tag(i, 3, j + 1)});
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 3; i++) {
    for (std::ptrdiff_t j = 0; j < 4; j++) {
      if (j == 0) {
        volumn_tag(0, j, i) = gmsh::model::geo::addVolume({surface_loop_tag(0, j, i)});
        volumn_tag(4, j, i) = gmsh::model::geo::addVolume({surface_loop_tag(4, j, i)});
      } else if (j == 1) {
        volumn_tag(0, j, i) = gmsh::model::geo::addVolume({surface_loop_tag(0, j, i)});
        if (i != 1) {
          volumn_tag(2, j, i) = gmsh::model::geo::addVolume({surface_loop_tag(2, j, i)});
        }
        volumn_tag(4, j, i) = gmsh::model::geo::addVolume({surface_loop_tag(4, j, i)});
      } else if (j == 2) {
        if (i == 1) {
          volumn_tag(0, j, i) = gmsh::model::geo::addVolume({surface_loop_tag(0, j, i)});
          volumn_tag(4, j, i) = gmsh::model::geo::addVolume({surface_loop_tag(4, j, i)});
        } else {
          volumn_tag(0, j, i) = gmsh::model::geo::addVolume({surface_loop_tag(0, j, i)});
          volumn_tag(1, j, i) = gmsh::model::geo::addVolume({surface_loop_tag(1, j, i)});
          volumn_tag(3, j, i) = gmsh::model::geo::addVolume({surface_loop_tag(3, j, i)});
          volumn_tag(4, j, i) = gmsh::model::geo::addVolume({surface_loop_tag(4, j, i)});
        }
      } else {
        volumn_tag(0, j, i) = gmsh::model::geo::addVolume({surface_loop_tag(0, j, i)});
        volumn_tag(1, j, i) = gmsh::model::geo::addVolume({surface_loop_tag(1, j, i)});
        volumn_tag(3, j, i) = gmsh::model::geo::addVolume({surface_loop_tag(3, j, i)});
        volumn_tag(4, j, i) = gmsh::model::geo::addVolume({surface_loop_tag(4, j, i)});
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 5; j++) {
      if (j < 2) {
        gmsh::model::geo::mesh::setTransfiniteCurve(line_x_tag(0, j, i), 13, "Progression", -1.55);
        gmsh::model::geo::mesh::setTransfiniteCurve(line_x_tag(3, j, i), 13, "Progression", 1.55);
      } else {
        gmsh::model::geo::mesh::setTransfiniteCurve(line_x_tag(0, j, i), 13, "Progression", -1.55);
        gmsh::model::geo::mesh::setTransfiniteCurve(line_x_tag(1, j, i), 9);
        gmsh::model::geo::mesh::setTransfiniteCurve(line_x_tag(2, j, i), 9);
        gmsh::model::geo::mesh::setTransfiniteCurve(line_x_tag(3, j, i), 13, "Progression", 1.55);
      }
    }
  }

  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 5; j++) {
      if (j == 2) {
        gmsh::model::geo::mesh::setTransfiniteCurve(line_y_tag(0, j, i), 13, "Progression", -1.6);
        gmsh::model::geo::mesh::setTransfiniteCurve(line_y_tag(2, j, i), 9);
        gmsh::model::geo::mesh::setTransfiniteCurve(line_y_tag(3, j, i), 29, "Progression", 1.2);
      } else if (j == 1 || j == 3) {
        gmsh::model::geo::mesh::setTransfiniteCurve(line_y_tag(1, j, i), 9);
        gmsh::model::geo::mesh::setTransfiniteCurve(line_y_tag(2, j, i), 9);
        gmsh::model::geo::mesh::setTransfiniteCurve(line_y_tag(3, j, i), 29, "Progression", 1.2);
      } else {
        gmsh::model::geo::mesh::setTransfiniteCurve(line_y_tag(0, j, i), 13, "Progression", -1.6);
        gmsh::model::geo::mesh::setTransfiniteCurve(line_y_tag(1, j, i), 9);
        gmsh::model::geo::mesh::setTransfiniteCurve(line_y_tag(2, j, i), 9);
        gmsh::model::geo::mesh::setTransfiniteCurve(line_y_tag(3, j, i), 29, "Progression", 1.2);
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 5; i++) {
    if (i == 0 || i == 1) {
      for (std::ptrdiff_t j = 0; j < 5; j++) {
        if (j == 1 || j == 3) {
          continue;
        }
        gmsh::model::geo::mesh::setTransfiniteCurve(line_z_tag(0, j, i), 13, "Progression", -1.6);
        gmsh::model::geo::mesh::setTransfiniteCurve(line_z_tag(1, j, i), 4);
        gmsh::model::geo::mesh::setTransfiniteCurve(line_z_tag(2, j, i), 17, "Progression", 1.5);
      }
    } else if (i == 2) {
      for (std::ptrdiff_t j = 0; j < 5; j++) {
        if (j == 2) {
          gmsh::model::geo::mesh::setTransfiniteCurve(line_z_tag(0, j, i), 13, "Progression", -1.6);
          gmsh::model::geo::mesh::setTransfiniteCurve(line_z_tag(2, j, i), 17, "Progression", 1.5);
        } else {
          gmsh::model::geo::mesh::setTransfiniteCurve(line_z_tag(0, j, i), 13, "Progression", -1.6);
          gmsh::model::geo::mesh::setTransfiniteCurve(line_z_tag(1, j, i), 4);
          gmsh::model::geo::mesh::setTransfiniteCurve(line_z_tag(2, j, i), 17, "Progression", 1.5);
        }
      }
    } else {
      for (std::ptrdiff_t j = 0; j < 5; j++) {
        gmsh::model::geo::mesh::setTransfiniteCurve(line_z_tag(0, j, i), 13, "Progression", -1.6);
        gmsh::model::geo::mesh::setTransfiniteCurve(line_z_tag(1, j, i), 4);
        gmsh::model::geo::mesh::setTransfiniteCurve(line_z_tag(2, j, i), 17, "Progression", 1.5);
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 4; j++) {
      if (j == 0) {
        for (std::ptrdiff_t k = 0; k < 5; k++) {
          if (k == 0 || k == 4) {
            gmsh::model::geo::mesh::setTransfiniteSurface(surface_filling_x_tag(k, j, i));
            gmsh::model::geo::mesh::setRecombine(2, surface_filling_x_tag(k, j, i));
          } else {
            continue;
          }
        }
      } else if (j == 1) {
        for (std::ptrdiff_t k = 0; k < 5; k++) {
          if (k == 1 || k == 3) {
            continue;
          }
          gmsh::model::geo::mesh::setTransfiniteSurface(surface_filling_x_tag(k, j, i));
          gmsh::model::geo::mesh::setRecombine(2, surface_filling_x_tag(k, j, i));
        }
      } else {
        for (std::ptrdiff_t k = 0; k < 5; k++) {
          if (k == 2) {
            continue;
          }
          gmsh::model::geo::mesh::setTransfiniteSurface(surface_filling_x_tag(k, j, i));
          gmsh::model::geo::mesh::setRecombine(2, surface_filling_x_tag(k, j, i));
        }
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 5; i++) {
    for (std::ptrdiff_t j = 0; j < 4; j++) {
      if (i == 1 || i == 3) {
        for (std::ptrdiff_t k = 0; k < 4; k++) {
          if (k == 0) {
            continue;
          }
          gmsh::model::geo::mesh::setTransfiniteSurface(surface_filling_y_tag(k, j, i));
          gmsh::model::geo::mesh::setRecombine(2, surface_filling_y_tag(k, j, i));
        }
      } else if (i == 2) {
        if (j == 1) {
          for (std::ptrdiff_t k = 0; k < 4; k++) {
            if (k == 1 || k == 2) {
              continue;
            }
            gmsh::model::geo::mesh::setTransfiniteSurface(surface_filling_y_tag(k, j, i));
            gmsh::model::geo::mesh::setRecombine(2, surface_filling_y_tag(k, j, i));
          }
        } else {
          for (std::ptrdiff_t k = 0; k < 4; k++) {
            if (k == 1) {
              continue;
            }
            gmsh::model::geo::mesh::setTransfiniteSurface(surface_filling_y_tag(k, j, i));
            gmsh::model::geo::mesh::setRecombine(2, surface_filling_y_tag(k, j, i));
          }
        }
      } else {
        for (std::ptrdiff_t k = 0; k < 4; k++) {
          gmsh::model::geo::mesh::setTransfiniteSurface(surface_filling_y_tag(k, j, i));
          gmsh::model::geo::mesh::setRecombine(2, surface_filling_y_tag(k, j, i));
        }
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 5; i++) {
    for (std::ptrdiff_t j = 0; j < 4; j++) {
      if (i == 0 || i == 3) {
        for (std::ptrdiff_t k = 0; k < 3; k++) {
          if (k == 0 || k == 2) {
            gmsh::model::geo::mesh::setTransfiniteSurface(surface_filling_z_tag(i, k, j));
            gmsh::model::geo::mesh::setRecombine(2, surface_filling_z_tag(i, k, j));
          } else {
            continue;
          }
        }
      } else if (i == 2) {
        if (j == 1 || j == 2) {
          for (std::ptrdiff_t k = 0; k < 3; k++) {
            if (k == 1) {
              continue;
            }
            gmsh::model::geo::mesh::setTransfiniteSurface(surface_filling_z_tag(i, k, j));
            gmsh::model::geo::mesh::setRecombine(2, surface_filling_z_tag(i, k, j));
          }
        } else {
          for (std::ptrdiff_t k = 0; k < 3; k++) {
            gmsh::model::geo::mesh::setTransfiniteSurface(surface_filling_z_tag(i, k, j));
            gmsh::model::geo::mesh::setRecombine(2, surface_filling_z_tag(i, k, j));
          }
        }
      } else {
        for (std::ptrdiff_t k = 0; k < 3; k++) {
          gmsh::model::geo::mesh::setTransfiniteSurface(surface_filling_z_tag(i, k, j));
          gmsh::model::geo::mesh::setRecombine(2, surface_filling_z_tag(i, k, j));
        }
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 3; i++) {
    for (std::ptrdiff_t j = 0; j < 4; j++) {
      if (j == 0) {
        gmsh::model::geo::mesh::setTransfiniteVolume(volumn_tag(0, j, i));
        gmsh::model::geo::mesh::setTransfiniteVolume(volumn_tag(4, j, i));
        gmsh::model::geo::mesh::setRecombine(3, volumn_tag(0, j, i));
        gmsh::model::geo::mesh::setRecombine(3, volumn_tag(4, j, i));
      } else if (j == 1) {
        gmsh::model::geo::mesh::setTransfiniteVolume(volumn_tag(0, j, i));
        gmsh::model::geo::mesh::setRecombine(3, volumn_tag(0, j, i));
        if (i != 1) {
          gmsh::model::geo::mesh::setTransfiniteVolume(volumn_tag(2, j, i));
          gmsh::model::geo::mesh::setRecombine(3, volumn_tag(2, j, i));
        }
        gmsh::model::geo::mesh::setTransfiniteVolume(volumn_tag(4, j, i));
        gmsh::model::geo::mesh::setRecombine(3, volumn_tag(4, j, i));
      } else if (j == 2) {
        if (i == 1) {
          gmsh::model::geo::mesh::setTransfiniteVolume(volumn_tag(0, j, i));
          gmsh::model::geo::mesh::setTransfiniteVolume(volumn_tag(4, j, i));
          gmsh::model::geo::mesh::setRecombine(3, volumn_tag(0, j, i));
          gmsh::model::geo::mesh::setRecombine(3, volumn_tag(4, j, i));
        } else {
          gmsh::model::geo::mesh::setTransfiniteVolume(volumn_tag(0, j, i));
          gmsh::model::geo::mesh::setTransfiniteVolume(volumn_tag(1, j, i));
          gmsh::model::geo::mesh::setTransfiniteVolume(volumn_tag(3, j, i));
          gmsh::model::geo::mesh::setTransfiniteVolume(volumn_tag(4, j, i));
          gmsh::model::geo::mesh::setRecombine(3, volumn_tag(0, j, i));
          gmsh::model::geo::mesh::setRecombine(3, volumn_tag(1, j, i));
          gmsh::model::geo::mesh::setRecombine(3, volumn_tag(3, j, i));
          gmsh::model::geo::mesh::setRecombine(3, volumn_tag(4, j, i));
        }
      } else {
        gmsh::model::geo::mesh::setTransfiniteVolume(volumn_tag(0, j, i));
        gmsh::model::geo::mesh::setTransfiniteVolume(volumn_tag(1, j, i));
        gmsh::model::geo::mesh::setTransfiniteVolume(volumn_tag(3, j, i));
        gmsh::model::geo::mesh::setTransfiniteVolume(volumn_tag(4, j, i));
        gmsh::model::geo::mesh::setRecombine(3, volumn_tag(0, j, i));
        gmsh::model::geo::mesh::setRecombine(3, volumn_tag(1, j, i));
        gmsh::model::geo::mesh::setRecombine(3, volumn_tag(3, j, i));
        gmsh::model::geo::mesh::setRecombine(3, volumn_tag(4, j, i));
      }
    }
  }
  gmsh::model::geo::synchronize();
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    if (i == 0 || i == 3) {
      for (std::ptrdiff_t j = 0; j < 4; j++) {
        if (j == 0) {
          for (std::ptrdiff_t k = 0; k < 5; k++) {
            if (k == 0 || k == 4) {
              physical_group_tag[0].emplace_back(surface_filling_x_tag(k, j, i));
            } else {
              continue;
            }
          }
        } else if (j == 1) {
          for (std::ptrdiff_t k = 0; k < 5; k++) {
            if (k == 1 || k == 3) {
              continue;
            }
            physical_group_tag[0].emplace_back(surface_filling_x_tag(k, j, i));
          }
        } else {
          for (std::ptrdiff_t k = 0; k < 5; k++) {
            if (k == 2) {
              continue;
            }
            physical_group_tag[0].emplace_back(surface_filling_x_tag(k, j, i));
          }
        }
      }
    } else {
      for (std::ptrdiff_t j = 0; j < 4; j++) {
        if (j == 1) {
          for (std::ptrdiff_t k = 0; k < 5; k++) {
            if (k == 2) {
              physical_group_tag[1].emplace_back(surface_filling_x_tag(k, j, i));
            }
          }
        } else if (j == 2) {
          for (std::ptrdiff_t k = 0; k < 5; k++) {
            if (k == 1 || k == 3) {
              physical_group_tag[1].emplace_back(surface_filling_x_tag(k, j, i));
            }
          }
        }
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 5; i++) {
    for (std::ptrdiff_t j = 0; j < 3; j++) {
      if (i == 1 || i == 3) {
        if (j == 1) {
          physical_group_tag[1].emplace_back(surface_filling_y_tag(1, j, i));
          physical_group_tag[1].emplace_back(surface_filling_y_tag(2, j, i));
        }
      } else if (i == 2) {
        continue;
      } else {
        for (std::ptrdiff_t k = 0; k < 4; k++) {
          physical_group_tag[0].emplace_back(surface_filling_y_tag(k, j, i));
        }
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 5; i++) {
    for (std::ptrdiff_t j = 0; j < 4; j++) {
      if (i == 0) {
        if (j == 0 || j == 3) {
          for (std::ptrdiff_t k = 0; k < 3; k++) {
            physical_group_tag[0].emplace_back(surface_filling_z_tag(k, j, i));
          }
        } else {
          continue;
        }
      } else if (i == 3) {
        if (j == 1 || j == 2) {
          physical_group_tag[1].emplace_back(surface_filling_z_tag(1, j, i));
        }
      } else if (i == 4) {
        for (std::ptrdiff_t k = 0; k < 3; k++) {
          physical_group_tag[0].emplace_back(surface_filling_z_tag(k, j, i));
        }
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 3; i++) {
    for (std::ptrdiff_t j = 0; j < 4; j++) {
      if (j == 0) {
        physical_group_tag[2].emplace_back(volumn_tag(0, j, i));
        physical_group_tag[2].emplace_back(volumn_tag(4, j, i));
      } else if (j == 1) {
        physical_group_tag[2].emplace_back(volumn_tag(0, j, i));
        if (i != 1) {
          physical_group_tag[2].emplace_back(volumn_tag(2, j, i));
        }
        physical_group_tag[2].emplace_back(volumn_tag(4, j, i));
      } else if (j == 2) {
        if (i == 1) {
          physical_group_tag[2].emplace_back(volumn_tag(0, j, i));
          physical_group_tag[2].emplace_back(volumn_tag(4, j, i));
        } else {
          physical_group_tag[2].emplace_back(volumn_tag(0, j, i));
          physical_group_tag[2].emplace_back(volumn_tag(1, j, i));
          physical_group_tag[2].emplace_back(volumn_tag(3, j, i));
          physical_group_tag[2].emplace_back(volumn_tag(4, j, i));
        }
      } else {
        physical_group_tag[2].emplace_back(volumn_tag(0, j, i));
        physical_group_tag[2].emplace_back(volumn_tag(1, j, i));
        physical_group_tag[2].emplace_back(volumn_tag(3, j, i));
        physical_group_tag[2].emplace_back(volumn_tag(4, j, i));
      }
    }
  }
  gmsh::model::addPhysicalGroup(2, physical_group_tag[0], -1, "bc-1");
  gmsh::model::addPhysicalGroup(2, physical_group_tag[1], -1, "bc-2");
  gmsh::model::addPhysicalGroup(3, physical_group_tag[2], -1, "vc-1");
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::setOrder(SimulationControl::kPolynomialOrder);
  gmsh::model::mesh::optimize("HighOrder");
  gmsh::write(mesh_file_path);
}
