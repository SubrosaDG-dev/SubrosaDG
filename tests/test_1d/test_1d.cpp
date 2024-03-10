/**
 * @file test_1d.cpp
 * @brief The source file of SubrosaDG 1d test.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-12-21
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#include <gtest/gtest.h>

#include "SubrosaDG"

inline static const std::filesystem::path kTestDirectory{SubrosaDG::kProjectSourceDirectory / "build/out/test_1d"};

template <SubrosaDG::PolynomialOrderEnum P>
void generateMesh(const std::filesystem::path& mesh_file_path) {
  gmsh::model::add("test_1d");
  gmsh::model::geo::addPoint(0.0, 0.0, 0.0, 0.5);
  gmsh::model::geo::addPoint(1.0, 0.0, 0.0, 0.5);
  gmsh::model::geo::addLine(1, 2);
  gmsh::model::geo::synchronize();
  gmsh::model::addPhysicalGroup(0, {1}, -1, "bc-1");
  gmsh::model::addPhysicalGroup(0, {2}, -1, "bc-2");
  gmsh::model::addPhysicalGroup(1, {1}, -1, "vc-1");
  gmsh::model::mesh::generate(1);
  gmsh::model::mesh::setOrder(magic_enum::enum_integer(P));
  gmsh::write(mesh_file_path);
}

template <SubrosaDG::PolynomialOrderEnum P, SubrosaDG::ViewModelEnum ViewModelType>
void runTest() {
  using SimulationControl = SubrosaDG::SimulationControlEuler<
      1, P, SubrosaDG::MeshModelEnum::Line, SubrosaDG::ThermodynamicModelEnum::ConstantE,
      SubrosaDG::EquationOfStateEnum::IdealGas, SubrosaDG::ConvectiveFluxEnum::Central,
      SubrosaDG::TimeIntegrationEnum::TestInitialization, ViewModelType>;
  const std::string output_prefix = std::format("test_1d_{}", magic_enum::enum_name(P));
  SubrosaDG::System<SimulationControl> system{false};
  system.setMesh(kTestDirectory / (output_prefix + ".msh"), generateMesh<P>);
  system.addInitialCondition("vc-1", []([[maybe_unused]] const Eigen::Vector<SubrosaDG::Real, 1>& coordinate) {
    return Eigen::Vector<SubrosaDG::Real, SimulationControl::kPrimitiveVariableNumber>{1.4, 0.1, 1.0};
  });
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::NormalFarfield>("bc-1", {1.4, 0.1, 1.0});
  system.template addBoundaryCondition<SubrosaDG::BoundaryConditionEnum::RiemannFarfield>("bc-2", {1.4, 0.1, 1.0});
  system.synchronize();
  system.setTimeIntegration(false, 1, 1.0, 1e-10);
  system.setViewConfig(-1, kTestDirectory, output_prefix + "_D", SubrosaDG::ViewConfigEnum::Default);
  system.setViewVariable({SubrosaDG::ViewVariableEnum::Density, SubrosaDG::ViewVariableEnum::Velocity,
                          SubrosaDG::ViewVariableEnum::Temperature, SubrosaDG::ViewVariableEnum::Pressure,
                          SubrosaDG::ViewVariableEnum::SoundSpeed, SubrosaDG::ViewVariableEnum::MachNumber,
                          SubrosaDG::ViewVariableEnum::Entropy});
  system.solve();
  system.view(false);
}

TEST(Test1d, P1Dat) { runTest<SubrosaDG::PolynomialOrderEnum::P1, SubrosaDG::ViewModelEnum::Dat>(); }

TEST(Test1d, P1Vtu) { runTest<SubrosaDG::PolynomialOrderEnum::P1, SubrosaDG::ViewModelEnum::Vtu>(); }

TEST(Test1d, P2Dat) { runTest<SubrosaDG::PolynomialOrderEnum::P2, SubrosaDG::ViewModelEnum::Dat>(); }

TEST(Test1d, P2Vtu) { runTest<SubrosaDG::PolynomialOrderEnum::P2, SubrosaDG::ViewModelEnum::Vtu>(); }

TEST(Test1d, P3Dat) { runTest<SubrosaDG::PolynomialOrderEnum::P3, SubrosaDG::ViewModelEnum::Dat>(); }

TEST(Test1d, P3Vtu) { runTest<SubrosaDG::PolynomialOrderEnum::P3, SubrosaDG::ViewModelEnum::Vtu>(); }

TEST(Test1d, P4Dat) { runTest<SubrosaDG::PolynomialOrderEnum::P4, SubrosaDG::ViewModelEnum::Dat>(); }

TEST(Test1d, P4Vtu) { runTest<SubrosaDG::PolynomialOrderEnum::P4, SubrosaDG::ViewModelEnum::Vtu>(); }

TEST(Test1d, P5Dat) { runTest<SubrosaDG::PolynomialOrderEnum::P5, SubrosaDG::ViewModelEnum::Dat>(); }

TEST(Test1d, P5Vtu) { runTest<SubrosaDG::PolynomialOrderEnum::P5, SubrosaDG::ViewModelEnum::Vtu>(); }
