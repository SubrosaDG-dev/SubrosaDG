#[[
# @file CMakeLists.txt
# @brief The CMakeLists file for the develop notes.
#
# @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
# @date 2023-07-24
#
# @version 0.1.0
# @copyright Copyright (c) 2022 - 2025 by SubrosaDG developers. All rights reserved.
# SubrosaDG is free software and is distributed under the MIT license.
#]]

add_develop_note_with_bib(dg_method "${CMAKE_CURRENT_LIST_DIR}/dg_method.tex")

file(MAKE_DIRECTORY "${DEVELOP_NOTES_OUTPUT_DIRECTORY}/dg_method/figures")
