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

#include <dbg.h>                            // for type_name, DBG_MAP_1, DebugOutput, dbg
#include <gmsh.h>                           // for addCurveLoop, addLine, addPhysicalGroup, addPlaneSurface, add
#include <gtest/gtest.h>                    // for Message, TestPartResult, ASSERT_NEAR, TestInfo (ptr only), TEST_F
#include <Eigen/Core>                       // for Block, Vector, DenseBase<>::ColXpr, Matrix, SymbolExpr, CommaInit...
#include <Eigen/LU>                         // for MatrixBase::inverse
#include <algorithm>                        // for copy
#include <array>                            // for array
#include <filesystem>                       // for path, exists, operator/
#include <memory>                           // for allocator, unique_ptr
#include <string_view>                      // for string_view, hash, basic_string_view, operator""sv, operator==
#include <unordered_map>                    // for unordered_map
#include <utility>                          // for make_pair

#include "basic/config.hpp"                 // for InitVar, FarfieldVar, ThermoModel, TimeVar, kExplicitEuler
#include "basic/constant.hpp"               // for kEpsilon
#include "basic/data_type.hpp"              // for Real, Isize
#include "basic/enum.hpp"                   // for MeshType, Boundary, EquModel
#include "cmake.hpp"                        // for kProjectSourceDir
#include "mesh/element/cal_measure.hpp"     // for calElemMeasure
#include "mesh/get_mesh.hpp"                // for getMesh
#include "mesh/mesh_structure.hpp"          // for Mesh, AdjacencyElemTypeMesh, AdjacencyLineElemMesh, PerAdjacencyE...
#include "integral/integral_structure.hpp"  // for Integral, TriElemIntegral, AdjacencyLineElemIntegral, QuadElemInt...
#include "integral/get_integral.hpp"        // for getIntegral

// clang-format on

using namespace std::string_view_literals;

inline constexpr SubrosaDG::TimeVar<SubrosaDG::kExplicitEuler> kTimeVar{1000, 0.1, -10};

inline const std::unordered_map<std::string_view, SubrosaDG::Boundary> kBoundaryTMap{
    {"bc-1", SubrosaDG::Boundary::Farfield}};

inline constexpr SubrosaDG::ThermoModel<SubrosaDG::EquModel::Euler> kThermoModel{1.4, 1.0, 0.7142857142857143};

inline const std::unordered_map<std::string_view, int> kRegionIdMap{{"vc-1"sv, 1}};

inline constexpr std::array<SubrosaDG::InitVar<2>, 1> kInitVarArray{SubrosaDG::InitVar<2>{{1.0, 0.5}, 1.4, 1.0, 1.0}};

inline constexpr SubrosaDG::FarfieldVar<2> kFarfieldVar{{1.0, 0.5}, 1.4, 1.0, 1.0};

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
  static SubrosaDG::Mesh<2, SubrosaDG::MeshType::TriQuad>* mesh;
  static SubrosaDG::Integral<2, 2, SubrosaDG::MeshType::TriQuad>* integral;

  static void SetUpTestCase() {
    std::filesystem::path mesh_file = SubrosaDG::kProjectSourceDir / "build/out/test/mesh/test.msh";
    if (!std::filesystem::exists(mesh_file)) {
      generateMesh(mesh_file);
    }
    Test2d::mesh = new SubrosaDG::Mesh<2, SubrosaDG::MeshType::TriQuad>{mesh_file};
    SubrosaDG::getMesh(kBoundaryTMap, *Test2d::mesh);
    Test2d::integral = new SubrosaDG::Integral<2, 2, SubrosaDG::MeshType::TriQuad>;
    SubrosaDG::getIntegral(*Test2d::integral);
  }

  static void TearDownTestCase() {
    delete Test2d::mesh;
    Test2d::mesh = nullptr;
    delete Test2d::integral;
    Test2d::integral = nullptr;
  }
};

SubrosaDG::Mesh<2, SubrosaDG::MeshType::TriQuad>* Test2d::mesh;
SubrosaDG::Integral<2, 2, SubrosaDG::MeshType::TriQuad>* Test2d::integral;

TEST_F(Test2d, AdjacencyElemMesh) {
  ASSERT_EQ(dbg(mesh->line_.internal_.range_), std::make_pair(35L, 64L));
  Eigen::Vector<SubrosaDG::Real, 2> internal_line_node = mesh->line_.internal_.elem_(0).node_.col(1);
  ASSERT_NEAR(dbg(internal_line_node.x()), -0.3562499999998558, SubrosaDG::kEpsilon);
  ASSERT_NEAR(dbg(internal_line_node.y()), 0.1479166666665563, SubrosaDG::kEpsilon);

  ASSERT_EQ(dbg(mesh->line_.boundary_.range_), std::make_pair(1L, 12L));
  Eigen::Vector<SubrosaDG::Real, 2> boundary_line_node =
      mesh->line_.boundary_.elem_(mesh->line_.boundary_.num_ - 1).node_.col(1);
  ASSERT_NEAR(dbg(boundary_line_node.x()), -1.0, SubrosaDG::kEpsilon);
  ASSERT_NEAR(dbg(boundary_line_node.y()), -0.5, SubrosaDG::kEpsilon);
}

TEST_F(Test2d, AdjacencyElemIndex) {
  Eigen::Vector<SubrosaDG::Isize, 8> internal_line_index =
      mesh->line_.internal_.elem_(mesh->line_.internal_.num_ - 1).index_;
  ASSERT_EQ(dbg(internal_line_index), (Eigen::Vector<SubrosaDG::Isize, 8>() << 2, 20, 2, 3, 3, 3, 3, 2).finished());
  Eigen::Vector<SubrosaDG::Isize, 6> boundary_line_index =
      mesh->line_.boundary_.elem_(mesh->line_.boundary_.num_ - 1).index_;
  ASSERT_EQ(dbg(boundary_line_index), (Eigen::Vector<SubrosaDG::Isize, 6>() << 12, 1, 1, 2, 0, 1).finished());
}

TEST_F(Test2d, ElemMesh) {
  ASSERT_EQ(dbg(mesh->tri_.range_), std::make_pair(13L, 28L));
  Eigen::Vector<SubrosaDG::Real, 2> tri_node = mesh->tri_.elem_(mesh->tri_.num_ - 1).node_.col(2);
  ASSERT_NEAR(dbg(tri_node.x()), 0.2747662092153528, SubrosaDG::kEpsilon);
  ASSERT_NEAR(dbg(tri_node.y()), 0.0652513350269377, SubrosaDG::kEpsilon);

  ASSERT_EQ(dbg(mesh->quad_.range_), std::make_pair(29L, 34L));
  Eigen::Vector<SubrosaDG::Real, 2> quad_node = mesh->quad_.elem_(mesh->quad_.num_ - 1).node_.col(3);
  ASSERT_NEAR(dbg(quad_node.x()), 1.0, SubrosaDG::kEpsilon);
  ASSERT_NEAR(dbg(quad_node.y()), -0.5, SubrosaDG::kEpsilon);
}

TEST_F(Test2d, AdjacencyElemNormVec) {
  Eigen::Vector<SubrosaDG::Real, 2> line_internal_norm_vec = mesh->line_.internal_.elem_(0).norm_vec_;
  ASSERT_NEAR(dbg(line_internal_norm_vec.x()), -0.92580852301396133, SubrosaDG::kEpsilon);
  ASSERT_NEAR(dbg(line_internal_norm_vec.y()), -0.37799282891968655, SubrosaDG::kEpsilon);
  Eigen::Vector<SubrosaDG::Real, 2> line_boundary_norm_vec =
      mesh->line_.boundary_.elem_(mesh->line_.boundary_.num_ - 1).norm_vec_;
  ASSERT_NEAR(dbg(line_boundary_norm_vec.x()), -1.0, SubrosaDG::kEpsilon);
  ASSERT_NEAR(dbg(line_boundary_norm_vec.y()), 0.0, SubrosaDG::kEpsilon);
}

TEST_F(Test2d, AdjacencyElemJacobian) {
  auto line_length = SubrosaDG::calElemMeasure(mesh->line_);
  SubrosaDG::Real line_internal_jacobian = mesh->line_.internal_.elem_(0).jacobian_;
  ASSERT_NEAR(dbg(line_internal_jacobian), line_length->operator()(0) / 2.0, SubrosaDG::kEpsilon);
  SubrosaDG::Real line_boundary_jacobian = mesh->line_.boundary_.elem_(mesh->line_.boundary_.num_ - 1).jacobian_;
  ASSERT_NEAR(dbg(line_boundary_jacobian), line_length->operator()(Eigen::last) / 2.0, SubrosaDG::kEpsilon);
}

TEST_F(Test2d, ElemJacobian) {
  auto tri_area = SubrosaDG::calElemMeasure(mesh->tri_);
  SubrosaDG::Real tri_jacobian = mesh->tri_.elem_(mesh->tri_.num_ - 1).jacobian_;
  ASSERT_NEAR(dbg(tri_jacobian), tri_area->operator()(Eigen::last) * 2.0, SubrosaDG::kEpsilon);
  auto quad_area = SubrosaDG::calElemMeasure(mesh->quad_);
  SubrosaDG::Real quad_jacobian = mesh->quad_.elem_(mesh->quad_.num_ - 1).jacobian_;
  ASSERT_NEAR(dbg(quad_jacobian), quad_area->operator()(Eigen::last) / 4.0, SubrosaDG::kEpsilon);
}

TEST_F(Test2d, ElemIntegral) {
  Eigen::Vector<SubrosaDG::Real, 2> tri_basis_fun = integral->tri_.basis_fun_(Eigen::lastN(2), Eigen::last);
  ASSERT_NEAR(dbg(tri_basis_fun.x()), 0.32307437676754752, SubrosaDG::kEpsilon);
  ASSERT_NEAR(dbg(tri_basis_fun.y()), 0.041035826263138453, SubrosaDG::kEpsilon);
  ASSERT_NEAR(dbg(integral->tri_.local_mass_mat_inv_.inverse()(0, 0)), 0.016666666666666666, SubrosaDG::kEpsilon);

  Eigen::Vector<SubrosaDG::Real, 2> quad_grad_basis_fun = integral->quad_.grad_basis_fun_(Eigen::last, Eigen::lastN(2));
  ASSERT_NEAR(dbg(quad_grad_basis_fun.x()), 0.13524199845510998, SubrosaDG::kEpsilon);
  ASSERT_NEAR(dbg(quad_grad_basis_fun.y()), -0.61967733539318659, SubrosaDG::kEpsilon);
}

TEST_F(Test2d, AdjacencyElemIntegral) {
  Eigen::Vector<SubrosaDG::Real, 2> line_tri_basis_fun = integral->line_.tri_basis_fun_(Eigen::last, Eigen::lastN(2));
  ASSERT_NEAR(dbg(line_tri_basis_fun.x()), 0.0, SubrosaDG::kEpsilon);
  ASSERT_NEAR(dbg(line_tri_basis_fun.y()), 0.39999999999999997, SubrosaDG::kEpsilon);
  Eigen::Vector<SubrosaDG::Real, 2> line_quad_basis_fun = integral->line_.quad_basis_fun_(Eigen::last, Eigen::lastN(2));
  ASSERT_NEAR(dbg(line_quad_basis_fun.x()), 0.39999999999999991, SubrosaDG::kEpsilon);
  ASSERT_NEAR(dbg(line_quad_basis_fun.y()), 0.0, SubrosaDG::kEpsilon);
}
