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

#include "Utils/SystemControl.hpp"
#include "test_structure_2d.h"

TEST_F(Test2d, Develop) { system->solve(); }
