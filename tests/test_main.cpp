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
#include <Eigen/Core>                 // for Block, Matrix, CommaInitializer, StorageOptions, SymbolExpr, Vector, lastN
#include <utility>                    // for make_pair
#include <algorithm>                  // for copy
#include <memory>                     // for allocator, unique_ptr, make_unique
#include <functional>                 // for reference_wrapper, ref
#include <vector>                     // for vector

#include "basic/environments.h"       // for EnvironmentGardian
#include "cmake.h"                    // for kProjectSourceDir
#include "basic/data_types.h"         // for Isize, Real, Usize
#include "config/config_defines.h"    // for BoundaryType, EquationOfState, NoVisFluxType, SimulationType, TimeInteg...
#include "config/config_structure.h"  // for Config, FlowParameter, ThermodynamicModel, TimeIntegration
#include "config/read_config.h"       // for readConfig
#include "mesh/cal_mesh_measure.h"    // for calculateMeshMeasure
#include "mesh/mesh_structure.h"      // for Mesh2d, Edge, Element, MeshSupplementalInfo
#include "mesh/read_mesh.h"           // for readMesh, readMeshSupplementalInfo
#include "mesh/reconstruct_edge.h"    // for reconstructEdge

// clang-format on

// NOLINTBEGIN(readability-function-cognitive-complexity)

TEST(TestMain, TestMain) {
  SubrosaDG::Internal::Config config;
  SubrosaDG::Internal::readConfig(SubrosaDG::kProjectSourceDir / "tests/dat/test.toml", config);
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
  SubrosaDG::Internal::Mesh2d mesh{config.mesh_file_};
  SubrosaDG::Internal::readMesh(mesh);
  ASSERT_EQ(dbg(mesh.node_num_), 21);
  ASSERT_EQ(dbg(mesh.element_num_), 34);
  ASSERT_EQ(dbg(mesh.triangle_element_.element_num_), std::make_pair(13L, 28L));
  Eigen::Vector<SubrosaDG::Real, 3> triangle_node =
      mesh.triangle_element_.element_nodes_->operator()(Eigen::lastN(3), Eigen::last);
  ASSERT_EQ(dbg(triangle_node),
            (Eigen::Vector<SubrosaDG::Real, 3>() << 0.2747662092153528, 0.0652513350269377, 0.0).finished());
  ASSERT_EQ(dbg(mesh.quadrangle_element_.element_num_), std::make_pair(29L, 34L));
  Eigen::Vector<SubrosaDG::Real, 3> quadrangle_node =
      mesh.quadrangle_element_.element_nodes_->operator()(Eigen::lastN(3), Eigen::last);
  ASSERT_EQ(dbg(quadrangle_node), (Eigen::Vector<SubrosaDG::Real, 3>() << 1.0, -0.5, 0.0).finished());
  SubrosaDG::Internal::MeshSupplementalInfo mesh_supplemental_info;
  SubrosaDG::Internal::readMeshSupplementalInfo(config, mesh_supplemental_info);
  auto boundary_index =
      static_cast<SubrosaDG::Internal::BoundaryType>(mesh_supplemental_info.boundary_index_->operator()(Eigen::last));
  ASSERT_EQ(dbg(magic_enum::enum_name(boundary_index)),
            magic_enum::enum_name(SubrosaDG::Internal::BoundaryType::Farfield));
  auto region_index = static_cast<SubrosaDG::Usize>(mesh_supplemental_info.region_index_->operator()(Eigen::last));
  ASSERT_EQ(dbg(config.region_name_[region_index - 1]), "vc-1");
  mesh.edge_num_ = SubrosaDG::Internal::reconstructEdge(
      mesh.nodes_, std::make_pair(std::ref(mesh.interior_edge_), std::ref(mesh.boundary_edge_)),
      mesh_supplemental_info);
  ASSERT_EQ(dbg(mesh.edge_num_), 42);
  ASSERT_EQ(dbg(mesh.interior_edge_.edge_num_), std::make_pair(13L, 42L));
  ASSERT_EQ(dbg(mesh.boundary_edge_.edge_num_), std::make_pair(1L, 12L));
  Eigen::Vector<SubrosaDG::Isize, 4> interior_edge_index =
      mesh.interior_edge_.edge_index_->operator()(Eigen::lastN(4), Eigen::last);
  ASSERT_EQ(dbg(interior_edge_index), (Eigen::Vector<SubrosaDG::Isize, 4>() << 2, 20, 31, 32).finished());
  Eigen::Vector<SubrosaDG::Isize, 4> boundary_edge_index =
      mesh.boundary_edge_.edge_index_->operator()(Eigen::lastN(4), Eigen::last);
  ASSERT_EQ(dbg(boundary_edge_index), (Eigen::Vector<SubrosaDG::Isize, 4>() << 12, 1, 14, -1).finished());
  SubrosaDG::Internal::calculateMeshMeasure(mesh.triangle_element_);
  SubrosaDG::Internal::calculateMeshMeasure(mesh.quadrangle_element_);
  SubrosaDG::Real area = mesh.triangle_element_.element_area_->sum() + mesh.quadrangle_element_.element_area_->sum();
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
    auto points = std::make_unique<Eigen::Matrix<double, 6, 3, Eigen::RowMajor>>();
    (*points) << -1.0, -0.5, 0.0, 0.0, -0.5, 0.0, 1.0, -0.5, 0.0, 1.0, 0.5, 0.0, 0.0, 0.5, 0.0, -1.0, 0.5, 0.0;
    constexpr double kLc1 = 0.5;
    gmsh::model::add("test");
    for (const auto& row : points->rowwise()) {
      gmsh::model::geo::addPoint(row.x(), row.y(), row.z(), kLc1);
    }
    for (int i = 0; i < points->rows(); i++) {
      gmsh::model::geo::addLine(i + 1, (i + 1) % points->rows() + 1);
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
