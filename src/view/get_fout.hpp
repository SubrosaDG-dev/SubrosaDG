/**
 * @file get_fout.hpp
 * @brief The head file to get fout.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-08-10
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_GET_FOUT_HPP_
#define SUBROSA_DG_GET_FOUT_HPP_

#include <fmt/core.h>

#include <filesystem>
#include <fstream>
#include <string_view>

#include "basic/constant.hpp"
#include "basic/enum.hpp"
#include "config/view_config.hpp"

namespace SubrosaDG {

inline void makeDir(const ViewConfig& view_config) {
  std::filesystem::path dir;
  switch (view_config.type_) {
  case ViewType::Dat:
    dir = view_config.dir_ / "dat";
    break;
  case ViewType::Plt:
    dir = view_config.dir_ / "plt";
    break;
  }
  if (std::filesystem::exists(dir)) {
    std::filesystem::remove_all(dir);
  }
  std::filesystem::create_directory(dir);
}

inline void getFout(const int step, const ViewConfig& view_config, std::ofstream& fout) {
  switch (view_config.type_) {
  case ViewType::Dat:
    fout.open(view_config.dir_ / fmt::format("dat/{}_{}.dat", view_config.name_prefix_, step));
    break;
  case ViewType::Plt:
    fout.open(view_config.dir_ / fmt::format("plt/{}_{}.plt", view_config.name_prefix_, step));
    break;
  }
}

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_GET_FOUT_HPP_
