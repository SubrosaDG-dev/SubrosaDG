/**
 * @file test_solver_2d.cpp
 * @brief The source file of test solver 2d.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-07-03
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#include <gtest/gtest.h>

#include <Eigen/Core>
#include <Eigen/LU>
#include <map>
#include <stdexcept>
#include <vector>

#include "SubrosaDG"
#include "test_structure_2d.h"

TEST_F(Test2d, GetIntegral) { SubrosaDG::getIntegral(*integral); }

TEST_F(Test2d, GetMesh) { SubrosaDG::getMesh(kBoundaryTMap, *integral, *mesh); }

TEST_F(Test2d, Develop) {
  SubrosaDG::getSolver<decltype(kSpatialDiscrete)>(*integral, *mesh, kThermoModel, kTimeVar, kInitVar, kFarfieldVar,
                                                   kViewConfig, *solver);
  SubrosaDG::getView(*mesh, kThermoModel, kTimeVar, kViewConfig, *view);
}
