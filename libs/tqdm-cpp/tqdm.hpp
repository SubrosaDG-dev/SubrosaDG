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
#include <iomanip>
#include <iostream>
#include <sstream>

// -------------------- chrono stuff --------------------

namespace Tqdm
{

using index = std::size_t;
using time_point_t = std::chrono::time_point<std::chrono::steady_clock>;
using seconds = std::chrono::duration<double>;

class Chronometer
{
public:
    Chronometer() : start_(std::chrono::steady_clock::now()) {}

    double reset()
    {
        auto previous = start_;
        start_ = std::chrono::steady_clock::now();

        return std::chrono::duration_cast<seconds>(start_ - previous).count();
    }

    [[nodiscard]] double peek() const
    {
        auto now = std::chrono::steady_clock::now();

        return std::chrono::duration_cast<seconds>(now - start_).count();
    }

    [[nodiscard]] time_point_t get_start() const { return start_; }

private:
    time_point_t start_;
};

// -------------------- ProgressBar --------------------

class ProgressBar
{
public:
    ProgressBar(int cycle_num) : cycle_num_(static_cast<index>(cycle_num)) {}
    ProgressBar(index cycle_num) : cycle_num_(cycle_num) {}
    ProgressBar(std::ptrdiff_t cycle_num)
        : cycle_num_(static_cast<index>(cycle_num))
    {}
    ~ProgressBar() { (*os_) << std::endl; }

    void restart()
    {
        chronometer_.reset();
        refresh_.reset();
    }

    void update()
    {
        double proc = static_cast<double>(++progress_) /
          static_cast<double>(cycle_num_ - 1);

        if (time_since_refresh() > min_time_per_update_ || proc == 0 || proc == 1)
        {
            reset_refresh_timer();
            display(proc);
        }
        suffix_.str("");
    }

    void set_ostream(std::ostream& os) { os_ = &os; }
    void set_prefix(std::string s) { prefix_ = std::move(s); }
    void set_bar_size(index size) { bar_size_ = size; }
    void set_min_update_time(double time) { min_time_per_update_ = time; }

    template <class T>
    ProgressBar& operator<<(const T& t)
    {
        suffix_ << t;
        return *this;
    }

    double elapsed_time() const { return chronometer_.peek(); }

private:
    void display(double proc)
    {
        auto flags = os_->flags();

        double t = chronometer_.peek();
        double eta = t / proc - t;

        std::stringstream bar;

        bar << '\r' << prefix_ << ' ' << '{' << std::fixed
            << std::setprecision(1) << std::setw(5) << 100 * proc << "%} ";

        print_bar(bar, proc);

        bar << " (" << t << "s < " << eta << "s) ";

        std::string sbar = bar.str();
        std::string suffix = suffix_.str();

        index out_size = sbar.size() + suffix.size();
        term_cols_ = std::max(term_cols_, out_size);
        index num_blank = term_cols_ - out_size;

        (*os_) << sbar << suffix << std::string(num_blank, ' ') << std::flush;

        os_->flags(flags);
    }

    void print_bar(std::stringstream& ss, double filled) const
    {
        auto num_filled = static_cast<index>(
          std::round(filled * static_cast<double>(bar_size_)));
        ss << '[' << std::string(num_filled, '#')
           << std::string(bar_size_ - num_filled, ' ') << ']';
    }

    double time_since_refresh() const { return refresh_.peek(); }
    void reset_refresh_timer() { refresh_.reset(); }

    Chronometer chronometer_{};
    Chronometer refresh_{};

    index progress_{0};
    index cycle_num_;

    double min_time_per_update_{0.10}; // found experimentally

    std::ostream* os_{&std::cerr};

    index bar_size_{50};
    index term_cols_{1};

    std::string prefix_{};
    std::stringstream suffix_{};
};

} // namespace Tqdm
