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

#include <dbg.h>                       // for type_name, DBG_MAP_1, DebugOutput, dbg
#include <gtest/gtest.h>               // for ASSERT_EQ, Message, TestPartResult, Test, InitGoogleTest, RUN_ALL_TESTS
#include <gmsh.h>                      // for addCurveLoop, addLine, addPhysicalGroup, addPlaneSurface, add, addPoint
#include <filesystem>                  // for operator/, path, exists
#include <Eigen/Core>                  // for Block, Matrix, CommaInitializer, SymbolExpr, Vector, DenseBase::operat...
#include <memory>                      // for allocator, unique_ptr
#include <algorithm>                   // for copy
#include <array>                       // for array
#include <map>                         // for map
#include <string_view>                 // for string_view, hash, basic_string_view, operator""sv, operator==, string...
#include <unordered_map>               // for unordered_map
#include <utility>                     // for make_pair
#include <vector>                      // for vector

#include "basic/environments.hpp"      // for EnvironmentGardian
#include "cmake.hpp"                   // for kProjectSourceDir
#include "mesh/mesh_structure.hpp"     // for Mesh2d, AdjanentLineElement, QuadrangleElement, TriangleElement
#include "mesh/get_mesh.hpp"           // for getMesh
#include "basic/enums.hpp"             // for BoundaryType, EquationOfState, TimeIntegrationType
#include "mesh/calculate_measure.hpp"  // for calculateElementMeasure
#include "basic/configs.hpp"           // for FlowParameter, ThermodynamicModel, TimeIntegration
#include "basic/data_types.hpp"        // for Isize, Real

// clang-format on

using namespace std::string_view_literals;

inline constexpr SubrosaDG::TimeIntegration<SubrosaDG::TimeIntegrationType::ExplicitEuler> kTimeIntegration{1000, 0.1,
                                                                                                            -10};

inline const std::unordered_map<std::string_view, SubrosaDG::BoundaryType> kBoundaryTypeMap{
    {"bc-1", SubrosaDG::BoundaryType::Farfield}};

inline constexpr SubrosaDG::ThermodynamicModel<SubrosaDG::EquationOfState::IdealGas> kThermodynamicModel{
    1.4, 1.0, 0.7142857142857143};

inline const std::unordered_map<std::string_view, SubrosaDG::Isize> kRegionIdMap{{"vc-1"sv, 1L}};

inline constexpr std::array<SubrosaDG::FlowParameter, 1> kInitialParameter{
    SubrosaDG::FlowParameter{{1.0, 0.5, 0.0}, 1.4, 1.0, 1.0}};

inline constexpr SubrosaDG::FlowParameter kFarfieldParameter{{1.0, 0.5, 0.0}, 1.4, 1.0, 1.0};

// NOLINTBEGIN(readability-function-cognitive-complexity)

TEST(TestMain, TestMain) {
  SubrosaDG::Mesh2d<3> mesh{SubrosaDG::kProjectSourceDir / "build/out/test/mesh/test.msh"};
  SubrosaDG::getMesh(kBoundaryTypeMap, mesh);

  ASSERT_EQ(dbg(mesh.nodes_num_), 21);

  ASSERT_EQ(dbg(mesh.triangle_.elements_range_), std::make_pair(13L, 28L));
  Eigen::Vector<SubrosaDG::Real, 3> triangle_node = mesh.triangle_.elements_nodes_(Eigen::lastN(3), Eigen::last);
  ASSERT_EQ(dbg(triangle_node),
            (Eigen::Vector<SubrosaDG::Real, 3>() << 0.2747662092153528, 0.0652513350269377, 0.0).finished());
  ASSERT_EQ(dbg(mesh.quadrangle_.elements_range_), std::make_pair(29L, 34L));
  Eigen::Vector<SubrosaDG::Real, 3> quadrangle_node = mesh.quadrangle_.elements_nodes_(Eigen::lastN(3), Eigen::last);
  ASSERT_EQ(dbg(quadrangle_node), (Eigen::Vector<SubrosaDG::Real, 3>() << 1.0, -0.5, 0.0).finished());

  ASSERT_EQ(dbg(mesh.internal_line_.elements_range_), std::make_pair(35L, 64L));
  ASSERT_EQ(dbg(mesh.boundary_line_.elements_range_), std::make_pair(1L, 12L));
  Eigen::Vector<SubrosaDG::Isize, 4> internal_line_index =
      mesh.internal_line_.elements_index_(Eigen::lastN(4), Eigen::last);
  ASSERT_EQ(dbg(internal_line_index), (Eigen::Vector<SubrosaDG::Isize, 4>() << 2, 20, 31, 32).finished());
  Eigen::Vector<SubrosaDG::Isize, 4> boundary_line_index =
      mesh.boundary_line_.elements_index_(Eigen::lastN(4), Eigen::last);
  ASSERT_EQ(dbg(boundary_line_index), (Eigen::Vector<SubrosaDG::Isize, 4>() << 12, 1, 14, -1).finished());

  auto triangle_area = SubrosaDG::calculateElementMeasure(mesh.triangle_);
  auto quadrangle_area = SubrosaDG::calculateElementMeasure(mesh.quadrangle_);
  SubrosaDG::Real area = triangle_area->sum() + quadrangle_area->sum();
  ASSERT_EQ(dbg(area), 2.0);
}

// NOLINTEND(readability-function-cognitive-complexity)

void generateMesh(const std::filesystem::path& mesh_file) {
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

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  SubrosaDG::EnvironmentGardian environment_gardian;
  std::filesystem::path mesh_file = SubrosaDG::kProjectSourceDir / "build/out/test/mesh/test.msh";
  if (!std::filesystem::exists(mesh_file)) {
    generateMesh(mesh_file);
  }
  return RUN_ALL_TESTS();
}
