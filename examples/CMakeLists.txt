#[[
# @file CMakeLists.txt
# @brief The main cmake file for SubrosaDG examples.
#
# @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
# @date 2022-11-01
#
# @version 0.1.0
# @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers
#]]

cmake_minimum_required(VERSION 3.26 FATAL_ERROR)

project(subrosa_dg_examples)

# add custom target
add_custom_target(AllExamples)

# mkdir output folder
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/out)

# add example subdirectory
add_subdirectory(naca0012)

# make ALLCI depend on ALLExamples
add_dependencies(AllCI AllExamples)
