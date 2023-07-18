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

inline constexpr int kDim{2};

inline constexpr SubrosaDG::PolyOrder kPolyOrder{2};

inline constexpr SubrosaDG::MeshType kMeshType{SubrosaDG::MeshType::TriQuad};

inline constexpr SubrosaDG::EquModel kEquModel{SubrosaDG::EquModel::Euler};

inline const std::filesystem::path kProjectDir{SubrosaDG::kProjectSourceDir / "build/out/test_2d"};

inline constexpr SubrosaDG::TimeVar<SubrosaDG::TimeDiscrete::ForwardEuler> kTimeVar{1, 0.1, 1e-10};

inline constexpr SubrosaDG::SpatialDiscreteEuler<SubrosaDG::ConvectiveFlux::Roe> kSpatialDiscrete;

inline constexpr SubrosaDG::ThermoModel<SubrosaDG::EquModel::Euler> kThermoModel{1.4, 1.0 / 1.4};

inline const std::unordered_map<std::string_view, int> kRegionIdMap{{"vc-1"sv, 0}};

inline const std::vector<SubrosaDG::FlowVar<2>> kFlowVar{SubrosaDG::FlowVar<2>{{1.0, 0.0}, 1.4, 1.0, 1.0}};

inline const SubrosaDG::InitVar<2> kInitVar{kRegionIdMap, kFlowVar};

inline const std::unordered_map<std::string_view, SubrosaDG::Boundary> kBoundaryTMap{
    {"bc-1", SubrosaDG::Boundary::Farfield}};

inline constexpr SubrosaDG::FarfieldVar<2> kFarfieldVar{{1.0, 0.0}, 1.4, 1.0, 1.0};

void generateMesh(const std::filesystem::path& mesh_file) {
  // NOTE: if your gmsh doesn't compile with Blossom(such as fedora build file saying: blossoms is nonfree, see
  // contrib/blossoms/README.txt.) This mesh could be different from the version of gmsh which not uses Blossom.
  // gmsh::option::setNumber("Mesh.RecombinationAlgorithm", 1);
  Eigen::Matrix<double, 4, 3, Eigen::RowMajor> points;
  points << 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 0.0;
  gmsh::model::add("test_2d");
  for (const auto& row : points.rowwise()) {
    gmsh::model::occ::addPoint(row.x(), row.y(), row.z(), 1);
  }
  for (int i = 0; i < points.rows(); i++) {
    gmsh::model::occ::addLine(i + 1, (i + 1) % points.rows() + 1);
  }
  gmsh::model::occ::addLine(2, 4);
  gmsh::model::occ::addCurveLoop({1, 5, 4});
  gmsh::model::occ::addPlaneSurface({1});
  gmsh::model::occ::addCurveLoop({2, 3, -5});
  gmsh::model::occ::addPlaneSurface({2});
  gmsh::model::occ::synchronize();
  gmsh::model::addPhysicalGroup(1, {1, 2, 3, 4}, -1, "bc-1");
  gmsh::model::addPhysicalGroup(2, {1, 2}, -1, "vc-1");
  gmsh::model::mesh::setRecombine(2, 2);
  gmsh::model::mesh::generate(2);
  gmsh::write(mesh_file.string());
}

struct Test2d : testing::Test {
  static SubrosaDG::Mesh<kDim, kPolyOrder, kMeshType>* mesh;
  static SubrosaDG::Integral<kDim, kPolyOrder, kMeshType>* integral;
  static SubrosaDG::Solver<kDim, kPolyOrder, kEquModel, kMeshType>* solver;

  static void SetUpTestCase() {
    std::filesystem::path mesh_file = kProjectDir / "test_2d.msh";
    generateMesh(mesh_file);
    integral = new SubrosaDG::Integral<kDim, kPolyOrder, kMeshType>;
    mesh = new SubrosaDG::Mesh<kDim, kPolyOrder, kMeshType>{mesh_file};
    solver = new SubrosaDG::Solver<kDim, kPolyOrder, kEquModel, kMeshType>;
  }

  static void TearDownTestCase() {
    delete integral;
    integral = nullptr;
    delete mesh;
    mesh = nullptr;
    delete solver;
    solver = nullptr;
  }
};

SubrosaDG::Mesh<kDim, kPolyOrder, kMeshType>* Test2d::mesh;
SubrosaDG::Integral<kDim, kPolyOrder, kMeshType>* Test2d::integral;
SubrosaDG::Solver<kDim, kPolyOrder, kEquModel, kMeshType>* Test2d::solver;

#endif  // SUBROSA_DG_TEST_STRUCTURE_2D_H_
