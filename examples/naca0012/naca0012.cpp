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

#include <gmsh.h>

#include <Eigen/Core>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <vector>

#include "SubrosaDG"

void generateMesh() {
  auto naca0012_fun = [](double x) -> double {
    return 0.594689181 * (0.298222773 * std::sqrt(x) - 0.127125232 * x - 0.357907906 * x * x + 0.291984971 * x * x * x -
                          0.105174606 * x * x * x * x);
  };
  constexpr int kNaca0012PointNum = 101;
  Eigen::Matrix<double, kNaca0012PointNum, 3, Eigen::RowMajor> naca0012_point;
  naca0012_point.col(0) = Eigen::VectorXd::LinSpaced(kNaca0012PointNum, 0, 1);
  naca0012_point.col(1) = naca0012_point.col(0).unaryExpr(naca0012_fun);
  naca0012_point.col(2) = Eigen::VectorXd::Zero(kNaca0012PointNum);
  Eigen::Matrix<double, 4, 3, Eigen::RowMajor> farfield_points;
  farfield_points << -10, -10, 0, 10, -10, 0, 10, 10, 0, -10, 10, 0;
  std::vector<int> farfield_point_index;
  std::vector<int> naca0012_point_index;
  std::vector<int> farfield_line_index;
  gmsh::model::add("naca0012");
  for (const auto& row : farfield_points.rowwise()) {
    farfield_point_index.emplace_back(gmsh::model::geo::addPoint(row.x(), row.y(), row.z(), 1.0));
  }
  for (const auto& row : naca0012_point.rowwise()) {
    naca0012_point_index.emplace_back(gmsh::model::geo::addPoint(row.x(), row.y(), row.z(), 0.01));
  }
  for (const auto& row : naca0012_point.colwise().reverse()(Eigen::seq(1, Eigen::last - 1), Eigen::all).rowwise()) {
    naca0012_point_index.emplace_back(gmsh::model::geo::addPoint(row.x(), -row.y(), row.z(), 0.01));
  }
  for (SubrosaDG::Usize i = 0; i < farfield_point_index.size(); i++) {
    farfield_line_index.emplace_back(gmsh::model::geo::addLine(
        farfield_point_index[i], farfield_point_index[(i + 1) % farfield_point_index.size()]));
  }
  naca0012_point_index.emplace_back(naca0012_point_index.front());
  const int naca0012_line = gmsh::model::geo::addSpline(naca0012_point_index);
  const int farfield_line_loop = gmsh::model::geo::addCurveLoop(farfield_line_index);
  const int naca0012_line_loop = gmsh::model::geo::addCurveLoop({-naca0012_line});
  const int naca0012_plane_surface = gmsh::model::geo::addPlaneSurface({farfield_line_loop, naca0012_line_loop});
  gmsh::model::geo::synchronize();
  gmsh::model::addPhysicalGroup(1, farfield_line_index, -1, "bc-1");
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
  SubrosaDG::EnvGardian environment_gardian;
  generateMesh();
  return EXIT_SUCCESS;
}
