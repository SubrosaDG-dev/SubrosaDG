#[[
# @file CMakeLists.txt
# @brief The cmake file for SubrosaDG example naca0012.
#
# @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
# @date 2022-11-02
#
# @version 0.1.0
# @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
# SubrosaDG is free software and is distributed under the MIT license.
#]]

cmake_minimum_required(VERSION 3.26.3 FATAL_ERROR)

project(subrosa_dg_example_naca0012)

set(CMAKE_EXAMPLE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}")

# file(COPY ${CMAKE_CURRENT_LIST_DIR}/naca0012.cfg
#     DESTINATION ${CMAKE_EXAMPLE_RUNTIME_OUTPUT_DIRECTORY}
# )
# file(COPY ${CMAKE_CURRENT_LIST_DIR}/mesh/naca0012.msh
#     DESTINATION ${CMAKE_EXAMPLE_RUNTIME_OUTPUT_DIRECTORY}
# )

add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/naca0012.cpp)
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_EXAMPLE_RUNTIME_OUTPUT_DIRECTORY}"
)
target_link_libraries(${PROJECT_NAME} lib::subrosa_dg)
add_dependencies(all_examples ${PROJECT_NAME})
