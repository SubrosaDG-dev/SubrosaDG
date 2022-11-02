/**
 * @file test_main.cpp
 * @brief The main source file of SubroseDG tests.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2022-11-02
 *
 * @version 0.0.1
 * @copyright Copyright (c) 2022 - 2022 by SubrosaDG developers
 */

#include "subrosa_dg.h"
#include "gtest/gtest.h"

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
