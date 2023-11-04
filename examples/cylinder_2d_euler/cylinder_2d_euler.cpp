/**
 * @file cylinder_2d_euler.cpp
 * @brief The 2D cylinder flow example with Euler equations.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-07-15
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#include <gmsh.h>

#include <Eigen/Core>
#include <Eigen/LU>
#include <cstdlib>
#include <filesystem>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "SubrosaDG"

using namespace std::string_view_literals;

inline constexpr int kDim{2};

inline constexpr SubrosaDG::PolyOrder kPolyOrder{2};

inline constexpr int kStep{1000};

inline constexpr SubrosaDG::MeshType kMeshType{SubrosaDG::MeshType::TriQuad};

inline constexpr SubrosaDG::EquModel kEquModel{SubrosaDG::EquModel::Euler};

inline const std::filesystem::path kProjectDir{SubrosaDG::kProjectSourceDir / "build/out/cylinder_2d_euler"};

inline constexpr SubrosaDG::TimeVar<SubrosaDG::TimeDiscrete::RK3SSP> kTimeVar{
    kStep, 1 / (2 * static_cast<SubrosaDG::Real>(kPolyOrder) + 1), 1e-10};

inline constexpr SubrosaDG::SpatialDiscreteEuler<SubrosaDG::ConvectiveFlux::Roe> kSpatialDiscrete;

inline constexpr SubrosaDG::ThermoModel<SubrosaDG::EquModel::Euler> kThermoModel{1.4, 1.0 / 1.4};

inline const std::unordered_map<std::string_view, int> kRegionIdMap{{"vc-1"sv, 0}};

inline const std::vector<SubrosaDG::FlowVar<2, kEquModel>> kFlowVar{
    SubrosaDG::FlowVar<2, kEquModel>{{0.38, 0.0}, 1.4, 1.0, 1.0}};

inline const SubrosaDG::InitVar<2, kEquModel> kInitVar{kRegionIdMap, kFlowVar};

inline const std::unordered_map<std::string_view, SubrosaDG::Boundary> kBoundaryTMap{
    {"bc-1", SubrosaDG::Boundary::Farfield}, {"bc-2", SubrosaDG::Boundary::Wall}};

inline constexpr SubrosaDG::FarfieldVar<2, kEquModel> kFarfieldVar{{0.38, 0.0}, 1.4, 1.0, 1.0};

inline const SubrosaDG::ViewConfig kViewConfig{kStep, kProjectDir, "cylinder_2d", SubrosaDG::ViewType::Dat};

void generateMesh(const std::filesystem::path& mesh_file) {
  Eigen::Matrix<double, 4, 3, Eigen::RowMajor> farfield_point;
  farfield_point << -5, -5, 0, 5, -5, 0, 5, 5, 0, -5, 5, 0;
  Eigen::Matrix<double, 5, 3, Eigen::RowMajor> cylinder_point;
  cylinder_point << 0, 0, 0, -1, 0, 0, 0, -1, 0, 1, 0, 0, 0, 1, 0;
  std::vector<int> farfield_point_tag;
  std::vector<int> cylinder_point_tag;
  std::vector<int> farfield_line_tag;
  std::vector<int> cylinder_line_tag;
  gmsh::model::add("cylinder_2d");
  for (const auto& row : farfield_point.rowwise()) {
    farfield_point_tag.emplace_back(gmsh::model::geo::addPoint(row.x(), row.y(), row.z(), 1));
  }
  for (const auto& row : cylinder_point.rowwise()) {
    cylinder_point_tag.emplace_back(gmsh::model::geo::addPoint(row.x(), row.y(), row.z(), 0.2));
  }
  for (std::size_t i = 0; i < farfield_point_tag.size(); i++) {
    farfield_line_tag.emplace_back(
        gmsh::model::geo::addLine(farfield_point_tag[i], farfield_point_tag[(i + 1) % farfield_point_tag.size()]));
  }
  for (std::size_t i = 1; i < cylinder_point_tag.size(); i++) {
    cylinder_line_tag.emplace_back(gmsh::model::geo::addCircleArc(
        cylinder_point_tag[i], cylinder_point_tag[0], cylinder_point_tag[i % (cylinder_point_tag.size() - 1) + 1]));
  }
  const int farfield_line_loop = gmsh::model::geo::addCurveLoop(farfield_line_tag);
  const int cylinder_line_loop = gmsh::model::geo::addCurveLoop(cylinder_line_tag);
  const int cylinder_plane_surface = gmsh::model::geo::addPlaneSurface({farfield_line_loop, cylinder_line_loop});
  gmsh::model::geo::synchronize();
  std::vector<double> cylinder_line_tag_double_cast{cylinder_line_tag.begin(), cylinder_line_tag.end()};
  const int cylinder_boundary_layer = gmsh::model::mesh::field::add("BoundaryLayer");
  gmsh::model::mesh::field::setNumbers(cylinder_boundary_layer, "CurvesList", cylinder_line_tag_double_cast);
  gmsh::model::mesh::field::setNumber(cylinder_boundary_layer, "Size", 0.05);
  gmsh::model::mesh::field::setNumber(cylinder_boundary_layer, "Ratio", 1.1);
  gmsh::model::mesh::field::setNumber(cylinder_boundary_layer, "Quads", 1);
  gmsh::model::mesh::field::setNumber(cylinder_boundary_layer, "Thickness", 0.5);
  gmsh::model::mesh::field::setAsBoundaryLayer(cylinder_boundary_layer);
  gmsh::model::addPhysicalGroup(1, farfield_line_tag, -1, "bc-1");
  gmsh::model::addPhysicalGroup(1, cylinder_line_tag, -1, "bc-2");
  gmsh::model::addPhysicalGroup(2, {cylinder_plane_surface}, -1, "vc-1");
  gmsh::model::mesh::generate(kDim);
  gmsh::model::mesh::setOrder(static_cast<int>(kPolyOrder));
  gmsh::write(mesh_file.string());
}

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  SubrosaDG::EnvGardian environment_gardian;
  std::filesystem::path mesh_file = kProjectDir / "cylinder_2d.msh";
  generateMesh(mesh_file);
  SubrosaDG::Integral<kDim, kPolyOrder, kMeshType> integral;
  SubrosaDG::Mesh<kDim, kPolyOrder, kMeshType> mesh{mesh_file};
  SubrosaDG::Solver<kDim, kPolyOrder, kMeshType, kEquModel> solver;
  SubrosaDG::View<kDim, kPolyOrder, kMeshType, kEquModel> view;
  SubrosaDG::getIntegral(integral);
  SubrosaDG::getMesh(kBoundaryTMap, integral, mesh);
  SubrosaDG::getSolver<decltype(kSpatialDiscrete)>(integral, mesh, kThermoModel, kTimeVar, kInitVar, kFarfieldVar,
                                                   kViewConfig, solver);
  SubrosaDG::getView(mesh, kThermoModel, kTimeVar, kViewConfig, view);
  return EXIT_SUCCESS;
}
