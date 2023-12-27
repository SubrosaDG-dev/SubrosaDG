#[[
# @file CMakeLists.txt
# @brief The main cmake file for SubrosaDG tests.
#
# @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
# @date 2022-11-02
#
# @version 0.1.0
# @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
# SubrosaDG is free software and is distributed under the MIT license.
#]]

cmake_minimum_required(VERSION 3.27)

project(subrosa_dg_tests)

set(CMAKE_TESTS_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}")

find_package(GTest CONFIG REQUIRED)

add_custom_target(all_tests)

if(SUBROSA_DG_WITH_MPI)
    set(TEST_MAIN_CPP "${CMAKE_CURRENT_LIST_DIR}/test_main_mpi.cpp")
else()
    set(TEST_MAIN_CPP "${CMAKE_CURRENT_LIST_DIR}/test_main.cpp")
endif()

function(add_gtest testname testfile)
    add_executable(${testname} EXCLUDE_FROM_ALL ${testfile} ${ARGN} ${TEST_MAIN_CPP})
    target_link_libraries(${testname} GTest::gtest lib::subrosa_dg)
    set_target_properties(${testname} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_TESTS_RUNTIME_OUTPUT_DIRECTORY}"
    )
    add_dependencies(all_tests ${testname})
    add_test(NAME ${testname} COMMAND ${CMAKE_TESTS_RUNTIME_OUTPUT_DIRECTORY}/${testname})
endfunction()

add_dependencies(all_ci all_tests)

add_subdirectory(test_1d)

add_subdirectory(test_2d)
