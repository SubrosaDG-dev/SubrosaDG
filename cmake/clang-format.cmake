#[[
# @file clang-format.cmake
# @brief The cmake file to enable clang-format automaticly during compilation.
#
# @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
# @date 2022-11-01
#
# @version 0.0.1
# @copyright Copyright (c) 2022 - 2022 by SubrosaDG developers
#]]

# Find all source files
file(GLOB_RECURSE ALL_SOURCE_FILES CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/src/*.[hc]
    ${PROJECT_SOURCE_DIR}/src/*.[hc]pp
    ${PROJECT_SOURCE_DIR}/include/*.h
    ${PROJECT_SOURCE_DIR}/include/*.hpp
    ${PROJECT_SOURCE_DIR}/include/**/*.h
    ${PROJECT_SOURCE_DIR}/include/**/*.hpp
    ${PROJECT_SOURCE_DIR}/examples/**/*.[hc]
    ${PROJECT_SOURCE_DIR}/examples/**/*.[hc]pp)

# get all project files file
foreach (SOURCE_FILE ${ALL_SOURCE_FILES})
    foreach (EXCLUDE_PATTERN ${CLANG_FORMAT_EXCLUDE_PATTERNS})
        string(FIND ${SOURCE_FILE} ${EXCLUDE_PATTERN} EXCLUDE_FOUND)
        if (NOT ${EXCLUDE_FOUND} EQUAL -1)
            list(REMOVE_ITEM ALL_SOURCE_FILES ${SOURCE_FILE})
        endif ()
    endforeach ()
endforeach ()

# add clang-format target
add_custom_target(clang-format
    COMMENT "Running clang-format to change files"
    COMMAND ${CLANG_FORMAT}
    -style=file
    -i
    ${ALL_SOURCE_FILES}
)
