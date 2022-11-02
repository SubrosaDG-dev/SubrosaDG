#[[
# @file CMakeLists.txt
# @brief The cmake file for SubrosaDG example naca0012.
#
# @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
# @date 2022-11-02
#
# @version 0.0.1
# @copyright Copyright (c) 2022 - 2022 by SubrosaDG developers
#]]

cmake_minimum_required(VERSION 3.22)

project(subrosa_dg_example_naca0012)

add_example(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/naca0012.cpp)
