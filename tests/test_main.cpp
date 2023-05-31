/**
 * @file test_main.cpp
 * @brief The main source file of SubroseDG tests.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2022-11-02
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

// clang-format off

#include <gtest/gtest.h>          // for InitGoogleTest, RUN_ALL_TESTS

#include "basic/environment.hpp"  // for EnvGardian

// clang-format on

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  SubrosaDG::EnvGardian env_gardian;
  return RUN_ALL_TESTS();
}
