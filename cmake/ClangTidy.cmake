#[[
# @file ClangTidy.cmake
# @brief An on-demand clang-tidy target for the development translation unit.
#
# @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
# @date 2023-06-05
#
# @version 0.1.0
# @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
# SubrosaDG is free software and is distributed under the MIT license.
#]]

add_custom_target(clang-tidy
    COMMENT "Running clang-tidy on the develop target"
    COMMAND ${CLANG_TIDY}
    -p ${CMAKE_BINARY_DIR}
    --use-color
    ${SUBROSA_DG_DEVELOP_SOURCES}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    VERBATIM
)
