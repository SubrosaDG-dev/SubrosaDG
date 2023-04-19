#[[
# @file CMakeLists.txt
# @brief The main cmake file for SubrosaDG examples.
#
# @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
# @date 2022-11-01
#
# @version 0.1.0
# @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
# SubrosaDG is free software and is distributed under the MIT license.
#]]

cmake_minimum_required(VERSION 3.26.3 FATAL_ERROR)

project(subrosa_dg_examples)

# add custom target
add_custom_target(all_examples)

# mkdir output folder
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/out)

# add example subdirectory
add_subdirectory(naca0012)

# make all_ci depend on all_examples
add_dependencies(all_ci all_examples)
