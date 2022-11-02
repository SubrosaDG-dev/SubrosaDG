#[[
# @file CMakeLists.txt
# @brief The main cmake file for SubrosaDG Project.
#
# @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
# @date 2022-11-01
#
# @version 0.0.1
# @copyright Copyright (c) 2022 - 2022 by SubrosaDG developers
#]]

cmake_minimum_required(VERSION 3.22)

# add cmake module path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# set project
set(SUBROSA_DG_VERSION_STRING "0.0.1")
project(subrosa_dg LANGUAGES C CXX VERSION ${SUBROSA_DG_VERSION_STRING} DESCRIPTION "SubrosaDG")
message(STATUS "Build subrosa_dg: ${SUBROSA_DG_VERSION_STRING}")

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

# set CMAKE_TOOLCHAIN_FILE in CamkeLists.txt
# set(CMAKE_CROSSCOMPILING True)
# set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "CMake toolchain file")

# set C and CXX standard
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# set compilation option for develop
if (DEVELOP_COMPILER)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose Release or Debug" FORCE)
else()
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose Release or Debug" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# use clang-tidy to check file
if (DEVELOP_COMPILER)
    find_program(CLANG_TIDY "clang-tidy")
    if (CLANG_TIDY)
        set(CMAKE_C_CLANG_TIDY ${CLANG_TIDY})
        set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY})
        message(STATUS "Enable clang-tidy in ${CLANG_TIDY}")
    endif()
endif()

# use clang-format to check file
if (DEVELOP_COMPILER)
    find_program(CLANG_FORMAT "clang-format")
    if (CLANG_FORMAT)
        include(cmake/clang-format.cmake)
        message(STATUS "Enable clang-format in ${CLANG_FORMAT}")
    endif()
endif()

# use clangd for intellisense
find_program(CLANGD "clangd")
if (CLANGD)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    message(STATUS "Enable clangd in ${CLANGD}")
endif()

# use ccache to speed up compilation
find_program(CCACHE "ccache")
if (CCACHE)
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
    message(STATUS "Enable ccache in ${CCACHE}")
endif()

# set CXX compilation option
string(APPEND SUBROSA_DG_CXX_FLAGS "-mfma -Wno-narrowing")
# string(APPEND SUBROSA_DG_CXX_WARNING_FLAGS "-Wall -Wshadow -Wextra -pedantic-errors")
# string(APPEND SUBROSA_DG_CXX_FLAGS_DEBUG "${SUBROSA_DG_CXX_FLAGS} ${SUBROSA_DG_CXX_WARNING_FLAGS} \
#     -O0 -g")
# string(APPEND SUBROSA_DG_CXX_FLAGS_DEBUG "${SUBROSA_DG_CXX_FLAGS} \
#     -Wshadow -Wall -Wextra -pedantic-errors -Wsign-conversion -Wconversion -O0")
string(APPEND SUBROSA_DG_CXX_FLAGS_DEBUG "${SUBROSA_DG_CXX_FLAGS} -O0")
string(APPEND SUBROSA_DG_CXX_FLAGS_RELEASE "${SUBROSA_DG_CXX_FLAGS} -O3 -DNDEBUG -march=native")

set(SUBROSA_DG_FLAGS "$<$<CONFIG:Debug>:${SUBROSA_DG_CXX_FLAGS_DEBUG}> \
    $<$<CONFIG:Release>:${SUBROSA_DG_CXX_FLAGS_RELEASE}>")

string(REPLACE " " ";" SUBROSA_DG_FLAGS ${SUBROSA_DG_FLAGS})

# enable mutilthread support
set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(CMAKE_USE_PTHREADS_INIT 1)
set(THREADS_PREFER_PTHREAD_FLAG ON)

# set output path
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/libs)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/libs)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# set library and add compile option
aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/src subrosa_dg_src)
add_library(${PROJECT_NAME} SHARED ${subrosa_dg_src})
add_library(lib::subrosa_dg ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_compile_options(${PROJECT_NAME} PUBLIC ${SUBROSA_DG_FLAGS})

# find progressbar
find_path(PROGRESSBAR_DIR progressbar.hpp PATHS ${CMAKE_CURRENT_LIST_DIR}/libs/ProgressBar)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROGRESSBAR_DIR})

# find libconfig
find_path(LIBCONFIG_DIR libconfig.h++)
find_library(LIBCONFIG_LIBRARY NAMES libconfig)
target_include_directories(${PROJECT_NAME} PUBLIC ${LIBCONFIG_DIR})
target_link_libraries(${PROJECT_NAME} ${LIBCONFIG_LIBRARY})

# find fmt
find_package(fmt CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} fmt::fmt)

# find spdlog
find_package(spdlog CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} spdlog::spdlog spdlog::spdlog_header_only)

# find eigen3
find_package(Eigen3 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} Eigen3::Eigen)

# find hdf5
if (SUBROSA_DG_WITH_MPI)
    set(HDF5_PREFER_PARALLEL TRUE)
endif ()
set(HDF5_USE_STATIC_LIBRARIES ON)
include(FindHDF5)
find_package(HDF5 COMPONENTS C REQUIRED)
target_link_libraries(${PROJECT_NAME} hdf5::hdf5)
if (SUBROSA_DG_WITH_MPI)
    target_compile_definitions(${PROJECT_NAME} PUBLIC H5_HAVE_PARALLEL)
endif ()

# find cgns and enable hdf5
find_path(CGNS_DIR cgnslib.h)
find_library(CGNS_LIBRARY NAMES cgns)
target_include_directories(${PROJECT_NAME} PUBLIC ${CGNS_DIR})
target_link_libraries(${PROJECT_NAME} ${CGNS_LIBRARY})

# set for openmp
if (SUBROSA_DG_WITH_OPENMP)
    set(OpenMP_C "${CMAKE_C_COMPILER}")
    set(OpenMP_C_FLAGS "-fopenmp=libomp")
    set(OpenMP_C_LIB_NAMES "omp")
    set(OpenMP_omp_LIBRARY ${OpenMP_C_LIB_NAMES})
    set(OpenMP_CXX "${CMAKE_CXX_COMPILER}")
    set(OpenMP_CXX_FLAGS "-fopenmp=libomp")
    set(OpenMP_CXX_LIB_NAMES "omp")
    set(OpenMP_omp_LIBRARY ${OpenMP_CXX_LIB_NAMES})
    find_package(OpenMP REQUIRED)
    target_link_libraries(${PROJECT_NAME} OpenMP::OpenMP_C OpenMP::OpenMP_CXX)
endif()

# set for mpi
if (SUBROSA_DG_WITH_MPI)
    # TODO: link openmpi
endif()

# set compilation definitions
foreach (SUBROSA_DG_OPTION
        SUBROSA_DG_SINGLE_PRECISION
        SUBROSA_DG_WITH_OPENMP
        SUBROSA_DG_WITH_MPI)
    if (${SUBROSA_DG_OPTION})
        target_compile_definitions(${PROJECT_NAME} PUBLIC ${SUBROSA_DG_OPTION})
    endif ()
endforeach ()

# TODO: use install and XXXXConfig.cmake to link

# set(config_targets_file "${PROJECT_NAME}Config.cmake")
# set(export_dest_dir "${CMAKE_INSTALL_LIBDIR}/cmake")
# install(
#         TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}
#         LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#         ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
#         RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
# install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
# install(TARGETS ${PROJECT_NAME} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/libs")
# install(EXPORT ${PROJECT_NAME} DESTINATION ${export_dest_dir} NAMESPACE SUBROSA_DG:: FILE ${config_targets_file})

add_custom_target(AllCI)

# build examples
if (SUBROSA_DG_BUILD_EXAMPLES OR SUBROSA_DG_BUILD_ALL)
    add_subdirectory(examples)
    message(STATUS "Generating examples")
endif()

# build tests
if (SUBROSA_DG_BUILD_TESTS OR SUBROSA_DG_BUILD_ALL)
    enable_testing()
    add_subdirectory(tests)
    message(STATUS "Generating tests")
endif()

# build document
if (SUBROSA_DG_BUILD_DOCS OR SUBROSA_DG_BUILD_ALL)
    find_package(Doxygen REQUIRED)
    if (DOXYGEN_FOUND)
        include(cmake/Doxygen.cmake)
        message(STATUS "Generating document")
    else()
        message(FATAL_ERROR "Doxygen not found. Please install doxygen.")
    endif()
endif()
