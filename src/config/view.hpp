/**
 * @file view.hpp
 * @brief The head file to define some views.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-07-01
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_VIEW_HPP_
#define SUBROSA_DG_VIEW_HPP_

#include <basic/enum.hpp>
#include <filesystem>
#include <string_view>
#include <utility>

namespace SubrosaDG {

struct View {
  const int step_interval_;
  const std::filesystem::path dir_;
  const std::string_view name_;
  const ViewType type_;

  inline View(const int step_interval, std::filesystem::path dir, std::string_view name, ViewType type)
      : step_interval_(step_interval), dir_(std::move(dir)), name_(name), type_(type) {}
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_VIEW_HPP_
