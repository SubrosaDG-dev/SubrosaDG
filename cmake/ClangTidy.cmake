#[[
# @file ClangTidy.cmake
# @brief The cmake file to enable clang-tidy automaticly during compilation.
#
# @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
# @date 2023-06-05
#
# @version 0.1.0
# @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
# SubrosaDG is free software and is distributed under the MIT license.
#]]

set(CLANG_TIDY_SOURCE_FILES ${SUBROSA_DG_DEVELOP_SOURCES})

add_custom_target(clang-tidy
    COMMENT "Running clang-tidy to check code static analysis"
    COMMAND ${CLANG_TIDY}
    -p
    .
    --use-color
    --config-file=${PROJECT_SOURCE_DIR}/.clang-tidy
    --format-style=${PROJECT_SOURCE_DIR}/.clang-format
    ${CLANG_TIDY_SOURCE_FILES}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
