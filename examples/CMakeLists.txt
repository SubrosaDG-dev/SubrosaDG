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

cmake_minimum_required(VERSION 3.30)

project(subrosa_dg_examples LANGUAGES CXX)

add_custom_target(all_examples)

foreach(FILE ${SUBROSA_DG_EXAMPLES})
    get_filename_component(example_name ${FILE} NAME_WE)
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/out/${example_name})
    add_executable(${example_name} ${FILE})
    target_link_libraries(${example_name} lib::subrosa_dg)
    add_dependencies(all_examples ${example_name})
endforeach()
