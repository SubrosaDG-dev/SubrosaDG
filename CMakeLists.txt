#[[
# @file CMakeLists.txt
# @brief The main cmake file for SubrosaDG Project.
#
# @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
# @date 2022-11-01
#
# @version 0.1.0
# @copyright Copyright (c) 2022 - 2020 by SubrosaDG developers. All rights reserved.
# SubrosaDG is free software and is distributed under the MIT license.
#]]

cmake_minimum_required(VERSION 3.25)

# add cmake module path
include(${CMAKE_CURRENT_LIST_DIR}/cmake/files.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/utils.cmake)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# get git revision
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
string(SUBSTRING ${GIT_SHA1} 0 8 SHORT_SHA1)
git_local_changes(GIT_CHANGE)

# update submodules
find_package(Git REQUIRED)
if(NOT EXISTS ${CMAKE_BINARY_DIR}/.identify)
  execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    COMMAND_ERROR_IS_FATAL ANY)
  file(WRITE ${CMAKE_BINARY_DIR}/.identify "SubrosaDG")
endif()

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

# set vcpkg feature develop
if(SUBROSA_DG_DEVELOP)
    list(APPEND VCPKG_MANIFEST_FEATURES "develop")
endif()

# query the system cores number
cmake_host_system_information(RESULT NUMBER_OF_PHYSICAL_CORES QUERY NUMBER_OF_PHYSICAL_CORES)
message(STATUS "Number of physical cores: ${NUMBER_OF_PHYSICAL_CORES}")

# set project
project(subrosa_dg LANGUAGES C CXX VERSION ${SUBROSA_DG_VERSION} DESCRIPTION "SubrosaDG")

# message error if build in source dir
if(${PROJECT_SOURCE_DIR} STREQUAL ${PROJECT_BINARY_DIR})
    message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there.")
endif()

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(FATAL_ERROR "This project is build with clang.")
endif()

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

# check cxx20 support
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-std=c++2b cxx_std_20_support)
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

# check for interprocedural optimization
# if(CMAKE_BUILD_TYPE STREQUAL "Release")
#     include(CheckIPOSupported)
#     check_ipo_supported(RESULT IPO_SUPPORT)
#     if(IPO_SUPPORT)
#         message(STATUS "Enable interprocedural optimization")
#         set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
#     endif()
# endif()

# open color compiler information
set(CMAKE_COLOR_DIAGNOSTICS ON)

# set
if(SUBROSA_DG_DEVELOP)
    set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "cmake --verbose on" FORCE)
endif()

# create .gdbinit file
configure_file(${PROJECT_SOURCE_DIR}/cmake/.gdbinit.in ${PROJECT_SOURCE_DIR}/utils/gdb-eigen-pretty-print/.gdbinit @ONLY)

if(SUBROSA_DG_DEVELOP)
    file(MAKE_DIRECTORY ${PROJECT_SOURCE_DIR}/.vscode)
    configure_file(${PROJECT_SOURCE_DIR}/cmake/launch.json.in ${PROJECT_SOURCE_DIR}/.vscode/launch.json @ONLY)
    configure_file(${PROJECT_SOURCE_DIR}/cmake/settings.json.in ${PROJECT_SOURCE_DIR}/.vscode/settings.json @ONLY)
endif()

# use clang lld to link
find_program(LLD NAMES ld.lld)
if(LLD)
    set(CMAKE_LINKER "${LLD}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
    message_tool_version(lld "${LLD}")
endif()

# use clang-tidy to check file
if(SUBROSA_DG_DEVELOP)
    find_program(CLANG_TIDY NAMES clang-tidy)
    if(CLANG_TIDY)
        set(CMAKE_C_CLANG_TIDY "${CLANG_TIDY}")
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY}")
        message(STATUS "Enable clang-tidy in ${CLANG_TIDY}")
    endif()
endif()

# use clang-format to format file
if(SUBROSA_DG_DEVELOP)
    find_program(CLANG_FORMAT NAMES clang-format)
    if(CLANG_FORMAT)
        include(${PROJECT_SOURCE_DIR}/cmake/clang-format.cmake)
        message_tool_version(clang-format "${CLANG_FORMAT}")
    endif()
endif()

# use clangd for intellisense
find_program(CLANGD NAMES clangd)
if(CLANGD)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    message_tool_version(clangd "${CLANGD}")
endif()

# use iwyu to check header file
if(SUBROSA_DG_DEVELOP)
    find_program(IWYU NAMES include-what-you-use iwyu)
    if(IWYU)
        set(CMAKE_C_INCLUDE_WHAT_YOU_USE "${IWYU}")
        set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "${IWYU}")
        message_tool_version(include-what-you-use "${IWYU}")
    endif()
    find_program(IWYU_TOOL NAMES iwyu_tool.py HINTS ${PROJECT_SOURCE_DIR}/utils/include-what-you-use)
    find_program(IWYU_FIX_TOOL NAMES fix_includes.py HINTS ${PROJECT_SOURCE_DIR}/utils/include-what-you-use)
    if(IWYU_TOOL AND IWYU_FIX_TOOL)
        include(${PROJECT_SOURCE_DIR}/cmake/iwyu.cmake)
        list(APPEND CMAKE_CXX_INCLUDE_WHAT_YOU_USE "-Xiwyu"
            "--mapping_file=${CMAKE_CURRENT_LIST_DIR}/.iwyu.imp")
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
add_library(${PROJECT_NAME} SHARED ${SUBROSA_DG_SOURCES})
add_library(lib::subrosa_dg ALIAS ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${SUBROSA_DG_LIB_VERSION}
    SOVERSION ${SUBROSA_DG_VERSION_MAJOR})

# set c and cxx standard
# list(APPEND SUBROSA_DG_COMPILE_FEATURES c_std_20 cxx_std_20)
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

# set for interprocedural optimization
# if(ipo_support)
#     set_target_properties(${PROJECT_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
# endif()

# set cxx compilation option
set(SUBROSA_DG_CXX_FLAGS)
set(SUBROSA_DG_CXX_WARNING_FLAGS)
set(SUBROSA_DG_CXX_FLAGS_DEBUG)
set(SUBROSA_DG_CXX_FLAGS_RELEASE)
set(SUBROSA_DG_CXX_FLAGS_RELWITHDEBINFO)

# link to libc++ if available
# TODO link to libc++ if gmsh is also link to libc++
# find_library(LIBCXX NAMES libc++.so libc++abi.so)
# if(LIBCXX)
#     string(APPEND SUBROSA_DG_CXX_FLAGS "-stdlib=libc++ ")
#     set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")
# endif()

# add cxx compilation option
string(APPEND SUBROSA_DG_CXX_FLAGS "-mfma -Wno-narrowing")
string(APPEND SUBROSA_DG_CXX_WARNING_FLAGS "-Wall -Wshadow -Wextra -Wconversion -Wdocumentation")
# NOTE: -glldb seems not work here
string(APPEND SUBROSA_DG_CXX_FLAGS_DEBUG "${SUBROSA_DG_CXX_FLAGS} ${SUBROSA_DG_CXX_WARNING_FLAGS} -O0 -g3")
string(APPEND SUBROSA_DG_CXX_FLAGS_RELEASE "${SUBROSA_DG_CXX_FLAGS} -O3 -DNDEBUG -march=native")

# set different compile option
set(SUBROSA_DG_FLAGS "$<$<CONFIG:Debug>:${SUBROSA_DG_CXX_FLAGS_DEBUG}> \
    $<$<CONFIG:Release>:${SUBROSA_DG_CXX_FLAGS_RELEASE}> \
    $<$<CONFIG:RelWithDebInfo>:${SUBROSA_DG_CXX_FLAGS_RELWITHDEBINFO}>")
string(REPLACE " " ";" SUBROSA_DG_FLAGS ${SUBROSA_DG_FLAGS})



# create and include the cmake header
configure_file(${PROJECT_SOURCE_DIR}/cmake/cmake.h.in ${CMAKE_INCLUDE_OUTPUT_DIRECTORY}/cmake.h @ONLY)
list(APPEND SUBROSA_DG_INCLUDE_DIRS ${CMAKE_INCLUDE_OUTPUT_DIRECTORY})

# add include directory and compiler flags
list(APPEND SUBROSA_DG_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include)
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

# find libconfig
find_path(LIBCONFIG_DIR NAMES libconfig.h++)
find_library(LIBCONFIG_LIBRARY NAMES libconfig)
list(APPEND SUBROSA_DG_INCLUDE_SYSTEM_DIRS ${LIBCONFIG_DIR})
list(APPEND SUBROSA_DG_LINK_LIBRARIES ${LIBCONFIG_LIBRARY})

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
find_path(GMSH_DIR NAMES gmsh.h)
find_library(GMSH_LIBRARY NAMES libgmsh.so)
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
target_include_directories(${PROJECT_NAME} PUBLIC ${SUBROSA_DG_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${SUBROSA_DG_INCLUDE_SYSTEM_DIRS})
target_link_libraries(${PROJECT_NAME} ${SUBROSA_DG_LINK_LIBRARIES})
target_compile_options(${PROJECT_NAME} PUBLIC ${SUBROSA_DG_COMPILE_OPTIONS})
# target_compile_features(${PROJECT_NAME} PUBLIC ${SUBROSA_DG_COMPILE_FEATURES})
target_compile_definitions(${PROJECT_NAME} PUBLIC ${SUBROSA_DG_COMPILE_DEFINITIONS})

# TODO: use cmake install and XXXXConfig.cmake to link

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
endif()
