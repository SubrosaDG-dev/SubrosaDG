#[[
# @file CMakeLists.txt
# @brief The cmake file for SubrosaDG example delta_3d_ns.
#
# @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
# @date 2024-06-13
#
# @version 0.1.0
# @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
# SubrosaDG is free software and is distributed under the MIT license.
#]]

cmake_minimum_required(VERSION 3.27)

project(delta_3d_ns)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/out/delta_3d_ns)

add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/delta_3d_ns.cpp)
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_EXAMPLES_RUNTIME_OUTPUT_DIRECTORY}"
)
target_link_libraries(${PROJECT_NAME} lib::subrosa_dg)
add_dependencies(all_examples ${PROJECT_NAME})
