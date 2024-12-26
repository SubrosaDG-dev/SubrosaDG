/**
 * @file square_3d_incns.cpp
 * @brief The 3D square flow example with incompressible Navier-Stokes equations.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2024-11-29
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#include "SubrosaDG"

inline const std::string kExampleName{"square_3d_incns"};

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory / "build/out" / kExampleName};

// using SimulationControl = SubrosaDG::SimulationControl<
//     SubrosaDG::SolveControl<SubrosaDG::DimensionEnum::D3, SubrosaDG::PolynomialOrderEnum::P1,
//                             SubrosaDG::BoundaryTimeEnum::Steady, SubrosaDG::SourceTermEnum::None>,
//     SubrosaDG::NumericalControl<SubrosaDG::MeshModelEnum::Hexahedron, SubrosaDG::ShockCapturingEnum::None,
//                                 SubrosaDG::LimiterEnum::None, SubrosaDG::InitialConditionEnum::Function,
//                                 SubrosaDG::TimeIntegrationEnum::SSPRK3>,
//     SubrosaDG::IncompresibleNSVariable<SubrosaDG::ThermodynamicModelEnum::Constant,
//                                        SubrosaDG::EquationOfStateEnum::WeakCompressibleFluid,
//                                        SubrosaDG::TransportModelEnum::Constant,
//                                        SubrosaDG::ConvectiveFluxEnum::LaxFriedrichs,
//                                        SubrosaDG::ViscousFluxEnum::BR2>>;

// int main(int argc, char* argv[]) {
//   static_cast<void>(argc);
//   static_cast<void>(argv);
//   SubrosaDG::System<SimulationControl> system;
//   system.setMesh(kExampleDirectory / std::format("{}.msh", kExampleName), generateMesh);
//   system.addInitialCondition([](const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
//                                  -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
//     return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{
//         1.0_r, 0.0,
//         16.0_r * 0.45_r * coordinate.x() * (0.41_r - coordinate.x()) * coordinate.z() * (0.41_r - coordinate.z()) /
//             (0.41_r * 0.41_r * 0.41_r * 0.41_r),
//         0.0_r, 1.0_r};
//   });
//   system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::RiemannFarfield>(
//       "bc-1",
//       [](const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
//           -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
//         return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{
//             1.0_r, 0.0,
//         16.0_r * 0.45_r * coordinate.x() * (0.41_r - coordinate.x()) * coordinate.z() * (0.41_r - coordinate.z()) /
//             (0.41_r * 0.41_r * 0.41_r * 0.41_r),
//         0.0_r, 1.0_r};
//       });
//   system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticNonSlipWall>(
//       "bc-2",
//       []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
//           -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
//         return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{1.0_r, 0.0_r, 0.0_r,
//         0.0_r, 1.0_r};
//       });
//   system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticNonSlipWall>(
//       "bc-3",
//       []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
//           -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
//         return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{1.0_r, 0.0_r,0.0_r,
//         0.0_r, 1.0_r};
//       });
//   system.setThermodynamicModel<SimulationControl::kThermodynamicModel>(1.0_r, 1.0_r);
//   system.setEquationOfState<SimulationControl::kEquationOfState>(10.0_r, 1.0_r);
//   system.setTransportModel<SimulationControl::kTransportModel>(1.0_r * 0.2_r * 0.1_r / 20.0_r);
//   system.setTimeIntegration(1.0_r);
//   system.setViewConfig(kExampleDirectory, kExampleName);
//   system.addViewVariable({SubrosaDG::ViewVariableEnum::Density, SubrosaDG::ViewVariableEnum::Velocity,
//                           SubrosaDG::ViewVariableEnum::Pressure, SubrosaDG::ViewVariableEnum::Temperature,
//                           SubrosaDG::ViewVariableEnum::MachNumber, SubrosaDG::ViewVariableEnum::Vorticity});
//   system.synchronize();
//   system.solve();
//   system.view();
//   return EXIT_SUCCESS;
// }

using SimulationControl = SubrosaDG::SimulationControl<
    SubrosaDG::SolveControl<SubrosaDG::DimensionEnum::D3, SubrosaDG::PolynomialOrderEnum::P1,
                            SubrosaDG::BoundaryTimeEnum::Steady, SubrosaDG::SourceTermEnum::None>,
    SubrosaDG::NumericalControl<SubrosaDG::MeshModelEnum::Hexahedron, SubrosaDG::ShockCapturingEnum::None,
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
  system.setMesh(kExampleDirectory / std::format("{}.msh", kExampleName), generateMesh);
  system.addInitialCondition([](const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
                                 -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
    return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{
        1.0_r, 0.0,
        16.0_r * 2.25_r * coordinate.x() * (0.41_r - coordinate.x()) * coordinate.z() * (0.41_r - coordinate.z()) /
            (0.41_r * 0.41_r * 0.41_r * 0.41_r),
        0.0_r, 1.0_r};
  });
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::VelocityInflow>(
      "bc-1",
      [](const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
          -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
        return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{
            1.0_r, 0.0,
            16.0_r * 2.25_r * coordinate.x() * (0.41_r - coordinate.x()) * coordinate.z() * (0.41_r - coordinate.z()) /
                (0.41_r * 0.41_r * 0.41_r * 0.41_r),
            0.0_r, 1.0_r};
      });
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::PressureOutflow>(
      "bc-2",
      [](const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
          -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
        return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{
            1.0_r, 0.0,
            16.0_r * 2.25_r * coordinate.x() * (0.41_r - coordinate.x()) * coordinate.z() * (0.41_r - coordinate.z()) /
                (0.41_r * 0.41_r * 0.41_r * 0.41_r),
            0.0_r, 1.0_r};
      });
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticNonSlipWall>(
      "bc-3",
      []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
          -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
        return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{1.0_r, 0.0_r, 0.0_r, 0.0_r,
                                                                                           1.0_r};
      });
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticNonSlipWall>(
      "bc-4",
      []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
          -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
        return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{1.0_r, 0.0_r, 0.0_r, 0.0_r,
                                                                                           1.0_r};
      });
  system.setThermodynamicModel<SimulationControl::kThermodynamicModel>(1.0_r, 1.0_r);
  system.setEquationOfState<SimulationControl::kEquationOfState>(30.0_r, 1.0_r);
  system.setTransportModel<SimulationControl::kTransportModel>(1.0_r * 1.0_r * 0.1_r / 100.0_r);
  system.setTimeIntegration(0.8_r);
  system.setViewConfig(kExampleDirectory, kExampleName);
  system.addViewVariable({SubrosaDG::ViewVariableEnum::Density, SubrosaDG::ViewVariableEnum::Velocity,
                          SubrosaDG::ViewVariableEnum::Pressure, SubrosaDG::ViewVariableEnum::Temperature,
                          SubrosaDG::ViewVariableEnum::MachNumber, SubrosaDG::ViewVariableEnum::Vorticity});
  system.synchronize();
  // system.solve();
  system.view();
  return EXIT_SUCCESS;
}

// using SimulationControl = SubrosaDG::SimulationControl<
//     SubrosaDG::SolveControl<SubrosaDG::DimensionEnum::D3, SubrosaDG::PolynomialOrderEnum::P1,
//                             SubrosaDG::BoundaryTimeEnum::TimeVarying, SubrosaDG::SourceTermEnum::None>,
//     SubrosaDG::NumericalControl<SubrosaDG::MeshModelEnum::Hexahedron, SubrosaDG::ShockCapturingEnum::None,
//                                 SubrosaDG::LimiterEnum::None, SubrosaDG::InitialConditionEnum::Function,
//                                 SubrosaDG::TimeIntegrationEnum::SSPRK3>,
//     SubrosaDG::IncompresibleNSVariable<SubrosaDG::ThermodynamicModelEnum::Constant,
//                                        SubrosaDG::EquationOfStateEnum::WeakCompressibleFluid,
//                                        SubrosaDG::TransportModelEnum::Constant,
//                                        SubrosaDG::ConvectiveFluxEnum::LaxFriedrichs,
//                                        SubrosaDG::ViscousFluxEnum::BR2>>;

// int main(int argc, char* argv[]) {
//   static_cast<void>(argc);
//   static_cast<void>(argv);
//   SubrosaDG::System<SimulationControl> system;
//   system.setMesh(kExampleDirectory / std::format("{}.msh", kExampleName), generateMesh);
//   system.addInitialCondition([](const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
//                                  -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
//     return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{
//         1.0_r, 0.0,
//         16.0_r * 2.25_r * coordinate.x() * (0.41_r - coordinate.x()) * coordinate.z() * (0.41_r - coordinate.z()) /
//             (0.41_r * 0.41_r * 0.41_r * 0.41_r),
//         0.0_r, 1.0_r};
//   });
//   system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::RiemannFarfield>(
//       "bc-1",
//       [](const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate,
//          const SubrosaDG::Real time) -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
//         return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{
//             1.0_r, 0.0,
//             16.0_r * 2.25_r * std::sin(SubrosaDG::kPi * time / 8.0_r) * coordinate.x() * (0.41_r - coordinate.x()) *
//                 coordinate.z() * (0.41_r - coordinate.z()) / (0.41_r * 0.41_r * 0.41_r * 0.41_r),
//             0.0_r, 1.0_r};
//       });
//   system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticNonSlipWall>(
//       "bc-2",
//       []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate,
//          [[maybe_unused]] const SubrosaDG::Real time)
//           -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
//         return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{1.0_r, 0.0_r, 0.0_r,
//         0.0_r,
//                                                                                            1.0_r};
//       });
//   system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticNonSlipWall>(
//       "bc-3",
//       []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate,
//          [[maybe_unused]] const SubrosaDG::Real time)
//           -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
//         return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{1.0_r, 0.0_r, 0.0_r,
//         0.0_r,
//                                                                                            1.0_r};
//       });
//   system.setThermodynamicModel<SimulationControl::kThermodynamicModel>(1.0_r, 1.0_r);
//   system.setEquationOfState<SimulationControl::kEquationOfState>(20.0_r, 1.0_r);
//   system.setTransportModel<SimulationControl::kTransportModel>(1.0_r * 1.0_r * 0.1_r / 100.0_r);
//   system.setTimeIntegration(1.0_r);
//   system.setViewConfig(kExampleDirectory, kExampleName);
//   system.addViewVariable({SubrosaDG::ViewVariableEnum::Density, SubrosaDG::ViewVariableEnum::Velocity,
//                           SubrosaDG::ViewVariableEnum::Pressure, SubrosaDG::ViewVariableEnum::Temperature,
//                           SubrosaDG::ViewVariableEnum::MachNumber, SubrosaDG::ViewVariableEnum::Vorticity});
//   system.synchronize();
//   system.solve();
//   system.view();
//   return EXIT_SUCCESS;
// }

void generateMesh(const std::filesystem::path& mesh_file_path) {
  const double x = 0.05 / std::sqrt(2.0);
  Eigen::Vector<double, 2> point_coordinate_x;
  Eigen::Vector<double, 4> point_coordinate_y;
  Eigen::Vector<double, 4> point_coordinate_z;
  // clang-format off
  point_coordinate_x << 0.0, 0.41;
  point_coordinate_y << 0.0, 0.45, 0.55, 2.5;
  point_coordinate_z << 0.0, 0.15, 0.25, 0.41;
  // clang-format on
  Eigen::Tensor<int, 3> point_tag(2, 4, 4);
  Eigen::Tensor<int, 3> line_tag_x(1, 4, 4);
  Eigen::Tensor<int, 3> line_tag_y(3, 2, 4);
  Eigen::Tensor<int, 3> line_tag_z(3, 2, 4);
  Eigen::Tensor<int, 3> curve_loop_tag_x(1, 3, 4);
  Eigen::Tensor<int, 3> curve_loop_tag_y(3, 3, 2);
  Eigen::Tensor<int, 3> curve_loop_tag_z(3, 2, 4);
  Eigen::Tensor<int, 3> surface_filling_tag_x(1, 3, 4);
  Eigen::Tensor<int, 3> surface_filling_tag_y(3, 3, 2);
  Eigen::Tensor<int, 3> surface_filling_tag_z(3, 1, 4);
  Eigen::Tensor<int, 3> surface_loop_tag(1, 3, 3);
  Eigen::Tensor<int, 3> volume_tag(1, 3, 3);
  std::array<std::vector<int>, 5> physical_group_tag;
  gmsh::model::add("square_3d");
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 4; j++) {
      for (std::ptrdiff_t k = 0; k < 2; k++) {
        point_tag(k, j, i) =
            gmsh::model::geo::addPoint(point_coordinate_x(k), point_coordinate_y(j), point_coordinate_z(i));
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 4; j++) {
      for (std::ptrdiff_t k = 0; k < 1; k++) {
        line_tag_x(k, j, i) = gmsh::model::geo::addLine(point_tag(k, j, i), point_tag(k + 1, j, i));
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      for (std::ptrdiff_t k = 0; k < 3; k++) {
        line_tag_y(k, j, i) = gmsh::model::geo::addLine(point_tag(j, k, i), point_tag(j, k + 1, i));
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      for (std::ptrdiff_t k = 0; k < 3; k++) {
        line_tag_z(k, j, i) = gmsh::model::geo::addLine(point_tag(j, i, k), point_tag(j, i, k + 1));
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 3; j++) {
      for (std::ptrdiff_t k = 0; k < 1; k++) {
        curve_loop_tag_x(k, j, i) = gmsh::model::geo::addCurveLoop(
            {line_tag_x(k, j, i), line_tag_y(j, k + 1, i), -line_tag_x(k, j + 1, i), -line_tag_y(j, k, i)});
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    for (std::ptrdiff_t j = 0; j < 3; j++) {
      for (std::ptrdiff_t k = 0; k < 3; k++) {
        if (j == 1 && k == 1) {
          continue;
        }
        curve_loop_tag_y(k, j, i) = gmsh::model::geo::addCurveLoop(
            {line_tag_y(k, i, j), line_tag_z(j, i, k + 1), -line_tag_y(k, i, j + 1), -line_tag_z(j, i, k)});
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 1; j++) {
      for (std::ptrdiff_t k = 0; k < 3; k++) {
        curve_loop_tag_z(k, j, i) = gmsh::model::geo::addCurveLoop(
            {line_tag_z(k, j, i), line_tag_x(j, i, k + 1), -line_tag_z(k, j + 1, i), -line_tag_x(j, i, k)});
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 3; j++) {
      for (std::ptrdiff_t k = 0; k < 1; k++) {
        surface_filling_tag_x(k, j, i) = gmsh::model::geo::addSurfaceFilling({curve_loop_tag_x(k, j, i)});
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    for (std::ptrdiff_t j = 0; j < 3; j++) {
      for (std::ptrdiff_t k = 0; k < 3; k++) {
        if (j == 1 && k == 1) {
          continue;
        }
        surface_filling_tag_y(k, j, i) = gmsh::model::geo::addSurfaceFilling({curve_loop_tag_y(k, j, i)});
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 1; j++) {
      for (std::ptrdiff_t k = 0; k < 3; k++) {
        surface_filling_tag_z(k, j, i) = gmsh::model::geo::addSurfaceFilling({curve_loop_tag_z(k, j, i)});
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 3; i++) {
    for (std::ptrdiff_t j = 0; j < 3; j++) {
      for (std::ptrdiff_t k = 0; k < 1; k++) {
        if (i == 1 && j == 1) {
          continue;
        }
        surface_loop_tag(k, j, i) = gmsh::model::geo::addSurfaceLoop(
            {surface_filling_tag_x(k, j, i), surface_filling_tag_x(k, j, i + 1), surface_filling_tag_y(j, i, k),
             surface_filling_tag_y(j, i, k + 1), surface_filling_tag_z(i, k, j), surface_filling_tag_z(i, k, j + 1)});
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 3; i++) {
    for (std::ptrdiff_t j = 0; j < 3; j++) {
      for (std::ptrdiff_t k = 0; k < 1; k++) {
        if (i == 1 && j == 1) {
          continue;
        }
        volume_tag(k, j, i) = gmsh::model::geo::addVolume({surface_loop_tag(k, j, i)});
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 4; j++) {
      for (std::ptrdiff_t k = 0; k < 1; k++) {
        gmsh::model::geo::mesh::setTransfiniteCurve(line_tag_x(k, j, i), 16, "Bump", 0.15);
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      for (std::ptrdiff_t k = 0; k < 3; k++) {
        if (k == 0) {
          gmsh::model::geo::mesh::setTransfiniteCurve(line_tag_y(k, j, i), 14, "Progression", -1.2);
        } else if (k == 1) {
          gmsh::model::geo::mesh::setTransfiniteCurve(line_tag_y(k, j, i), 12);
        } else {
          gmsh::model::geo::mesh::setTransfiniteCurve(line_tag_y(k, j, i), 36, "Progression", 1.1);
        }
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      for (std::ptrdiff_t k = 0; k < 3; k++) {
        if (k == 0) {
          gmsh::model::geo::mesh::setTransfiniteCurve(line_tag_z(k, j, i), 13, "Bump", 0.30);
        } else if (k == 1) {
          gmsh::model::geo::mesh::setTransfiniteCurve(line_tag_z(k, j, i), 12);
        } else {
          gmsh::model::geo::mesh::setTransfiniteCurve(line_tag_z(k, j, i), 13, "Bump", 0.30);
        }
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 3; j++) {
      for (std::ptrdiff_t k = 0; k < 1; k++) {
        gmsh::model::geo::mesh::setTransfiniteSurface(surface_filling_tag_x(k, j, i));
        gmsh::model::geo::mesh::setRecombine(2, surface_filling_tag_x(k, j, i));
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    for (std::ptrdiff_t j = 0; j < 3; j++) {
      for (std::ptrdiff_t k = 0; k < 3; k++) {
        if (j == 1 && k == 1) {
          continue;
        }
        gmsh::model::geo::mesh::setTransfiniteSurface(surface_filling_tag_y(k, j, i));
        gmsh::model::geo::mesh::setRecombine(2, surface_filling_tag_y(k, j, i));
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 1; j++) {
      for (std::ptrdiff_t k = 0; k < 3; k++) {
        gmsh::model::geo::mesh::setTransfiniteSurface(surface_filling_tag_z(k, j, i));
        gmsh::model::geo::mesh::setRecombine(2, surface_filling_tag_z(k, j, i));
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 3; i++) {
    for (std::ptrdiff_t j = 0; j < 3; j++) {
      for (std::ptrdiff_t k = 0; k < 1; k++) {
        if (i == 1 && j == 1) {
          continue;
        }
        gmsh::model::geo::mesh::setTransfiniteVolume(volume_tag(k, j, i));
        gmsh::model::geo::mesh::setRecombine(3, volume_tag(k, j, i));
      }
    }
  }
  gmsh::model::geo::synchronize();
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 3; j++) {
      for (std::ptrdiff_t k = 0; k < 1; k++) {
        if (i == 0 || i == 3) {
          physical_group_tag[2].emplace_back(surface_filling_tag_x(k, j, i));
        } else {
          if (j == 1) {
            physical_group_tag[3].emplace_back(surface_filling_tag_x(k, j, i));
          }
        }
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    for (std::ptrdiff_t j = 0; j < 3; j++) {
      for (std::ptrdiff_t k = 0; k < 3; k++) {
        if (j == 1 && k == 1) {
          continue;
        }
        physical_group_tag[2].emplace_back(surface_filling_tag_y(k, j, i));
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 1; j++) {
      for (std::ptrdiff_t k = 0; k < 3; k++) {
        if (i == 0) {
          physical_group_tag[0].emplace_back(surface_filling_tag_z(k, j, i));
        } else if (i == 3) {
          physical_group_tag[1].emplace_back(surface_filling_tag_z(k, j, i));
        } else {
          if (k == 1) {
            physical_group_tag[3].emplace_back(surface_filling_tag_z(k, j, i));
          }
        }
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 3; i++) {
    for (std::ptrdiff_t j = 0; j < 3; j++) {
      for (std::ptrdiff_t k = 0; k < 1; k++) {
        if (i == 1 && j == 1) {
          continue;
        }
        physical_group_tag[4].emplace_back(volume_tag(k, j, i));
      }
    }
  }
  gmsh::model::addPhysicalGroup(2, physical_group_tag[0], -1, "bc-1");
  gmsh::model::addPhysicalGroup(2, physical_group_tag[1], -1, "bc-2");
  gmsh::model::addPhysicalGroup(2, physical_group_tag[2], -1, "bc-3");
  gmsh::model::addPhysicalGroup(2, physical_group_tag[3], -1, "bc-4");
  gmsh::model::addPhysicalGroup(3, physical_group_tag[4], -1, "vc-1");
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::setOrder(SimulationControl::kPolynomialOrder);
  gmsh::model::mesh::optimize("HighOrder");
  gmsh::write(mesh_file_path);
}
