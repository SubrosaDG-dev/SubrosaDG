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
#include <Eigen/Core>            // for Matrix, Block, CommaInitializer, indexed_based_stl_iterator_base, DenseCoeff...
#include <filesystem>            // for operator/, path
#include <fstream>               // for ifstream, basic_istream, ios_base
#include <vector>                // for vector, allocator
#include <cstdlib>               // for EXIT_SUCCESS

#include "basic/data_types.h"    // for Isize, Usize
#include "basic/environments.h"  // for EnvironmentGardian
#include "cmake.h"               // for kProjectSourceDir

// clang-format on

void generateMesh() {
  std::ifstream fin{(SubrosaDG::kProjectSourceDir / "examples/naca0012/naca0012.dat"), std::ios_base::in};
  SubrosaDG::Isize number{(fin >> number, number)};
  Eigen::Matrix<double, 3, Eigen::Dynamic> naca0012_points{3, number};
  for (SubrosaDG::Isize i = 0; i < number; i++) {
    fin >> naca0012_points(0, i) >> naca0012_points(1, i) >> naca0012_points(2, i);
  }
  fin.close();
  Eigen::Matrix<double, 4, 3, Eigen::RowMajor> farfield_points;
  farfield_points << -10, -10, 0, 10, -10, 0, 10, 10, 0, -10, 10, 0;
  constexpr double kLc1 = 1.0;
  constexpr double kLc2 = 1e-2;
  std::vector<int> farfield_points_index;
  std::vector<int> naca0012_points_index;
  std::vector<int> farfield_lines_index;
  gmsh::model::add("naca0012");
  for (const auto& row : farfield_points.rowwise()) {
    farfield_points_index.emplace_back(gmsh::model::geo::addPoint(row.x(), row.y(), row.z(), kLc1));
  }
  for (const auto& col : naca0012_points.colwise()) {
    naca0012_points_index.emplace_back(gmsh::model::geo::addPoint(col.x(), col.y(), col.z(), kLc2));
  }
  for (SubrosaDG::Usize i = 0; i < farfield_points_index.size(); i++) {
    farfield_lines_index.emplace_back(gmsh::model::geo::addLine(
        farfield_points_index[i], farfield_points_index[(i + 1) % farfield_points_index.size()]));
  }
  naca0012_points_index.emplace_back(naca0012_points_index.front());
  const int naca0012_line = gmsh::model::geo::addSpline(naca0012_points_index);
  const int farfield_line_loop = gmsh::model::geo::addCurveLoop(farfield_lines_index);
  const int naca0012_line_loop = gmsh::model::geo::addCurveLoop({-naca0012_line});
  const int naca0012_plane_surface = gmsh::model::geo::addPlaneSurface({farfield_line_loop, naca0012_line_loop});
  gmsh::model::geo::synchronize();
  gmsh::model::addPhysicalGroup(1, farfield_lines_index, -1, "bc-1");
  gmsh::model::addPhysicalGroup(1, {naca0012_line}, -1, "bc-2");
  gmsh::model::addPhysicalGroup(2, {naca0012_plane_surface}, -1, "vc-1");
  gmsh::model::mesh::generate(2);
  gmsh::model::mesh::optimize("Netgen");
  gmsh::write((SubrosaDG::kProjectSourceDir / "build/out/naca0012/mesh/naca0012.msh").string());
  gmsh::clear();
}

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::EnvironmentGardian environment_gardian;
  generateMesh();
  return EXIT_SUCCESS;
}
