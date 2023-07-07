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

#include <string_view>

namespace SubrosaDG {

struct View {
  const std::string_view model_name_;
  const int interval_;
  const std::string_view output_name_;

  inline consteval View(const std::string_view model_name, const int interval, const std::string_view output_name)
      : model_name_(model_name), interval_(interval), output_name_(output_name) {}
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_VIEW_HPP_
