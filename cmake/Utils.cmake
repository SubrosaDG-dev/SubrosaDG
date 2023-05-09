#[[
# @file Utils.cmake
# @brief The cmake file to build some utils.
#
# @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
# @date 2022-11-03
#
# @version 0.1.0
# @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
# SubrosaDG is free software and is distributed under the MIT license.
#]]

# function to extract version from version.h
function(subrosa_dg_extract_version)
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/include/utils/version.h" file_contents)
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
    set(SUBROSA_DG_LIB_VERSION "${ver_major}.${ver_minor}" CACHE STRING "SUBROSA_DG_VERSION" FORCE)
    set(SUBROSA_DG_VERSION "${ver_major}.${ver_minor}.${ver_patch}" CACHE STRING "SUBROSA_DG_VERSION" FORCE)
endfunction()

# function to message tool version
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

# colorize CMake output
# code adapted from stackoverflow: http://stackoverflow.com/a/19578320
# from post authored by https://stackoverflow.com/users/2556117/fraser
macro(define_colors)
  if(WIN32)
    # has no effect on WIN32
    set(ColourReset "")
    set(ColourBold "")
    set(Red "")
    set(Green "")
    set(Yellow "")
    set(Blue "")
    set(Magenta "")
    set(Cyan "")
    set(White "")
    set(BoldRed "")
    set(BoldGreen "")
    set(BoldYellow "")
    set(BoldBlue "")
    set(BoldMagenta "")
    set(BoldCyan "")
    set(BoldWhite "")
  else()
    string(ASCII 27 Esc)
    set(ColourReset "${Esc}[m")
    set(ColourBold "${Esc}[1m")
    set(Red "${Esc}[31m")
    set(Green "${Esc}[32m")
    set(Yellow "${Esc}[33m")
    set(Blue "${Esc}[34m")
    set(Magenta "${Esc}[35m")
    set(Cyan "${Esc}[36m")
    set(White "${Esc}[37m")
    set(BoldRed "${Esc}[1;31m")
    set(BoldGreen "${Esc}[1;32m")
    set(BoldYellow "${Esc}[1;33m")
    set(BoldBlue "${Esc}[1;34m")
    set(BoldMagenta "${Esc}[1;35m")
    set(BoldCyan "${Esc}[1;36m")
    set(BoldWhite "${Esc}[1;37m")
  endif()
endmacro()
