/**
 * @file test_2d.cpp
 * @brief The source file to test 2d cases.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-05-17
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

// IWYU pragma: private, include "gtest/gtest.h"

// clang-format off

#include <dbg.h>                               // for type_name, DBG_MAP_1, DebugOutput, dbg
#include <gtest/gtest.h>                       // for Message, TestPartResult, ASSERT_NEAR, TestInfo (ptr only), ASS...
#include <gmsh.h>                              // for addCurveLoop, addLine, addPhysicalGroup, addPlaneSurface, add
#include <filesystem>                          // for path, exists, operator/
#include <Eigen/Core>                          // for Block, DenseBase::operator(), Vector, SymbolExpr, Matrix, Comm...
#include <Eigen/LU>                            // for MatrixBase::inverse
#include <algorithm>                           // for copy
#include <array>                               // for array
#include <string_view>                         // for string_view, hash, basic_string_view, operator""sv, operator==
#include <unordered_map>                       // for unordered_map
#include <utility>                             // for make_pair, pair
#include <vector>                              // for vector
#include <memory>                              // for allocator, unique_ptr
#include <map>                                 // for map

#include "cmake.hpp"                           // for kProjectSourceDir
#include "basic/constants.hpp"                 // for kEpsilon
#include "mesh/mesh_structure.hpp"             // for Mesh2d, AdjacencyLineElement, TriangleElement, QuadrangleElement
#include "mesh/get_mesh.hpp"                   // for getMesh
#include "basic/enums.hpp"                     // for Boundary, EquationOfState
#include "basic/configs.hpp"                   // for FlowParameter, ThermodynamicModel, TimeParameter, kExplicitEuler
#include "basic/data_types.hpp"                // for Real, Isize
#include "mesh/element/calculate_measure.hpp"  // for FElementMeasure
#include "mesh/element_types.hpp"              // for ElementType, kQuadrangle, kTriangle, kLine

// clang-format on

using namespace std::string_view_literals;

inline constexpr SubrosaDG::TimeParameter<SubrosaDG::kExplicitEuler> kTimeIntegration{1000, 0.1, -10};

inline const std::unordered_map<std::string_view, SubrosaDG::Boundary> kBoundaryTypeMap{
    {"bc-1", SubrosaDG::Boundary::Farfield}};

inline constexpr SubrosaDG::ThermodynamicModel<SubrosaDG::EquationOfState::IdealGas> kThermodynamicModel{
    1.4, 1.0, 0.7142857142857143};

inline const std::unordered_map<std::string_view, SubrosaDG::Isize> kRegionIdMap{{"vc-1"sv, 1L}};

inline constexpr std::array<SubrosaDG::FlowParameter<2>, 1> kInitialParameter{
    SubrosaDG::FlowParameter<2>{{1.0, 0.5}, 1.4, 1.0, 1.0}};

inline constexpr SubrosaDG::FlowParameter<2> kFarfieldParameter{{1.0, 0.5}, 1.4, 1.0, 1.0};

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

struct Test2d : testing::Test {
  static SubrosaDG::Mesh2d<2>* mesh;

  static void SetUpTestCase() {
    std::filesystem::path mesh_file = SubrosaDG::kProjectSourceDir / "build/out/test/mesh/test.msh";
    if (!std::filesystem::exists(mesh_file)) {
      generateMesh(mesh_file);
    }
    Test2d::mesh = new SubrosaDG::Mesh2d<2>{mesh_file};
    SubrosaDG::getMesh(kBoundaryTypeMap, *Test2d::mesh);
  }

  static void TearDownTestCase() {
    delete Test2d::mesh;
    Test2d::mesh = nullptr;
  }
};

SubrosaDG::Mesh2d<2>* Test2d::mesh;

TEST_F(Test2d, MeshIntegralTest) {
  ASSERT_EQ(dbg(mesh->line_.integral_nodes_num), 3);
  Eigen::Vector<SubrosaDG::Real, 1> line_integral_node = mesh->line_.integral_nodes(Eigen::all, Eigen::last);
  ASSERT_NEAR(dbg(line_integral_node.x()), 0.7745966692414834, SubrosaDG::kEpsilon);
}

TEST_F(Test2d, MeshGradIntegralTest) {
  Eigen::Vector<SubrosaDG::Real, 2> triangle_integral_node = mesh->triangle_.integral_nodes(Eigen::all, Eigen::last);
  ASSERT_NEAR(dbg(triangle_integral_node.x()), 0.79742698535308698, SubrosaDG::kEpsilon);
  ASSERT_NEAR(dbg(triangle_integral_node.y()), 0.101286507323456, SubrosaDG::kEpsilon);

  ASSERT_NEAR(dbg(mesh->triangle_.local_mass_matrix_inverse.inverse()(0, 0)), 0.016666666666666666,
              SubrosaDG::kEpsilon);
  ASSERT_NEAR(dbg(mesh->quadrangle_.local_mass_matrix_inverse(Eigen::last, Eigen::last)), 1.2656250000000002,
              SubrosaDG::kEpsilon);
  ASSERT_EQ(dbg(mesh->nodes_num_), 21);
}

TEST_F(Test2d, MeshAdjacencyElementMeshTest) {
  ASSERT_EQ(dbg(mesh->line_.internal_elements_range_), std::make_pair(35L, 64L));
  Eigen::Vector<SubrosaDG::Real, 2> internal_line_node = mesh->line_.elements_nodes_(Eigen::lastN(2), 0);
  ASSERT_NEAR(dbg(internal_line_node.x()), -0.3562499999998558, SubrosaDG::kEpsilon);
  ASSERT_NEAR(dbg(internal_line_node.y()), 0.1479166666665563, SubrosaDG::kEpsilon);

  ASSERT_EQ(dbg(mesh->line_.boundary_elements_range_), std::make_pair(1L, 12L));
  Eigen::Vector<SubrosaDG::Real, 2> boundary_line_node = mesh->line_.elements_nodes_(Eigen::lastN(2), Eigen::last);
  ASSERT_NEAR(dbg(boundary_line_node.x()), -1.0, SubrosaDG::kEpsilon);
  ASSERT_NEAR(dbg(boundary_line_node.y()), -0.5, SubrosaDG::kEpsilon);
}

TEST_F(Test2d, MeshAdjacencyElementIndexTest) {
  Eigen::Vector<SubrosaDG::Isize, 4> internal_line_index =
      mesh->line_.elements_index_(Eigen::all, mesh->line_.elements_num_.first - 1);
  ASSERT_EQ(dbg(internal_line_index), (Eigen::Vector<SubrosaDG::Isize, 4>() << 2, 20, 31, 32).finished());
  Eigen::Vector<SubrosaDG::Isize, 4> boundary_line_index = mesh->line_.elements_index_(Eigen::all, Eigen::last);
  ASSERT_EQ(dbg(boundary_line_index), (Eigen::Vector<SubrosaDG::Isize, 4>() << 12, 1, 14, -1).finished());
}

TEST_F(Test2d, MeshElementMeshTest) {
  ASSERT_EQ(dbg(mesh->triangle_.elements_range_), std::make_pair(13L, 28L));
  Eigen::Vector<SubrosaDG::Real, 2> triangle_node = mesh->triangle_.elements_nodes_(Eigen::lastN(2), Eigen::last);
  ASSERT_NEAR(dbg(triangle_node.x()), 0.2747662092153528, SubrosaDG::kEpsilon);
  ASSERT_NEAR(dbg(triangle_node.y()), 0.0652513350269377, SubrosaDG::kEpsilon);

  ASSERT_EQ(dbg(mesh->quadrangle_.elements_range_), std::make_pair(29L, 34L));
  Eigen::Vector<SubrosaDG::Real, 2> quadrangle_node = mesh->quadrangle_.elements_nodes_(Eigen::lastN(2), Eigen::last);
  ASSERT_NEAR(dbg(quadrangle_node.x()), 1.0, SubrosaDG::kEpsilon);
  ASSERT_NEAR(dbg(quadrangle_node.y()), -0.5, SubrosaDG::kEpsilon);
}

TEST_F(Test2d, MeshElementTest) {
  ASSERT_EQ(dbg(mesh->elements_range_), std::make_pair(13L, 34L));
  ASSERT_EQ(dbg(mesh->elements_type_(mesh->triangle_.elements_num_ - 1)), SubrosaDG::kTriangle.kElementTag);
  ASSERT_EQ(dbg(mesh->elements_type_(mesh->triangle_.elements_num_)), SubrosaDG::kQuadrangle.kElementTag);
}

TEST_F(Test2d, MeshAdjacencyElementJacobianTest) {
  auto line_length = SubrosaDG::FElementMeasure<2, SubrosaDG::kLine>::calculate(mesh->line_);
  ASSERT_NEAR(dbg(mesh->line_.elements_jacobian_(Eigen::last)), line_length->operator()(Eigen::last) / 2.0,
              SubrosaDG::kEpsilon);
}

TEST_F(Test2d, MeshElementJacobianTest) {
  auto triangle_area = SubrosaDG::FElementMeasure<2, SubrosaDG::kTriangle>::calculate(mesh->triangle_);
  ASSERT_NEAR(dbg(mesh->triangle_.elements_jacobian_(Eigen::last)), triangle_area->operator()(Eigen::last) * 2.0,
              SubrosaDG::kEpsilon);
  auto quadrangle_area = SubrosaDG::FElementMeasure<2, SubrosaDG::kQuadrangle>::calculate(mesh->quadrangle_);
  ASSERT_NEAR(dbg(mesh->quadrangle_.elements_jacobian_(Eigen::last)), quadrangle_area->operator()(Eigen::last) / 4.0,
              SubrosaDG::kEpsilon);
}
