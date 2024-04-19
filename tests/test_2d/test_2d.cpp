/**
 * @file test_2d.cpp
 * @brief The source file of SubrosaDG 2d test.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-12-25
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#include <gtest/gtest.h>

#include "SubrosaDG"

inline static const std::filesystem::path kTestDirectory{SubrosaDG::kProjectSourceDirectory / "build/out/test_2d"};

template <SubrosaDG::PolynomialOrderEnum P, SubrosaDG::MeshModelEnum MeshModelType>
  requires(MeshModelType == SubrosaDG::MeshModelEnum::Triangle)
void generateMesh(const std::filesystem::path& mesh_file_path) {
  gmsh::model::add("test_2d");
  gmsh::model::geo::addPoint(0.0, 0.0, 0.0, 2.0);
  gmsh::model::geo::addPoint(1.0, 0.5, 0.0, 2.0);
  gmsh::model::geo::addPoint(0.0, 1.0, 0.0, 2.0);
  gmsh::model::geo::addLine(1, 2);
  gmsh::model::geo::addLine(2, 3);
  gmsh::model::geo::addLine(3, 1);
  gmsh::model::geo::addCurveLoop({1, 2, 3});
  gmsh::model::geo::addPlaneSurface({1});
  gmsh::model::geo::synchronize();
  gmsh::model::addPhysicalGroup(1, {2, 3}, -1, "bc-1");
  gmsh::model::addPhysicalGroup(1, {1}, -1, "bc-2");
  gmsh::model::addPhysicalGroup(2, {1}, -1, "vc-1");
  gmsh::model::mesh::generate(2);
  gmsh::model::mesh::setOrder(magic_enum::enum_integer(P));
  gmsh::write(mesh_file_path);
}

template <SubrosaDG::PolynomialOrderEnum P, SubrosaDG::MeshModelEnum MeshModelType>
  requires(MeshModelType == SubrosaDG::MeshModelEnum::Quadrangle)
void generateMesh(const std::filesystem::path& mesh_file_path) {
  gmsh::model::add("test_2d");
  gmsh::model::geo::addPoint(0.0, 0.0, 0.0, 1.0);
  gmsh::model::geo::addPoint(1.0, 0.0, 0.0, 1.0);
  gmsh::model::geo::addPoint(1.0, 1.0, 0.0, 1.0);
  gmsh::model::geo::addPoint(0.0, 1.0, 0.0, 1.0);
  gmsh::model::geo::addLine(1, 2);
  gmsh::model::geo::addLine(2, 3);
  gmsh::model::geo::addLine(3, 4);
  gmsh::model::geo::addLine(4, 1);
  gmsh::model::geo::addCurveLoop({1, 2, 3, 4});
  gmsh::model::geo::addPlaneSurface({1});
  gmsh::model::geo::synchronize();
  gmsh::model::addPhysicalGroup(1, {2, 3, 4}, -1, "bc-1");
  gmsh::model::addPhysicalGroup(1, {1}, -1, "bc-2");
  gmsh::model::addPhysicalGroup(2, {1}, -1, "vc-1");
  gmsh::model::mesh::setTransfiniteAutomatic();
  gmsh::model::mesh::setRecombine(2, 1);
  gmsh::model::mesh::generate(2);
  gmsh::model::mesh::setOrder(magic_enum::enum_integer(P));
  gmsh::write(mesh_file_path);
}

template <SubrosaDG::PolynomialOrderEnum P, SubrosaDG::MeshModelEnum MeshModelType>
  requires(MeshModelType == SubrosaDG::MeshModelEnum::TriangleQuadrangle)
void generateMesh(const std::filesystem::path& mesh_file_path) {
  gmsh::model::add("test_2d");
  gmsh::model::geo::addPoint(0.0, 0.0, 0.0, 1.0);
  gmsh::model::geo::addPoint(1.0, 0.0, 0.0, 1.0);
  gmsh::model::geo::addPoint(1.0, 1.0, 0.0, 1.0);
  gmsh::model::geo::addPoint(0.0, 1.0, 0.0, 1.0);
  gmsh::model::geo::addPoint(2.0, 0.5, 0.0, 2.0);
  gmsh::model::geo::addLine(1, 2);
  gmsh::model::geo::addLine(2, 3);
  gmsh::model::geo::addLine(3, 4);
  gmsh::model::geo::addLine(4, 1);
  gmsh::model::geo::addLine(2, 5);
  gmsh::model::geo::addLine(5, 3);
  gmsh::model::geo::addCurveLoop({1, 2, 3, 4}, 1);
  gmsh::model::geo::addCurveLoop({5, 6, -2}, 2);
  gmsh::model::geo::addPlaneSurface({1}, 1);
  gmsh::model::geo::addPlaneSurface({2}, 2);
  gmsh::model::geo::synchronize();
  gmsh::model::addPhysicalGroup(1, {3, 4, 5, 6}, -1, "bc-1");
  gmsh::model::addPhysicalGroup(1, {1}, -1, "bc-2");
  gmsh::model::addPhysicalGroup(2, {1, 2}, -1, "vc-1");
  gmsh::model::mesh::setTransfiniteAutomatic();
  gmsh::model::mesh::generate(2);
  gmsh::model::mesh::setOrder(magic_enum::enum_integer(P));
  gmsh::write(mesh_file_path);
}

template <SubrosaDG::PolynomialOrderEnum P, SubrosaDG::MeshModelEnum MeshModelType,
          SubrosaDG::ViewModelEnum ViewModelType>
void runTest() {
  using SimulationControl = SubrosaDG::SimulationControlEuler<
      2, P, MeshModelType, SubrosaDG::ThermodynamicModelEnum::ConstantE, SubrosaDG::EquationOfStateEnum::IdealGas,
      SubrosaDG::ConvectiveFluxEnum::Central, SubrosaDG::TimeIntegrationEnum::TestInitialization, ViewModelType>;
  constexpr auto kGenerateMeshPrefix = []() {
    if constexpr (MeshModelType == SubrosaDG::MeshModelEnum::Triangle) {
      return "T";
    } else if constexpr (MeshModelType == SubrosaDG::MeshModelEnum::Quadrangle) {
      return "Q";
    } else if constexpr (MeshModelType == SubrosaDG::MeshModelEnum::TriangleQuadrangle) {
      return "TQ";
    }
  };
  const std::string output_prefix = std::format("test_2d_{}_{}", magic_enum::enum_name(P), kGenerateMeshPrefix());
  SubrosaDG::System<SimulationControl> system{false};
  system.setMesh(kTestDirectory / (output_prefix + ".msh"), generateMesh<P, MeshModelType>);
  system.addInitialCondition("vc-1", []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, 2>& coordinate) {
    return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{1.4, 0.1, 0.0, 1.0};
  });
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::RiemannFarfield>("bc-1", {1.4, 0.1, 0.0, 1.0});
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::AdiabaticNoSlipWall>("bc-2");
  system.synchronize();
  system.setTimeIntegration(false, 1, 1.0);
  system.setViewConfig(-1, kTestDirectory, output_prefix, SubrosaDG::ViewConfigEnum::Default);
  system.setViewVariable({SubrosaDG::ViewVariableEnum::Density, SubrosaDG::ViewVariableEnum::Velocity,
                          SubrosaDG::ViewVariableEnum::Temperature, SubrosaDG::ViewVariableEnum::Pressure,
                          SubrosaDG::ViewVariableEnum::SoundSpeed, SubrosaDG::ViewVariableEnum::MachNumber,
                          SubrosaDG::ViewVariableEnum::Entropy});
  system.solve();
  system.view(false);
}

TEST(Test2d, P1TriangleDat) {
  runTest<SubrosaDG::PolynomialOrderEnum::P1, SubrosaDG::MeshModelEnum::Triangle, SubrosaDG::ViewModelEnum::Dat>();
}

TEST(Test2d, P1TriangleVtu) {
  runTest<SubrosaDG::PolynomialOrderEnum::P1, SubrosaDG::MeshModelEnum::Triangle, SubrosaDG::ViewModelEnum::Vtu>();
}

TEST(Test2d, P1QuadrangleDat) {
  runTest<SubrosaDG::PolynomialOrderEnum::P1, SubrosaDG::MeshModelEnum::Quadrangle, SubrosaDG::ViewModelEnum::Dat>();
}

TEST(Test2d, P1QuadrangleVtu) {
  runTest<SubrosaDG::PolynomialOrderEnum::P1, SubrosaDG::MeshModelEnum::Quadrangle, SubrosaDG::ViewModelEnum::Vtu>();
}

TEST(Test2d, P1TriangleQuadrangleDat) {
  runTest<SubrosaDG::PolynomialOrderEnum::P1, SubrosaDG::MeshModelEnum::TriangleQuadrangle,
          SubrosaDG::ViewModelEnum::Dat>();
}

TEST(Test2d, P1TriangleQuadrangleVtu) {
  runTest<SubrosaDG::PolynomialOrderEnum::P1, SubrosaDG::MeshModelEnum::TriangleQuadrangle,
          SubrosaDG::ViewModelEnum::Vtu>();
}

TEST(Test2d, P2TriangleDat) {
  runTest<SubrosaDG::PolynomialOrderEnum::P2, SubrosaDG::MeshModelEnum::Triangle, SubrosaDG::ViewModelEnum::Dat>();
}

TEST(Test2d, P2TriangleVtu) {
  runTest<SubrosaDG::PolynomialOrderEnum::P2, SubrosaDG::MeshModelEnum::Triangle, SubrosaDG::ViewModelEnum::Vtu>();
}

TEST(Test2d, P2QuadrangleDat) {
  runTest<SubrosaDG::PolynomialOrderEnum::P2, SubrosaDG::MeshModelEnum::Quadrangle, SubrosaDG::ViewModelEnum::Dat>();
}

TEST(Test2d, P2QuadrangleVtu) {
  runTest<SubrosaDG::PolynomialOrderEnum::P2, SubrosaDG::MeshModelEnum::Quadrangle, SubrosaDG::ViewModelEnum::Vtu>();
}

TEST(Test2d, P2TriangleQuadrangleDat) {
  runTest<SubrosaDG::PolynomialOrderEnum::P2, SubrosaDG::MeshModelEnum::TriangleQuadrangle,
          SubrosaDG::ViewModelEnum::Dat>();
}

TEST(Test2d, P2TriangleQuadrangleVtu) {
  runTest<SubrosaDG::PolynomialOrderEnum::P2, SubrosaDG::MeshModelEnum::TriangleQuadrangle,
          SubrosaDG::ViewModelEnum::Vtu>();
}

TEST(Test2d, P3TriangleDat) {
  runTest<SubrosaDG::PolynomialOrderEnum::P3, SubrosaDG::MeshModelEnum::Triangle, SubrosaDG::ViewModelEnum::Dat>();
}

TEST(Test2d, P3TriangleVtu) {
  runTest<SubrosaDG::PolynomialOrderEnum::P3, SubrosaDG::MeshModelEnum::Triangle, SubrosaDG::ViewModelEnum::Vtu>();
}

TEST(Test2d, P3QuadrangleDat) {
  runTest<SubrosaDG::PolynomialOrderEnum::P3, SubrosaDG::MeshModelEnum::Quadrangle, SubrosaDG::ViewModelEnum::Dat>();
}

TEST(Test2d, P3QuadrangleVtu) {
  runTest<SubrosaDG::PolynomialOrderEnum::P3, SubrosaDG::MeshModelEnum::Quadrangle, SubrosaDG::ViewModelEnum::Vtu>();
}

TEST(Test2d, P3TriangleQuadrangleDat) {
  runTest<SubrosaDG::PolynomialOrderEnum::P3, SubrosaDG::MeshModelEnum::TriangleQuadrangle,
          SubrosaDG::ViewModelEnum::Dat>();
}

TEST(Test2d, P3TriangleQuadrangleVtu) {
  runTest<SubrosaDG::PolynomialOrderEnum::P3, SubrosaDG::MeshModelEnum::TriangleQuadrangle,
          SubrosaDG::ViewModelEnum::Vtu>();
}

TEST(Test2d, P4TriangleDat) {
  runTest<SubrosaDG::PolynomialOrderEnum::P4, SubrosaDG::MeshModelEnum::Triangle, SubrosaDG::ViewModelEnum::Dat>();
}

TEST(Test2d, P4TriangleVtu) {
  runTest<SubrosaDG::PolynomialOrderEnum::P4, SubrosaDG::MeshModelEnum::Triangle, SubrosaDG::ViewModelEnum::Vtu>();
}

TEST(Test2d, P4QuadrangleDat) {
  runTest<SubrosaDG::PolynomialOrderEnum::P4, SubrosaDG::MeshModelEnum::Quadrangle, SubrosaDG::ViewModelEnum::Dat>();
}

TEST(Test2d, P4QuadrangleVtu) {
  runTest<SubrosaDG::PolynomialOrderEnum::P4, SubrosaDG::MeshModelEnum::Quadrangle, SubrosaDG::ViewModelEnum::Vtu>();
}

TEST(Test2d, P4TriangleQuadrangleDat) {
  runTest<SubrosaDG::PolynomialOrderEnum::P4, SubrosaDG::MeshModelEnum::TriangleQuadrangle,
          SubrosaDG::ViewModelEnum::Dat>();
}

TEST(Test2d, P4TriangleQuadrangleVtu) {
  runTest<SubrosaDG::PolynomialOrderEnum::P4, SubrosaDG::MeshModelEnum::TriangleQuadrangle,
          SubrosaDG::ViewModelEnum::Vtu>();
}

TEST(Test2d, P5TriangleDat) {
  runTest<SubrosaDG::PolynomialOrderEnum::P5, SubrosaDG::MeshModelEnum::Triangle, SubrosaDG::ViewModelEnum::Dat>();
}

TEST(Test2d, P5TriangleVtu) {
  runTest<SubrosaDG::PolynomialOrderEnum::P5, SubrosaDG::MeshModelEnum::Triangle, SubrosaDG::ViewModelEnum::Vtu>();
}

TEST(Test2d, P5QuadrangleDat) {
  runTest<SubrosaDG::PolynomialOrderEnum::P5, SubrosaDG::MeshModelEnum::Quadrangle, SubrosaDG::ViewModelEnum::Dat>();
}

TEST(Test2d, P5QuadrangleVtu) {
  runTest<SubrosaDG::PolynomialOrderEnum::P5, SubrosaDG::MeshModelEnum::Quadrangle, SubrosaDG::ViewModelEnum::Vtu>();
}

TEST(Test2d, P5TriangleQuadrangleDat) {
  runTest<SubrosaDG::PolynomialOrderEnum::P5, SubrosaDG::MeshModelEnum::TriangleQuadrangle,
          SubrosaDG::ViewModelEnum::Dat>();
}

TEST(Test2d, P5TriangleQuadrangleVtu) {
  runTest<SubrosaDG::PolynomialOrderEnum::P5, SubrosaDG::MeshModelEnum::TriangleQuadrangle,
          SubrosaDG::ViewModelEnum::Vtu>();
}
