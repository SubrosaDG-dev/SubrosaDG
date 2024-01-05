#[[
# @file CMakeLists.txt
# @brief The cmake file for SubrosaDG 2d tests.
#
# @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
# @date 2022-11-02
#
# @version 0.1.0
# @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
# SubrosaDG is free software and is distributed under the MIT license.
#]]

cmake_minimum_required(VERSION 3.27)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/out/test_1d)

add_gtest(test_1d "test_1d.cpp")
