#[[
# @file Utils.cmake
# @brief The cmake file to build some utils.
#
# @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
# @date 2022-11-03
#
# @version 0.1.0
# @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
# SubrosaDG is free software and is distributed under the MIT license.
#]]

function(subrosa_dg_extract_version)
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/Utils/Version.cpp" file_contents)
    string(REGEX MATCH "SUBROSA_DG_VERSION_MAJOR ([0-9]+)" _ "${file_contents}")
    if(NOT ${CMAKE_MATCH_COUNT} EQUAL 1)
        message(FATAL_ERROR "Could not extract major version number from version.h")
    endif()
    set(ver_major "${CMAKE_MATCH_1}")

    string(REGEX MATCH "SUBROSA_DG_VERSION_MINOR ([0-9]+)" _ "${file_contents}")
    if(NOT ${CMAKE_MATCH_COUNT} EQUAL 1)
        message(FATAL_ERROR "Could not extract minor version number from version.h")
    endif()

    set(ver_minor "${CMAKE_MATCH_1}")
    string(REGEX MATCH "SUBROSA_DG_VERSION_PATCH ([0-9]+)" _ "${file_contents}")
    if(NOT ${CMAKE_MATCH_COUNT} EQUAL 1)
        message(FATAL_ERROR "Could not extract patch version number from version.h")
    endif()
    set(ver_patch "${CMAKE_MATCH_1}")

    set(SUBROSA_DG_VERSION_MAJOR ${ver_major} CACHE STRING "SUBROSA_DG_VERSION_MAJOR" FORCE)
    set(SUBROSA_DG_VERSION_MINOR ${ver_minor} CACHE STRING "SUBROSA_DG_VERSION_MINOR" FORCE)
    set(SUBROSA_DG_VERSION_PATCH ${ver_patch} CACHE STRING "SUBROSA_DG_VERSION_PATCH" FORCE)
    set(SUBROSA_DG_VERSION "${ver_major}.${ver_minor}.${ver_patch}" CACHE STRING "SUBROSA_DG_VERSION" FORCE)
endfunction()

function(message_tool_version toolname toolpath)
    execute_process(
        COMMAND ${toolpath} --version
        RESULT_VARIABLE _
        OUTPUT_VARIABLE _out
    )
    string(REGEX REPLACE "\n" ";" _out "${_out}")
    string(REGEX MATCHALL "([^;]+;|[^;]+$)" _out_list "${_out}")
    list(GET _out_list 0 _output)
    message(STATUS "Found ${toolname}: ${_output}")
endfunction()
