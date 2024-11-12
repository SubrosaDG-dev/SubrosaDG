#[[
# @file CMakeLists.txt
# @brief The CMakeLists file for the develop notes.
#
# @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
# @date 2023-07-24
#
# @version 0.1.0
# @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
# SubrosaDG is free software and is distributed under the MIT license.
#]]

cmake_minimum_required(VERSION 3.30)

project(subrosa_dg_develop_notes)

set(DEVELOP_NOTES_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/docs/develop-notes")
file(MAKE_DIRECTORY ${DEVELOP_NOTES_OUTPUT_DIRECTORY})
file(COPY ${CMAKE_CURRENT_LIST_DIR}/develop-note.cls DESTINATION ${DEVELOP_NOTES_OUTPUT_DIRECTORY})

add_custom_target(all_develop_notes)

function(add_develop_note notename notefile)
    set(NOTE_OUTPUT_DIRECTORY "${DEVELOP_NOTES_OUTPUT_DIRECTORY}/${notename}")
    file(MAKE_DIRECTORY ${NOTE_OUTPUT_DIRECTORY})
    file(COPY ${DEVELOP_NOTES_OUTPUT_DIRECTORY}/develop-note.cls DESTINATION ${NOTE_OUTPUT_DIRECTORY})
    add_custom_target(${notename}
        COMMAND ${XELATEX_COMPILER} ${LATEX_COMPILER_FLAGS} -output-directory ${NOTE_OUTPUT_DIRECTORY} ${notefile}
        COMMAND ${XELATEX_COMPILER} ${LATEX_COMPILER_FLAGS} -output-directory ${NOTE_OUTPUT_DIRECTORY} ${notefile}
        WORKING_DIRECTORY ${NOTE_OUTPUT_DIRECTORY}
    )
    add_dependencies(all_develop_notes ${notename})
endfunction()

function(add_develop_note_with_bib notename notefile)
    set(NOTE_OUTPUT_DIRECTORY "${DEVELOP_NOTES_OUTPUT_DIRECTORY}/${notename}")
    file(MAKE_DIRECTORY ${NOTE_OUTPUT_DIRECTORY})
    file(COPY ${DEVELOP_NOTES_OUTPUT_DIRECTORY}/develop-note.cls DESTINATION ${NOTE_OUTPUT_DIRECTORY})
    file(COPY ${CMAKE_CURRENT_LIST_DIR}/references.bib DESTINATION ${NOTE_OUTPUT_DIRECTORY})
    add_custom_target(${notename}
        COMMAND ${XELATEX_COMPILER} ${LATEX_COMPILER_FLAGS} -output-directory ${NOTE_OUTPUT_DIRECTORY} ${notefile}
        COMMAND ${BIBTEX_COMPILER} ${notename}.aux
        COMMAND ${XELATEX_COMPILER} ${LATEX_COMPILER_FLAGS} -output-directory ${NOTE_OUTPUT_DIRECTORY} ${notefile}
        COMMAND ${XELATEX_COMPILER} ${LATEX_COMPILER_FLAGS} -output-directory ${NOTE_OUTPUT_DIRECTORY} ${notefile}
        WORKING_DIRECTORY ${NOTE_OUTPUT_DIRECTORY}
    )
    add_dependencies(all_develop_notes ${notename})
endfunction()

add_dependencies(all_docs all_develop_notes)

add_subdirectory(dg_method)
