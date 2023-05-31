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

#include <dbg.h>                         // for type_name, DBG_MAP_1, DebugOutput, dbg
#include <gmsh.h>                        // for addCurveLoop, addLine, addPhysicalGroup, addPlaneSurface, add, addPoint
#include <gtest/gtest.h>                 // for Message, TestPartResult, ASSERT_NEAR, TestInfo (ptr only), ASSERT_EQ
#include <Eigen/Core>                    // for Block, DenseBase::operator(), Vector, SymbolExpr, Matrix, CommaIniti...
#include <Eigen/LU>                      // for MatrixBase::inverse
#include <algorithm>                     // for copy
#include <array>                         // for array
#include <filesystem>                    // for path, exists, operator/
#include <map>                           // for map
#include <memory>                        // for allocator, unique_ptr
#include <string_view>                   // for string_view, hash, basic_string_view, operator""sv, operator==, stri...
#include <unordered_map>                 // for unordered_map
#include <utility>                       // for make_pair, pair
#include <vector>                        // for vector

#include "basic/config.hpp"              // for InitVar, FarfieldVar, FlowVar (ptr only), ThermodynamicModel, TimeVar
#include "basic/constant.hpp"            // for kEpsilon
#include "basic/data_type.hpp"           // for Real, Isize
#include "basic/enum.hpp"                // for Boundary, SimulationEquation
#include "cmake.hpp"                     // for kProjectSourceDir
#include "mesh/elem_type.hpp"            // for ElemInfo, kQuad, kTri
#include "mesh/element/cal_measure.hpp"  // for calElemMeasure
#include "mesh/get_mesh.hpp"             // for getMesh
#include "mesh/mesh_structure.hpp"       // for Mesh2d, AdjacencyLineElem, TriElem, QuadElem

// clang-format on

using namespace std::string_view_literals;

inline constexpr SubrosaDG::TimeVar<SubrosaDG::kExplicitEuler> kTimeVar{1000, 0.1, -10};

inline const std::unordered_map<std::string_view, SubrosaDG::Boundary> kBoundaryTMap{
    {"bc-1", SubrosaDG::Boundary::Farfield}};

inline constexpr SubrosaDG::ThermodynamicModel<SubrosaDG::SimulationEquation::Euler> kThermodynamicModel{
    1.4, 1.0, 0.7142857142857143};

inline const std::unordered_map<std::string_view, SubrosaDG::Isize> kRegionIdMap{{"vc-1"sv, 1L}};

inline constexpr std::array<SubrosaDG::FlowVar<2>, 1> kInitVar{SubrosaDG::InitVar<2>{{1.0, 0.5}, 1.4, 1.0, 1.0}};

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
  static SubrosaDG::Mesh2d<2>* mesh;

  static void SetUpTestCase() {
    std::filesystem::path mesh_file = SubrosaDG::kProjectSourceDir / "build/out/test/mesh/test.msh";
    if (!std::filesystem::exists(mesh_file)) {
      generateMesh(mesh_file);
    }
    Test2d::mesh = new SubrosaDG::Mesh2d<2>{mesh_file};
    SubrosaDG::getMesh(kBoundaryTMap, *Test2d::mesh);
  }

  static void TearDownTestCase() {
    delete Test2d::mesh;
    Test2d::mesh = nullptr;
  }
};

SubrosaDG::Mesh2d<2>* Test2d::mesh;

TEST_F(Test2d, ElemIntegral) {
  Eigen::Vector<SubrosaDG::Real, 2> tri_basis_funs = mesh->tri_.basis_funs(Eigen::lastN(2), Eigen::last);
  ASSERT_NEAR(dbg(tri_basis_funs.x()), 0.32307437676754752, SubrosaDG::kEpsilon);
  ASSERT_NEAR(dbg(tri_basis_funs.y()), 0.041035826263138453, SubrosaDG::kEpsilon);
  ASSERT_NEAR(dbg(mesh->tri_.local_mass_mat_inv.inverse()(0, 0)), 0.016666666666666666, SubrosaDG::kEpsilon);

  Eigen::Vector<SubrosaDG::Real, 2> quad_grad_basis_funs = mesh->quad_.grad_basis_funs(Eigen::lastN(2), Eigen::last);
  ASSERT_NEAR(dbg(quad_grad_basis_funs.x()), 0.13524199845510998, SubrosaDG::kEpsilon);
  ASSERT_NEAR(dbg(quad_grad_basis_funs.y()), -0.61967733539318659, SubrosaDG::kEpsilon);
}

TEST_F(Test2d, AdjacencyElemIntegral) {
  ASSERT_EQ(dbg(mesh->line_.integral_num), 3);
  Eigen::Vector<SubrosaDG::Real, 2> line_tri_basis_funs = mesh->line_.tri_basis_funs(Eigen::lastN(2), Eigen::last);
  ASSERT_NEAR(dbg(line_tri_basis_funs.x()), 0.0, SubrosaDG::kEpsilon);
  ASSERT_NEAR(dbg(line_tri_basis_funs.y()), 0.39999999999999997, SubrosaDG::kEpsilon);
  Eigen::Vector<SubrosaDG::Real, 2> line_quad_basis_funs = mesh->line_.quad_basis_funs(Eigen::lastN(2), Eigen::last);
  ASSERT_NEAR(dbg(line_quad_basis_funs.x()), 0.39999999999999991, SubrosaDG::kEpsilon);
  ASSERT_NEAR(dbg(line_quad_basis_funs.y()), 0.0, SubrosaDG::kEpsilon);
}

TEST_F(Test2d, AdjacencyElemMesh) {
  ASSERT_EQ(dbg(mesh->line_.internal_range_), std::make_pair(35L, 64L));
  Eigen::Vector<SubrosaDG::Real, 2> internal_line_node = mesh->line_.node_(Eigen::lastN(2), 0);
  ASSERT_NEAR(dbg(internal_line_node.x()), -0.3562499999998558, SubrosaDG::kEpsilon);
  ASSERT_NEAR(dbg(internal_line_node.y()), 0.1479166666665563, SubrosaDG::kEpsilon);

  ASSERT_EQ(dbg(mesh->line_.boundary_range_), std::make_pair(1L, 12L));
  Eigen::Vector<SubrosaDG::Real, 2> boundary_line_node = mesh->line_.node_(Eigen::lastN(2), Eigen::last);
  ASSERT_NEAR(dbg(boundary_line_node.x()), -1.0, SubrosaDG::kEpsilon);
  ASSERT_NEAR(dbg(boundary_line_node.y()), -0.5, SubrosaDG::kEpsilon);
}

TEST_F(Test2d, AdjacencyElemIndex) {
  Eigen::Vector<SubrosaDG::Isize, 6> internal_line_index =
      mesh->line_.index_(Eigen::all, mesh->line_.num_tag_.first - 1);
  ASSERT_EQ(dbg(internal_line_index), (Eigen::Vector<SubrosaDG::Isize, 6>() << 2, 20, 31, 3, 32, 2).finished());
  Eigen::Vector<SubrosaDG::Isize, 6> boundary_line_index = mesh->line_.index_(Eigen::all, Eigen::last);
  ASSERT_EQ(dbg(boundary_line_index), (Eigen::Vector<SubrosaDG::Isize, 6>() << 12, 1, 14, 0, -1, 0).finished());
}

TEST_F(Test2d, ElemMesh) {
  ASSERT_EQ(dbg(mesh->tri_.range_), std::make_pair(13L, 28L));
  Eigen::Vector<SubrosaDG::Real, 2> tri_node = mesh->tri_.node_(Eigen::lastN(2), Eigen::last);
  ASSERT_NEAR(dbg(tri_node.x()), 0.2747662092153528, SubrosaDG::kEpsilon);
  ASSERT_NEAR(dbg(tri_node.y()), 0.0652513350269377, SubrosaDG::kEpsilon);

  ASSERT_EQ(dbg(mesh->quad_.range_), std::make_pair(29L, 34L));
  Eigen::Vector<SubrosaDG::Real, 2> quad_node = mesh->quad_.node_(Eigen::lastN(2), Eigen::last);
  ASSERT_NEAR(dbg(quad_node.x()), 1.0, SubrosaDG::kEpsilon);
  ASSERT_NEAR(dbg(quad_node.y()), -0.5, SubrosaDG::kEpsilon);
}

TEST_F(Test2d, MeshElem) {
  ASSERT_EQ(dbg(mesh->node_num_), 21);
  ASSERT_EQ(dbg(mesh->elem_range_), std::make_pair(13L, 34L));
  ASSERT_EQ(dbg(mesh->elem_type_(mesh->tri_.num_ - 1)), SubrosaDG::kTri.kTag);
  ASSERT_EQ(dbg(mesh->elem_type_(mesh->tri_.num_)), SubrosaDG::kQuad.kTag);
}

TEST_F(Test2d, AdjacencyElemNormVec) {
  Eigen::Vector<SubrosaDG::Real, 2> line_norm_vec = mesh->line_.norm_vec_(Eigen::all, 0);
  ASSERT_NEAR(dbg(line_norm_vec.x()), -0.92580852301396133, SubrosaDG::kEpsilon);
  ASSERT_NEAR(dbg(line_norm_vec.y()), -0.37799282891968655, SubrosaDG::kEpsilon);
}

TEST_F(Test2d, AdjacencyElemJacobian) {
  auto line_length = SubrosaDG::calElemMeasure(mesh->line_);
  ASSERT_NEAR(dbg(mesh->line_.jacobian_(Eigen::last)), line_length->operator()(Eigen::last) / 2.0, SubrosaDG::kEpsilon);
}

TEST_F(Test2d, ElemJacobian) {
  auto tri_area = SubrosaDG::calElemMeasure(mesh->tri_);
  ASSERT_NEAR(dbg(mesh->tri_.jacobian_(Eigen::last)), tri_area->operator()(Eigen::last) * 2.0, SubrosaDG::kEpsilon);
  auto quad_area = SubrosaDG::calElemMeasure(mesh->quad_);
  ASSERT_NEAR(dbg(mesh->quad_.jacobian_(Eigen::last)), quad_area->operator()(Eigen::last) / 4.0, SubrosaDG::kEpsilon);
}
