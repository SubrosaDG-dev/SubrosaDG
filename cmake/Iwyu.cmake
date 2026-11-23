#[[
# @file Iwyu.cmake
# @brief On-demand targets for include-what-you-use.
#
# @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
# @date 2022-04-20
#
# @version 0.1.0
# @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
# SubrosaDG is free software and is distributed under the MIT license.
#]]

set(IWYU_COMPILE_DATABASE ${CMAKE_BINARY_DIR}/iwyu-compile_commands.json)
set(IWYU_REPORT ${CMAKE_BINARY_DIR}/iwyu.txt)

add_custom_target(iwyu-search
    COMMENT "Running IWYU to check header includes"
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/utils/iwyu_compile_commands.py
    ${CMAKE_BINARY_DIR}/compile_commands.json ${IWYU_COMPILE_DATABASE}
    COMMAND ${CMAKE_COMMAND} -E env "IWYU_BINARY=${IWYU}" ${IWYU_TOOL}
    -v
    -j
    ${NUMBER_OF_PHYSICAL_CORES}
    -p
    ${IWYU_COMPILE_DATABASE}
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
    "--check_also=${SUBROSA_DG_SOURCES_DIR}/*/*.cpp"
    > ${IWYU_REPORT}
    BYPRODUCTS ${IWYU_COMPILE_DATABASE} ${IWYU_REPORT}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    VERBATIM
)

if(IWYU_FIX_TOOL)
    add_custom_target(iwyu-fix
        COMMENT "Applying IWYU include suggestions"
        COMMAND ${IWYU_FIX_TOOL} --nosafe_headers < ${IWYU_REPORT}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        VERBATIM
    )
    add_dependencies(iwyu-fix iwyu-search)
endif()
