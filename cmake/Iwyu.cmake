#[[
# @file Iwyu.cmake
# @brief The cmake file to enable iwyu automaticly during compilation.
#
# @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
# @date 2022-04-20
#
# @version 0.1.0
# @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
# SubrosaDG is free software and is distributed under the MIT license.
#]]

# add iwyu-search target
add_custom_target(iwyu-search
    COMMENT "Running iwyu to check file head"
    COMMAND ${IWYU_TOOL}
    -v
    -j
    ${NUMBER_OF_PHYSICAL_CORES}
    -p
    .
    --
    -Xiwyu
    --mapping_file=${PROJECT_SOURCE_DIR}/.iwyu.imp
    -Xiwyu
    --no_fwd_decls
    -Xiwyu
    --max_line_length=120
    -Xiwyu
    --update_comments
    -Xiwyu
    --check_also=**/SubrosaDG/src/**
    > iwyu.txt
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# add iwyu-fix target
add_custom_target(iwyu-fix
    COMMENT "Running iwyu to fix file head"
    COMMAND ${IWYU_FIX_TOOL}
    --nosafe_headers
    < iwyu.txt
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# add iwyu dependency
add_dependencies(iwyu-fix iwyu-search)
