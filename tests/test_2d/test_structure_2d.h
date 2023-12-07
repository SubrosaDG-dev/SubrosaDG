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

#include "SubrosaDG"

inline const std::filesystem::path kTestDirectory{SubrosaDG::kProjectSourceDirectory / "build/out/test_2d"};

using SimulationControl =
    SubrosaDG::SimulationControlEuler<2, SubrosaDG::PolynomialOrder::P1, SubrosaDG::MeshModel::TriangleQuadrangle,
                                      SubrosaDG::MeshHighOrderModel::Straight, SubrosaDG::ThermodynamicModel::ConstantE,
                                      SubrosaDG::EquationOfState::IdealGas, SubrosaDG::ConvectiveFlux::LaxFriedrichs,
                                      SubrosaDG::TimeIntegration::ForwardEuler, SubrosaDG::ViewModel::Dat>;

void generateMesh() {
  // NOTE: if your gmsh doesn't compile with Blossom(such as fedora build file saying: blossoms is nonfree, see
  // contrib/blossoms/README.txt.) This mesh could be different from the version of gmsh which not uses Blossom.
  // gmsh::option::setNumber("Mesh.RecombinationAlgorithm", 1);
  gmsh::option::setNumber("Mesh.SecondOrderLinear", 1);
  Eigen::Matrix<double, 4, 3, Eigen::RowMajor> points;
  points << -1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0;
  gmsh::model::add("test_2d");
  for (const auto& row : points.rowwise()) {
    gmsh::model::geo::addPoint(row.x(), row.y(), row.z(), 1);
  }
  gmsh::model::geo::addLine(1, 2);
  gmsh::model::geo::addLine(2, 3);
  gmsh::model::geo::addLine(2, 4);
  gmsh::model::geo::addCircleArc(3, 2, 4);
  gmsh::model::geo::addCircleArc(4, 2, 1);
  gmsh::model::geo::addCurveLoop({1, 3, 5});
  gmsh::model::geo::addPlaneSurface({1});
  gmsh::model::geo::addCurveLoop({2, 4, -3});
  gmsh::model::geo::addPlaneSurface({2});
  gmsh::model::geo::synchronize();
  gmsh::model::addPhysicalGroup(1, {4, 5}, -1, "bc-1");
  gmsh::model::addPhysicalGroup(1, {1, 2}, -1, "bc-2");
  gmsh::model::addPhysicalGroup(2, {1, 2}, -1, "vc-1");
  gmsh::model::mesh::setRecombine(2, 2);
  gmsh::model::mesh::generate(SimulationControl::kDimension);
  gmsh::model::mesh::setOrder(static_cast<int>(SimulationControl::kPolynomialOrder));
  gmsh::write(kTestDirectory / "test_2d.msh");
}

struct Test2d : testing::Test {
  static SubrosaDG::System<SimulationControl>* system;

  static void SetUpTestCase() {
    system = new SubrosaDG::System<SimulationControl>(generateMesh, kTestDirectory / "test_2d.msh");
    system->addInitialCondition("vc-1", []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, 2>& coordinate) {
      return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{1.4, 0.1, 0.0, 1.0};
    });
    system->addBoundaryCondition<SubrosaDG::BoundaryCondition::RiemannFarfield>("bc-1", {1.4, 0.1, 0.0, 1.0});
    system->addBoundaryCondition<SubrosaDG::BoundaryCondition::AdiabaticFreeSlipWall>("bc-2");
    system->setTimeIntegration(true, 1, 0.5, 1e-10);
    system->setViewConfig(-1, kTestDirectory, "test_2d", {SubrosaDG::ViewElementVariable::Density});
  }

  static void TearDownTestCase() {
    delete system;
    system = nullptr;
  }
};

SubrosaDG::System<SimulationControl>* Test2d::system{nullptr};

#endif  // SUBROSA_DG_TEST_STRUCTURE_2D_H_
