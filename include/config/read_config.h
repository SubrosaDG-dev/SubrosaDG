/**
 * @file read_config.h
 * @brief The head file for reading config.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-04-27
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_READ_CONFIG_H_
#define SUBROSA_DG_READ_CONFIG_H_

// clang-format off

#include <filesystem>  // for path

// clang-format on

namespace SubrosaDG::Internal {

struct Config;

void readConfig(const std::filesystem::path& config_file, Config& config);

}  // namespace SubrosaDG::Internal

#endif  // SUBROSA_DG_READ_CONFIG_H_
