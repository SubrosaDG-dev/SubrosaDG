/**
 * @file CommandLine.cpp
 * @brief The header file of SubrosaDG command line output.
 *
 * @author Yufei.Liu, Calm.Liu#outlook.com | Chenyu.Bao, bcynuaa#163.com
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
#include <chrono>
#include <cmath>
#include <concepts>
#include <deque>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <tqdm.hpp>
#include <type_traits>
#include <vector>

#include "Cmake.cpp"
#include "Solver/TimeIntegration.cpp"
#include "Utils/BasicDataType.cpp"
#include "Utils/Constant.cpp"
#include "Utils/Enum.cpp"

namespace SubrosaDG {

// clang-format off

#define RESET       "\033[0m"
#define BLACK       "\033[30m"             /* Black */
#define RED         "\033[31m"             /* Red */
#define GREEN       "\033[32m"             /* Green */
#define YELLOW      "\033[33m"             /* Yellow */
#define BLUE        "\033[34m"             /* Blue */
#define MAGENTA     "\033[35m"             /* Magenta */
#define CYAN        "\033[36m"             /* Cyan */
#define WHITE       "\033[37m"             /* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */

// clang-format on

struct CommandLineJson {
  std::string fields_;

  static std::string quote(const std::string_view value) {
    std::string result{"\""};
    for (const char character : value) {
      switch (character) {
      case '"':
        result += "\\\"";
        break;
      case '\\':
        result += "\\\\";
        break;
      case '\n':
        result += "\\n";
        break;
      case '\r':
        result += "\\r";
        break;
      case '\t':
        result += "\\t";
        break;
      default:
        if (static_cast<unsigned char>(character) < 0x20) {
          result += std::format("\\u{:04x}", static_cast<unsigned char>(character));
        } else {
          result += static_cast<char>(character);
        }
      }
    }
    return result + '"';
  }

  CommandLineJson& addEncoded(const std::string_view key, const std::string_view value) {
    if (!this->fields_.empty()) {
      this->fields_ += ',';
    }
    this->fields_ += quote(key) + ':' + std::string(value);
    return *this;
  }

  CommandLineJson& add(const std::string_view key, const std::string_view value) {
    return this->addEncoded(key, quote(value));
  }

  template <std::integral Value>
  CommandLineJson& add(const std::string_view key, const Value value) {
    return this->addEncoded(key, std::format("{}", value));
  }

  template <std::floating_point Value>
  CommandLineJson& add(const std::string_view key, const Value value) {
    // std::format without the L specifier is independent of the global locale.
    return this->addEncoded(key, std::isfinite(value) ? std::format("{}", value) : "null");
  }

  CommandLineJson& add(const std::string_view key, const CommandLineJson& value) {
    return this->addEncoded(key, value.str());
  }

  CommandLineJson& addNull(const std::string_view key) { return this->addEncoded(key, "null"); }

  [[nodiscard]] std::string str() const { return '{' + this->fields_ + '}'; }
};

template <typename SimulationControl>
struct CommandLineResidual {
  static std::string_view variableName(const int index) {
    constexpr std::string_view kNames[]{"rho", "rho*u", "rho*v", "rho*w"};
    if (index <= SimulationControl::kDimension) {
      return kNames[index];
    }
    if constexpr (IsCompressible<SimulationControl::kEquationModel>) {
      return "rho*E";
    } else if constexpr (IsIncompressible<SimulationControl::kEquationModel>) {
      return "rho*e";
    }
  }

  static std::string header() {
    std::string line = std::format("|{:^13}|", "Time");
    for (int i = 0; i < SimulationControl::kConservedVariableNumber; ++i) {
      line += std::format("{:^13}|", variableName(i));
    }
    return line;
  }

  static std::string row(const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& residual,
                         const Real time) {
    std::string line = std::format("|{:^13.5e}|", time);
    for (int i = 0; i < SimulationControl::kConservedVariableNumber; ++i) {
      line += std::format("{:^13.5e}|", residual(i));
    }
    return line;
  }

  static CommandLineJson values(const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& residual) {
    CommandLineJson fields;
    for (int i = 0; i < SimulationControl::kConservedVariableNumber; ++i) {
      fields.add(variableName(i), residual(i));
    }
    return fields;
  }

  static CommandLineJson nonfiniteValues(
      const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& residual) {
    CommandLineJson fields;
    for (int i = 0; i < SimulationControl::kConservedVariableNumber; ++i) {
      if (!std::isfinite(residual(i))) {
        std::string_view value;
        if (std::isnan(residual(i))) {
          value = "nan";
        } else if (residual(i) > 0) {
          value = "inf";
        } else {
          value = "-inf";
        }
        fields.add(variableName(i), value);
      }
    }
    return fields;
  }
};

template <typename SimulationControl>
struct CommandLineTerminal {
  static constexpr int kHistoryLines = 10;

  struct Sample {
    Real time_;
    Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> residual_;
  };

  std::deque<Sample> history_;
  Tqdm::ProgressBar solver_progress_bar_;
  Tqdm::ProgressBar view_progress_bar_;

  std::string solverTable() const {
    std::string table = CommandLineResidual<SimulationControl>::header() + '\n';
    for (const auto& sample : this->history_) {
      table += CommandLineResidual<SimulationControl>::row(sample.residual_, sample.time_) + '\n';
    }
    return table;
  }

  static void printGmshInformation() {
    std::string information;
    gmsh::option::getString("General.BuildInfo", information);
    const std::regex separator(";\\s*");
    std::cout << "Gmsh Info:\n" << std::regex_replace(information, separator, "\n") << "\n\n";
  }

  void initializeSolver(const int start, const int end, const int interval) {
    this->history_.assign(kHistoryLines,
                          Sample{0.0_r, Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>::Zero()});
    this->solver_progress_bar_.restart();
    this->solver_progress_bar_ << this->solverTable();
    this->solver_progress_bar_.initialize(start, end, interval, kHistoryLines + 2);
  }

  void updateSolver(const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& residual, const Real time) {
    this->history_.pop_front();
    this->history_.push_back({time, residual});
    this->solver_progress_bar_ << this->solverTable();
    this->solver_progress_bar_.update();
  }

  void initializeView(const int count) {
    std::cout << '\n';
    this->view_progress_bar_.restart();
    this->view_progress_bar_.initialize(0, count);
  }

  void updateView() { this->view_progress_bar_.update(); }

  void printInformation() {
    std::stringstream information;
    information << "\n";
    information << R"(#)" << R"(#######################################################)" << R"(#)" << '\n'
                << R"(#)" << RED << R"(   ____        _                        )" << BLUE << R"( ____   ____   )"
                << RESET << R"(#)" << '\n'
                << R"(#)" << RED << R"(  / ___| _   _| |__  _ __ ___  ___  __ _)" << BLUE << R"(|  _ \ / ___|  )"
                << RESET << R"(#)" << '\n'
                << R"(#)" << RED << R"(  \___ \| | | | '_ \| '__/ _ \/ __|/ _` )" << MAGENTA << R"(|)" << BLUE
                << R"( | | | |  _   )" << RESET << R"(#)" << '\n'
                << R"(#)" << RED << R"(   ___) | |_| | |_) | | | (_) \__ \ (_| )" << MAGENTA << R"(|)" << BLUE
                << R"( |_| | |_| |  )" << RESET << R"(#)" << '\n'
                << R"(#)" << RED << R"(  |____/ \__,_|_.__/|_|  \___/|___/\__,_)" << MAGENTA << R"(|)" << BLUE
                << R"(____/ \____|  )" << RESET << R"(#)" << '\n'
                << R"(#)" << R"(                                                       )" << R"(#)" << '\n'
                << R"(#)" << R"(#######################################################)" << R"(#)" << '\n';
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
    information << std::format("Memory Size: {:.2f} GiB",
                               static_cast<Real>(kDevice.get_info<sycl::info::device::global_mem_size>()) /
                                   std::pow(1024.0_r, 3))
                << '\n';
    information << std::format("Compute Capability: {}", kDevice.get_info<sycl::info::device::backend_version>())
                << '\n';
#endif  // SUBROSA_DG_GPU
    std::cout << information.str() << '\n';
  }

  CommandLineTerminal() { printGmshInformation(); }
};

template <typename SimulationControl>
struct CommandLineAgent {
  using Clock = std::chrono::steady_clock;
  static constexpr std::chrono::seconds kProgressInterval{5};
  Clock::time_point run_start_{Clock::now()};
  Clock::time_point phase_start_{run_start_};
  Clock::time_point last_progress_{run_start_};
  Real delta_time_{0.0_r};
  int step_end_{0};
  int residual_step_{-1};
  int snapshots_total_{0};
  int snapshots_completed_{0};
  Eigen::Vector<Real, SimulationControl::kConservedVariableNumber> latest_residual_;
  std::filesystem::path output_directory_;
  std::filesystem::path mesh_file_;

  static double secondsSince(const Clock::time_point start) {
    return std::chrono::duration<double>(Clock::now() - start).count();
  }

  [[nodiscard]] bool progressDue() const { return Clock::now() - this->last_progress_ >= kProgressInterval; }

  void emit(const std::string_view event, CommandLineJson fields = {}) {
    fields.add("event", event).add("elapsed_s", secondsSince(this->run_start_));
    std::cout << fields.str() + '\n' << std::flush;
    this->last_progress_ = Clock::now();
  }

  [[nodiscard]] CommandLineJson solverFields(const int step) const {
    CommandLineJson fields;
    fields.add("step", step)
        .add("step_end", this->step_end_)
        .add("time", static_cast<Real>(step) * this->delta_time_)
        .add("phase_elapsed_s", secondsSince(this->phase_start_));
    if (this->residual_step_ < 0) {
      fields.addNull("residual_step").addNull("residual");
    } else {
      fields.add("residual_step", this->residual_step_)
          .add("residual", CommandLineResidual<SimulationControl>::values(this->latest_residual_));
    }
    return fields;
  }

  [[nodiscard]] CommandLineJson viewFields() const {
    return CommandLineJson{}
        .add("snapshots_completed", this->snapshots_completed_)
        .add("snapshots_total", this->snapshots_total_)
        .add("phase_elapsed_s", secondsSince(this->phase_start_));
  }

  void diagnostic(const std::string_view level, const std::string_view phase, const std::string_view code,
                  const std::string_view message) {
    this->emit(level, CommandLineJson{}.add("phase", phase).add("code", code).add("message", message));
  }

  void initializeMesh(const std::filesystem::path& mesh_file) {
    this->mesh_file_ = mesh_file;
    this->phase_start_ = Clock::now();
    this->emit("mesh_started", CommandLineJson{}.add("mesh_file", mesh_file.string()));
  }

  void finalizeMesh(const Isize nodes, const Isize volume_elements) {
    this->emit("mesh_ready", CommandLineJson{}
                                 .add("mesh_file", this->mesh_file_.string())
                                 .add("dimension", SimulationControl::kDimension)
                                 .add("polynomial_order", SimulationControl::kPolynomialOrder)
                                 .add("nodes", nodes)
                                 .add("volume_elements", volume_elements)
                                 .add("phase_elapsed_s", secondsSince(this->phase_start_)));
  }

  void initializeSolver(const int start, const int end, const Real dt, const std::filesystem::path& directory) {
    this->phase_start_ = Clock::now();
    this->step_end_ = end;
    this->delta_time_ = dt;
    this->residual_step_ = -1;
    this->output_directory_ = directory;
    this->emit("solver_started", CommandLineJson{}
                                     .add("step_start", start)
                                     .add("step_end", end)
                                     .add("dt", dt)
                                     .add("output_directory", directory.string())
                                     .add("error_file", (directory / "error.txt").string())
                                     .add("progress_interval_s", kProgressInterval.count()));
  }

  void updateSolver(const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& residual, const int step) {
    this->latest_residual_ = residual;
    this->residual_step_ = step;
    if (this->progressDue()) {
      this->emit("solver_progress", this->solverFields(step));
    }
  }

  void reportNonfiniteResidual(const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& residual,
                               const int step) {
    this->emit("error", CommandLineJson{}
                            .add("phase", "solver")
                            .add("code", "nonfinite_residual")
                            .add("message", "Non-finite residual detected")
                            .add("step", step)
                            .add("variables", CommandLineResidual<SimulationControl>::nonfiniteValues(residual)));
  }

  void finalizeSolver(const int step, const bool completed) {
    auto fields = this->solverFields(step);
    fields.add("status", completed ? "completed" : "failed")
        .add("reason", completed ? "reached_end" : "nonfinite_residual")
        .add("error_file", (this->output_directory_ / "error.txt").string());
    this->emit("solver_finished", fields);
  }

  void initializeView(const int count, const std::filesystem::path& directory) {
    this->phase_start_ = Clock::now();
    this->snapshots_total_ = count;
    this->snapshots_completed_ = 0;
    this->output_directory_ = directory;
    this->emit("view_started",
               CommandLineJson{}.add("snapshots_total", count).add("output_directory", directory.string()));
  }

  void updateView() {
    ++this->snapshots_completed_;
    if (this->progressDue()) {
      this->emit("view_progress", this->viewFields());
    }
  }

  void finalizeView() {
    auto fields = this->viewFields();
    fields.add("status", "completed").add("output_directory", this->output_directory_.string());
    this->emit("view_finished", fields);
  }

  CommandLineAgent() {
    gmsh::option::setNumber("General.Terminal", 0);
    gmsh::option::setNumber("General.Verbosity", 2);
    gmsh::logger::start();
    this->emit("run_started", CommandLineJson{}
                                  .add("version", kSubrosaDGVersionString)
                                  .add("build_type", kSubrosaDGBuildType)
                                  .add("precision", std::is_same_v<Real, double> ? "double" : "float")
                                  .add("device", kDevice.get_info<sycl::info::device::name>()));
  }

  ~CommandLineAgent() { gmsh::logger::stop(); }
};

// Own the selected renderer and the residual file format; simulation policy stays in System.
template <typename SimulationControl>
struct CommandLine {
  CommandLineEnum command_line_type_;
  Real delta_time_{0.0_r};
  std::optional<CommandLineTerminal<SimulationControl>> terminal_;
  std::optional<CommandLineAgent<SimulationControl>> agent_;

  explicit CommandLine(const CommandLineEnum type) : command_line_type_(type) {
    switch (type) {
    case CommandLineEnum::Open:
      this->terminal_.emplace();
      break;
    case CommandLineEnum::Agent:
      this->agent_.emplace();
      break;
    case CommandLineEnum::Close:
      gmsh::option::setNumber("General.Terminal", 0);
      break;
    }
  }

  void printInformation() {
    if (this->terminal_) {
      this->terminal_->printInformation();
    }
  }

  void diagnostic(const std::string_view level, const std::string_view phase, const std::string_view code,
                  const std::string_view message) {
    if (this->agent_) {
      this->agent_->diagnostic(level, phase, code, message);
    } else {
      // Fatal diagnostics remain visible even when ordinary progress is disabled.
      std::cerr << std::format("{} [{} / {}]: {}\n", level, phase, code, message) << std::flush;
    }
  }
  void initializeMesh(const std::filesystem::path& file) {
    if (this->agent_) {
      this->agent_->initializeMesh(file);
    }
  }

  void finalizeMesh(const Isize nodes, const Isize elements) {
    if (this->agent_) {
      this->agent_->finalizeMesh(nodes, elements);
    }
  }

  void initializeSolver(const TimeIntegration<SimulationControl>& time, std::fstream& error_file,
                        const int error_interval, const std::filesystem::path& directory = {}) {
    this->delta_time_ = time.delta_time_;
    if constexpr (SimulationControl::kInitialCondition != InitialConditionEnum::LastStep) {
      error_file << CommandLineResidual<SimulationControl>::header() << '\n'
                 << CommandLineResidual<SimulationControl>::row(
                        Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>::Zero(), 0.0_r)
                 << '\n';
    }
    if (this->terminal_) {
      this->terminal_->initializeSolver(time.iteration_start_, time.iteration_end_, error_interval);
    }
    if (this->agent_) {
      this->agent_->initializeSolver(time.iteration_start_, time.iteration_end_, time.delta_time_, directory);
    }
  }

  void updateSolver(const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& residual,
                    std::fstream& error_file, const int step) {
    const Real time = static_cast<Real>(step) * this->delta_time_;
    error_file << CommandLineResidual<SimulationControl>::row(residual, time) << '\n';
    if (this->terminal_) {
      this->terminal_->updateSolver(residual, time);
    }
    if (this->agent_) {
      this->agent_->updateSolver(residual, step);
    }
  }

  void reportNonfiniteResidual(const Eigen::Vector<Real, SimulationControl::kConservedVariableNumber>& residual,
                               const int step) {
    if (this->agent_) {
      this->agent_->reportNonfiniteResidual(residual, step);
    } else {
      this->diagnostic("error", "solver", "nonfinite_residual", std::format("Non-finite residual at step {}", step));
    }
  }

  void finalizeSolver(const int step, const bool completed) {
    if (this->agent_) {
      this->agent_->finalizeSolver(step, completed);
    }
  }

  void initializeView(const int count, const std::filesystem::path& directory = {}) {
    if (this->terminal_) {
      this->terminal_->initializeView(count);
    }
    if (this->agent_) {
      this->agent_->initializeView(count, directory);
    }
  }

  void updateView() {
    if (this->terminal_) {
      this->terminal_->updateView();
    }
    if (this->agent_) {
      this->agent_->updateView();
    }
  }

  void finalizeView() {
    if (this->agent_) {
      this->agent_->finalizeView();
    }
  }
};

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_COMMAND_LINE_CPP_
