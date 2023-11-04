#[[
# @file CMakeLists.txt
# @brief The main cmake file for SubrosaDG Project.
#
# @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
# @date 2022-11-01
#
# @version 0.1.0
# @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
# SubrosaDG is free software and is distributed under the MIT license.
#]]

cmake_minimum_required(VERSION 3.25)

# add cmake module path
include(${CMAKE_CURRENT_LIST_DIR}/cmake/GlobFiles.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/Utils.cmake)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# get git revision
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
string(SUBSTRING ${GIT_SHA1} 0 8 SHORT_SHA1)
git_local_changes(GIT_CHANGE)

# update submodules
# find_package(Git REQUIRED)
# if(NOT EXISTS ${CMAKE_BINARY_DIR}/.identify)
#   execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init
#     WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
#     COMMAND_ERROR_IS_FATAL ANY)
#   file(WRITE ${CMAKE_BINARY_DIR}/.identify "SubrosaDG")
# endif()

# define color
# TODO: vscode output do not support ansi color until now
define_colors()

# extract version from version.h and set version string
subrosa_dg_extract_version()
set(SUBROSA_DG_VERSION_STRING "${SUBROSA_DG_VERSION} - ${SHORT_SHA1} - ${GIT_CHANGE}")

# set option for develop build
option(SUBROSA_DG_DEVELOP "Build SubrosaDG in Develop" OFF)

# set option to build different target
option(SUBROSA_DG_BUILD_ALL "Build all artifacts" OFF)
option(SUBROSA_DG_BUILD_EXAMPLES "Build SubrosaDG example cases" OFF)
option(SUBROSA_DG_BUILD_TESTS "Build SubrosaDG tests" OFF)
option(SUBROSA_DG_BUILD_DOCS "Build SubrosaDG document" OFF)

# set option for floating point precision
option(SUBROSA_DG_SINGLE_PRECISION "Build SubrosaDG with float" OFF)

# set option for different compilation
option(SUBROSA_DG_WITH_OPENMP "Build SubrosaDG with OpenMP support" ON)
option(SUBROSA_DG_WITH_MPI "Build SubrosaDG with MPI support" OFF)

# set vcpkg feature tests
if(SUBROSA_DG_BUILD_TESTS OR SUBROSA_DG_BUILD_ALL)
    list(APPEND VCPKG_MANIFEST_FEATURES "tests")
endif()

# set vcpkg feature mpi
if(SUBROSA_DG_WITH_MPI)
    list(APPEND VCPKG_MANIFEST_FEATURES "mpi")
endif()

# set project
project(subrosa_dg LANGUAGES C CXX VERSION ${SUBROSA_DG_VERSION} DESCRIPTION "SubrosaDG")

# message error if build in source dir
if(${PROJECT_SOURCE_DIR} STREQUAL ${PROJECT_BINARY_DIR})
    message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run \
            CMake from there.")
endif()

# message error if not build with clang
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(FATAL_ERROR "This project is build with clang.")
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(COMPILER_PATH /usr/bin)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(COMPILER_PATH /opt/homebrew/opt/llvm/bin)
endif ()

# add vcpkg cmake
include($ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)

# message build version
message(STATUS "Build SubrosaDG: ${SUBROSA_DG_VERSION_STRING}")

# set compilation option for develop
if(SUBROSA_DG_DEVELOP)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose Release or Debug" FORCE)
else()
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose Release or Debug" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# query the system cores number
cmake_host_system_information(RESULT NUMBER_OF_PHYSICAL_CORES QUERY NUMBER_OF_PHYSICAL_CORES)
message(STATUS "Number of physical cores: ${NUMBER_OF_PHYSICAL_CORES}")

# check cxx20 support
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-std=c++20 cxx_std_20_support)
if (NOT cxx_std_20_support)
    message(FATAL_ERROR "SubrosaDG requires C++20 to compile.")
endif ()

# set c and cxx standard for all target
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# open color compiler information
# NOTE: the output still not colorized due to ninja
# https://github.com/ninja-build/ninja/wiki/FAQ#why-does-my-program-with-colored-output-not-have-color-under-ninja
set(CMAKE_COLOR_DIAGNOSTICS ON)

# set cmake output verbose on
if(SUBROSA_DG_DEVELOP)
    set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "cmake --verbose on" FORCE)
endif()

# create .gdbinit file
configure_file(${PROJECT_SOURCE_DIR}/cmake/.gdbinit.in ${PROJECT_SOURCE_DIR}/utils/gdb-eigen-pretty-print/.gdbinit @ONLY)

# create .lldbinit file
configure_file(${PROJECT_SOURCE_DIR}/cmake/.lldbinit.in ${PROJECT_SOURCE_DIR}/utils/lldb-eigen-pretty-print/.lldbinit @ONLY)

# create vscode launch.json and settings.json
if(SUBROSA_DG_DEVELOP)
    file(MAKE_DIRECTORY ${PROJECT_SOURCE_DIR}/.vscode)
    configure_file(${PROJECT_SOURCE_DIR}/cmake/launch.json.in ${PROJECT_SOURCE_DIR}/.vscode/launch.json @ONLY)
    configure_file(${PROJECT_SOURCE_DIR}/cmake/settings.json.in ${PROJECT_SOURCE_DIR}/.vscode/settings.json @ONLY)
endif()

# use clang lld to link
find_program(LLD NAMES ld.lld PATHS ${COMPILER_PATH})
if(LLD)
    set(CMAKE_LINKER "${LLD}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
    message_tool_version(lld "${LLD}")
endif()

# use clang-tidy to check file
if(SUBROSA_DG_DEVELOP)
    find_program(CLANG_TIDY NAMES clang-tidy PATHS ${COMPILER_PATH})
    if(CLANG_TIDY)
        # set(CMAKE_C_CLANG_TIDY "${CLANG_TIDY}")
        # set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY}")
        include(${PROJECT_SOURCE_DIR}/cmake/ClangTidy.cmake)
        message(STATUS "Enable clang-tidy in ${CLANG_TIDY}")
    endif()
endif()

# use clang-format to format file
if(SUBROSA_DG_DEVELOP)
    find_program(CLANG_FORMAT NAMES clang-format PATHS ${COMPILER_PATH})
    if(CLANG_FORMAT)
        include(${PROJECT_SOURCE_DIR}/cmake/ClangFormat.cmake)
        message_tool_version(clang-format "${CLANG_FORMAT}")
    endif()
endif()

# use clangd for intellisense
find_program(CLANGD NAMES clangd PATHS ${COMPILER_PATH} NO_DEFAULT_PATH)
if(CLANGD)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    message_tool_version(clangd "${CLANGD}")
endif()

# use iwyu to check header file
if(SUBROSA_DG_DEVELOP)
    find_program(IWYU NAMES include-what-you-use iwyu PATHS ${COMPILER_PATH})
    if(IWYU)
        # set(CMAKE_C_INCLUDE_WHAT_YOU_USE "${IWYU}")
        # set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "${IWYU}")
        message_tool_version(include-what-you-use "${IWYU}")
    endif()
    find_program(IWYU_TOOL NAMES iwyu_tool.py HINTS ${PROJECT_SOURCE_DIR}/utils/include-what-you-use)
    find_program(IWYU_FIX_TOOL NAMES fix_includes.py HINTS ${PROJECT_SOURCE_DIR}/utils/include-what-you-use)
    if(IWYU_TOOL AND IWYU_FIX_TOOL)
        include(${PROJECT_SOURCE_DIR}/cmake/Iwyu.cmake)
        # list(APPEND CMAKE_CXX_INCLUDE_WHAT_YOU_USE "-Xiwyu"
        #     "--mapping_file=${CMAKE_CURRENT_LIST_DIR}/.iwyu.imp")
    endif()
endif()

if(SUBROSA_DG_DEVELOP)
    find_program(GCOVR NAMES gcovr)
    if(GCOVR)
        include(${PROJECT_SOURCE_DIR}/cmake/Gcovr.cmake)
        message_tool_version(gcovr "${GCOVR}")
    endif()
endif()

# use ccache to speed up compilation
find_program(CCACHE NAMES ccache)
if(CCACHE)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE}")
    message_tool_version(ccache "${CCACHE}")
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
# set(SUBROSA_DG_COMPILE_FEATURES)
set(SUBROSA_DG_COMPILE_DEFINITIONS)

# set library and add compile option
add_library(${PROJECT_NAME} INTERFACE)
add_library(lib::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# set c and cxx standard
# list(APPEND SUBROSA_DG_COMPILE_FEATURES c_std_17 cxx_std_20)
# set_target_properties(
#     ${PROJECT_NAME} PROPERTIES
#     C_EXTENSIONS OFF
#     C_STANDARD_REQUIRED ON
# )
# set_target_properties(
#     ${PROJECT_NAME} PROPERTIES
#     CXX_EXTENSIONS OFF
#     CXX_STANDARD_REQUIRED ON
# )

# check for interprocedural optimization
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT IPO_SUPPORT)
    if(IPO_SUPPORT)
        message(STATUS "Enable interprocedural optimization")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
#       set_target_properties(${PROJECT_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endif()

# set cxx compilation option
set(SUBROSA_DG_CXX_FLAGS)
set(SUBROSA_DG_CXX_WARNING_FLAGS)
set(SUBROSA_DG_CXX_COVER_FLAGS)
set(SUBROSA_DG_CXX_FLAGS_DEBUG)
set(SUBROSA_DG_CXX_FLAGS_RELEASE)

# link to libc++ if available
# find_library(LIBCXX NAMES libc++.so)
# if(LIBCXX)
#     string(APPEND SUBROSA_DG_CXX_FLAGS "-stdlib=libc++ ")
# #   the following two lines choose one of them
#     set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
#     set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-stdlib=libc++")
# endif()

# add coverage compilation option
# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")

# add cxx compilation option
string(APPEND SUBROSA_DG_CXX_FLAGS "-Wno-narrowing")
string(APPEND SUBROSA_DG_CXX_WARNING_FLAGS "-Wall -Wshadow -Wextra -Wconversion -Wdocumentation")
# string(APPEND SUBROSA_DG_CXX_COVER_FLAGS "-fprofile-arcs -ftest-coverage")
string(APPEND SUBROSA_DG_CXX_FLAGS_DEBUG "${SUBROSA_DG_CXX_FLAGS} ${SUBROSA_DG_CXX_WARNING_FLAGS} \
    ${SUBROSA_DG_CXX_COVER_FLAGS} -O0 -g3")
string(APPEND SUBROSA_DG_CXX_FLAGS_RELEASE "${SUBROSA_DG_CXX_FLAGS} -O3 -DNDEBUG \
    $<$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},x86_64>:-march=native>")

# set different compile option
set(SUBROSA_DG_FLAGS "$<$<CONFIG:Debug>:${SUBROSA_DG_CXX_FLAGS_DEBUG}> \
    $<$<CONFIG:Release>:${SUBROSA_DG_CXX_FLAGS_RELEASE}>")
string(REPLACE " " ";" SUBROSA_DG_FLAGS ${SUBROSA_DG_FLAGS})

# create and include the cmake header
configure_file(${PROJECT_SOURCE_DIR}/cmake/cmake.hpp.in ${CMAKE_INCLUDE_OUTPUT_DIRECTORY}/cmake.hpp @ONLY)
list(APPEND SUBROSA_DG_INCLUDE_DIRS ${CMAKE_INCLUDE_OUTPUT_DIRECTORY})

# create and include the SubrosaDG header
configure_file(${PROJECT_SOURCE_DIR}/cmake/SubrosaDG.in ${SUBROSA_DG_SOURCES_DIR}/SubrosaDG @ONLY)

# add include directory and compiler flags
list(APPEND SUBROSA_DG_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/src)
list(APPEND SUBROSA_DG_COMPILE_OPTIONS ${SUBROSA_DG_FLAGS})

# enable mutilthread support
set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(CMAKE_USE_PTHREADS_INIT 1)
set(THREADS_PREFER_PTHREAD_FLAG ON)

# find tqdm-cpp
find_path(TQDM_DIR NAMES tqdm.hpp PATHS ${PROJECT_SOURCE_DIR}/libs/tqdm-cpp)
list(APPEND SUBROSA_DG_INCLUDE_SYSTEM_DIRS ${TQDM_DIR})

# find dbg-macro
if(SUBROSA_DG_DEVELOP)
    find_path(DBG_MACRO_DIR NAMES dbg.h)
    list(APPEND SUBROSA_DG_INCLUDE_SYSTEM_DIRS ${DBG_MACRO_DIR})
endif()

# find fmt
find_package(fmt CONFIG REQUIRED)
list(APPEND SUBROSA_DG_LINK_LIBRARIES fmt::fmt)

# find spdlog
find_package(spdlog CONFIG REQUIRED)
list(APPEND SUBROSA_DG_LINK_LIBRARIES spdlog::spdlog)

# find eigen3
find_package(Eigen3 CONFIG REQUIRED)
list(APPEND SUBROSA_DG_LINK_LIBRARIES Eigen3::Eigen)

# find gmsh
find_program(GMSH NAMES gmsh PATHS $ENV{GMSH_ROOT}/bin)
find_path(GMSH_DIR NAMES gmsh.h PATHS $ENV{GMSH_ROOT}/include)
find_library(GMSH_LIBRARY NAMES libgmsh${CMAKE_SHARED_LIBRARY_SUFFIX} PATHS $ENV{GMSH_ROOT}/lib)
if (GMSH AND GMSH_DIR AND GMSH_LIBRARY)
    message_tool_version(Gmsh "${GMSH}")
else()
    message(FATAL_ERROR "Some component of Gmsh not found.")
endif()
list(APPEND SUBROSA_DG_INCLUDE_SYSTEM_DIRS ${GMSH_DIR})
list(APPEND SUBROSA_DG_LINK_LIBRARIES ${GMSH_LIBRARY})

# set for openmp
if(SUBROSA_DG_WITH_OPENMP)
    find_package(OpenMP REQUIRED)
    list(APPEND SUBROSA_DG_LINK_LIBRARIES OpenMP::OpenMP_C OpenMP::OpenMP_CXX)
endif()

# set for mpi
if(SUBROSA_DG_WITH_MPI)
    find_package(MPI REQUIRED)
    list(APPEND SUBROSA_DG_LINK_LIBRARIES MPI::MPI_C MPI::MPI_CXX)
endif()

# set compilation definitions
foreach(SUBROSA_DG_OPTION IN ITEMS
        SUBROSA_DG_DEVELOP
        SUBROSA_DG_SINGLE_PRECISION
        SUBROSA_DG_WITH_OPENMP
        SUBROSA_DG_WITH_MPI)
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
target_compile_options(${PROJECT_NAME} INTERFACE ${SUBROSA_DG_COMPILE_OPTIONS})
# target_compile_features(${PROJECT_NAME} INTERFACE ${SUBROSA_DG_COMPILE_FEATURES})
target_compile_definitions(${PROJECT_NAME} INTERFACE ${SUBROSA_DG_COMPILE_DEFINITIONS})

# TODO: use cmake install and XXXXConfig.cmake to link

# modify the compile definitions to string for doxygen
string(REPLACE ";" " " SUBROSA_DG_COMPILE_DEFINITIONS "${SUBROSA_DG_COMPILE_DEFINITIONS}")

# all_ci = all_examples + all_tests
add_custom_target(all_ci
    COMMENT "Build all examples and tests"
)

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
if(SUBROSA_DG_BUILD_DOCS OR SUBROSA_DG_BUILD_ALL)
    find_package(Doxygen REQUIRED dot)
    if(DOXYGEN_FOUND)
        include(cmake/Doxygen.cmake)
        message(STATUS "Found Doxygen: ${DOXYGEN_VERSION}")
        message(STATUS "Generating documents")
    else()
        message(FATAL_ERROR "Doxygen not found. Please install doxygen.")
    endif()
    find_package(LATEX COMPONENTS PDFLATEX BIBTEX)
    if(LATEX_PDFLATEX_FOUND)
        add_subdirectory(docs)
        message_tool_version(PdfLaTeX "${PDFLATEX_COMPILER}")
        message_tool_version(BibTeX "${BIBTEX_COMPILER}")
    else()
        message(FATAL_ERROR "PdfLaTeX or BibTexnot found. Please install texlive.")
    endif()
endif()
