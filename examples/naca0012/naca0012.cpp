/**
 * @file naca0012.cpp
 * @brief The source file for SubrosaDG example naca0012.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2022-11-02
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

// clang-format off

#include <gmsh.h>                // for addPhysicalGroup, addCurveLoop, addPoint, add, addLine, addPlaneSurface, add...
#include <Eigen/Core>            // for Matrix, CommaInitializer, DenseCoeffsBase, Block, indexed_based_stl_iterator...
#include <filesystem>            // for operator/, path
#include <fstream>               // for basic_istream, ifstream, ios_base
#include <memory>                // for allocator, make_unique, unique_ptr
#include <vector>                // for vector
#include <cstddef>               // for EXIT_SUCCESS

#include "basic/data_type.h"     // for Index
#include "basic/environments.h"  // for EnvironmentGardian
#include "cmake.h"               // for kProjectSourceDir

// clang-format on

void generateMesh() {
  std::ifstream fin{(SubrosaDG::kProjectSourceDir / "examples/naca0012/naca0012.dat"), std::ios_base::in};
  SubrosaDG::Index number{(fin >> number, number)};
  auto naca0012_points = std::make_unique<Eigen::Matrix<double, 3, Eigen::Dynamic>>(3, number);
  for (Eigen::Index i = 0; i < static_cast<Eigen::Index>(number); i++) {
    fin >> (*naca0012_points)(0, i) >> (*naca0012_points)(1, i) >> (*naca0012_points)(2, i);
  }
  fin.close();
  auto farfield_points = std::make_unique<Eigen::Matrix<double, 4, 3, Eigen::RowMajor>>();
  (*farfield_points) << -10, -10, 0, 10, -10, 0, 10, 10, 0, -10, 10, 0;
  constexpr double kLc1 = 1.0;
  constexpr double kLc2 = 1e-2;
  std::vector<int> farfield_points_index;
  std::vector<int> naca0012_points_index;
  std::vector<int> farfield_lines_index;
  gmsh::model::add("naca0012");
  for (const auto& row : farfield_points->rowwise()) {
    farfield_points_index.emplace_back(gmsh::model::geo::addPoint(row(0), row(1), row(2), kLc1));
  }
  for (const auto& col : naca0012_points->colwise()) {
    naca0012_points_index.emplace_back(gmsh::model::geo::addPoint(col(0), col(1), col(2), kLc2));
  }
  for (SubrosaDG::Index i = 0; i < farfield_points_index.size(); i++) {
    farfield_lines_index.emplace_back(gmsh::model::geo::addLine(
        farfield_points_index[i], farfield_points_index[(i + 1) % farfield_points_index.size()]));
  }
  naca0012_points_index.emplace_back(naca0012_points_index.front());
  const int naca0012_line = gmsh::model::geo::addSpline(naca0012_points_index);
  const int farfield_line_loop = gmsh::model::geo::addCurveLoop(farfield_lines_index);
  const int naca0012_line_loop = gmsh::model::geo::addCurveLoop({naca0012_line});
  const int naca0012_plane_surface = gmsh::model::geo::addPlaneSurface({farfield_line_loop, naca0012_line_loop});
  gmsh::model::geo::synchronize();
  gmsh::model::addPhysicalGroup(1, farfield_lines_index, -1, "farfield");
  gmsh::model::addPhysicalGroup(1, {naca0012_line}, -1, "wall");
  gmsh::model::addPhysicalGroup(2, {naca0012_plane_surface}, -1, "air");
  gmsh::model::mesh::generate(2);
  gmsh::model::mesh::optimize("Netgen");
  gmsh::write((SubrosaDG::kProjectSourceDir / "build/out/naca0012.msh").string());
}

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::EnvironmentGardian environment_gardian;
  generateMesh();
  return EXIT_SUCCESS;
}
