/**
 * @file sphere_3d_cns.cpp
 * @brief The 3D sphere flow example with Navier-Stokes equations.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2024-05-15
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2025 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#include "SubrosaDG.cpp"

inline const std::string kExampleName{"sphere_3d_cns"};

inline const std::filesystem::path kExampleDirectory{SubrosaDG::kProjectSourceDirectory / "build/out" / kExampleName};

using SimulationControl = SubrosaDG::SimulationControl<
    SubrosaDG::SolveControl<SubrosaDG::DimensionEnum::D3, SubrosaDG::PolynomialOrderEnum::P3,
                            SubrosaDG::BoundaryTimeEnum::Steady, SubrosaDG::SourceTermEnum::None>,
    SubrosaDG::NumericalControl<SubrosaDG::MeshModelEnum::Hexahedron, SubrosaDG::ShockCapturingEnum::None,
                                SubrosaDG::LimiterEnum::None, SubrosaDG::InitialConditionEnum::Function,
                                SubrosaDG::TimeIntegrationEnum::SSPRK3>,
    SubrosaDG::CompresibleNSVariable<SubrosaDG::ThermodynamicModelEnum::Constant,
                                     SubrosaDG::EquationOfStateEnum::IdealGas, SubrosaDG::TransportModelEnum::Constant,
                                     SubrosaDG::ConvectiveFluxEnum::HLLC, SubrosaDG::ViscousFluxEnum::BR2>>;

template <typename SimulationControl>
inline Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>
SubrosaDG::InitialCondition<SimulationControl>::calculatePrimitiveFromCoordinate(
    [[maybe_unused]] const Eigen::Vector<Real, SimulationControl::kDimension>& coordinate) const {
  return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{1.4_r, 0.0_r, 0.2_r, 0.0_r, 1.0_r};
}

template <typename SimulationControl>
inline Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>
SubrosaDG::BoundaryCondition<SimulationControl>::calculatePrimitiveFromCoordinate(
    [[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, SimulationControl::kDimension>& coordinate,
    const SubrosaDG::Isize gmsh_physical_index) const {
  if (gmsh_physical_index == 1) {
    return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{1.4_r, 0.0_r, 0.2_r, 0.0_r,
                                                                                       1.0_r};
  }
  if (gmsh_physical_index == 2) {
    return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{1.4_r, 0.0_r, 0.0_r, 0.0_r,
                                                                                       1.0_r};
  }
  return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>::Zero();
}

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::System<SimulationControl> system;
  system.setMesh(kExampleDirectory / "sphere_3d_cns.msh", generateMesh);
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::RiemannFarfield>(1);
  system.addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticNonSlipWall>(2);
  system.setThermodynamicModel<SimulationControl::kThermodynamicModel>(2.5_r, 25.0_r / 14.0_r);
  system.setTransportModel<SimulationControl::kTransportModel>(1.4_r * 0.2_r / 200.0_r);
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
  const double x = std::sqrt(3.0) / 6.0;
  Eigen::Vector<double, 4> farfield_point_coordinate;
  Eigen::Vector<double, 2> sphere_point_coordinate;
  // clang-format off
  farfield_point_coordinate << -5.0, -2.0 * x, 2.0 * x, 5.0;
  sphere_point_coordinate << -x, x;
  // clang-format on
  Eigen::Tensor<int, 3> farfield_point_tag(4, 4, 4);
  Eigen::Tensor<int, 3> sphere_point_tag(2, 2, 2);
  Eigen::Tensor<int, 4> farfield_line_tag(3, 4, 4, 3);
  Eigen::Tensor<int, 4> sphere_line_tag(1, 2, 2, 3);
  Eigen::Tensor<int, 3> connection_line_tag(2, 2, 2);
  Eigen::Tensor<int, 4> farfield_curve_loop_tag(3, 3, 4, 3);
  Eigen::Tensor<int, 4> sphere_curve_loop_tag(1, 1, 2, 3);
  Eigen::Tensor<int, 3> connection_curve_loop_tag(2, 2, 3);
  Eigen::Tensor<int, 4> farfield_surface_filling_tag(3, 3, 4, 3);
  Eigen::Tensor<int, 4> sphere_surface_filling_tag(1, 1, 2, 3);
  Eigen::Tensor<int, 3> connection_surface_filling_tag(2, 2, 3);
  Eigen::Tensor<int, 3> farfield_surface_loop_tag(3, 3, 3);
  Eigen::Tensor<int, 2> sphere_surface_loop_tag(2, 3);
  Eigen::Tensor<int, 3> farfield_volume_tag(3, 3, 3);
  Eigen::Tensor<int, 2> sphere_volume_tag(2, 3);
  std::array<std::vector<int>, 3> physical_group_tag;
  gmsh::model::add("sphere_3d");
  const int center_point_tag = gmsh::model::geo::addPoint(0.0, 0.0, 0.0);
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      for (int k = 0; k < 4; k++) {
        farfield_point_tag(k, j, i) = gmsh::model::geo::addPoint(
            farfield_point_coordinate(k), farfield_point_coordinate(j), farfield_point_coordinate(i));
      }
    }
  }
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      for (int k = 0; k < 2; k++) {
        sphere_point_tag(k, j, i) = gmsh::model::geo::addPoint(sphere_point_coordinate(k), sphere_point_coordinate(j),
                                                               sphere_point_coordinate(i));
      }
    }
  }
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      for (int k = 0; k < 3; k++) {
        if ((i == 1 || i == 2) && (j == 1 || j == 2) && (k == 1)) {
          farfield_line_tag(k, j, i, 0) = gmsh::model::geo::addCircleArc(farfield_point_tag(k, j, i), center_point_tag,
                                                                         farfield_point_tag(k + 1, j, i));
          farfield_line_tag(k, j, i, 1) = gmsh::model::geo::addCircleArc(farfield_point_tag(j, k, i), center_point_tag,
                                                                         farfield_point_tag(j, k + 1, i));
          farfield_line_tag(k, j, i, 2) = gmsh::model::geo::addCircleArc(farfield_point_tag(j, i, k), center_point_tag,
                                                                         farfield_point_tag(j, i, k + 1));
        } else {
          farfield_line_tag(k, j, i, 0) =
              gmsh::model::geo::addLine(farfield_point_tag(k, j, i), farfield_point_tag(k + 1, j, i));
          farfield_line_tag(k, j, i, 1) =
              gmsh::model::geo::addLine(farfield_point_tag(j, k, i), farfield_point_tag(j, k + 1, i));
          farfield_line_tag(k, j, i, 2) =
              gmsh::model::geo::addLine(farfield_point_tag(j, i, k), farfield_point_tag(j, i, k + 1));
        }
      }
    }
  }
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      for (int k = 0; k < 1; k++) {
        sphere_line_tag(k, j, i, 0) =
            gmsh::model::geo::addCircleArc(sphere_point_tag(k, j, i), center_point_tag, sphere_point_tag(k + 1, j, i));
        sphere_line_tag(k, j, i, 1) =
            gmsh::model::geo::addCircleArc(sphere_point_tag(j, k, i), center_point_tag, sphere_point_tag(j, k + 1, i));
        sphere_line_tag(k, j, i, 2) =
            gmsh::model::geo::addCircleArc(sphere_point_tag(j, i, k), center_point_tag, sphere_point_tag(j, i, k + 1));
      }
    }
  }
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      for (int k = 0; k < 2; k++) {
        connection_line_tag(k, j, i) =
            gmsh::model::geo::addLine(sphere_point_tag(k, j, i), farfield_point_tag(k + 1, j + 1, i + 1));
      }
    }
  }
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 3; j++) {
      for (int k = 0; k < 3; k++) {
        farfield_curve_loop_tag(k, j, i, 0) =
            gmsh::model::geo::addCurveLoop({farfield_line_tag(k, j, i, 0), farfield_line_tag(j, k + 1, i, 1),
                                            -farfield_line_tag(k, j + 1, i, 0), -farfield_line_tag(j, k, i, 1)});
        farfield_curve_loop_tag(k, j, i, 1) =
            gmsh::model::geo::addCurveLoop({farfield_line_tag(k, i, j, 1), farfield_line_tag(j, i, k + 1, 2),
                                            -farfield_line_tag(k, i, j + 1, 1), -farfield_line_tag(j, i, k, 2)});
        farfield_curve_loop_tag(k, j, i, 2) =
            gmsh::model::geo::addCurveLoop({farfield_line_tag(k, j, i, 2), farfield_line_tag(j, i, k + 1, 0),
                                            -farfield_line_tag(k, j + 1, i, 2), -farfield_line_tag(j, i, k, 0)});
      }
    }
  }
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 1; j++) {
      for (int k = 0; k < 1; k++) {
        sphere_curve_loop_tag(k, j, i, 0) =
            gmsh::model::geo::addCurveLoop({sphere_line_tag(k, j, i, 0), sphere_line_tag(j, k + 1, i, 1),
                                            -sphere_line_tag(k, j + 1, i, 0), -sphere_line_tag(j, k, i, 1)});
        sphere_curve_loop_tag(k, j, i, 1) =
            gmsh::model::geo::addCurveLoop({sphere_line_tag(k, i, j, 1), sphere_line_tag(j, i, k + 1, 2),
                                            -sphere_line_tag(k, i, j + 1, 1), -sphere_line_tag(j, i, k, 2)});
        sphere_curve_loop_tag(k, j, i, 2) =
            gmsh::model::geo::addCurveLoop({sphere_line_tag(k, j, i, 2), sphere_line_tag(j, i, k + 1, 0),
                                            -sphere_line_tag(k, j + 1, i, 2), -sphere_line_tag(j, i, k, 0)});
      }
    }
  }
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      connection_curve_loop_tag(j, i, 0) =
          gmsh::model::geo::addCurveLoop({connection_line_tag(0, j, i), farfield_line_tag(1, j + 1, i + 1, 0),
                                          -connection_line_tag(1, j, i), -sphere_line_tag(0, j, i, 0)});
      connection_curve_loop_tag(j, i, 1) =
          gmsh::model::geo::addCurveLoop({connection_line_tag(j, 0, i), farfield_line_tag(1, j + 1, i + 1, 1),
                                          -connection_line_tag(j, 1, i), -sphere_line_tag(0, j, i, 1)});
      connection_curve_loop_tag(j, i, 2) =
          gmsh::model::geo::addCurveLoop({connection_line_tag(j, i, 0), farfield_line_tag(1, j + 1, i + 1, 2),
                                          -connection_line_tag(j, i, 1), -sphere_line_tag(0, j, i, 2)});
    }
  }
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 4; j++) {
      for (int k = 0; k < 3; k++) {
        for (int l = 0; l < 3; l++) {
          farfield_surface_filling_tag(l, k, j, i) =
              gmsh::model::geo::addSurfaceFilling({farfield_curve_loop_tag(l, k, j, i)});
        }
      }
    }
  }
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 1; j++) {
      for (int k = 0; k < 1; k++) {
        sphere_surface_filling_tag(k, j, 0, i) =
            gmsh::model::geo::addSurfaceFilling({sphere_curve_loop_tag(k, j, 0, i)});
        sphere_surface_filling_tag(k, j, 1, i) =
            gmsh::model::geo::addSurfaceFilling({-sphere_curve_loop_tag(k, j, 1, i)});
      }
    }
  }
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 2; j++) {
      for (int k = 0; k < 2; k++) {
        connection_surface_filling_tag(k, j, i) =
            gmsh::model::geo::addSurfaceFilling({connection_curve_loop_tag(k, j, i)});
      }
    }
  }
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      for (int k = 0; k < 3; k++) {
        if ((i == 1) && (j == 1) && (k == 1)) {
          continue;
        }
        farfield_surface_loop_tag(k, j, i) = gmsh::model::geo::addSurfaceLoop(
            {farfield_surface_filling_tag(k, j, i, 0), farfield_surface_filling_tag(k, j, i + 1, 0),
             farfield_surface_filling_tag(j, i, k, 1), farfield_surface_filling_tag(j, i, k + 1, 1),
             farfield_surface_filling_tag(i, k, j, 2), farfield_surface_filling_tag(i, k, j + 1, 2)});
        farfield_volume_tag(k, j, i) = gmsh::model::geo::addVolume({farfield_surface_loop_tag(k, j, i)});
      }
    }
  }
  for (int i = 0; i < 2; i++) {
    sphere_surface_loop_tag(i, 0) = gmsh::model::geo::addSurfaceLoop(
        {connection_surface_filling_tag(0, i, 0), connection_surface_filling_tag(1, i, 0),
         connection_surface_filling_tag(0, i, 1), connection_surface_filling_tag(1, i, 1),
         sphere_surface_filling_tag(0, 0, i, 0), farfield_surface_filling_tag(1, 1, i + 1, 0)});
    sphere_surface_loop_tag(i, 1) = gmsh::model::geo::addSurfaceLoop(
        {connection_surface_filling_tag(i, 0, 1), connection_surface_filling_tag(i, 1, 1),
         connection_surface_filling_tag(i, 0, 2), connection_surface_filling_tag(i, 1, 2),
         sphere_surface_filling_tag(0, 0, i, 1), farfield_surface_filling_tag(1, 1, i + 1, 1)});
    sphere_surface_loop_tag(i, 2) = gmsh::model::geo::addSurfaceLoop(
        {connection_surface_filling_tag(0, i, 2), connection_surface_filling_tag(1, i, 2),
         connection_surface_filling_tag(i, 0, 0), connection_surface_filling_tag(i, 1, 0),
         sphere_surface_filling_tag(0, 0, i, 2), farfield_surface_filling_tag(1, 1, i + 1, 2)});
  }
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 2; j++) {
      sphere_volume_tag(j, i) = gmsh::model::geo::addVolume({sphere_surface_loop_tag(j, i)});
    }
  }
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 4; j++) {
      for (int k = 0; k < 4; k++) {
        for (int l = 0; l < 3; l++) {
          switch (l) {
          case 0:
            gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag(l, k, j, i), 10, "Progression", -1.3);
            break;
          case 1:
            gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag(l, k, j, i), 12);
            break;
          case 2:
            gmsh::model::geo::mesh::setTransfiniteCurve(farfield_line_tag(l, k, j, i), 10, "Progression", 1.3);
            break;
          }
        }
      }
    }
  }
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 2; j++) {
      for (int k = 0; k < 2; k++) {
        for (int l = 0; l < 1; l++) {
          gmsh::model::geo::mesh::setTransfiniteCurve(sphere_line_tag(l, k, j, i), 12);
        }
      }
    }
  }
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      for (int k = 0; k < 2; k++) {
        gmsh::model::geo::mesh::setTransfiniteCurve(connection_line_tag(k, j, i), 10, "Progression", 1.2);
      }
    }
  }
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 4; j++) {
      for (int k = 0; k < 3; k++) {
        for (int l = 0; l < 3; l++) {
          gmsh::model::geo::mesh::setTransfiniteSurface(farfield_surface_filling_tag(l, k, j, i));
          gmsh::model::geo::mesh::setRecombine(2, farfield_surface_filling_tag(l, k, j, i));
        }
      }
    }
  }
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 2; j++) {
      for (int k = 0; k < 1; k++) {
        for (int l = 0; l < 1; l++) {
          gmsh::model::geo::mesh::setTransfiniteSurface(sphere_surface_filling_tag(l, k, j, i));
          gmsh::model::geo::mesh::setRecombine(2, sphere_surface_filling_tag(l, k, j, i));
        }
      }
    }
  }
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 2; j++) {
      for (int k = 0; k < 2; k++) {
        gmsh::model::geo::mesh::setTransfiniteSurface(connection_surface_filling_tag(k, j, i));
        gmsh::model::geo::mesh::setRecombine(2, connection_surface_filling_tag(k, j, i));
      }
    }
  }
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      for (int k = 0; k < 3; k++) {
        if (i == 1 && j == 1 && k == 1) {
          continue;
        }
        gmsh::model::geo::mesh::setTransfiniteVolume(farfield_volume_tag(k, j, i));
        gmsh::model::geo::mesh::setRecombine(3, farfield_volume_tag(k, j, i));
      }
    }
  }
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 2; j++) {
      gmsh::model::geo::mesh::setTransfiniteVolume(sphere_volume_tag(j, i));
      gmsh::model::geo::mesh::setRecombine(3, sphere_volume_tag(j, i));
    }
  }
  gmsh::model::geo::synchronize();
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 4; j++) {
      for (int k = 0; k < 3; k++) {
        for (int l = 0; l < 3; l++) {
          if (j == 0 || j == 3) {
            physical_group_tag[0].emplace_back(farfield_surface_filling_tag(l, k, j, i));
          }
        }
      }
    }
  }
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 2; j++) {
      for (int k = 0; k < 1; k++) {
        for (int l = 0; l < 1; l++) {
          physical_group_tag[1].emplace_back(sphere_surface_filling_tag(l, k, j, i));
        }
      }
    }
  }
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      for (int k = 0; k < 3; k++) {
        if (i == 1 && j == 1 && k == 1) {
          continue;
        }
        physical_group_tag[2].emplace_back(farfield_volume_tag(k, j, i));
      }
    }
  }
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 2; j++) {
      physical_group_tag[2].emplace_back(sphere_volume_tag(j, i));
    }
  }
  gmsh::model::addPhysicalGroup(2, physical_group_tag[0], 1, "bc-1");
  gmsh::model::addPhysicalGroup(2, physical_group_tag[1], 2, "bc-2");
  gmsh::model::addPhysicalGroup(3, physical_group_tag[2], 3, "vc-1");
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::setOrder(SimulationControl::kPolynomialOrder);
  gmsh::model::mesh::optimize("HighOrder");
  gmsh::write(mesh_file_path);
}
