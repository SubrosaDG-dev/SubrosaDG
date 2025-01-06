#[[
# @file CMakeLists.txt
# @brief The main cmake file for SubrosaDG Project.
#
# @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
# @date 2022-11-01
#
# @version 0.1.0
# @copyright Copyright (c) 2022 - 2025 by SubrosaDG developers. All rights reserved.
# SubrosaDG is free software and is distributed under the MIT license.
#]]

cmake_minimum_required(VERSION 3.30)

# add cmake module path
include(${CMAKE_CURRENT_LIST_DIR}/cmake/GlobFiles.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/Utils.cmake)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# get git revision
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
string(SUBSTRING ${GIT_SHA1} 0 8 SHORT_SHA1)
git_local_changes(GIT_CHANGE)

# extract version from version.h and set version string
subrosa_dg_extract_version()
set(SUBROSA_DG_VERSION_STRING "${SUBROSA_DG_VERSION} - ${SHORT_SHA1} - ${GIT_CHANGE}")

# set option for develop build
option(SUBROSA_DG_DEVELOP "Build SubrosaDG in Develop" OFF)

# set option for gpu vendor
option(SUBROSA_DG_ONEAPI "Build SubrosaDG with oneAPI" OFF)
option(SUBROSA_DG_CUDA "Build SubrosaDG with CUDA" OFF)
option(SUBROSA_DG_ROCM "Build SubrosaDG with ROCm" OFF)

# set option to build different target
option(SUBROSA_DG_BUILD_EXAMPLES "Build SubrosaDG example cases" OFF)
option(SUBROSA_DG_BUILD_DOCS "Build SubrosaDG document" OFF)

# set compiler
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    set(COMPILER_PATH "/opt/intel/oneapi/compiler/latest/bin/compiler")
    set(CMAKE_CXX_COMPILER "/opt/intel/oneapi/compiler/latest/bin/icpx")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    set(COMPILER_PATH "/opt/homebrew/opt/llvm/bin")
    set(CMAKE_CXX_COMPILER "/opt/homebrew/opt/llvm/bin/clang++")
endif()

# set project
project(subrosa_dg LANGUAGES CXX VERSION ${SUBROSA_DG_VERSION} DESCRIPTION "SubrosaDG")

# message error if build in source dir
if(${PROJECT_SOURCE_DIR} STREQUAL ${PROJECT_BINARY_DIR})
    message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run \
            CMake from there.")
endif()

# set vcpkg overlay ports path
set(VCPKG_OVERLAY_PORTS "${CMAKE_CURRENT_LIST_DIR}/cmake/vcpkg-ports")

# set vcpkg overlay triplets path
set(VCPKG_OVERLAY_TRIPLETS "${CMAKE_CURRENT_LIST_DIR}/cmake/vcpkg-triplets")

# set vcpkg triplet
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    set(VCPKG_HOST_TRIPLET "x64-linux-static")
    set(VCPKG_TARGET_TRIPLET "x64-linux-static")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    set(VCPKG_HOST_TRIPLET "arm64-osx-static")
    set(VCPKG_TARGET_TRIPLET "arm64-osx-static")
endif()

# add vcpkg cmake
include($ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)

# message build version
message(STATUS "Build SubrosaDG: ${SUBROSA_DG_VERSION_STRING}")

# message build type
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# query the system cores number
cmake_host_system_information(RESULT NUMBER_OF_PHYSICAL_CORES QUERY NUMBER_OF_PHYSICAL_CORES)
message(STATUS "Number of physical cores: ${NUMBER_OF_PHYSICAL_CORES}")

# add cmake check
include(CheckCXXCompilerFlag)

# check cxx23 support
check_cxx_compiler_flag(-std=c++23 cxx_std_23_support)
if(NOT cxx_std_23_support)
    message(FATAL_ERROR "SubrosaDG requires C++23 to compile.")
endif()

# set cxx standard for all target
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# set cmake output verbose on
set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "cmake --verbose on" FORCE)

# create .gdbinit and .lldbinit file
configure_file(${PROJECT_SOURCE_DIR}/cmake/debuginit/.gdbinit.in
    ${PROJECT_SOURCE_DIR}/utils/gdb-pretty-print/.gdbinit @ONLY)

# create vscode launch.json and settings.json
file(MAKE_DIRECTORY ${PROJECT_SOURCE_DIR}/.vscode)
configure_file(${PROJECT_SOURCE_DIR}/utils/vscode-setting/launch.json.in
    ${PROJECT_SOURCE_DIR}/.vscode/launch.json @ONLY)
configure_file(${PROJECT_SOURCE_DIR}/utils/vscode-setting/settings.json.in
    ${PROJECT_SOURCE_DIR}/.vscode/settings.json @ONLY)

# create devcontainer devcontainer.json and Dockerfile
file(MAKE_DIRECTORY ${PROJECT_SOURCE_DIR}/.devcontainer)
configure_file(${PROJECT_SOURCE_DIR}/utils/vscode-setting/devcontainer.json.in
    ${PROJECT_SOURCE_DIR}/.devcontainer/devcontainer.json @ONLY)
configure_file(${PROJECT_SOURCE_DIR}/Dockerfile ${PROJECT_SOURCE_DIR}/.devcontainer/Dockerfile @ONLY)

# use clang-tidy to check file
find_program(CLANG_TIDY NAMES clang-tidy PATHS ${COMPILER_PATH} NO_DEFAULT_PATH)
if(CLANG_TIDY)
    include(${PROJECT_SOURCE_DIR}/cmake/ClangTidy.cmake)
    message(STATUS "Enable clang-tidy in ${CLANG_TIDY}")
else()
    message(WARNING "clang-tidy not found. Please install clang-tidy.")
endif()

# use clang-format to format file
find_program(CLANG_FORMAT NAMES clang-format PATHS ${COMPILER_PATH} NO_DEFAULT_PATH)
if(CLANG_FORMAT)
    include(${PROJECT_SOURCE_DIR}/cmake/ClangFormat.cmake)
    message(STATUS "Enable clang-format in ${CLANG_FORMAT}")
else()
    message(WARNING "clang-format not found. Please install clang-format.")
endif()

# use clangd for intellisense
find_program(CLANGD NAMES clangd PATHS ${COMPILER_PATH} NO_DEFAULT_PATH)
if(CLANGD)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Generate compile commands" FORCE)
    message(STATUS "Enable clangd in ${CLANGD}")
else()
    message(WARNING "clangd not found. Please install clangd.")
endif()

# use ccache to speed up compilation
find_program(CCACHE NAMES ccache)
if(CCACHE)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE}")
    message(STATUS "Enable ccache in ${CCACHE}")
else()
    message(WARNING "ccache not found. Please install ccache.")
endif()

# set output path
set(CMAKE_INCLUDE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/include")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/libs")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/libs")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# set library compiler variable
set(SUBROSA_DG_INCLUDE_DIRS)
set(SUBROSA_DG_INCLUDE_SYSTEM_DIRS)
set(SUBROSA_DG_LINK_LIBRARIES)
set(SUBROSA_DG_LINK_OPTIONS)
set(SUBROSA_DG_COMPILE_OPTIONS)
set(SUBROSA_DG_COMPILE_DEFINITIONS)

# set library
add_library(${PROJECT_NAME} INTERFACE)
add_library(lib::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# set compilation flags
set(SUBROSA_DG_DEBUG_FLAGS)
set(SUBROSA_DG_RELEASE_FLAGS)
set(SUBROSA_DG_FLAGS)

# add compilation flags
string(APPEND SUBROSA_DG_DEBUG_FLAGS "-Wall -Wshadow -Wextra -Wconversion -Wdocumentation -Wno-narrowing ")
string(APPEND SUBROSA_DG_RELEASE_FLAGS "-march=native ")

# add oneapi compilation flags
if(SUBROSA_DG_ONEAPI)
    string(APPEND SUBROSA_DG_DEBUG_FLAGS "-fsycl-targets=spir64_gen ")
    string(APPEND SUBROSA_DG_RELEASE_FLAGS "-fsycl-targets=spir64_gen ")
    list(APPEND SUBROSA_DG_LINK_OPTIONS -fsycl-targets=spir64_gen)
endif()

# add cuda compilation flags
if(SUBROSA_DG_CUDA)
    string(APPEND SUBROSA_DG_DEBUG_FLAGS "-fsycl-targets=nvptx64-nvidia-cuda ")
    string(APPEND SUBROSA_DG_RELEASE_FLAGS "-fsycl-targets=nvptx64-nvidia-cuda ")
    list(APPEND SUBROSA_DG_LINK_OPTIONS -fsycl-targets=nvptx64-nvidia-cuda)
endif()

# add rocm compilation flags
if(SUBROSA_DG_ROCM)
    string(APPEND SUBROSA_DG_DEBUG_FLAGS "-fsycl-targets=amdgcn-amd-amdhsa ")
    string(APPEND SUBROSA_DG_RELEASE_FLAGS "-fsycl-targets=amdgcn-amd-amdhsa ")
    list(APPEND SUBROSA_DG_LINK_OPTIONS -fsycl-targets=amdgcn-amd-amdhsa)
endif()

# set compile flags
set(SUBROSA_DG_FLAGS "$<$<CONFIG:Debug>:${SUBROSA_DG_DEBUG_FLAGS}> \
    $<$<CONFIG:Release>:${SUBROSA_DG_RELEASE_FLAGS}>")
string(REPLACE " " ";" SUBROSA_DG_FLAGS ${SUBROSA_DG_FLAGS})

# add compile option
list(APPEND SUBROSA_DG_COMPILE_OPTIONS ${SUBROSA_DG_FLAGS})

# create and include the cmake header
configure_file(${PROJECT_SOURCE_DIR}/cmake/source/Cmake.cpp.in ${CMAKE_INCLUDE_OUTPUT_DIRECTORY}/Cmake.cpp @ONLY)
list(APPEND SUBROSA_DG_INCLUDE_DIRS ${CMAKE_INCLUDE_OUTPUT_DIRECTORY})

# create and include the SubrosaDG header
configure_file(${PROJECT_SOURCE_DIR}/cmake/source/SubrosaDG.cpp.in ${SUBROSA_DG_SOURCES_DIR}/SubrosaDG.cpp @ONLY)

# add include directory
list(APPEND SUBROSA_DG_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/src)

# find sycl
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    find_package(IntelSYCL REQUIRED PATHS /opt/intel/oneapi/compiler/latest/lib/cmake/IntelSYCL)
    list(APPEND SUBROSA_DG_LINK_LIBRARIES IntelSYCL::SYCL_CXX)
endif()

# find tqdm-cpp
find_path(TQDM_DIR NAMES tqdm.hpp PATHS ${PROJECT_SOURCE_DIR}/libs/tqdm-cpp)
list(APPEND SUBROSA_DG_INCLUDE_SYSTEM_DIRS ${TQDM_DIR})

#find vtu11
find_path(VTU11_DIR NAMES vtu11-cpp17.hpp PATHS ${PROJECT_SOURCE_DIR}/libs/vtu11)
list(APPEND SUBROSA_DG_INCLUDE_SYSTEM_DIRS ${VTU11_DIR})

# find dbg-macro
find_path(DBG_MACRO_DIR NAMES dbg-macro/dbg.h)
list(APPEND SUBROSA_DG_INCLUDE_SYSTEM_DIRS ${DBG_MACRO_DIR})

# find eigen3
find_package(Eigen3 CONFIG REQUIRED)
list(APPEND SUBROSA_DG_LINK_LIBRARIES Eigen3::Eigen)

# find fmt
find_package(fmt CONFIG REQUIRED)
list(APPEND SUBROSA_DG_LINK_LIBRARIES fmt::fmt)

# find gmsh
find_package(unofficial-gmsh CONFIG REQUIRED)
list(APPEND SUBROSA_DG_LINK_LIBRARIES unofficial::gmsh::lib)

# find magic_enum
find_package(magic_enum CONFIG REQUIRED)
list(APPEND SUBROSA_DG_LINK_LIBRARIES magic_enum::magic_enum)

# find zlib
find_package(ZLIB REQUIRED)
list(APPEND SUBROSA_DG_LINK_LIBRARIES ZLIB::ZLIB)

# set for openmp
find_package(OpenMP REQUIRED)
list(APPEND SUBROSA_DG_LINK_LIBRARIES OpenMP::OpenMP_CXX)

# set compilation definitions
foreach(SUBROSA_DG_OPTION IN ITEMS
        SUBROSA_DG_DEVELOP
        SUBROSA_DG_ONEAPI
        SUBROSA_DG_CUDA
        SUBROSA_DG_ROCM)
    if(${SUBROSA_DG_OPTION})
        list(APPEND SUBROSA_DG_COMPILE_DEFINITIONS ${SUBROSA_DG_OPTION})
        message(STATUS "Option: ${SUBROSA_DG_OPTION}: ON")
    else()
        message(STATUS "Option: ${SUBROSA_DG_OPTION}: OFF")
    endif()
endforeach()

# set library compiler option
target_include_directories(${PROJECT_NAME} INTERFACE ${SUBROSA_DG_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} SYSTEM INTERFACE ${SUBROSA_DG_INCLUDE_SYSTEM_DIRS})
target_link_libraries(${PROJECT_NAME} INTERFACE ${SUBROSA_DG_LINK_LIBRARIES})
target_link_options(${PROJECT_NAME} INTERFACE ${SUBROSA_DG_LINK_OPTIONS})
target_compile_options(${PROJECT_NAME} INTERFACE "${SUBROSA_DG_COMPILE_OPTIONS}")
target_compile_definitions(${PROJECT_NAME} INTERFACE ${SUBROSA_DG_COMPILE_DEFINITIONS})

# TODO: use cmake install and XXXXConfig.cmake to link

# modify the compile definitions to string for doxygen
string(REPLACE ";" " " SUBROSA_DG_COMPILE_DEFINITIONS "${SUBROSA_DG_COMPILE_DEFINITIONS}")

# add develop target
if(SUBROSA_DG_DEVELOP)
    add_executable(develop ${SUBROSA_DG_DEVELOP_SOURCES})
    target_link_libraries(develop lib::subrosa_dg)
endif()

# build examples
if(SUBROSA_DG_BUILD_EXAMPLES)
    add_subdirectory(examples)
    message(STATUS "Generating examples")
endif()

# build documents
if(SUBROSA_DG_BUILD_DOCS)
    find_package(Doxygen REQUIRED dot)
    if(DOXYGEN_FOUND)
        include(cmake/Doxygen.cmake)
        message(STATUS "Found Doxygen: ${DOXYGEN_VERSION}")
        message(STATUS "Generating documents")
    else()
        message(WARNING "Doxygen not found. Please install doxygen.")
    endif()
    find_package(LATEX COMPONENTS XELATEX BIBTEX)
    if(LATEX_XELATEX_FOUND)
        add_subdirectory(docs)
        message_tool_version(XeLaTeX "${XELATEX_COMPILER}")
        message_tool_version(BibTeX "${BIBTEX_COMPILER}")
    else()
        message(WARNING "XeLaTeX or BibTex not found. Please install texlive.")
    endif()
endif()
