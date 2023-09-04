#[[
# @file CMakeLists.txt
# @brief The CMakeLists file for the documents.
#
# @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
# @date 2023-07-24
#
# @version 0.1.0
# @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
# SubrosaDG is free software and is distributed under the MIT license.
#]]

cmake_minimum_required(VERSION 3.25)

project(subrosa_dg_docs)

set(LATEX_COMPILER_FLAGS "-interaction=batchmode -file-line-error -halt-on-error -no-shell-escape")
string(REPLACE " " ";" LATEX_COMPILER_FLAGS ${LATEX_COMPILER_FLAGS})

add_custom_target(all_docs)

add_subdirectory(develop-notes)
