/**
 * @file test_structure_2d.h
 * @brief The head file of test structure 2d.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-07-03
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_TEST_STRUCTURE_2D_H_
#define SUBROSA_DG_TEST_STRUCTURE_2D_H_

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

inline constexpr SubrosaDG::TimeVar<SubrosaDG::TimeDiscrete::ForwardEuler> kTimeVar{10, 0.1, 1e-10};

inline constexpr SubrosaDG::SpatialDiscreteEuler<SubrosaDG::ConvectiveFlux::Roe> kSpatialDiscrete;

inline constexpr SubrosaDG::ThermoModel<SubrosaDG::EquModel::Euler> kThermoModel{1.4, 1.0, 0.7142857142857143};

inline const std::unordered_map<std::string_view, int> kRegionIdMap{{"vc-1"sv, 0}};

inline const std::vector<SubrosaDG::FlowVar<2>> kFlowVar{SubrosaDG::FlowVar<2>{{1.0, 0.0}, 1.4, 1.0, 1.0}};

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
  gmsh::model::geo::synchronize();
  gmsh::model::addPhysicalGroup(1, {1, 2, 3, 4, 5, 6}, -1, "bc-1");
  gmsh::model::addPhysicalGroup(2, {1, 2}, -1, "vc-1");
  gmsh::model::mesh::setRecombine(2, 2);
  gmsh::model::mesh::generate(2);
  gmsh::write(mesh_file.string());
  gmsh::clear();
}

struct Test2d : testing::Test {
  static SubrosaDG::Mesh<2, SubrosaDG::PolyOrder::P2, SubrosaDG::MeshType::TriQuad>* mesh;
  static SubrosaDG::Integral<2, SubrosaDG::PolyOrder::P2, SubrosaDG::MeshType::TriQuad>* integral;
  static SubrosaDG::Solver<2, SubrosaDG::PolyOrder::P2, SubrosaDG::EquModel::Euler, SubrosaDG::MeshType::TriQuad>*
      solver;

  static void SetUpTestCase() {
    std::filesystem::path mesh_file = SubrosaDG::kProjectSourceDir / "build/out/test_2d/test_2d.msh";
    if (!std::filesystem::exists(mesh_file)) {
      generateMesh(mesh_file);
    }
    Test2d::mesh = new SubrosaDG::Mesh<2, SubrosaDG::PolyOrder::P2, SubrosaDG::MeshType::TriQuad>{mesh_file};
    Test2d::integral = new SubrosaDG::Integral<2, SubrosaDG::PolyOrder::P2, SubrosaDG::MeshType::TriQuad>;
    Test2d::solver =
        new SubrosaDG::Solver<2, SubrosaDG::PolyOrder::P2, SubrosaDG::EquModel::Euler, SubrosaDG::MeshType::TriQuad>;
  }

  static void TearDownTestCase() {
    delete Test2d::mesh;
    Test2d::mesh = nullptr;
    delete Test2d::integral;
    Test2d::integral = nullptr;
    delete Test2d::solver;
    Test2d::solver = nullptr;
  }
};

SubrosaDG::Mesh<2, SubrosaDG::PolyOrder::P2, SubrosaDG::MeshType::TriQuad>* Test2d::mesh;
SubrosaDG::Integral<2, SubrosaDG::PolyOrder::P2, SubrosaDG::MeshType::TriQuad>* Test2d::integral;
SubrosaDG::Solver<2, SubrosaDG::PolyOrder::P2, SubrosaDG::EquModel::Euler, SubrosaDG::MeshType::TriQuad>*
    Test2d::solver;

#endif  // SUBROSA_DG_TEST_STRUCTURE_2D_H_
