/**
 * @file CommandLine.cpp
 * @brief The header file of SubrosaDG command line output.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-12-06
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_COMMAND_LINE_CPP_
#define SUBROSA_DG_COMMAND_LINE_CPP_

#include <gmsh.h>

#include <Eigen/Core>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <deque>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <sycl/sycl.hpp>
#include <tqdm.hpp>
#include <type_traits>

#include "Cmake.cpp"
#include "Solver/TimeIntegration.cpp"
#include "Utils/BasicDataType.cpp"
#include "Utils/Concept.cpp"
#include "Utils/Constant.cpp"
#include "Utils/Enum.cpp"

namespace SubrosaDG {

inline constexpr std::string_view kAnsiReset{"\033[0m"};
inline constexpr std::string_view kAnsiRed{"\033[31m"};
inline constexpr std::string_view kAnsiBlue{"\033[34m"};
inline constexpr std::string_view kAnsiMagenta{"\033[35m"};

template <typename SimulationControl>
struct CommandLine {
  using Clock = std::chrono::steady_clock;

  static constexpr int kHistoryLineNumber = 10;
  static constexpr std::chrono::seconds kProgressInterval{5};

  struct ResidualRecord {
    Real time_;
    Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> residual_;
  };

  CommandLineEnum command_line_type_;
  Real delta_time_{0.0_r};

  std::deque<ResidualRecord> residual_history_;
  Tqdm::ProgressBar solver_progress_bar_;
  Tqdm::ProgressBar view_progress_bar_;

  Clock::time_point run_start_{Clock::now()};
  Clock::time_point phase_start_{run_start_};
  Clock::time_point last_progress_{run_start_};
  int iteration_end_{0};
  int residual_step_{-1};
  int snapshot_total_{0};
  int snapshot_completed_{0};
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> latest_residual_{
      Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>::Zero()};
  std::filesystem::path output_directory_;
  std::filesystem::path mesh_file_path_;

  [[nodiscard]] static constexpr std::string_view getVariableName(const int index) {
    constexpr std::array<std::string_view, 4> kNames{"rho", "rho*u", "rho*v", "rho*w"};
    if (index <= SimulationControl::kDimension) {
      return kNames[static_cast<Usize>(index)];
    }
    if constexpr (IsCompressible<SimulationControl::kEquationModel>) {
      return "rho*E";
    } else if constexpr (IsIncompressible<SimulationControl::kEquationModel>) {
      return "rho*e";
    }
  }

  [[nodiscard]] static std::string getResidualHeader() {
    std::string line = std::format("|{:^13}|", "Time");
    for (int i = 0; i < SimulationControl::kConservedVariableNumber; i++) {
      line += std::format("{:^13}|", CommandLine::getVariableName(i));
    }
    return line;
  }

  [[nodiscard]] static std::string getResidualLine(
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& residual, const Real time_value) {
    std::string line = std::format("|{:^13.5e}|", time_value);
    for (int i = 0; i < SimulationControl::kConservedVariableNumber; i++) {
      line += std::format("{:^13.5e}|", residual(i));
    }
    return line;
  }

  [[nodiscard]] static nlohmann::ordered_json getResidualJson(
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& residual) {
    nlohmann::ordered_json residual_json = nlohmann::ordered_json::object();
    for (int i = 0; i < SimulationControl::kConservedVariableNumber; i++) {
      const std::string variable_name{CommandLine::getVariableName(i)};
      if (std::isfinite(residual(i))) {
        residual_json[variable_name] = residual(i);
      } else {
        residual_json[variable_name] = nullptr;
      }
    }
    return residual_json;
  }

  [[nodiscard]] static nlohmann::ordered_json getNonfiniteResidualJson(
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& residual) {
    nlohmann::ordered_json nonfinite_json = nlohmann::ordered_json::object();
    for (int i = 0; i < SimulationControl::kConservedVariableNumber; i++) {
      if (!std::isfinite(residual(i))) {
        std::string_view value;
        if (std::isnan(residual(i))) {
          value = "nan";
        } else if (residual(i) > 0.0_r) {
          value = "inf";
        } else {
          value = "-inf";
        }
        nonfinite_json[std::string{CommandLine::getVariableName(i)}] = value;
      }
    }
    return nonfinite_json;
  }

  [[nodiscard]] static double getElapsedSeconds(const Clock::time_point start_time) {
    return std::chrono::duration<double>(Clock::now() - start_time).count();
  }

  [[nodiscard]] bool isProgressDue() const { return Clock::now() - this->last_progress_ >= kProgressInterval; }

  void emitEvent(const std::string_view event, const nlohmann::ordered_json& payload) {
    nlohmann::ordered_json record = nlohmann::ordered_json::object({
        {"event", event},
        {"elapsed_s", CommandLine::getElapsedSeconds(this->run_start_)},
    });
    record.update(payload);
    std::cout << record.dump() << '\n' << std::flush;
    this->last_progress_ = Clock::now();
  }

  [[nodiscard]] nlohmann::ordered_json getSolverEventJson(const int step) const {
    nlohmann::ordered_json event_json = nlohmann::ordered_json::object({
        {"step", step},
        {"step_end", this->iteration_end_},
        {"time", static_cast<Real>(step) * this->delta_time_},
        {"phase_elapsed_s", CommandLine::getElapsedSeconds(this->phase_start_)},
    });
    if (this->residual_step_ < 0) {
      event_json["residual_step"] = nullptr;
      event_json["residual"] = nullptr;
    } else {
      event_json["residual_step"] = this->residual_step_;
      event_json["residual"] = CommandLine::getResidualJson(this->latest_residual_);
    }
    return event_json;
  }

  [[nodiscard]] nlohmann::ordered_json getViewEventJson() const {
    return nlohmann::ordered_json::object({
        {"snapshots_completed", this->snapshot_completed_},
        {"snapshots_total", this->snapshot_total_},
        {"phase_elapsed_s", CommandLine::getElapsedSeconds(this->phase_start_)},
    });
  }

  [[nodiscard]] std::string getSolverTable() const {
    std::string table = CommandLine::getResidualHeader() + '\n';
    for (const ResidualRecord& record : this->residual_history_) {
      table += CommandLine::getResidualLine(record.residual_, record.time_) + '\n';
    }
    return table;
  }

  static void printGmshInformation() {
    std::string gmsh_information;
    gmsh::option::getString("General.BuildInfo", gmsh_information);
    const std::regex separator(";\\s*");
    std::cout << "Gmsh Info:\n" << std::regex_replace(gmsh_information, separator, "\n") << "\n\n";
  }

  void printInformation() const {
    if (this->command_line_type_ != CommandLineEnum::Open) {
      return;
    }
    std::stringstream information;
    information << '\n';
    information << R"(#)" << R"(#######################################################)" << R"(#)" << '\n'
                << R"(#)" << kAnsiRed << R"(   ____        _                        )" << kAnsiBlue
                << R"( ____   ____   )" << kAnsiReset << R"(#)" << '\n'
                << R"(#)" << kAnsiRed << R"(  / ___| _   _| |__  _ __ ___  ___  __ _)" << kAnsiBlue
                << R"(|  _ \ / ___|  )" << kAnsiReset << R"(#)" << '\n'
                << R"(#)" << kAnsiRed << R"(  \___ \| | | | '_ \| '__/ _ \/ __|/ _` )" << kAnsiMagenta << R"(|)"
                << kAnsiBlue << R"( | | | |  _   )" << kAnsiReset << R"(#)" << '\n'
                << R"(#)" << kAnsiRed << R"(   ___) | |_| | |_) | | | (_) \__ \ (_| )" << kAnsiMagenta << R"(|)"
                << kAnsiBlue << R"( |_| | |_| |  )" << kAnsiReset << R"(#)" << '\n'
                << R"(#)" << kAnsiRed << R"(  |____/ \__,_|_.__/|_|  \___/|___/\__,_)" << kAnsiMagenta << R"(|)"
                << kAnsiBlue << R"(____/ \____|  )" << kAnsiReset << R"(#)" << '\n'
                << R"(#)" << R"(                                                       )" << R"(#)" << '\n'
                << R"(#)" << R"(#######################################################)" << R"(#)" << '\n';
    information << std::format("Version: {}", kSubrosaDGVersionString) << '\n';
    information << std::format("Build type: {}", kSubrosaDGBuildType) << '\n';
    information << std::format("Data type: {}: {}", std::is_same_v<Real, double> ? "double" : "float", kRealEpsilon)
                << '\n';
#ifndef SUBROSA_DG_GPU
    information << std::format("CPU Device: {}", kDevice.get_info<sycl::info::device::name>()) << '\n';
    information << std::format("Number of physical cores: {}", kNumberOfPhysicalCores) << '\n';
    information << std::format("Eigen SIMD Instructions: {}", Eigen::SimdInstructionSetsInUse()) << '\n';
#else   // SUBROSA_DG_GPU
    information << std::format("GPU Device: {}", kDevice.get_info<sycl::info::device::name>()) << '\n';
    information << std::format("Memory Size: {:.2f} GiB",
                               static_cast<Real>(kDevice.get_info<sycl::info::device::global_mem_size>()) /
                                   (1024.0_r * 1024.0_r * 1024.0_r))
                << '\n';
    information << std::format("Compute Capability: {}", kDevice.get_info<sycl::info::device::backend_version>())
                << '\n';
#endif  // SUBROSA_DG_GPU
    std::cout << information.str() << '\n';
  }

  void diagnostic(const std::string_view level, const std::string_view phase, const std::string_view code,
                  const std::string_view message) {
    if (this->command_line_type_ == CommandLineEnum::Agent) {
      this->emitEvent(level, nlohmann::ordered_json::object({
                                 {"phase", phase},
                                 {"code", code},
                                 {"message", message},
                             }));
    } else {
      // Fatal diagnostics remain visible even when ordinary progress is disabled.
      std::cerr << std::format("{} [{} / {}]: {}\n", level, phase, code, message) << std::flush;
    }
  }

  void initializeMesh(const std::filesystem::path& mesh_file_path) {
    if (this->command_line_type_ == CommandLineEnum::Agent) {
      this->mesh_file_path_ = mesh_file_path;
      this->phase_start_ = Clock::now();
      this->emitEvent("mesh_started", nlohmann::ordered_json::object({{"mesh_file", mesh_file_path.string()}}));
    }
  }

  void finalizeMesh(const Isize node_number, const Isize volume_element_number) {
    if (this->command_line_type_ == CommandLineEnum::Agent) {
      this->emitEvent("mesh_ready", nlohmann::ordered_json::object({
                                        {"mesh_file", this->mesh_file_path_.string()},
                                        {"dimension", SimulationControl::kDimension},
                                        {"polynomial_order", SimulationControl::kPolynomialOrder},
                                        {"nodes", node_number},
                                        {"volume_elements", volume_element_number},
                                        {"phase_elapsed_s", CommandLine::getElapsedSeconds(this->phase_start_)},
                                    }));
    }
  }

  void initializeSolver(const TimeIntegration<SimulationControl>& time_integration, std::fstream& error_finout,
                        const int error_output_interval, const std::filesystem::path& output_directory = {}) {
    this->delta_time_ = time_integration.delta_time_;
    if constexpr (SimulationControl::kInitialCondition != InitialConditionEnum::LastStep) {
      error_finout << CommandLine::getResidualHeader() << '\n'
                   << CommandLine::getResidualLine(
                          Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>::Zero(), 0.0_r)
                   << '\n';
    }
    if (this->command_line_type_ == CommandLineEnum::Open) {
      this->residual_history_.assign(
          static_cast<std::size_t>(kHistoryLineNumber),
          ResidualRecord{0.0_r, Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>::Zero()});
      this->solver_progress_bar_.restart();
      this->solver_progress_bar_ << this->getSolverTable();
      this->solver_progress_bar_.initialize(time_integration.iteration_start_, time_integration.iteration_end_,
                                            error_output_interval, kHistoryLineNumber + 2);
    } else if (this->command_line_type_ == CommandLineEnum::Agent) {
      this->phase_start_ = Clock::now();
      this->iteration_end_ = time_integration.iteration_end_;
      this->residual_step_ = -1;
      this->output_directory_ = output_directory;
      this->emitEvent("solver_started", nlohmann::ordered_json::object({
                                            {"step_start", time_integration.iteration_start_},
                                            {"step_end", time_integration.iteration_end_},
                                            {"dt", time_integration.delta_time_},
                                            {"output_directory", output_directory.string()},
                                            {"error_file", (output_directory / "error.txt").string()},
                                            {"progress_interval_s", kProgressInterval.count()},
                                        }));
    }
  }

  void updateSolver(const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& residual,
                    std::fstream& error_finout, const int step) {
    const Real time_value = static_cast<Real>(step) * this->delta_time_;
    error_finout << CommandLine::getResidualLine(residual, time_value) << '\n';
    if (this->command_line_type_ == CommandLineEnum::Open) {
      this->residual_history_.pop_front();
      this->residual_history_.push_back(ResidualRecord{time_value, residual});
      this->solver_progress_bar_ << this->getSolverTable();
      this->solver_progress_bar_.update();
    } else if (this->command_line_type_ == CommandLineEnum::Agent) {
      this->latest_residual_ = residual;
      this->residual_step_ = step;
      if (this->isProgressDue()) {
        this->emitEvent("solver_progress", this->getSolverEventJson(step));
      }
    }
  }

  void reportNonfiniteResidual(const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& residual,
                               const int step) {
    if (this->command_line_type_ == CommandLineEnum::Agent) {
      this->emitEvent("error", nlohmann::ordered_json::object({
                                   {"phase", "solver"},
                                   {"code", "nonfinite_residual"},
                                   {"message", "Non-finite residual detected"},
                                   {"step", step},
                                   {"variables", CommandLine::getNonfiniteResidualJson(residual)},
                               }));
    } else {
      this->diagnostic("error", "solver", "nonfinite_residual", std::format("Non-finite residual at step {}", step));
    }
  }

  void finalizeSolver(const int step, const bool completed) {
    if (this->command_line_type_ == CommandLineEnum::Agent) {
      nlohmann::ordered_json event_json = this->getSolverEventJson(step);
      event_json["status"] = completed ? "completed" : "failed";
      event_json["reason"] = completed ? "reached_end" : "nonfinite_residual";
      event_json["error_file"] = (this->output_directory_ / "error.txt").string();
      this->emitEvent("solver_finished", event_json);
    }
  }

  void initializeView(const int snapshot_total, const std::filesystem::path& output_directory = {}) {
    if (this->command_line_type_ == CommandLineEnum::Open) {
      std::cout << '\n';
      this->view_progress_bar_.restart();
      this->view_progress_bar_.initialize(0, snapshot_total);
    } else if (this->command_line_type_ == CommandLineEnum::Agent) {
      this->phase_start_ = Clock::now();
      this->snapshot_total_ = snapshot_total;
      this->snapshot_completed_ = 0;
      this->output_directory_ = output_directory;
      this->emitEvent("view_started", nlohmann::ordered_json::object({
                                          {"snapshots_total", snapshot_total},
                                          {"output_directory", output_directory.string()},
                                      }));
    }
  }

  void updateView() {
    if (this->command_line_type_ == CommandLineEnum::Open) {
      this->view_progress_bar_.update();
    } else if (this->command_line_type_ == CommandLineEnum::Agent) {
      this->snapshot_completed_++;
      if (this->isProgressDue()) {
        this->emitEvent("view_progress", this->getViewEventJson());
      }
    }
  }

  void finalizeView() {
    if (this->command_line_type_ == CommandLineEnum::Agent) {
      nlohmann::ordered_json event_json = this->getViewEventJson();
      event_json["status"] = "completed";
      event_json["output_directory"] = this->output_directory_.string();
      this->emitEvent("view_finished", event_json);
    }
  }

  explicit CommandLine(const CommandLineEnum command_line_type) : command_line_type_(command_line_type) {
    if (this->command_line_type_ == CommandLineEnum::Open) {
      CommandLine::printGmshInformation();
    } else if (this->command_line_type_ == CommandLineEnum::Agent) {
      gmsh::option::setNumber("General.Terminal", 0);
      gmsh::option::setNumber("General.Verbosity", 2);
      this->emitEvent("run_started", nlohmann::ordered_json::object({
                                         {"version", kSubrosaDGVersionString},
                                         {"build_type", kSubrosaDGBuildType},
                                         {"precision", std::is_same_v<Real, double> ? "double" : "float"},
                                         {"device", kDevice.get_info<sycl::info::device::name>()},
                                     }));
    } else {
      gmsh::option::setNumber("General.Terminal", 0);
    }
  }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_COMMAND_LINE_CPP_
