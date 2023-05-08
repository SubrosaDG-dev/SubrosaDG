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

#include <fmt/core.h>      // for format
#include <toml++/toml.h>   // for table
#include <magic_enum.hpp>  // for enum_cast
#include <filesystem>      // for path
#include <optional>        // for optional
#include <stdexcept>       // for out_of_range
#include <string_view>     // for basic_string_view, string_view

// clang-format on

namespace SubrosaDG::Internal {

struct Config;

template <typename T>
T getValueFromToml(const toml::table& config_table, const std::string_view& key) {
  std::optional<T> optional_value = config_table.at_path(key).value<T>();
  if (optional_value.has_value()) {
    return optional_value.value();
  }
  throw std::out_of_range(fmt::format("Error: {} is not found in config file.", key));
}

template <typename T>
T castStringToEnum(const std::string_view& enum_string) {
  std::optional<T> enum_value = magic_enum::enum_cast<T>(enum_string);
  if (enum_value.has_value()) {
    return enum_value.value();
  }
  throw std::out_of_range(fmt::format("Error: {} does not have a valid value in config file.", enum_string));
}

void readConfig(const std::filesystem::path& config_file, Config& config);

}  // namespace SubrosaDG::Internal

#endif  // SUBROSA_DG_READ_CONFIG_H_
