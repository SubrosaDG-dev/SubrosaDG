#[[
# @file CMakeLists.txt
# @brief The main cmake file for SubrosaDG examples.
#
# @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
# @date 2022-11-01
#
# @version 0.1.0
# @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
# SubrosaDG is free software and is distributed under the MIT license.
#]]

cmake_minimum_required(VERSION 3.27)

project(subrosa_dg_examples)

set(CMAKE_EXAMPLES_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}")

add_custom_target(all_examples)

add_subdirectory(blasius_3d_ns)

add_subdirectory(cylinder_2d_euler)

add_subdirectory(delta_3d_ns)

add_subdirectory(gravity_2d_ns)

add_subdirectory(instability_2d_euler)

add_subdirectory(naca0012_2d_euler)

add_subdirectory(naca0012_2d_ns)

add_subdirectory(periodic_1d_euler)

add_subdirectory(periodic_2d_euler)

add_subdirectory(periodic_3d_euler)

add_subdirectory(pipe_2d_ns)

add_subdirectory(protuberance_3d_euler)

add_subdirectory(rae2822_2d_ns)

add_subdirectory(sod_1d_euler)

add_subdirectory(sphere_3d_ns)

add_subdirectory(vortex_2d_ns)

add_dependencies(all_ci all_examples)
