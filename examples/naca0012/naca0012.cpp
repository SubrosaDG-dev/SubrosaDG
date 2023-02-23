/**
 * @file naca0012.cpp
 * @brief The source file for SubrosaDG example naca0012.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2022-11-02
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2022 by SubrosaDG developers
 */

#include <fmt/color.h>
#include <fmt/format.h>

#include <filesystem>
#include <iostream>

#include "subrosa_dg.h"

int main(int argc, char* argv[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  std::cout << fmt::format("SubrosaDG Version: {}", fmt::styled(SubrosaDG::kSubrosaDgVersion, fmt::fg(fmt::color::red)))
            << std::endl;
  const std::filesystem::path meshfile{SubrosaDG::kProjectSourceDir / "examples/naca0012/mesh/naca0012_mesh.cgns"};
  auto mesh = std::make_shared<SubrosaDG::MeshStructure>();
  SubrosaDG::openCgnsFile(mesh, meshfile);
  SubrosaDG::readBasicData(mesh);
  SubrosaDG::readBaseNode(mesh);
  SubrosaDG::readZone(mesh);
  SubrosaDG::readCoord(mesh);
  SubrosaDG::readZoneBC(mesh);
  SubrosaDG::readElement(mesh);
  SubrosaDG::readFamily(mesh);
  std::cout << mesh->fn_ << std::endl;
  std::cout << mesh->coord_[0]->operator()(1) << std::endl;
  return EXIT_SUCCESS;
}
