/**
 * @file test_main.cpp
 * @brief The main source file of SubroseDG tests.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2022-11-02
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

// IWYU pragma: private, include "gtest/gtest.h"

// clang-format off

#include <dbg.h>                      // for type_name, DBG_MAP_1, DebugOutput, dbg
#include <gtest/gtest.h>              // for ASSERT_EQ, Message, TestPartResult, Test, InitGoogleTest, RUN_ALL_TESTS
#include <magic_enum.hpp>             // for enum_name
#include <gmsh.h>                     // for addCurveLoop, addLine, addPhysicalGroup, addPlaneSurface, add, addPoint
#include <filesystem>                 // for operator/, path, exists
#include <string>                     // for string, operator==, hash
#include <unordered_map>              // for unordered_map
#include <Eigen/Core>                 // for Block, Matrix, CommaInitializer, Vector, SymbolExpr, DenseBase::operator()
#include <utility>                    // for make_pair
#include <algorithm>                  // for copy
#include <memory>                     // for allocator, unique_ptr

#include "basic/environments.h"       // for EnvironmentGardian
#include "cmake.h"                    // for kProjectSourceDir
#include "basic/data_types.h"         // for Isize, Real
#include "config/config_defines.h"    // for BoundaryType, EquationOfState, NoVisFluxType, SimulationType, TimeInteg...
#include "config/config_structure.h"  // for Config, FlowParameter, ThermodynamicModel, TimeIntegration
#include "config/get_config.h"        // for getConfig
#include "mesh/calculate_measure.h"   // for calculateElementMeasure
#include "mesh/mesh_structure.h"      // for Mesh2d, AdjanencyElement, Element
#include "mesh/get_mesh.h"            // for getMesh

// clang-format on

// NOLINTBEGIN(readability-function-cognitive-complexity)

TEST(TestMain, TestMain) {
  SubrosaDG::Internal::Config config;
  SubrosaDG::Internal::getConfig(SubrosaDG::kProjectSourceDir / "tests/dat/test.toml", config);

  ASSERT_EQ(dbg(config.dimension_), 2);
  ASSERT_EQ(dbg(config.polynomial_order_), 2);
  ASSERT_EQ(dbg(magic_enum::enum_name(config.simulation_type_)),
            magic_enum::enum_name(SubrosaDG::Internal::SimulationType::Euler));
  ASSERT_EQ(dbg(magic_enum::enum_name(config.no_vis_flux_type_)),
            magic_enum::enum_name(SubrosaDG::Internal::NoVisFluxType::Central));
  ASSERT_EQ(dbg(config.mesh_file_), SubrosaDG::kProjectSourceDir / "build/out/test/mesh/test.msh");
  ASSERT_EQ(dbg(magic_enum::enum_name(config.time_integration_.time_integration_type_)),
            magic_enum::enum_name(SubrosaDG::Internal::TimeIntegrationType::ExplicitEuler));
  ASSERT_EQ(dbg(config.time_integration_.iteration_), 2000);
  ASSERT_EQ(dbg(config.time_integration_.cfl_), 0.1);
  ASSERT_EQ(dbg(config.time_integration_.tolerance_), -10);
  ASSERT_EQ(dbg(magic_enum::enum_name(config.boundary_condition_["bc-1"])),
            magic_enum::enum_name(SubrosaDG::Internal::BoundaryType::Farfield));
  ASSERT_EQ(dbg(magic_enum::enum_name(config.thermodynamic_model_.equation_of_state_)),
            magic_enum::enum_name(SubrosaDG::Internal::EquationOfState::IdealGas));
  ASSERT_EQ(dbg(config.thermodynamic_model_.gamma_), 1.4);
  ASSERT_EQ(dbg(config.thermodynamic_model_.c_p_), 1.0);
  ASSERT_EQ(dbg(config.thermodynamic_model_.r_), 0.7142857142857143);
  ASSERT_EQ(dbg(config.initial_condition_["vc-1"].u_),
            (Eigen::Vector<SubrosaDG::Real, 3>() << 1.0, 0.5, 0.0).finished());
  ASSERT_EQ(dbg(config.initial_condition_["vc-1"].rho_), 1.4);
  ASSERT_EQ(dbg(config.initial_condition_["vc-1"].p_), 1.0);
  ASSERT_EQ(dbg(config.initial_condition_["vc-1"].t_), 1.0);
  ASSERT_EQ(dbg(config.farfield_parameter_.u_), (Eigen::Vector<SubrosaDG::Real, 3>() << 1.0, 0.5, 0.0).finished());
  ASSERT_EQ(dbg(config.farfield_parameter_.rho_), 1.4);
  ASSERT_EQ(dbg(config.farfield_parameter_.p_), 1.0);
  ASSERT_EQ(dbg(config.farfield_parameter_.t_), 1.0);

  SubrosaDG::Internal::Mesh2d mesh{config.mesh_file_, config.polynomial_order_};
  SubrosaDG::Internal::getMesh(config, mesh);

  ASSERT_EQ(dbg(mesh.nodes_num_), 21);

  ASSERT_EQ(dbg(mesh.triangle_.elements_range_), std::make_pair(13L, 28L));
  Eigen::Vector<SubrosaDG::Real, 3> triangle_node = mesh.triangle_.elements_nodes_(Eigen::lastN(3), Eigen::last);
  ASSERT_EQ(dbg(triangle_node),
            (Eigen::Vector<SubrosaDG::Real, 3>() << 0.2747662092153528, 0.0652513350269377, 0.0).finished());
  ASSERT_EQ(dbg(mesh.quadrangle_.elements_range_), std::make_pair(29L, 34L));
  Eigen::Vector<SubrosaDG::Real, 3> quadrangle_node = mesh.quadrangle_.elements_nodes_(Eigen::lastN(3), Eigen::last);
  ASSERT_EQ(dbg(quadrangle_node), (Eigen::Vector<SubrosaDG::Real, 3>() << 1.0, -0.5, 0.0).finished());

  ASSERT_EQ(dbg(mesh.interior_line_.elements_range_), std::make_pair(13L, 42L));
  ASSERT_EQ(dbg(mesh.boundary_line_.elements_range_), std::make_pair(1L, 12L));
  Eigen::Vector<SubrosaDG::Isize, 4> interior_line_index =
      mesh.interior_line_.elements_index_(Eigen::lastN(4), Eigen::last);
  ASSERT_EQ(dbg(interior_line_index), (Eigen::Vector<SubrosaDG::Isize, 4>() << 2, 20, 31, 32).finished());
  Eigen::Vector<SubrosaDG::Isize, 4> boundary_line_index =
      mesh.boundary_line_.elements_index_(Eigen::lastN(4), Eigen::last);
  ASSERT_EQ(dbg(boundary_line_index), (Eigen::Vector<SubrosaDG::Isize, 4>() << 12, 1, 14, -1).finished());

  auto triangle_area = SubrosaDG::Internal::calculateElementMeasure(mesh.triangle_);
  auto quadrangle_area = SubrosaDG::Internal::calculateElementMeasure(mesh.quadrangle_);
  SubrosaDG::Real area = triangle_area->sum() + quadrangle_area->sum();
  ASSERT_EQ(dbg(area), 2.0);
}

// NOLINTEND(readability-function-cognitive-complexity)

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  SubrosaDG::EnvironmentGardian environment_gardian;
  std::filesystem::path mesh_file = SubrosaDG::kProjectSourceDir / "build/out/test/mesh/test.msh";
  if (!std::filesystem::exists(mesh_file)) {
    // NOTE: This gmsh doesn't compile with Blossom(fedora build file said: blossoms is nonfree, see
    // contrib/blossoms/README.txt), so this mesh is different from other version of gmsh which defaultly uses Blossom.
    // Gmsh version 4.10.5.
    // gmsh::option::setNumber("Mesh.RecombinationAlgorithm", 1);
    Eigen::Matrix<double, 6, 3, Eigen::RowMajor> points;
    points << -1.0, -0.5, 0.0, 0.0, -0.5, 0.0, 1.0, -0.5, 0.0, 1.0, 0.5, 0.0, 0.0, 0.5, 0.0, -1.0, 0.5, 0.0;
    constexpr double kLc1 = 0.5;
    gmsh::model::add("test");
    for (const auto& row : points.rowwise()) {
      gmsh::model::geo::addPoint(row.x(), row.y(), row.z(), kLc1);
    }
    for (int i = 0; i < points.rows(); i++) {
      gmsh::model::geo::addLine(i + 1, (i + 1) % points.rows() + 1);
    }
    gmsh::model::geo::addLine(2, 5);
    gmsh::model::geo::addCurveLoop({1, 7, 5, 6});
    gmsh::model::geo::addPlaneSurface({1});
    gmsh::model::geo::addCurveLoop({2, 3, 4, -7});
    gmsh::model::geo::addPlaneSurface({2});
    gmsh::model::geo::mesh::setRecombine(2, 2);
    gmsh::model::geo::synchronize();
    gmsh::model::addPhysicalGroup(1, {1, 2, 3, 4, 5, 6}, -1, "bc-1");
    gmsh::model::addPhysicalGroup(2, {1, 2}, -1, "vc-1");
    gmsh::model::mesh::generate(2);
    gmsh::write(mesh_file.string());
    gmsh::clear();
  }
  return RUN_ALL_TESTS();
}
