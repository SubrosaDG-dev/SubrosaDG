#[[
# @file CMakeLists.txt
# @brief The main cmake file for SubrosaDG Project.
#
# @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
# @date 2022-11-01
#
# @version 0.1.0
# @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
# SubrosaDG is free software and is distributed under the MIT license.
#]]

cmake_minimum_required(VERSION 3.27)

# add cmake module path
include(${CMAKE_CURRENT_LIST_DIR}/cmake/GlobFiles.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/Utils.cmake)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# get git revision
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
string(SUBSTRING ${GIT_SHA1} 0 8 SHORT_SHA1)
git_local_changes(GIT_CHANGE)

# define color
# NOTE: vscode output do not support ansi color until now
define_colors()

# extract version from version.h and set version string
subrosa_dg_extract_version()
set(SUBROSA_DG_VERSION_STRING "${SUBROSA_DG_VERSION} - ${SHORT_SHA1} - ${GIT_CHANGE}")

# set option for develop build
option(SUBROSA_DG_DEVELOP "Build SubrosaDG in Develop" OFF)

# set option for cuda
option(SUBROSA_DG_CUDA "Build SubrosaDG with CUDA" OFF)

# set option to build different target
option(SUBROSA_DG_BUILD_ALL "Build all artifacts" OFF)
option(SUBROSA_DG_BUILD_EXAMPLES "Build SubrosaDG example cases" OFF)
option(SUBROSA_DG_BUILD_TESTS "Build SubrosaDG tests" OFF)
option(SUBROSA_DG_BUILD_DOCS "Build SubrosaDG document" OFF)

#set vcpkg feature cuda
if(SUBROSA_DG_CUDA)
    list(APPEND VCPKG_MANIFEST_FEATURES "cuda")
endif()

# set vcpkg feature tests
if(SUBROSA_DG_BUILD_TESTS OR SUBROSA_DG_BUILD_ALL)
    list(APPEND VCPKG_MANIFEST_FEATURES "tests")
endif()

# set compiler
set(CMAKE_C_COMPILER "/usr/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")

# set cuda compiler
if(SUBROSA_DG_CUDA)
    set(CMAKE_CUDA_COMPILER "/usr/local/cuda/bin/nvcc")
    set(CMAKE_CUDA_HOST_COMPILER "/usr/bin/clang")
endif()

# set project
if(SUBROSA_DG_CUDA)
    project(subrosa_dg LANGUAGES C CXX CUDA VERSION ${SUBROSA_DG_VERSION} DESCRIPTION "SubrosaDG")
else()
    project(subrosa_dg LANGUAGES C CXX VERSION ${SUBROSA_DG_VERSION} DESCRIPTION "SubrosaDG")
endif()

# message error if build in source dir
if(${PROJECT_SOURCE_DIR} STREQUAL ${PROJECT_BINARY_DIR})
    message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run \
            CMake from there.")
endif()

# message error if not build with clang
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(FATAL_ERROR "This project is build with clang.")
endif()

# set vcpkg overlay ports path
set(VCPKG_OVERLAY_PORTS "${CMAKE_CURRENT_LIST_DIR}/cmake/vcpkg-ports")

# set vcpkg overlay triplets path
set(VCPKG_OVERLAY_TRIPLETS "${CMAKE_CURRENT_LIST_DIR}/cmake/vcpkg-triplets")

# set vcpkg triplet
set(VCPKG_HOST_TRIPLET "x64-linux-dynamic")
set(VCPKG_TARGET_TRIPLET "x64-linux-dynamic")

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

# check cxx20 support
check_cxx_compiler_flag(-std=c++20 cxx_std_20_support)
if(NOT cxx_std_20_support)
    message(FATAL_ERROR "SubrosaDG requires C++20 to compile.")
endif()

# set c and cxx standard for all target
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# set cuda standard
if(SUBROSA_DG_CUDA)
    set(CMAKE_CUDA_STANDARD 20)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
    set(CMAKE_CUDA_EXTENSIONS OFF)
    set(CMAKE_CUDA_SEPARABLE_COMPILATION ON)
    set(CMAKE_CUDA_ARCHITECTURES native)
endif()

# open color compiler information
# NOTE: the output still not colorized due to ninja
# https://github.com/ninja-build/ninja/wiki/FAQ#why-does-my-program-with-colored-output-not-have-color-under-ninja
set(CMAKE_COLOR_DIAGNOSTICS ON)

# set cmake output verbose on
set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "cmake --verbose on" FORCE)

# create .gdbinit and .lldbinit file
configure_file(${PROJECT_SOURCE_DIR}/cmake/debuginit/.gdbinit.in
    ${PROJECT_SOURCE_DIR}/utils/gdb-pretty-print/.gdbinit @ONLY)
configure_file(${PROJECT_SOURCE_DIR}/cmake/debuginit/.lldbinit.in
    ${PROJECT_SOURCE_DIR}/utils/lldb-pretty-print/.lldbinit @ONLY)

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
if(SUBROSA_DG_DEVELOP)
    find_program(CLANG_TIDY NAMES clang-tidy)
    if(CLANG_TIDY)
        include(${PROJECT_SOURCE_DIR}/cmake/ClangTidy.cmake)
        message(STATUS "Enable clang-tidy in ${CLANG_TIDY}")
    else()
        message(WARNING "clang-tidy not found. Please install clang-tidy.")
    endif()
endif()

# use clang-format to format file
if(SUBROSA_DG_DEVELOP)
    find_program(CLANG_FORMAT NAMES clang-format)
    if(CLANG_FORMAT)
        include(${PROJECT_SOURCE_DIR}/cmake/ClangFormat.cmake)
        message_tool_version(clang-format "${CLANG_FORMAT}")
    else()
        message(WARNING "clang-format not found. Please install clang-format.")
    endif()
endif()

# use clangd for intellisense
find_program(CLANGD NAMES clangd)
if(CLANGD)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    message_tool_version(clangd "${CLANGD}")
else()
    message(WARNING "clangd not found. Please install clangd.")
endif()

# use iwyu to check header file
if(SUBROSA_DG_DEVELOP)
    find_program(IWYU NAMES include-what-you-use iwyu)
    if(IWYU)
        message_tool_version(include-what-you-use "${IWYU}")
    else()
        message(WARNING "include-what-you-use not found. Please install include-what-you-use.")
    endif()
    find_program(IWYU_TOOL NAMES iwyu_tool.py)
    find_program(IWYU_FIX_TOOL NAMES fix_includes.py)
    if(IWYU_TOOL AND IWYU_FIX_TOOL)
        include(${PROJECT_SOURCE_DIR}/cmake/Iwyu.cmake)
    endif()
endif()

if(SUBROSA_DG_DEVELOP)
    find_program(GCOVR NAMES gcovr)
    if(GCOVR)
        include(${PROJECT_SOURCE_DIR}/cmake/Gcovr.cmake)
        message_tool_version(gcovr "${GCOVR}")
    else()
        message(WARNING "gcovr not found. Please install gcovr.")
    endif()
endif()

# use ccache to speed up compilation
find_program(CCACHE NAMES ccache)
if(CCACHE)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE}")
    message_tool_version(ccache "${CCACHE}")
else()
    message(WARNING "ccache not found. Please install ccache.")
endif()

# use clang lld to link
find_program(LLD NAMES ld.lld)
if(LLD)
    set(CMAKE_LINKER "${LLD}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
else()
    message(FATAL_ERROR "lld not found. Please install lld.")
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
set(SUBROSA_DG_COMPILE_OPTIONS)
set(SUBROSA_DG_COMPILE_DEFINITIONS)

# set library and add compile option
if(SUBROSA_DG_CUDA)
    add_library(${PROJECT_NAME} SHARED ${SUBROSA_DG_CUDA_SOURCES})
else()
    add_library(${PROJECT_NAME} INTERFACE)
endif()
add_library(lib::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# check for interprocedural optimization
if(NOT SUBROSA_DG_DEVELOP)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT IPO_SUPPORT)
    if(IPO_SUPPORT)
        message(STATUS "Enable interprocedural optimization")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    endif()
endif()

# set compilation option
set(SUBROSA_DG_WARNING_FLAGS)
set(SUBROSA_DG_COVER_FLAGS)
set(SUBROSA_DG_DEBUG_FLAGS)
set(SUBROSA_DG_RELWITHDEBINFO_FLAGS)
set(SUBROSA_DG_RELEASE_FLAGS)

# add coverage compilation option
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")

# add cxx compilation option
string(APPEND SUBROSA_DG_WARNING_FLAGS "-Wall -Wshadow -Wextra -Wconversion -Wdocumentation -Wno-narrowing")
# string(APPEND SUBROSA_DG_COVER_FLAGS "-fprofile-arcs -ftest-coverage")
string(APPEND SUBROSA_DG_DEBUG_FLAGS "${SUBROSA_DG_WARNING_FLAGS} ${SUBROSA_DG_COVER_FLAGS}")
string(APPEND SUBROSA_DG_RELWITHDEBINFO_FLAGS "$<$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},x86_64>:-march=native> \
    $<$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},arm64>:-mcpu=native>")
string(APPEND SUBROSA_DG_RELEASE_FLAGS "$<$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},x86_64>:-march=native> \
    $<$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},arm64>:-mcpu=native>")

# set different compile option
set(SUBROSA_DG_FLAGS "$<$<CONFIG:Debug>:${SUBROSA_DG_DEBUG_FLAGS}> \
    $<$<CONFIG:RelWithDebInfo>:${SUBROSA_DG_RELWITHDEBINFO_FLAGS}> \
    $<$<CONFIG:Release>:${SUBROSA_DG_RELEASE_FLAGS}>")
string(REPLACE " " ";" SUBROSA_DG_FLAGS ${SUBROSA_DG_FLAGS})

# create and include the cmake header
configure_file(${PROJECT_SOURCE_DIR}/cmake/source/Cmake.hpp.in ${CMAKE_INCLUDE_OUTPUT_DIRECTORY}/Cmake.hpp @ONLY)
list(APPEND SUBROSA_DG_INCLUDE_DIRS ${CMAKE_INCLUDE_OUTPUT_DIRECTORY})

# create and include the SubrosaDG header
configure_file(${PROJECT_SOURCE_DIR}/cmake/source/SubrosaDG.in ${SUBROSA_DG_SOURCES_DIR}/SubrosaDG @ONLY)

# add include directory and compiler flags
list(APPEND SUBROSA_DG_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/src)
list(APPEND SUBROSA_DG_COMPILE_OPTIONS ${SUBROSA_DG_FLAGS})

# find tqdm-cpp
find_path(TQDM_DIR NAMES tqdm.hpp PATHS ${PROJECT_SOURCE_DIR}/libs/tqdm-cpp)
list(APPEND SUBROSA_DG_INCLUDE_SYSTEM_DIRS ${TQDM_DIR})

#find vtu11
find_path(VTU11_DIR NAMES vtu11-cpp17.hpp PATHS ${PROJECT_SOURCE_DIR}/libs/vtu11)
list(APPEND SUBROSA_DG_INCLUDE_SYSTEM_DIRS ${VTU11_DIR})

# find dbg-macro
find_path(DBG_MACRO_DIR NAMES dbg.h)
list(APPEND SUBROSA_DG_INCLUDE_SYSTEM_DIRS ${DBG_MACRO_DIR})

# find fmt
find_package(fmt CONFIG REQUIRED)
list(APPEND SUBROSA_DG_LINK_LIBRARIES fmt::fmt-header-only)

# find magic_enum
find_package(magic_enum CONFIG REQUIRED)
list(APPEND SUBROSA_DG_LINK_LIBRARIES magic_enum::magic_enum)

# find zlib
find_package(ZLIB REQUIRED)
list(APPEND SUBROSA_DG_LINK_LIBRARIES ZLIB::ZLIB)

# find eigen3
find_package(Eigen3 CONFIG REQUIRED)
list(APPEND SUBROSA_DG_LINK_LIBRARIES Eigen3::Eigen)

# find gmsh
find_program(GMSH NAMES gmsh)
find_path(GMSH_DIR NAMES gmsh.h)
find_library(GMSH_LIBRARY NAMES libgmsh${CMAKE_SHARED_LIBRARY_SUFFIX})
message_tool_version(Gmsh "${GMSH}")
list(APPEND SUBROSA_DG_INCLUDE_SYSTEM_DIRS ${GMSH_DIR})
list(APPEND SUBROSA_DG_LINK_LIBRARIES ${GMSH_LIBRARY})

# set for openmp
find_package(OpenMP REQUIRED)
list(APPEND SUBROSA_DG_LINK_LIBRARIES OpenMP::OpenMP_CXX)

# set for cuda-api-wrappers
if(SUBROSA_DG_CUDA)
    find_package(cuda-api-wrappers CONFIG REQUIRED)
    list(APPEND SUBROSA_DG_LINK_LIBRARIES cuda-api-wrappers::runtime-and-driver)
endif()

# set for nvidia-cutlass
if(SUBROSA_DG_CUDA)
    find_package(NvidiaCutlass CONFIG REQUIRED)
    list(APPEND SUBROSA_DG_LINK_LIBRARIES nvidia::cutlass::cutlass)
endif()

# set compilation definitions
foreach(SUBROSA_DG_OPTION IN ITEMS
        SUBROSA_DG_DEVELOP
        SUBROSA_DG_CUDA)
    if(${SUBROSA_DG_OPTION})
        list(APPEND SUBROSA_DG_COMPILE_DEFINITIONS ${SUBROSA_DG_OPTION})
        message(STATUS "Option: ${SUBROSA_DG_OPTION}: ON")
    else()
        message(STATUS "Option: ${SUBROSA_DG_OPTION}: OFF")
    endif()
endforeach()

# set library compiler property
if(SUBROSA_DG_CUDA)
    set(PROJECT_PROPERTY PUBLIC)
else()
    set(PROJECT_PROPERTY INTERFACE)
endif()

# set library compiler option
target_include_directories(${PROJECT_NAME} ${PROJECT_PROPERTY} ${SUBROSA_DG_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} SYSTEM ${PROJECT_PROPERTY} ${SUBROSA_DG_INCLUDE_SYSTEM_DIRS})
target_link_libraries(${PROJECT_NAME} ${PROJECT_PROPERTY} ${SUBROSA_DG_LINK_LIBRARIES})
target_compile_options(${PROJECT_NAME} ${PROJECT_PROPERTY} ${SUBROSA_DG_COMPILE_OPTIONS})
target_compile_definitions(${PROJECT_NAME} ${PROJECT_PROPERTY} ${SUBROSA_DG_COMPILE_DEFINITIONS})

# TODO: use cmake install and XXXXConfig.cmake to link

# modify the compile definitions to string for doxygen
string(REPLACE ";" " " SUBROSA_DG_COMPILE_DEFINITIONS "${SUBROSA_DG_COMPILE_DEFINITIONS}")

# add develop target
if(SUBROSA_DG_DEVELOP)
    add_executable(develop ${SUBROSA_DG_DEVELOP_SOURCES})
    target_link_libraries(develop lib::subrosa_dg)
endif()

# all_ci = all_examples + all_tests
add_custom_target(all_ci COMMENT "Build all examples and tests")

# build examples
if(SUBROSA_DG_BUILD_EXAMPLES OR SUBROSA_DG_BUILD_ALL)
    add_subdirectory(examples)
    message(STATUS "Generating examples")
endif()

# build tests
if(SUBROSA_DG_BUILD_TESTS OR SUBROSA_DG_BUILD_ALL)
    enable_testing()
    add_subdirectory(tests)
    message(STATUS "Generating tests")
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
    find_package(LATEX COMPONENTS PDFLATEX BIBTEX)
    if(LATEX_PDFLATEX_FOUND)
        add_subdirectory(docs)
        message_tool_version(PdfLaTeX "${PDFLATEX_COMPILER}")
        message_tool_version(BibTeX "${BIBTEX_COMPILER}")
    else()
        message(WARNING "PdfLaTeX or BibTexnot found. Please install texlive.")
    endif()
endif()
