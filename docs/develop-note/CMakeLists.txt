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

cmake_minimum_required(VERSION 3.30)

project(subrosa_dg_develop_note LANGUAGES NONE)

set(DEVELOP_NOTE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/docs/develop-note")

file(MAKE_DIRECTORY "${DEVELOP_NOTE_OUTPUT_DIRECTORY}/figures")

file(COPY ${CMAKE_CURRENT_LIST_DIR}/develop-note.cls DESTINATION ${DEVELOP_NOTE_OUTPUT_DIRECTORY})
file(COPY ${CMAKE_CURRENT_LIST_DIR}/references.bib DESTINATION ${DEVELOP_NOTE_OUTPUT_DIRECTORY})

add_custom_target(develop-note
    COMMAND ${Python_EXECUTABLE} "${CMAKE_CURRENT_LIST_DIR}/draw_figures.py"
    COMMAND ${XELATEX_COMPILER} ${LATEX_COMPILER_FLAGS} -output-directory ${DEVELOP_NOTE_OUTPUT_DIRECTORY}
        "${CMAKE_CURRENT_LIST_DIR}/develop-note.tex"
    COMMAND ${BIBTEX_COMPILER} develop-note.aux
    COMMAND ${XELATEX_COMPILER} ${LATEX_COMPILER_FLAGS} -output-directory ${DEVELOP_NOTE_OUTPUT_DIRECTORY}
        "${CMAKE_CURRENT_LIST_DIR}/develop-note.tex"
    COMMAND ${XELATEX_COMPILER} ${LATEX_COMPILER_FLAGS} -output-directory ${DEVELOP_NOTE_OUTPUT_DIRECTORY}
        "${CMAKE_CURRENT_LIST_DIR}/develop-note.tex"
    WORKING_DIRECTORY ${DEVELOP_NOTE_OUTPUT_DIRECTORY}
)
