/**
 * @file CommandLine.hpp
 * @brief The header file of SubrosaDG command line output.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-12-06
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_COMMAND_LINE_HPP_
#define SUBROSA_DG_COMMAND_LINE_HPP_

#include <gmsh.h>

#include <Eigen/Core>
#include <deque>
#include <format>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <string_view>
#include <tqdm.hpp>
#include <vector>

#include "Cmake.hpp"
#include "Utils/BasicDataType.hpp"

namespace SubrosaDG {

template <typename SimulationControl>
struct CommandLine {
  bool is_open_{true};
  Real time_value_{0.0};
  std::deque<Real> time_value_deque_;
  const int line_number_{10};
  Tqdm::ProgressBar solver_progress_bar_;
  Tqdm::ProgressBar view_progress_bar_;
  std::deque<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>> error_deque_;

  inline std::string getVariableList() {
    if constexpr (SimulationControl::kDimension == 1) {
      return std::format(R"(|{:^13}|{:^13}|{:^13}|{:^13}|)", "Time", "rho", "rho*u", "rho*E");
    } else if constexpr (SimulationControl::kDimension == 2) {
      return std::format(R"(|{:^13}|{:^13}|{:^13}|{:^13}|{:^13}|)", "Time", "rho", "rho*u", "rho*v", "rho*E");
    }
  }

  inline std::string getLineInformation(const Real time_value,
                                        const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& error) {
    if constexpr (SimulationControl::kDimension == 1) {
      return std::format(R"(|{:^13.5e}|{:^13.5e}|{:^13.5e}|{:^13.5e}|)", time_value, error(0), error(1), error(2));
    } else if constexpr (SimulationControl::kDimension == 2) {
      return std::format(R"(|{:^13.5e}|{:^13.5e}|{:^13.5e}|{:^13.5e}|{:^13.5e}|)", time_value, error(0), error(1),
                         error(2), error(3));
    }
  }

  inline void updateSolver(const int iteration_number, const int step, const Real delta_time,
                           const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& new_error,
                           std::fstream& error_finout) {
    this->time_value_ += delta_time;
    error_finout << this->getLineInformation(this->time_value_, new_error) << '\n';
    std::string error_string;
    error_string += this->getVariableList() + '\n';
    if (step % this->line_number_ == 0) {
      this->time_value_deque_.pop_front();
      this->error_deque_.pop_front();
      this->time_value_deque_.emplace_back(this->time_value_);
      this->error_deque_.emplace_back(new_error);
    }
    for (Usize i = 0; i < static_cast<Usize>(this->line_number_); i++) {
      error_string += this->getLineInformation(this->time_value_deque_[i], this->error_deque_[i]) + '\n';
    }
    if (this->is_open_) {
      if (step == 1) {
        std::cout << '\n';
        this->view_progress_bar_.restart();
        this->solver_progress_bar_.initialize(iteration_number, this->line_number_ + 2);
      }
      this->solver_progress_bar_ << error_string;
      this->solver_progress_bar_.update();
    }
  }

  inline void updateView(const int iteration_number, const int step) {
    if (this->is_open_) {
      if (step == 1) {
        std::cout << '\n';
        this->view_progress_bar_.restart();
        this->view_progress_bar_.initialize(iteration_number, 1);
      }
      this->view_progress_bar_.update();
    }
  }

  inline void initializeCommandLine(std::fstream& error_finout) {
    error_finout << this->getVariableList() << '\n';
    for (int i = 0; i < this->line_number_; i++) {
      this->time_value_deque_.emplace_back(0.0);
      this->error_deque_.emplace_back(Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>::Zero());
    }
  }

  inline CommandLine(const bool open_command_line) {
    this->is_open_ = open_command_line;
    if (this->is_open_) {
      std::cout << "SubrosaDG Info:" << '\n';
      std::cout << std::format("Version: {}", kSubrosaDGVersionString) << '\n';
#ifdef SUBROSA_DG_DEVELOP
      std::cout << "Build type: Debug" << '\n';
#else   // SUBROSA_DG_DEVELOP
      std::cout << "Build type: Release" << '\n';
#endif  // SUBROSA_DG_DEVELOP
#if defined(SUBROSA_DG_WITH_OPENMP) && !defined(SUBROSA_DG_DEVELOP)
      std::cout << std::format("Number of physical cores: {}", kNumberOfPhysicalCores) << '\n';
#else   // SUBROSA_DG_WITH_OPENMP && !SUBROSA_DG_DEVELOP
      std::cout << "Number of physical cores: 1" << '\n';
#endif  // SUBROSA_DG_WITH_OPENMP && !SUBROSA_DG_DEVELOP
      std::cout << "Eigen SIMD Instructions: " << Eigen::SimdInstructionSetsInUse() << "\n\n";
      std::cout << "Gmsh Info:" << '\n';
      std::string info;
      gmsh::option::getString("General.BuildInfo", info);
      std::regex re(";\\s*");
      std::vector<std::string> lines{std::sregex_token_iterator(info.begin(), info.end(), re, -1),
                                     std::sregex_token_iterator()};
      for (const auto& line : lines) {
        std::cout << line << '\n';
      };
      std::cout << '\n';
    } else {
      gmsh::option::setNumber("General.Terminal", 0);
    }
  }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_COMMAND_LINE_HPP_
