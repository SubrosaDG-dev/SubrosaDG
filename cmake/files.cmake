#[[
# @file clang-format.cmake
# @brief The cmake file to search all files automaticly during compilation.
#
# @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
# @date 2022-11-03
#
# @version 0.1.0
# @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers
#]]

# set project dirs
set(SUBROSA_DG_SOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

# set source file
file(GLOB_RECURSE SUBROSA_DG_SOURCES CONFIGURE_DEPENDS
    ${SUBROSA_DG_SOURCES_DIR}/*.c
    ${SUBROSA_DG_SOURCES_DIR}/*.cpp
)
