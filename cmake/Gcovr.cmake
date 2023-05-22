#[[
# @file Gcovr.cmake
# @brief The cmake file to enable gcovr automaticly after compilation.
#
# @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
# @date 2022-05-17
#
# @version 0.1.0
# @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
# SubrosaDG is free software and is distributed under the MIT license.
#]]

add_custom_target(gcovr
    COMMENT "Running gcovr to check file coverage"
    COMMAND ${GCOVR}
    --gcov-executable
    "llvm-cov gcov"
    --filter
    src/
    --output
    build/gcovr.out
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)
