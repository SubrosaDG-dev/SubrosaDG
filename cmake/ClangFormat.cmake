#[[
# @file ClangFormat.cmake
# @brief The cmake file to enable clang-format automaticly during compilation.
#
# @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
# @date 2022-11-01
#
# @version 0.1.0
# @copyright Copyright (c) 2022 - 2025 by SubrosaDG developers. All rights reserved.
# SubrosaDG is free software and is distributed under the MIT license.
#]]

set(CLANG_FORMAT_SOURCE_FILES ${SUBROSA_DG_SOURCES} ${SUBROSA_DG_EXAMPLES})

add_custom_target(clang-format
    COMMENT "Running clang-format to format file"
    COMMAND ${CLANG_FORMAT}
    -style=file
    -i
    ${CLANG_FORMAT_SOURCE_FILES}
)
