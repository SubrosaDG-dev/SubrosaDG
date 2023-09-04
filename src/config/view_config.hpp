/**
 * @file view_config.hpp
 * @brief The head file to configure the view structure.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-08-11
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_VIEW_CONFIG_HPP_
#define SUBROSA_DG_VIEW_CONFIG_HPP_

#include <filesystem>
#include <string_view>
#include <utility>

#include "basic/enum.hpp"

namespace SubrosaDG {

struct ViewConfig {
  const int write_interval_;
  const std::filesystem::path dir_;
  const std::string_view name_prefix_;
  const ViewType type_;

  inline ViewConfig(const int write_interval, std::filesystem::path dir, std::string_view name_prefix, ViewType type)
      : write_interval_(write_interval), dir_(std::move(dir)), name_prefix_(name_prefix), type_(type) {}
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_VIEW_CONFIG_HPP_
