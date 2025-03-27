/**
 * @file SubroseDG.cpp
 * @brief The main head file of SubroseDG project.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2022-11-01
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2025 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CPP_
#define SUBROSA_DG_CPP_

// #define SUBROSA_DG_SINGLE_PRECISION

#ifndef SUBROSA_DG_DEVELOP
#define DBG_MACRO_DISABLE
#endif  // SUBROSA_DG_DEVELOP

#define VTU11_ENABLE_ZLIB

#define EIGEN_STACK_ALLOCATION_LIMIT 0
#define EIGEN_DONT_PARALLELIZE

#include <Eigen/Geometry>
#include <eigen3/unsupported/Eigen/CXX11/Tensor>
#include <filesystem>

// clang-format off

#include "Cmake.cpp"
#include "Mesh/Adjacency.cpp"
#include "Mesh/BasisFunction.cpp"
#include "Mesh/Element.cpp"
#include "Mesh/Geometry.cpp"
#include "Mesh/Quadrature.cpp"
#include "Mesh/ReadControl.cpp"
#include "Solver/BoundaryCondition.cpp"
#include "Solver/ConvectiveFlux.cpp"
#include "Solver/InitialCondition.cpp"
#include "Solver/PhysicalModel.cpp"
#include "Solver/SimulationControl.cpp"
#include "Solver/SolveControl.cpp"
#include "Solver/SourceTerm.cpp"
#include "Solver/SpatialDiscrete.cpp"
#include "Solver/TimeIntegration.cpp"
#include "Solver/TurbulenceModel.cpp"
#include "Solver/VariableConvertor.cpp"
#include "Solver/ViscousFlux.cpp"
#include "Utils/BasicDataType.cpp"
#include "Utils/Concept.cpp"
#include "Utils/Constant.cpp"
#include "Utils/Enum.cpp"
#include "Utils/Environment.cpp"
#include "Utils/SystemControl.cpp"
#include "Utils/Version.cpp"
#include "View/CommandLine.cpp"
#include "View/IOControl.cpp"
#include "View/Paraview.cpp"
#include "View/RawBinary.cpp"

// clang-format on

using namespace SubrosaDG::Literals;

void generateMesh(const std::filesystem::path& mesh_file_path);

#endif  // SUBROSA_DG_CPP_
