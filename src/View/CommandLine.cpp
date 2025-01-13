/**
 * @file CommandLine.cpp
 * @brief The header file of SubrosaDG command line output.
 *
 * @author Yufei.Liu, Calm.Liu#outlook.com | Chenyu.Bao, bcynuaa#163.com
 * @date 2023-12-06
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2025 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_COMMAND_LINE_CPP_
#define SUBROSA_DG_COMMAND_LINE_CPP_

#include <gmsh.h>

#include <Eigen/Core>
#include <deque>
#include <format>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <tqdm.hpp>
#include <type_traits>
#include <vector>

#include "Cmake.cpp"
#include "Solver/TimeIntegration.cpp"
#include "Utils/BasicDataType.cpp"
#include "Utils/Constant.cpp"
#include "Utils/Enum.cpp"

namespace SubrosaDG {

template <typename SimulationControl>
struct CommandLine {
  bool is_open_;
  Real delta_time_;
  std::deque<Real> time_value_deque_;
  const int line_number_{10};
  Tqdm::ProgressBar solver_progress_bar_;
  Tqdm::ProgressBar view_progress_bar_;
  std::deque<Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>> error_deque_;

  inline std::string getVariableList() {
    if constexpr (SimulationControl::kEquationModel == EquationModelEnum::CompresibleEuler ||
                  SimulationControl::kEquationModel == EquationModelEnum::CompresibleNS) {
      if constexpr (SimulationControl::kDimension == 1) {
        return std::format(R"(|{:^13}|{:^13}|{:^13}|{:^13}|)", "Time", "rho", "rho*u", "rho*E");
      } else if constexpr (SimulationControl::kDimension == 2) {
        return std::format(R"(|{:^13}|{:^13}|{:^13}|{:^13}|{:^13}|)", "Time", "rho", "rho*u", "rho*v", "rho*E");
      } else if constexpr (SimulationControl::kDimension == 3) {
        return std::format(R"(|{:^13}|{:^13}|{:^13}|{:^13}|{:^13}|{:^13}|)", "Time", "rho", "rho*u", "rho*v", "rho*w",
                           "rho*E");
      }
    }
    if constexpr (SimulationControl::kEquationModel == EquationModelEnum::IncompresibleEuler ||
                  SimulationControl::kEquationModel == EquationModelEnum::IncompresibleNS) {
      if constexpr (SimulationControl::kDimension == 1) {
        return std::format(R"(|{:^13}|{:^13}|{:^13}|{:^13}|)", "Time", "rho", "rho*u", "rho*e");
      } else if constexpr (SimulationControl::kDimension == 2) {
        return std::format(R"(|{:^13}|{:^13}|{:^13}|{:^13}|{:^13}|)", "Time", "rho", "rho*u", "rho*v", "rho*e");
      } else if constexpr (SimulationControl::kDimension == 3) {
        return std::format(R"(|{:^13}|{:^13}|{:^13}|{:^13}|{:^13}|{:^13}|)", "Time", "rho", "rho*u", "rho*v", "rho*w",
                           "rho*e");
      }
    }
  }

  inline std::string getLineInformation(const Real time_value,
                                        const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& error) {
    if constexpr (SimulationControl::kDimension == 1) {
      return std::format(R"(|{:^13.5e}|{:^13.5e}|{:^13.5e}|{:^13.5e}|)", time_value, error(0), error(1), error(2));
    } else if constexpr (SimulationControl::kDimension == 2) {
      return std::format(R"(|{:^13.5e}|{:^13.5e}|{:^13.5e}|{:^13.5e}|{:^13.5e}|)", time_value, error(0), error(1),
                         error(2), error(3));
    } else if constexpr (SimulationControl::kDimension == 3) {
      return std::format(R"(|{:^13.5e}|{:^13.5e}|{:^13.5e}|{:^13.5e}|{:^13.5e}|{:^13.5e}|)", time_value, error(0),
                         error(1), error(2), error(3), error(4));
    }
  }

  inline void initializeSolver(const TimeIntegration<SimulationControl>& time_integration, std::fstream& error_finout) {
    this->delta_time_ = time_integration.delta_time_;
    if (this->is_open_) {
      this->solver_progress_bar_.restart();
      this->solver_progress_bar_.initialize(time_integration.iteration_start_, time_integration.iteration_end_,
                                            this->line_number_ + 2);
    }
    if constexpr (SimulationControl::kInitialCondition != InitialConditionEnum::LastStep) {
      error_finout << this->getVariableList() << '\n'
                   << this->getLineInformation(0.0_r,
                                               Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>::Zero())
                   << '\n';
    } else {
      error_finout.seekg(0, std::ios::beg);
      std::string line;
      for (int i = 0; i < time_integration.iteration_start_ + 2 && std::getline(error_finout, line); i++) {
        ;
      }
    }
  }

  inline void updateSolver(const int step,
                           const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& new_error,
                           std::fstream& error_finout) {
    Real time_value = static_cast<Real>(step) * this->delta_time_;
    error_finout << this->getLineInformation(time_value, new_error) << '\n';
    std::string error_string;
    error_string += this->getVariableList() + '\n';
    if (step % this->line_number_ == 0) {
      this->time_value_deque_.pop_front();
      this->error_deque_.pop_front();
      this->time_value_deque_.emplace_back(time_value);
      this->error_deque_.emplace_back(new_error);
    }
    for (Usize i = 0; i < static_cast<Usize>(this->line_number_); i++) {
      error_string += this->getLineInformation(this->time_value_deque_[i], this->error_deque_[i]) + '\n';
    }
    if (this->is_open_) {
      this->solver_progress_bar_ << error_string;
      this->solver_progress_bar_.update();
    }
  }

  inline void initializeView(const int iteration_number) {
    if (this->is_open_) {
      std::cout << '\n';
      this->view_progress_bar_.restart();
      this->view_progress_bar_.initialize(0, iteration_number, 1);
    }
  }

  inline void updateView() {
    if (this->is_open_) {
      this->view_progress_bar_.update();
    }
  }

  inline void printInformation() {
    if (this->is_open_) {
      std::stringstream information;
      information << "\n";
      information << R"(########################################################)" << '\n'
                  << R"(#   ____        _                         ____   ____  #)" << '\n'
                  << R"(#  / ___| _   _| |__  _ __ ___  ___  __ _|  _ \ / ___| #)" << '\n'
                  << R"(#  \___ \| | | | '_ \| '__/ _ \/ __|/ _` | | | | |  _  #)" << '\n'
                  << R"(#   ___) | |_| | |_) | | | (_) \__ \ (_| | |_| | |_| | #)" << '\n'
                  << R"(#  |____/ \__,_|_.__/|_|  \___/|___/\__,_|____/ \____| #)" << '\n'
                  << R"(#                                                      #)" << '\n'
                  << R"(########################################################)" << '\n';
      information << std::format("Version: {}", kSubrosaDGVersionString) << '\n';
      information << std::format("Build type: {}", kSubrosaDGBuildType) << '\n';
      information << std::format("Data type: {}: {}", std::is_same_v<Real, double> ? "double" : "float", kRealEpsilon)
                  << '\n';
#ifndef SUBROSA_DG_GPU
      information << std::format("CPU Device: {}", kDevice.get_info<sycl::info::device::name>()) << '\n';
      information << std::format("Number of physical cores: {}", kNumberOfPhysicalCores) << "\n";
      information << std::format("Eigen SIMD Instructions: {}", Eigen::SimdInstructionSetsInUse()) << "\n";
#else   // SUBROSA_DG_GPU
      information << std::format("GPU Device: {}", kDevice.get_info<sycl::info::device::name>()) << '\n';
      information << std::format("Compute Capability: {}", kDevice.get_info<sycl::info::device::backend_version>())
                  << '\n';
#endif  // SUBROSA_DG_GPU
      std::cout << information.str() << '\n';
    }
  }

  inline CommandLine(const bool open_command_line) {
    this->is_open_ = open_command_line;
    if (this->is_open_) {
      std::stringstream information;
      information << "Gmsh Info:" << '\n';
      std::string gmsh_info;
      gmsh::option::getString("General.BuildInfo", gmsh_info);
      std::regex re(";\\s*");
      std::vector<std::string> lines{std::sregex_token_iterator(gmsh_info.begin(), gmsh_info.end(), re, -1),
                                     std::sregex_token_iterator()};
      for (const auto& line : lines) {
        information << line << '\n';
      };
      std::cout << information.str() << '\n';
    } else {
      gmsh::option::setNumber("General.Terminal", 0);
    }
    for (int i = 0; i < this->line_number_; i++) {
      this->time_value_deque_.emplace_back(0.0_r);
      this->error_deque_.emplace_back(Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>::Zero());
    }
  }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_COMMAND_LINE_CPP_
