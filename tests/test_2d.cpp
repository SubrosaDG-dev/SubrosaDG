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

#include <dbg.h>  // IWYU pragma: keep
#include <gmsh.h>
#include <gtest/gtest.h>

#include <Eigen/Core>
#include <Eigen/LU>
#include <algorithm>
#include <filesystem>
#include <map>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "SubrosaDG"

using namespace std::string_view_literals;

inline constexpr SubrosaDG::TimeVar kTimeVar{1000, 0.1, 1e-10};

inline constexpr SubrosaDG::SpatialDiscreteEuler<SubrosaDG::ConvectiveFlux::Roe> kSpatialDiscrete;

inline constexpr SubrosaDG::ThermoModel<SubrosaDG::EquModel::Euler> kThermoModel{1.4, 1.0, 0.7142857142857143};

inline const std::unordered_map<std::string_view, int> kRegionIdMap{{"vc-1"sv, 0}};

inline const std::vector<SubrosaDG::FlowVar<2>> kFlowVar{SubrosaDG::FlowVar<2>{{1.0, 0.5}, 1.4, 1.0, 1.0}};

inline const SubrosaDG::InitVar<2> kInitVar{kRegionIdMap, kFlowVar};

inline const std::unordered_map<std::string_view, SubrosaDG::Boundary> kBoundaryTMap{
    {"bc-1", SubrosaDG::Boundary::Farfield}};

inline constexpr SubrosaDG::FarfieldVar<2> kFarfieldVar{{1.0, 0.5}, 1.4, 1.0, 1.0};

void generateMesh(const std::filesystem::path& mesh_file) {
  // NOTE: if your gmsh doesn't compile with Blossom(such as fedora build file saying: blossoms is nonfree, see
  // contrib/blossoms/README.txt.) This mesh could be different from the version of gmsh which not uses Blossom.
  // gmsh::option::setNumber("Mesh.RecombinationAlgorithm", 1);
  Eigen::Matrix<double, 6, 3, Eigen::RowMajor> points;
  points << -1.0, -0.5, 0.0, 0.0, -0.5, 0.0, 1.0, -0.5, 0.0, 1.0, 0.5, 0.0, 0.0, 0.5, 0.0, -1.0, 0.5, 0.0;
  gmsh::model::add("test2d");
  for (const auto& row : points.rowwise()) {
    gmsh::model::geo::addPoint(row.x(), row.y(), row.z(), 0.5);
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
  static SubrosaDG::Integral<2, SubrosaDG::PolyOrder::P2, SubrosaDG::MeshType::TriQuad>* integral;
  static SubrosaDG::Solver<2, SubrosaDG::PolyOrder::P2, SubrosaDG::EquModel::Euler, SubrosaDG::MeshType::TriQuad>*
      solver;
  static SubrosaDG::SolverSupplemental<2, SubrosaDG::EquModel::Euler, SubrosaDG::TimeDiscrete::ExplicitEuler>*
      solver_supplemental;

  static void SetUpTestCase() {
    std::filesystem::path mesh_file = SubrosaDG::kProjectSourceDir / "build/out/test/mesh/test2d.msh";
    if (!std::filesystem::exists(mesh_file)) {
      generateMesh(mesh_file);
    }
    Test2d::mesh = new SubrosaDG::Mesh<2, SubrosaDG::MeshType::TriQuad>{mesh_file};
    SubrosaDG::getMesh(kBoundaryTMap, *Test2d::mesh);
    Test2d::integral = new SubrosaDG::Integral<2, SubrosaDG::PolyOrder::P2, SubrosaDG::MeshType::TriQuad>;
    SubrosaDG::getIntegral(*Test2d::integral);
    Test2d::solver =
        new SubrosaDG::Solver<2, SubrosaDG::PolyOrder::P2, SubrosaDG::EquModel::Euler, SubrosaDG::MeshType::TriQuad>;
    Test2d::solver_supplemental =
        new SubrosaDG::SolverSupplemental<2, SubrosaDG::EquModel::Euler, SubrosaDG::TimeDiscrete::ExplicitEuler>{
            kThermoModel, kTimeVar};
  }

  static void TearDownTestCase() {
    delete Test2d::mesh;
    Test2d::mesh = nullptr;
    delete Test2d::integral;
    Test2d::integral = nullptr;
    delete Test2d::solver;
    Test2d::solver = nullptr;
    delete Test2d::solver_supplemental;
    Test2d::solver_supplemental = nullptr;
  }
};

SubrosaDG::Mesh<2, SubrosaDG::MeshType::TriQuad>* Test2d::mesh;
SubrosaDG::Integral<2, SubrosaDG::PolyOrder::P2, SubrosaDG::MeshType::TriQuad>* Test2d::integral;
SubrosaDG::Solver<2, SubrosaDG::PolyOrder::P2, SubrosaDG::EquModel::Euler, SubrosaDG::MeshType::TriQuad>*
    Test2d::solver;
SubrosaDG::SolverSupplemental<2, SubrosaDG::EquModel::Euler, SubrosaDG::TimeDiscrete::ExplicitEuler>*
    Test2d::solver_supplemental;

TEST_F(Test2d, ElemMesh) {
  ASSERT_EQ(mesh->tri_.range_, std::make_pair(13L, 26L));
  Eigen::Vector<SubrosaDG::Real, 2> tri_node = mesh->tri_.elem_(mesh->tri_.num_ - 1).node_.col(2);
  ASSERT_NEAR(tri_node.x(), -0.3562499999998558, SubrosaDG::kEpsilon);
  ASSERT_NEAR(tri_node.y(), 0.1479166666665563, SubrosaDG::kEpsilon);

  ASSERT_EQ(mesh->quad_.range_, std::make_pair(27L, 32L));
  Eigen::Vector<SubrosaDG::Real, 2> quad_node = mesh->quad_.elem_(mesh->quad_.num_ - 1).node_.col(3);
  ASSERT_NEAR(quad_node.x(), 0.4999592608267473, SubrosaDG::kEpsilon);
  ASSERT_NEAR(quad_node.y(), -0.0008681601799060465, SubrosaDG::kEpsilon);
}

TEST_F(Test2d, ElemProjectionMeasure) {
  Eigen::Vector<SubrosaDG::Real, 2> tri_projection_measure = mesh->tri_.elem_(mesh->tri_.num_ - 1).projection_measure_;
  ASSERT_NEAR(tri_projection_measure.x(), 0.35624999999985579, SubrosaDG::kEpsilon);
  ASSERT_NEAR(tri_projection_measure.y(), 0.35416666666679764, SubrosaDG::kEpsilon);

  Eigen::Vector<SubrosaDG::Real, 2> quad_projection_measure =
      mesh->quad_.elem_(mesh->quad_.num_ - 1).projection_measure_;
  ASSERT_NEAR(quad_projection_measure.x(), 0.4999592608267473, SubrosaDG::kEpsilon);
  ASSERT_NEAR(quad_projection_measure.y(), 0.6494286120982754, SubrosaDG::kEpsilon);
}

TEST_F(Test2d, ElemJacobian) {
  auto tri_area = SubrosaDG::calElemMeasure(mesh->tri_);
  SubrosaDG::Real tri_jacobian = mesh->tri_.elem_(mesh->tri_.num_ - 1).jacobian_;
  ASSERT_NEAR(tri_jacobian, tri_area->operator()(Eigen::last) / integral->tri_.measure, SubrosaDG::kEpsilon);

  auto quad_area = SubrosaDG::calElemMeasure(mesh->quad_);
  SubrosaDG::Real quad_jacobian = mesh->quad_.elem_(mesh->quad_.num_ - 1).jacobian_;
  ASSERT_NEAR(quad_jacobian, quad_area->operator()(Eigen::last) / integral->quad_.measure, SubrosaDG::kEpsilon);
}

TEST_F(Test2d, AdjacencyElemMesh) {
  ASSERT_EQ(mesh->line_.internal_.range_, std::make_pair(33L, 59L));
  Eigen::Vector<SubrosaDG::Real, 2> internal_line_node = mesh->line_.internal_.elem_(0).node_.col(1);
  ASSERT_NEAR(internal_line_node.x(), -0.3562499999998558, SubrosaDG::kEpsilon);
  ASSERT_NEAR(internal_line_node.y(), 0.1479166666665563, SubrosaDG::kEpsilon);

  ASSERT_EQ(mesh->line_.boundary_.range_, std::make_pair(1L, 12L));
  Eigen::Vector<SubrosaDG::Real, 2> boundary_line_node =
      mesh->line_.boundary_.elem_(mesh->line_.boundary_.num_ - 1).node_.col(1);
  ASSERT_NEAR(boundary_line_node.x(), -1.0, SubrosaDG::kEpsilon);
  ASSERT_NEAR(boundary_line_node.y(), -0.5, SubrosaDG::kEpsilon);
}

TEST_F(Test2d, AdjacencyInternalElemIndex) {
  Eigen::Vector<SubrosaDG::Isize, 2> internal_line_index =
      mesh->line_.internal_.elem_(mesh->line_.internal_.num_ - 1).index_;
  ASSERT_EQ(internal_line_index, (Eigen::Vector<SubrosaDG::Isize, 2>() << 18, 19).finished());
  Eigen::Vector<SubrosaDG::Isize, 2> internal_line_parent_index =
      mesh->line_.internal_.elem_(mesh->line_.internal_.num_ - 1).parent_index_;
  ASSERT_EQ(internal_line_parent_index, (Eigen::Vector<SubrosaDG::Isize, 2>() << 3, 4).finished());
  Eigen::Vector<SubrosaDG::Isize, 2> internal_line_adjacency_index =
      mesh->line_.internal_.elem_(mesh->line_.internal_.num_ - 1).adjacency_index_;
  ASSERT_EQ(internal_line_adjacency_index, (Eigen::Vector<SubrosaDG::Isize, 2>() << 1, 2).finished());
  Eigen::Vector<int, 2> internal_line_typology_index =
      mesh->line_.internal_.elem_(mesh->line_.internal_.num_ - 1).typology_index_;
  ASSERT_EQ(internal_line_typology_index, (Eigen::Vector<int, 2>() << 3, 3).finished());
}

TEST_F(Test2d, AdjacencyBoundaryElemIndex) {
  Eigen::Vector<SubrosaDG::Isize, 2> boundary_line_index =
      mesh->line_.boundary_.elem_(mesh->line_.boundary_.num_ - 1).index_;
  ASSERT_EQ(boundary_line_index, (Eigen::Vector<SubrosaDG::Isize, 2>() << 12, 1).finished());
  Eigen::Vector<SubrosaDG::Isize, 2> boundary_parent_index =
      mesh->line_.boundary_.elem_(mesh->line_.boundary_.num_ - 1).parent_index_;
  ASSERT_EQ(boundary_parent_index, (Eigen::Vector<SubrosaDG::Isize, 2>() << 1, 1).finished());
  Eigen::Vector<SubrosaDG::Isize, 1> boundary_adjacency_index =
      mesh->line_.boundary_.elem_(mesh->line_.boundary_.num_ - 1).adjacency_index_;
  ASSERT_EQ(boundary_adjacency_index, (Eigen::Vector<SubrosaDG::Isize, 1>() << 0).finished());
  Eigen::Vector<int, 1> boundary_typology_index =
      mesh->line_.boundary_.elem_(mesh->line_.boundary_.num_ - 1).typology_index_;
  ASSERT_EQ(boundary_typology_index, (Eigen::Vector<int, 1>() << 2).finished());
}

TEST_F(Test2d, AdjacencyElemNormVec) {
  Eigen::Vector<SubrosaDG::Real, 2> line_internal_norm_vec = mesh->line_.internal_.elem_(0).norm_vec_;
  ASSERT_NEAR(line_internal_norm_vec.x(), -0.92580852301396133, SubrosaDG::kEpsilon);
  ASSERT_NEAR(line_internal_norm_vec.y(), -0.37799282891968655, SubrosaDG::kEpsilon);
  Eigen::Vector<SubrosaDG::Real, 2> line_boundary_norm_vec =
      mesh->line_.boundary_.elem_(mesh->line_.boundary_.num_ - 1).norm_vec_;
  ASSERT_NEAR(line_boundary_norm_vec.x(), -1.0, SubrosaDG::kEpsilon);
  ASSERT_NEAR(line_boundary_norm_vec.y(), 0.0, SubrosaDG::kEpsilon);
}

TEST_F(Test2d, AdjacencyElemJacobian) {
  auto line_length = SubrosaDG::calElemMeasure(mesh->line_);
  SubrosaDG::Real line_internal_jacobian = mesh->line_.internal_.elem_(0).jacobian_;
  ASSERT_NEAR(line_internal_jacobian, line_length->operator()(0) / 2.0, SubrosaDG::kEpsilon);
  SubrosaDG::Real line_boundary_jacobian = mesh->line_.boundary_.elem_(mesh->line_.boundary_.num_ - 1).jacobian_;
  ASSERT_NEAR(line_boundary_jacobian, line_length->operator()(Eigen::last) / 2.0, SubrosaDG::kEpsilon);
}

TEST_F(Test2d, ElemIntegral) {
  Eigen::Vector<SubrosaDG::Real, 2> tri_basis_fun = integral->tri_.basis_fun_(Eigen::last, Eigen::lastN(2));
  ASSERT_NEAR(tri_basis_fun.x(), 0.29921523099278707, SubrosaDG::kEpsilon);
  ASSERT_NEAR(tri_basis_fun.y(), 0.03354481152314831, SubrosaDG::kEpsilon);
  ASSERT_NEAR(integral->tri_.local_mass_mat_inv_.inverse()(0, 0), 0.016666666666666666, SubrosaDG::kEpsilon);

  Eigen::Vector<SubrosaDG::Real, 2> quad_grad_basis_fun = integral->quad_.grad_basis_fun_(Eigen::last, Eigen::lastN(2));
  ASSERT_NEAR(quad_grad_basis_fun.x(), 0.13524199845510998, SubrosaDG::kEpsilon);
  ASSERT_NEAR(quad_grad_basis_fun.y(), -0.61967733539318659, SubrosaDG::kEpsilon);
}

TEST_F(Test2d, AdjacencyElemIntegral) {
  Eigen::Vector<SubrosaDG::Real, 2> line_tri_basis_fun =
      integral->line_.tri_.adjacency_basis_fun_(Eigen::last, Eigen::lastN(2));
  ASSERT_NEAR(line_tri_basis_fun.x(), 0.0, SubrosaDG::kEpsilon);
  ASSERT_NEAR(line_tri_basis_fun.y(), 0.39999999999999997, SubrosaDG::kEpsilon);

  Eigen::Vector<SubrosaDG::Real, 2> line_quad_basis_fun =
      integral->line_.quad_.adjacency_basis_fun_(Eigen::last, Eigen::lastN(2));
  ASSERT_NEAR(line_quad_basis_fun.x(), 0.39999999999999991, SubrosaDG::kEpsilon);
  ASSERT_NEAR(line_quad_basis_fun.y(), 0.0, SubrosaDG::kEpsilon);
}

TEST_F(Test2d, Develop) {
  SubrosaDG::initSolver(*mesh, kInitVar, kFarfieldVar, *solver_supplemental, *solver);
  SubrosaDG::stepTime<decltype(kSpatialDiscrete)>(*mesh, *integral, *solver_supplemental, *solver);
}
