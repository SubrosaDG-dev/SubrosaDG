/**
 * @file cylinder_2d_incns.cpp
 * @brief The main file of SubrosaDG cylinder_2d_incns.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2024-11-25
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

namespace SubrosaDG {

struct PhysicalModelData {
  // EquationOfState
  static constexpr double kReferenceSoundSpeed = 10.0;
  static constexpr double kReferenceDensity = 1.0;

  // ThermodynamicModel
  static constexpr double kSpecificHeatConstantPressure = 1.0;
  static constexpr double kSpecificHeatConstantVolume = 1.0;

  // TransportModel
  static constexpr double kReynoldsNumber = 100.0;
  static constexpr double kDynamicViscosity = 1.0 * 1.0 * 0.1 / kReynoldsNumber;
  static constexpr double kPrandtlNumber = 0.71;
  static constexpr double kThermalConductivity = kSpecificHeatConstantPressure * kDynamicViscosity / kPrandtlNumber;
};

}  // namespace SubrosaDG

#include "SubrosaDG.cpp"

inline const std::string kExampleName{"cylinder_2d_incns"};

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory / "build/out" / kExampleName};

// using SimulationControl = SubrosaDG::SimulationControl<
//     SubrosaDG::SolveControl<SubrosaDG::DimensionEnum::D2, SubrosaDG::PolynomialOrderEnum::P1,
//                             SubrosaDG::BoundaryTimeEnum::Steady, SubrosaDG::SourceTermEnum::None>,
//     SubrosaDG::NumericalControl<SubrosaDG::MeshModelEnum::Quadrangle,
//                                  SubrosaDG::InitialConditionEnum::Function,
//                                 SubrosaDG::TimeIntegrationEnum::SSPRK3>,
//     SubrosaDG::IncompressibleNSVariable<SubrosaDG::ThermodynamicModelEnum::Constant,
//                                         SubrosaDG::EquationOfStateEnum::WeakCompressibleFluid,
//                                         SubrosaDG::TransportModelEnum::Constant,
//                                         SubrosaDG::ConvectiveFluxEnum::LaxFriedrichs,
//                                         SubrosaDG::ViscousFluxEnum::BR2>>;

// template <typename SimulationControl>
// inline void SubrosaDG::InitialCondition<SimulationControl>::computePrimitiveFromCoordinate(
//     const Eigen::Vector<Real, SimulationControl::kDimension>& coordinate,
//     Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber>& initial_primitive_variable) {
//   initial_primitive_variable = {1.0_r, 4.0_r * 1.5_r * coordinate.y() * (0.41_r - coordinate.y()) / (0.41_r *
//   0.41_r),
//                                 0.0_r, 1.0_r};
// }

// template <typename SimulationControl>
// inline void SubrosaDG::BoundaryCondition<SimulationControl>::computePrimitiveFromCoordinate(
//     const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate,
//     Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber>& boundary_primitive_variable,
//     const SubrosaDG::Isize gmsh_physical_index) {
//   if (gmsh_physical_index == 1) {
//     boundary_primitive_variable = {
//         1.0_r, 4.0_r * 1.5_r * coordinate.y() * (0.41_r - coordinate.y()) / (0.41_r * 0.41_r), 0.0_r, 1.0_r};
//   } else if (gmsh_physical_index == 2) {
//     boundary_primitive_variable = {
//         1.0_r, 4.0_r * 1.5_r * coordinate.y() * (0.41_r - coordinate.y()) / (0.41_r * 0.41_r), 0.0_r, 1.0_r};
//   } else if (gmsh_physical_index == 3) {
//     boundary_primitive_variable = {1.0_r, 0.0_r, 0.0_r, 1.0_r};
//   } else if (gmsh_physical_index == 4) {
//     boundary_primitive_variable = {1.0_r, 0.0_r, 0.0_r, 1.0_r};
//   }
// }

// int main(int argc, char* argv[]) {
//   static_cast<void>(argc);
//   static_cast<void>(argv);
//   SubrosaDG::System<SimulationControl> system;
//   system.setMesh(kExampleDirectory / std::format("{}.msh", kExampleName), generateMesh);
//   system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::VelocityInflow>(1);
//   system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::PressureOutflow>(2);
//   system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticNonSlipWall>(3);
//   system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticNonSlipWall>(4);
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
    SubrosaDG::SolveControl<SubrosaDG::DimensionEnum::D2, SubrosaDG::PolynomialOrderEnum::P1,
                            SubrosaDG::BoundaryTimeEnum::TimeVarying, SubrosaDG::SourceTermEnum::None>,
    SubrosaDG::NumericalControl<SubrosaDG::MeshModelEnum::Quadrangle, SubrosaDG::InitialConditionEnum::Function,
                                SubrosaDG::TimeIntegrationEnum::SSPRK3>,
    SubrosaDG::IncompressibleNSVariable<SubrosaDG::ThermodynamicModelEnum::Constant,
                                        SubrosaDG::EquationOfStateEnum::WeakCompressibleFluid,
                                        SubrosaDG::TransportModelEnum::Constant,
                                        SubrosaDG::ConvectiveFluxEnum::LaxFriedrichs, SubrosaDG::ViscousFluxEnum::BR2>>;

template <typename SimulationControl>
inline void SubrosaDG::InitialCondition<SimulationControl>::computePrimitiveFromCoordinate(
    [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& coordinate,
    Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber>& initial_primitive_variable) {
  initial_primitive_variable = {1.0_r, 0.0_r, 0.0_r, 1.0_r};
}

template <typename SimulationControl>
inline void SubrosaDG::BoundaryCondition<SimulationControl>::computePrimitiveFromCoordinate(
    const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate,
    Eigen::Vector<Real, SimulationControl::kPrimitiveVariableNumber>& boundary_primitive_variable,
    const SubrosaDG::Real time, const SubrosaDG::Isize gmsh_physical_index) {
  if (gmsh_physical_index == 1) {
    boundary_primitive_variable = {1.0_r,
                                   4.0_r * 1.5_r * std::sin(SubrosaDG::kPi * time / 8.0_r) * coordinate.y() *
                                       (0.41_r - coordinate.y()) / (0.41_r * 0.41_r),
                                   0.0_r, 1.0_r};
  } else if (gmsh_physical_index == 2) {
    boundary_primitive_variable = {1.0_r,
                                   4.0_r * 1.5_r * std::sin(SubrosaDG::kPi * time / 8.0_r) * coordinate.y() *
                                       (0.41_r - coordinate.y()) / (0.41_r * 0.41_r),
                                   0.0_r, 1.0_r};
  } else if (gmsh_physical_index == 3) {
    boundary_primitive_variable = {1.0_r, 0.0_r, 0.0_r, 1.0_r};
  } else if (gmsh_physical_index == 4) {
    boundary_primitive_variable = {1.0_r, 0.0_r, 0.0_r, 1.0_r};
  }
}

template <typename SimulationControl>
inline void SubrosaDG::BoundaryConditionDevice<SimulationControl>::computePrimitiveFromCoordinate(
    const SubrosaDG::Device::View<const Device::Vector<SubrosaDG::Real, SimulationControl::kDimension>> coordinate,
    SubrosaDG::Device::StaticVector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>&
        boundary_primitive_variable,
    const SubrosaDG::Real time, const SubrosaDG::Isize gmsh_physical_index) {
  if (gmsh_physical_index == 1) {
    boundary_primitive_variable(0) = 1.0_r;
    boundary_primitive_variable(1) = 4.0_r * 1.5_r * sycl::sin(SubrosaDG::kPi * time / 8.0_r) * coordinate(1) *
                                     (0.41_r - coordinate(1)) / (0.41_r * 0.41_r);
    boundary_primitive_variable(2) = 0.0_r;
    boundary_primitive_variable(3) = 1.0_r;
  } else if (gmsh_physical_index == 2) {
    boundary_primitive_variable(0) = 1.0_r;
    boundary_primitive_variable(1) = 4.0_r * 1.5_r * sycl::sin(SubrosaDG::kPi * time / 8.0_r) * coordinate(1) *
                                     (0.41_r - coordinate(1)) / (0.41_r * 0.41_r);
    boundary_primitive_variable(2) = 0.0_r;
    boundary_primitive_variable(3) = 1.0_r;
  } else if (gmsh_physical_index == 3) {
    boundary_primitive_variable(0) = 1.0_r;
    boundary_primitive_variable(1) = 0.0_r;
    boundary_primitive_variable(2) = 0.0_r;
    boundary_primitive_variable(3) = 1.0_r;
  } else if (gmsh_physical_index == 4) {
    boundary_primitive_variable(0) = 1.0_r;
    boundary_primitive_variable(1) = 0.0_r;
    boundary_primitive_variable(2) = 0.0_r;
    boundary_primitive_variable(3) = 1.0_r;
  }
}

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::System<SimulationControl> system;
  system.setMesh(kExampleDirectory / std::format("{}.msh", kExampleName), generateMesh);
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::VelocityInflow>(1);
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::PressureOutflow>(2);
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticNonSlipWall>(3);
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticNonSlipWall>(4);
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
  const double x = 0.05 / std::numbers::sqrt2_v<double>;
  Eigen::Matrix<double, 2, 4, Eigen::RowMajor> farfield_point_coordinate;
  Eigen::Vector<double, 2> cylinder_point_coordinate;
  // clang-format off
  farfield_point_coordinate << 0.0, 0.2 - 2 * x, 0.2 + 2 * x, 2.2,
                               0.0, 0.2 - 2 * x, 0.2 + 2 * x, 0.41;
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
  std::array<std::vector<int>, 5> physical_group_tag;
  gmsh::model::add("cylinder_2d");
  const int center_point_tag = gmsh::model::geo::addPoint(0.2, 0.2, 0.0);
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      farfield_point_tag(j, i) =
          gmsh::model::geo::addPoint(farfield_point_coordinate(0, j), farfield_point_coordinate(1, i), 0.0);
    }
  }
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      cylinder_point_tag(j, i) =
          gmsh::model::geo::addPoint(cylinder_point_coordinate(j), cylinder_point_coordinate(i), 0.0);
    }
  }
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 3; j++) {
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
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 1; j++) {
      cylinder_line_tag(j, i, 0) =
          gmsh::model::geo::addCircleArc(cylinder_point_tag(j, i), center_point_tag, cylinder_point_tag(j + 1, i));
      cylinder_line_tag(j, i, 1) =
          gmsh::model::geo::addCircleArc(cylinder_point_tag(i, j), center_point_tag, cylinder_point_tag(i, j + 1));
    }
  }
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      connection_line_tag(j, i) = gmsh::model::geo::addLine(cylinder_point_tag(j, i), farfield_point_tag(j + 1, i + 1));
    }
  }
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
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
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      if (i == 1 && j == 1) {
        continue;
      }
      farfield_plane_surface_tag(j, i) = gmsh::model::geo::addPlaneSurface({farfield_curve_loop_tag(j, i)});
    }
  }
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      cylinder_plane_surface_tag(j, i) = gmsh::model::geo::addPlaneSurface({cylinder_curve_loop_tag(j, i)});
    }
  }
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 3; j++) {
      switch (j) {
      case 0:
        gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag(j, i, 0), 10, "Progression", -1.1);
        gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag(j, i, 1), 20, "Progression", 1.04);
        break;
      case 1:
        gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag(j, i, 0), 12);
        gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag(j, i, 1), 12);
        break;
      case 2:
        gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag(j, i, 0), 60, "Progression", 1.04);
        gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag(j, i, 1), 20, "Progression", -1.04);
        break;
      }
    }
  }
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      for (int k = 0; k < 1; k++) {
        gmsh::model::geo::mesh::setTransfiniteCurve(cylinder_line_tag(k, j, i), 12);
      }
    }
  }
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      gmsh::model::geo::mesh::setTransfiniteCurve(connection_line_tag(j, i), 12, "Progression", 1.1);
    }
  }
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      if (i == 1 && j == 1) {
        continue;
      }
      gmsh::model::geo::mesh::setTransfiniteSurface(farfield_plane_surface_tag(j, i));
      gmsh::model::geo::mesh::setRecombine(2, farfield_plane_surface_tag(j, i));
    }
  }
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      gmsh::model::geo::mesh::setTransfiniteSurface(cylinder_plane_surface_tag(j, i));
      gmsh::model::geo::mesh::setRecombine(2, cylinder_plane_surface_tag(j, i));
    }
  }
  gmsh::model::geo::synchronize();
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 4; j++) {
      for (int k = 0; k < 3; k++) {
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
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      for (int k = 0; k < 1; k++) {
        physical_group_tag[3].emplace_back(cylinder_line_tag(k, j, i));
      }
    }
  }
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      if (i == 1 && j == 1) {
        continue;
      }
      physical_group_tag[4].emplace_back(farfield_plane_surface_tag(j, i));
    }
  }
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      physical_group_tag[4].emplace_back(cylinder_plane_surface_tag(j, i));
    }
  }
  gmsh::model::addPhysicalGroup(1, physical_group_tag[0], 1, "bc-1");
  gmsh::model::addPhysicalGroup(1, physical_group_tag[1], 2, "bc-2");
  gmsh::model::addPhysicalGroup(1, physical_group_tag[2], 3, "bc-3");
  gmsh::model::addPhysicalGroup(1, physical_group_tag[3], 4, "bc-4");
  gmsh::model::addPhysicalGroup(2, physical_group_tag[4], 5, "vc-1");
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::setOrder(SimulationControl::kPolynomialOrder);
  gmsh::model::mesh::optimize("HighOrder");
  gmsh::write(mesh_file_path);
}
