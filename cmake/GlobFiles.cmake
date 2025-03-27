#[[
# @file GlobFiles.cmake
# @brief The cmake file to search all files automaticly during compilation.
#
# @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
# @date 2022-11-03
#
# @version 0.1.0
# @copyright Copyright (c) 2022 - 2025 by SubrosaDG developers. All rights reserved.
# SubrosaDG is free software and is distributed under the MIT license.
#]]

set(SUBROSA_DG_SOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(SUBROSA_DG_CMAKE_IN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cmake/source)
set(SUBROSA_DG_EXAMPLES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/examples)

file(GLOB_RECURSE SUBROSA_DG_SOURCES CONFIGURE_DEPENDS
    ${SUBROSA_DG_SOURCES_DIR}/*.cpp
)

file(GLOB_RECURSE SUBROSA_DG_CMAKE_IN CONFIGURE_DEPENDS
    ${SUBROSA_DG_CMAKE_IN_DIR}/*.cpp.in
)

file(GLOB_RECURSE SUBROSA_DG_SOURCES_RELATIVE RELATIVE ${SUBROSA_DG_SOURCES_DIR} CONFIGURE_DEPENDS
    ${SUBROSA_DG_SOURCES_DIR}/*.cpp
)

foreach(FILE ${SUBROSA_DG_SOURCES_RELATIVE})
    if(${FILE} MATCHES SubrosaDG.cpp$)
        list(REMOVE_ITEM SUBROSA_DG_SOURCES_RELATIVE ${FILE})
    endif()
endforeach()

file(GLOB_RECURSE SUBROSA_DG_CMAKE_IN_RELATIVE RELATIVE ${SUBROSA_DG_CMAKE_IN_DIR} CONFIGURE_DEPENDS
    ${SUBROSA_DG_CMAKE_IN_DIR}/*.cpp.in
)

foreach(FILE ${SUBROSA_DG_CMAKE_IN_RELATIVE})
    if(${FILE} MATCHES SubrosaDG.cpp.in$)
        list(REMOVE_ITEM SUBROSA_DG_CMAKE_IN_RELATIVE ${FILE})
    endif()
endforeach()

set(SUBROSA_DG_SOURCES_RELATIVE_INCLUDE)
foreach(FILE ${SUBROSA_DG_CMAKE_IN_RELATIVE})
    string(REPLACE ".in" "" FILE ${FILE})
    list(APPEND SUBROSA_DG_SOURCES_RELATIVE_INCLUDE "#include \"${FILE}\"\n")
endforeach()
foreach(FILE ${SUBROSA_DG_SOURCES_RELATIVE})
    list(APPEND SUBROSA_DG_SOURCES_RELATIVE_INCLUDE "#include \"${FILE}\"\n")
endforeach()
string(REPLACE ";" "" SUBROSA_DG_SOURCES_RELATIVE_INCLUDE ${SUBROSA_DG_SOURCES_RELATIVE_INCLUDE})

file(GLOB_RECURSE SUBROSA_DG_EXAMPLES CONFIGURE_DEPENDS
    ${SUBROSA_DG_EXAMPLES_DIR}/*.cpp
)

set(SUBROSA_DG_DEVELOP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/utils/develop.cpp)
