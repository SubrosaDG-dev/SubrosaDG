#[[
# @file CMakeLists.txt
# @brief The cmake file for SubrosaDG example sod_1d_ceuler.
#
# @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
# @date 2023-06-24
#
# @version 0.1.0
# @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
# SubrosaDG is free software and is distributed under the MIT license.
#]]

cmake_minimum_required(VERSION 3.30)

project(sod_1d_ceuler)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/out/sod_1d_ceuler)

add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/sod_1d_ceuler.cpp)
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_EXAMPLES_RUNTIME_OUTPUT_DIRECTORY}"
)
target_link_libraries(${PROJECT_NAME} lib::subrosa_dg)
add_dependencies(all_examples ${PROJECT_NAME})
