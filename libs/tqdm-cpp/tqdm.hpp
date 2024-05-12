#pragma once

/*
 *Copyright (c) 2018-2019 <Miguel Raggi> <mraggi@gmail.com>
 *
 *Permission is hereby granted, free of charge, to any person
 *obtaining a copy of this software and associated documentation
 *files (the "Software"), to deal in the Software without
 *restriction, including without limitation the rights to use,
 *copy, modify, merge, publish, distribute, sublicense, and/or sell
 *copies of the Software, and to permit persons to whom the
 *Software is furnished to do so, subject to the following
 *conditions:
 *
 *The above copyright notice and this permission notice shall be
 *included in all copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 *OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 *HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *OTHER DEALINGS IN THE SOFTWARE.
 */

#include <chrono>
#include <cmath>
#include <format>
#include <iostream>
#include <sstream>

// -------------------- chrono stuff --------------------

namespace Tqdm {

using index = std::size_t;
using time_point_t = std::chrono::time_point<std::chrono::steady_clock>;
using seconds = std::chrono::duration<double>;

class Chronometer {
 public:
  Chronometer() : start_(std::chrono::steady_clock::now()) {}

  double reset() {
    auto previous = start_;
    start_ = std::chrono::steady_clock::now();

    return std::chrono::duration_cast<seconds>(start_ - previous).count();
  }

  [[nodiscard]] double peek() const {
    auto now = std::chrono::steady_clock::now();

    return std::chrono::duration_cast<seconds>(now - start_).count();
  }

  [[nodiscard]] time_point_t get_start() const { return start_; }

 private:
  time_point_t start_;
};

// -------------------- ProgressBar --------------------

class ProgressBar {
 public:
  void initialize(int cycle_start, int cycle_end, int delete_line) {
    progress_ = static_cast<index>(cycle_start);
    cycle_start_ = static_cast<index>(cycle_start);
    cycle_end_ = static_cast<index>(cycle_end);
    num_order_ = log10(cycle_end_) + 1;
    delete_line_ = delete_line;

    for (int i = 0; i < delete_line_; i++) {
      (*os_) << '\n';
    }
  }

  ~ProgressBar() {}

  void restart() {
    chronometer_.reset();
    refresh_.reset();
  }

  void update() {
    double proc = static_cast<double>(++progress_ - cycle_start_) / static_cast<double>(cycle_end_ - cycle_start_);

    if (time_since_refresh() > min_time_per_update_ || proc == 0 || proc == 1) {
      reset_refresh_timer();
      display(proc);
    }
    suffix_.str("");
  }

  void set_ostream(std::ostream& os) { os_ = &os; }
  void set_bar_size(index size) { bar_size_ = size; }
  void set_min_update_time(double time) { min_time_per_update_ = time; }

  template <class T>
  ProgressBar& operator<<(const T& t) {
    suffix_ << t;
    return *this;
  }

  double elapsed_time() const { return chronometer_.peek(); }

 private:
  void display(double proc) {
    auto flags = os_->flags();

    double t = chronometer_.peek();
    double eta = t / proc - t;

    std::chrono::hh_mm_ss t_ss{seconds(t)};
    std::chrono::hh_mm_ss eta_ss{seconds(eta)};

    std::stringstream bar;

    bar << "\e[2K"
        << "\e[" << delete_line_ << 'A';

    bar << std::format("Step: {:>{}} ", progress_, num_order_) << std::format("{{{:5.1f}%}} ", 100 * proc);

    print_bar(bar, proc);

    // NOTE: the format of std::chrono::hh_mm_ss is not implemented in libc++ yet P2372R3
    // bar << std::format("({:%T} < {:%T}) ", t_ss, eta_ss);

    bar << std::format("({:02}:{:02}:{:02} < {:02}:{:02}:{:02})", t_ss.hours().count(), t_ss.minutes().count(),
                       t_ss.seconds().count(), eta_ss.hours().count(), eta_ss.minutes().count(),
                       eta_ss.seconds().count());

    std::string sbar = bar.str();
    std::string suffix = suffix_.str();

    (*os_) << sbar << '\n' << suffix << std::flush;

    os_->flags(flags);
  }

  void print_bar(std::stringstream& ss, double filled) const {
    auto num_filled = static_cast<index>(std::round(filled * static_cast<double>(bar_size_)));
    ss << std::format("[{:>{}}{:>{}}] ", std::string(num_filled, '#'), num_filled, "", bar_size_ - num_filled);
  }

  double time_since_refresh() const { return refresh_.peek(); }
  void reset_refresh_timer() { refresh_.reset(); }

  Chronometer chronometer_{};
  Chronometer refresh_{};

  index progress_;
  int delete_line_;
  index cycle_start_;
  index cycle_end_;
  int num_order_;

  double min_time_per_update_{0.10};  // found experimentally

  std::ostream* os_{&std::cerr};

  index bar_size_{60};

  std::stringstream suffix_{};
};

}  // namespace Tqdm
