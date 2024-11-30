/**
 * @file cylinder_3d_incns.cpp
 * @brief The 3D cylinder flow example with incompressible Navier-Stokes equations.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-11-29
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#include "SubrosaDG"

inline const std::string kExampleName{"cylinder_3d_incns"};

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
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::RiemannFarfield>(
      "bc-1",
      [](const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
          -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
        return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{
            1.0_r, 0.0,
            16.0_r * 2.25_r * coordinate.x() * (0.41_r - coordinate.x()) * coordinate.z() * (0.41_r - coordinate.z()) /
                (0.41_r * 0.41_r * 0.41_r * 0.41_r),
            0.0_r, 1.0_r};
      });
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticNonSlipWall>(
      "bc-2",
      []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
          -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
        return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{1.0_r, 0.0_r, 0.0_r, 0.0_r,
                                                                                           1.0_r};
      });
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticNonSlipWall>(
      "bc-3",
      []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate)
          -> Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber> {
        return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{1.0_r, 0.0_r, 0.0_r, 0.0_r,
                                                                                           1.0_r};
      });
  system.setThermodynamicModel<SimulationControl::kThermodynamicModel>(1.0_r, 1.0_r);
  system.setEquationOfState<SimulationControl::kEquationOfState>(20.0_r, 1.0_r);
  system.setTransportModel<SimulationControl::kTransportModel>(1.0_r * 1.0_r * 0.1_r / 100.0_r);
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
  Eigen::Vector<double, 2> coordinate_x;
  Eigen::Vector<double, 4> farfield_point_coordinate_y;
  Eigen::Vector<double, 4> farfield_point_coordinate_z;
  Eigen::Vector<double, 2> cylinder_point_coordinate_y;
  Eigen::Vector<double, 2> cylinder_point_coordinate_z;
  // clang-format off
  coordinate_x << 0.0, 0.41;
  farfield_point_coordinate_y << 0.0, 0.5 - 2 * x, 0.5 + 2 * x, 2.5;
  farfield_point_coordinate_z << 0.0, 0.2 - 2 * x, 0.2 + 2 * x, 0.41;
  cylinder_point_coordinate_y << 0.5 - x, 0.5 + x;
  cylinder_point_coordinate_z << 0.2 - x, 0.2 + x;
  // clang-format on
  Eigen::Vector<int, 2> center_point_tag;
  Eigen::Tensor<int, 3> farfield_point_tag(2, 4, 4);
  Eigen::Tensor<int, 3> cylinder_point_tag(2, 2, 2);
  Eigen::Tensor<int, 3> farfield_line_tag_x(1, 4, 4);
  Eigen::Tensor<int, 3> farfield_line_tag_y(3, 2, 4);
  Eigen::Tensor<int, 3> farfield_line_tag_z(3, 2, 4);
  Eigen::Tensor<int, 4> cylinder_line_tag(1, 2, 2, 3);
  Eigen::Tensor<int, 3> connection_line_tag(2, 2, 2);
  Eigen::Tensor<int, 3> farfield_curve_loop_tag_x(1, 3, 4);
  Eigen::Tensor<int, 3> farfield_curve_loop_tag_y(3, 3, 2);
  Eigen::Tensor<int, 3> farfield_curve_loop_tag_z(3, 2, 4);
  Eigen::Tensor<int, 4> cylinder_curve_loop_tag(1, 1, 2, 3);
  Eigen::Tensor<int, 3> connection_curve_loop_tag(2, 2, 3);
  Eigen::Tensor<int, 3> farfield_surface_filling_tag_x(1, 3, 4);
  Eigen::Tensor<int, 3> farfield_surface_filling_tag_y(3, 3, 2);
  Eigen::Tensor<int, 3> farfield_surface_filling_tag_z(3, 1, 4);
  Eigen::Tensor<int, 4> cylinder_surface_filling_tag(1, 1, 2, 3);
  Eigen::Tensor<int, 3> connection_surface_filling_tag(2, 2, 3);
  Eigen::Tensor<int, 3> farfield_surface_loop_tag(1, 3, 3);
  Eigen::Tensor<int, 2> cylinder_surface_loop_tag(2, 3);
  Eigen::Tensor<int, 3> farfield_volume_tag(1, 3, 3);
  Eigen::Tensor<int, 2> cylinder_volume_tag(2, 3);
  std::array<std::vector<int>, 4> physical_group_tag;
  gmsh::model::add("cylinder_3d");
  center_point_tag[0] = gmsh::model::geo::addPoint(coordinate_x[0], 0.5, 0.2);
  center_point_tag[1] = gmsh::model::geo::addPoint(coordinate_x[1], 0.5, 0.2);
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 4; j++) {
      for (std::ptrdiff_t k = 0; k < 2; k++) {
        farfield_point_tag(k, j, i) =
            gmsh::model::geo::addPoint(coordinate_x(k), farfield_point_coordinate_y(j), farfield_point_coordinate_z(i));
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      for (std::ptrdiff_t k = 0; k < 2; k++) {
        cylinder_point_tag(k, j, i) =
            gmsh::model::geo::addPoint(coordinate_x(k), cylinder_point_coordinate_y(j), cylinder_point_coordinate_z(i));
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 4; j++) {
      for (std::ptrdiff_t k = 0; k < 1; k++) {
        farfield_line_tag_x(k, j, i) =
            gmsh::model::geo::addLine(farfield_point_tag(k, j, i), farfield_point_tag(k + 1, j, i));
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      for (std::ptrdiff_t k = 0; k < 3; k++) {
        if ((i == 1 || i == 2) && (k == 1)) {
          farfield_line_tag_y(k, j, i) = gmsh::model::geo::addCircleArc(
              farfield_point_tag(j, k, i), center_point_tag[j], farfield_point_tag(j, k + 1, i));
        } else {
          farfield_line_tag_y(k, j, i) =
              gmsh::model::geo::addLine(farfield_point_tag(j, k, i), farfield_point_tag(j, k + 1, i));
        }
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      for (std::ptrdiff_t k = 0; k < 3; k++) {
        if ((i == 1 || i == 2) && (k == 1)) {
          farfield_line_tag_z(k, j, i) = gmsh::model::geo::addCircleArc(
              farfield_point_tag(j, i, k), center_point_tag[j], farfield_point_tag(j, i, k + 1));
        } else {
          farfield_line_tag_z(k, j, i) =
              gmsh::model::geo::addLine(farfield_point_tag(j, i, k), farfield_point_tag(j, i, k + 1));
        }
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      for (std::ptrdiff_t k = 0; k < 1; k++) {
        cylinder_line_tag(k, j, i, 0) =
            gmsh::model::geo::addLine(cylinder_point_tag(k, j, i), cylinder_point_tag(k + 1, j, i));
        cylinder_line_tag(k, j, i, 1) = gmsh::model::geo::addCircleArc(cylinder_point_tag(j, k, i), center_point_tag[j],
                                                                       cylinder_point_tag(j, k + 1, i));
        cylinder_line_tag(k, j, i, 2) = gmsh::model::geo::addCircleArc(cylinder_point_tag(j, i, k), center_point_tag[j],
                                                                       cylinder_point_tag(j, i, k + 1));
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      for (std::ptrdiff_t k = 0; k < 2; k++) {
        connection_line_tag(k, j, i) =
            gmsh::model::geo::addLine(cylinder_point_tag(k, j, i), farfield_point_tag(k, j + 1, i + 1));
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 3; j++) {
      for (std::ptrdiff_t k = 0; k < 1; k++) {
        farfield_curve_loop_tag_x(k, j, i) =
            gmsh::model::geo::addCurveLoop({farfield_line_tag_x(k, j, i), farfield_line_tag_y(j, k + 1, i),
                                            -farfield_line_tag_x(k, j + 1, i), -farfield_line_tag_y(j, k, i)});
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    for (std::ptrdiff_t j = 0; j < 3; j++) {
      for (std::ptrdiff_t k = 0; k < 3; k++) {
        if (j == 1 && k == 1) {
          continue;
        }
        farfield_curve_loop_tag_y(k, j, i) =
            gmsh::model::geo::addCurveLoop({farfield_line_tag_y(k, i, j), farfield_line_tag_z(j, i, k + 1),
                                            -farfield_line_tag_y(k, i, j + 1), -farfield_line_tag_z(j, i, k)});
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 1; j++) {
      for (std::ptrdiff_t k = 0; k < 3; k++) {
        farfield_curve_loop_tag_z(k, j, i) =
            gmsh::model::geo::addCurveLoop({farfield_line_tag_z(k, j, i), farfield_line_tag_x(j, i, k + 1),
                                            -farfield_line_tag_z(k, j + 1, i), -farfield_line_tag_x(j, i, k)});
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    for (std::ptrdiff_t j = 0; j < 1; j++) {
      for (std::ptrdiff_t k = 0; k < 1; k++) {
        cylinder_curve_loop_tag(k, j, i, 0) =
            gmsh::model::geo::addCurveLoop({cylinder_line_tag(k, j, i, 0), cylinder_line_tag(j, k + 1, i, 1),
                                            -cylinder_line_tag(k, j + 1, i, 0), -cylinder_line_tag(j, k, i, 1)});
        cylinder_curve_loop_tag(k, j, i, 2) =
            gmsh::model::geo::addCurveLoop({cylinder_line_tag(k, j, i, 2), cylinder_line_tag(j, i, k + 1, 0),
                                            -cylinder_line_tag(k, j + 1, i, 2), -cylinder_line_tag(j, i, k, 0)});
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      connection_curve_loop_tag(j, i, 0) =
          gmsh::model::geo::addCurveLoop({connection_line_tag(0, j, i), farfield_line_tag_x(0, j + 1, i + 1),
                                          -connection_line_tag(1, j, i), -cylinder_line_tag(0, j, i, 0)});
      connection_curve_loop_tag(j, i, 1) =
          gmsh::model::geo::addCurveLoop({connection_line_tag(j, 0, i), farfield_line_tag_y(1, j, i + 1),
                                          -connection_line_tag(j, 1, i), -cylinder_line_tag(0, j, i, 1)});
      connection_curve_loop_tag(j, i, 2) =
          gmsh::model::geo::addCurveLoop({connection_line_tag(j, i, 0), farfield_line_tag_z(1, j, i + 1),
                                          -connection_line_tag(j, i, 1), -cylinder_line_tag(0, j, i, 2)});
    }
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 3; j++) {
      for (std::ptrdiff_t k = 0; k < 1; k++) {
        farfield_surface_filling_tag_x(k, j, i) =
            gmsh::model::geo::addSurfaceFilling({farfield_curve_loop_tag_x(k, j, i)});
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    for (std::ptrdiff_t j = 0; j < 3; j++) {
      for (std::ptrdiff_t k = 0; k < 3; k++) {
        if (j == 1 && k == 1) {
          continue;
        }
        farfield_surface_filling_tag_y(k, j, i) =
            gmsh::model::geo::addSurfaceFilling({farfield_curve_loop_tag_y(k, j, i)});
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 1; j++) {
      for (std::ptrdiff_t k = 0; k < 3; k++) {
        farfield_surface_filling_tag_z(k, j, i) =
            gmsh::model::geo::addSurfaceFilling({farfield_curve_loop_tag_z(k, j, i)});
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 3; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      for (std::ptrdiff_t k = 0; k < 1; k++) {
        for (std::ptrdiff_t l = 0; l < 1; l++) {
          if (i == 1) {
            continue;
          }
          cylinder_surface_filling_tag(l, k, j, i) =
              gmsh::model::geo::addSurfaceFilling({cylinder_curve_loop_tag(l, k, j, i)});
        }
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 3; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      for (std::ptrdiff_t k = 0; k < 2; k++) {
        connection_surface_filling_tag(k, j, i) =
            gmsh::model::geo::addSurfaceFilling({connection_curve_loop_tag(k, j, i)});
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 3; i++) {
    for (std::ptrdiff_t j = 0; j < 3; j++) {
      for (std::ptrdiff_t k = 0; k < 1; k++) {
        if (i == 1 && j == 1) {
          continue;
        }
        farfield_surface_loop_tag(k, j, i) = gmsh::model::geo::addSurfaceLoop(
            {farfield_surface_filling_tag_x(k, j, i), farfield_surface_filling_tag_x(k, j, i + 1),
             farfield_surface_filling_tag_y(j, i, k), farfield_surface_filling_tag_y(j, i, k + 1),
             farfield_surface_filling_tag_z(i, k, j), farfield_surface_filling_tag_z(i, k, j + 1)});
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    cylinder_surface_loop_tag(i, 0) = gmsh::model::geo::addSurfaceLoop(
        {connection_surface_filling_tag(0, i, 0), connection_surface_filling_tag(1, i, 0),
         connection_surface_filling_tag(0, i, 1), connection_surface_filling_tag(1, i, 1),
         cylinder_surface_filling_tag(0, 0, i, 0), farfield_surface_filling_tag_x(0, 1, i + 1)});
    cylinder_surface_loop_tag(i, 2) = gmsh::model::geo::addSurfaceLoop(
        {connection_surface_filling_tag(0, i, 2), connection_surface_filling_tag(1, i, 2),
         connection_surface_filling_tag(i, 0, 0), connection_surface_filling_tag(i, 1, 0),
         cylinder_surface_filling_tag(0, 0, i, 2), farfield_surface_filling_tag_z(1, 0, i + 1)});
  }
  for (std::ptrdiff_t i = 0; i < 3; i++) {
    for (std::ptrdiff_t j = 0; j < 3; j++) {
      for (std::ptrdiff_t k = 0; k < 1; k++) {
        if (i == 1 && j == 1) {
          continue;
        }
        farfield_volume_tag(k, j, i) = gmsh::model::geo::addVolume({farfield_surface_loop_tag(k, j, i)});
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 3; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      if (i == 1) {
        continue;
      }
      cylinder_volume_tag(j, i) = gmsh::model::geo::addVolume({cylinder_surface_loop_tag(j, i)});
    }
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 4; j++) {
      for (std::ptrdiff_t k = 0; k < 1; k++) {
        gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag_x(k, j, i), 16, "Bump", 0.15);
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      for (std::ptrdiff_t k = 0; k < 3; k++) {
        if (k == 0) {
          gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag_y(k, j, i), 14, "Progression", -1.1);
        } else if (k == 1) {
          gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag_y(k, j, i), 12);
        } else {
          gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag_y(k, j, i), 36, "Progression", 1.06);
        }
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      for (std::ptrdiff_t k = 0; k < 3; k++) {
        if (k == 0) {
          gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag_z(k, j, i), 13, "Progression", 1.1);
        } else if (k == 1) {
          gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag_z(k, j, i), 12);
        } else {
          gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag_z(k, j, i), 13, "Progression", -1.1);
        }
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      for (std::ptrdiff_t k = 0; k < 1; k++) {
        gmsh::model::geo::mesh::setTransfiniteCurve(cylinder_line_tag(k, j, i, 0), 16, "Bump", 0.15);
        gmsh::model::geo::mesh::setTransfiniteCurve(cylinder_line_tag(k, j, i, 1), 12);
        gmsh::model::geo::mesh::setTransfiniteCurve(cylinder_line_tag(k, j, i, 2), 12);
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      for (std::ptrdiff_t k = 0; k < 2; k++) {
        gmsh::model::geo::mesh::setTransfiniteCurve(connection_line_tag(k, j, i), 10, "Progression", 1.2);
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 3; j++) {
      for (std::ptrdiff_t k = 0; k < 1; k++) {
        gmsh::model::geo::mesh::setTransfiniteSurface(farfield_surface_filling_tag_x(k, j, i));
        gmsh::model::geo::mesh::setRecombine(2, farfield_surface_filling_tag_x(k, j, i));
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 2; i++) {
    for (std::ptrdiff_t j = 0; j < 3; j++) {
      for (std::ptrdiff_t k = 0; k < 3; k++) {
        if (j == 1 && k == 1) {
          continue;
        }
        gmsh::model::geo::mesh::setTransfiniteSurface(farfield_surface_filling_tag_y(k, j, i));
        gmsh::model::geo::mesh::setRecombine(2, farfield_surface_filling_tag_y(k, j, i));
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 1; j++) {
      for (std::ptrdiff_t k = 0; k < 3; k++) {
        gmsh::model::geo::mesh::setTransfiniteSurface(farfield_surface_filling_tag_z(k, j, i));
        gmsh::model::geo::mesh::setRecombine(2, farfield_surface_filling_tag_z(k, j, i));
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 3; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      for (std::ptrdiff_t k = 0; k < 1; k++) {
        for (std::ptrdiff_t l = 0; l < 1; l++) {
          if (i == 1) {
            continue;
          }
          gmsh::model::geo::mesh::setTransfiniteSurface(cylinder_surface_filling_tag(l, k, j, i));
          gmsh::model::geo::mesh::setRecombine(2, cylinder_surface_filling_tag(l, k, j, i));
        }
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 3; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      for (std::ptrdiff_t k = 0; k < 2; k++) {
        gmsh::model::geo::mesh::setTransfiniteSurface(connection_surface_filling_tag(k, j, i));
        gmsh::model::geo::mesh::setRecombine(2, connection_surface_filling_tag(k, j, i));
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 3; i++) {
    for (std::ptrdiff_t j = 0; j < 3; j++) {
      for (std::ptrdiff_t k = 0; k < 1; k++) {
        if (i == 1 && j == 1) {
          continue;
        }
        gmsh::model::geo::mesh::setTransfiniteVolume(farfield_volume_tag(k, j, i));
        gmsh::model::geo::mesh::setRecombine(3, farfield_volume_tag(k, j, i));
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 3; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      if (i == 1) {
        continue;
      }
      gmsh::model::geo::mesh::setTransfiniteVolume(cylinder_volume_tag(j, i));
      gmsh::model::geo::mesh::setRecombine(3, cylinder_volume_tag(j, i));
    }
  }
  gmsh::model::geo::synchronize();
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 3; j++) {
      for (std::ptrdiff_t k = 0; k < 1; k++) {
        if (i == 0 || i == 3) {
          physical_group_tag[1].emplace_back(farfield_surface_filling_tag_x(k, j, i));
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
        physical_group_tag[1].emplace_back(farfield_surface_filling_tag_y(k, j, i));
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 4; i++) {
    for (std::ptrdiff_t j = 0; j < 1; j++) {
      for (std::ptrdiff_t k = 0; k < 3; k++) {
        if (i == 0 || i == 3) {
          physical_group_tag[0].emplace_back(farfield_surface_filling_tag_z(k, j, i));
        }
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 3; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      for (std::ptrdiff_t k = 0; k < 1; k++) {
        for (std::ptrdiff_t l = 0; l < 1; l++) {
          if (i == 1) {
            continue;
          }
          physical_group_tag[2].emplace_back(cylinder_surface_filling_tag(l, k, j, i));
        }
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 3; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      for (std::ptrdiff_t k = 0; k < 2; k++) {
        if (i == 0) {
          continue;
        }
        physical_group_tag[1].emplace_back(connection_surface_filling_tag(k, j, i));
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 3; i++) {
    for (std::ptrdiff_t j = 0; j < 3; j++) {
      for (std::ptrdiff_t k = 0; k < 1; k++) {
        if (i == 1 && j == 1) {
          continue;
        }
        physical_group_tag[3].emplace_back(farfield_volume_tag(k, j, i));
      }
    }
  }
  for (std::ptrdiff_t i = 0; i < 3; i++) {
    for (std::ptrdiff_t j = 0; j < 2; j++) {
      if (i == 1) {
        continue;
      }
      physical_group_tag[3].emplace_back(cylinder_volume_tag(j, i));
    }
  }
  gmsh::model::addPhysicalGroup(2, physical_group_tag[0], -1, "bc-1");
  gmsh::model::addPhysicalGroup(2, physical_group_tag[1], -1, "bc-2");
  gmsh::model::addPhysicalGroup(2, physical_group_tag[2], -1, "bc-3");
  gmsh::model::addPhysicalGroup(3, physical_group_tag[3], -1, "vc-1");
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::setOrder(SimulationControl::kPolynomialOrder);
  gmsh::model::mesh::optimize("HighOrder");
  gmsh::write(mesh_file_path);
}
