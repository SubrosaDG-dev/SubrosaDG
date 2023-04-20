#[[
# @file files.cmake
# @brief The cmake file to search all files automaticly during compilation.
#
# @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
# @date 2022-11-03
#
# @version 0.1.0
# @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
# SubrosaDG is free software and is distributed under the MIT license.
#]]

# set project dirs
set(SUBROSA_DG_HEADERS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(SUBROSA_DG_SOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(SUBROSA_DG_EXAMPLES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/examples")
set(SUBROSA_DG_TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")
set(SUBROSA_DG_CMAKE_IN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# search source file
file(GLOB_RECURSE SUBROSA_DG_HEADERS CONFIGURE_DEPENDS
    ${SUBROSA_DG_HEADERS_DIR}/*.h
    ${SUBROSA_DG_HEADERS_DIR}/*.hpp
)
file(GLOB_RECURSE SUBROSA_DG_SOURCES CONFIGURE_DEPENDS
    ${SUBROSA_DG_SOURCES_DIR}/*.c
    ${SUBROSA_DG_SOURCES_DIR}/*.cpp
)
file(GLOB_RECURSE SUBROSA_DG_EXAMPLES CONFIGURE_DEPENDS
    ${SUBROSA_DG_EXAMPLES_DIR}/*.h
    ${SUBROSA_DG_EXAMPLES_DIR}/*.hpp
    ${SUBROSA_DG_EXAMPLES_DIR}/*.c
    ${SUBROSA_DG_EXAMPLES_DIR}/*.cpp
)
file(GLOB_RECURSE SUBROSA_DG_TESTS CONFIGURE_DEPENDS
    ${SUBROSA_DG_TESTS_DIR}/*.h
    ${SUBROSA_DG_TESTS_DIR}/*.hpp
    ${SUBROSA_DG_TESTS_DIR}/*.c
    ${SUBROSA_DG_TESTS_DIR}/*.cpp
)
file(GLOB_RECURSE SUBROSA_DG_CMAKE_IN CONFIGURE_DEPENDS
    ${SUBROSA_DG_CMAKE_IN_DIR}/*.h.in
    ${SUBROSA_DG_CMAKE_IN_DIR}/*.hpp.in
)
