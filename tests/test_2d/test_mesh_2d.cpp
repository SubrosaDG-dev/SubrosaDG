/**
 * @file test_mesh_2d.cpp
 * @brief The source file of test mesh 2d.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-07-03
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#include <gtest/gtest.h>

#include <Eigen/Core>
#include <Eigen/LU>
#include <memory>
#include <utility>

#include "SubrosaDG"
#include "test_structure_2d.h"

TEST_F(Test2d, GetIntegral) { SubrosaDG::getIntegral(*integral); }

TEST_F(Test2d, GetMesh) { SubrosaDG::getMesh(kBoundaryTMap, *integral, *mesh); }

TEST_F(Test2d, ElemIntegral) {
  Eigen::Vector<SubrosaDG::Real, 2> tri_basis_fun = integral->tri_.basis_fun_(Eigen::last, Eigen::lastN(2));
  ASSERT_NEAR(tri_basis_fun.x(), 0.29921523099278707, SubrosaDG::kEpsilon);
  ASSERT_NEAR(tri_basis_fun.y(), 0.03354481152314831, SubrosaDG::kEpsilon);

  Eigen::Vector<SubrosaDG::Real, 2> quad_grad_basis_fun = integral->quad_.grad_basis_fun_(Eigen::last, Eigen::lastN(2));
  ASSERT_NEAR(quad_grad_basis_fun.x(), 0.13524199845510998, SubrosaDG::kEpsilon);
  ASSERT_NEAR(quad_grad_basis_fun.y(), -0.61967733539318659, SubrosaDG::kEpsilon);
}

TEST_F(Test2d, AdjacencyElemIntegral) {
  Eigen::Vector<SubrosaDG::Real, 2> line_tri_basis_fun = integral->line_.tri_.basis_fun_(Eigen::last, Eigen::lastN(2));
  ASSERT_NEAR(line_tri_basis_fun.x(), 0.0, SubrosaDG::kEpsilon);
  ASSERT_NEAR(line_tri_basis_fun.y(), 0.39999999999999997, SubrosaDG::kEpsilon);

  Eigen::Vector<SubrosaDG::Real, 2> line_quad_basis_fun =
      integral->line_.quad_.basis_fun_(Eigen::last, Eigen::lastN(2));
  ASSERT_NEAR(line_quad_basis_fun.x(), 0.39999999999999991, SubrosaDG::kEpsilon);
  ASSERT_NEAR(line_quad_basis_fun.y(), 0.0, SubrosaDG::kEpsilon);
}

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
  SubrosaDG::Real tri_jacobian = mesh->tri_.elem_(mesh->tri_.num_ - 1).jacobian_det_(Eigen::last);
  ASSERT_NEAR(tri_jacobian, tri_area->operator()(Eigen::last) / integral->tri_.measure, SubrosaDG::kEpsilon);

  auto quad_area = SubrosaDG::calElemMeasure(mesh->quad_);
  SubrosaDG::Real quad_jacobian = mesh->quad_.elem_(mesh->quad_.num_ - 1).jacobian_det_(Eigen::last);
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
  SubrosaDG::Real line_internal_jacobian = mesh->line_.internal_.elem_(0).jacobian_det_(Eigen::last);
  ASSERT_NEAR(line_internal_jacobian, line_length->operator()(0) / 2.0, SubrosaDG::kEpsilon);
  SubrosaDG::Real line_boundary_jacobian =
      mesh->line_.boundary_.elem_(mesh->line_.boundary_.num_ - 1).jacobian_det_(Eigen::last);
  ASSERT_NEAR(line_boundary_jacobian, line_length->operator()(Eigen::last) / 2.0, SubrosaDG::kEpsilon);
}
