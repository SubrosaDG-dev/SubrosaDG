/**
 * @file BasicDataType.cpp
 * @brief The head file of SubrosaDG basic data type.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2022-11-02
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_BASIC_DATA_TYPE_CPP_
#define SUBROSA_DG_BASIC_DATA_TYPE_CPP_

#include <dbg-macro/dbg.h>

#include <array>
#include <cstddef>
#include <functional>
#include <iostream>
#include <sycl/sycl.hpp>
#include <unordered_map>
#include <vector>

namespace SubrosaDG {

#if defined(SUBROSA_DG_ONEAPI) || defined(SUBROSA_DG_CUDA) || defined(SUBROSA_DG_HIP)
#define SUBROSA_DG_GPU
#endif  // SUBROSA_DG_ONEAPI || SUBROSA_DG_CUDA || SUBROSA_DG_HIP

using Usize = std::size_t;
using Isize = std::ptrdiff_t;

#ifdef SUBROSA_DG_SINGLE_PRECISION
using Real = float;
#else
using Real = double;
#endif

#ifndef SUBROSA_DG_GPU
const sycl::device kDevice = sycl::device(sycl::cpu_selector_v);
#else   // SUBROSA_DG_GPU
const sycl::device kDevice = sycl::device(sycl::gpu_selector_v);
#endif  // SUBROSA_DG_GPU

inline namespace Literals {

inline constexpr Real operator""_r(long double x) { return static_cast<Real>(x); }

}  // namespace Literals

}  // namespace SubrosaDG

// NOLINTBEGIN

template <typename T, std::size_t... sizes>
constexpr auto concatenate(const std::array<T, sizes>&... arrays) {
  std::array<T, (sizes + ...)> result;
  std::size_t index{};
  ((std::copy_n(arrays.begin(), sizes, result.begin() + index), index += sizes), ...);
  return result;
}

template <typename T, std::size_t N>
struct unordered_array : std::array<T, N> {};

template <typename T>
class ordered_set {
 private:
  std::vector<T> vec_;
  std::unordered_map<T, std::size_t> map_;

 public:
  typename std::vector<T>::iterator begin() { return vec_.begin(); }
  typename std::vector<T>::iterator end() { return vec_.end(); }

  typename std::vector<T>::const_iterator begin() const { return vec_.cbegin(); }
  typename std::vector<T>::const_iterator end() const { return vec_.cend(); }

  void emplace_back(const T& value) {
    if (!map_.contains(value)) {
      vec_.emplace_back(value);
      map_[value] = vec_.size() - 1;
    }
  }

  std::size_t size() const { return vec_.size(); }

  std::size_t find_index(const T& value) const {
    auto iter = map_.find(value);
    if (iter != map_.end()) {
      return iter->second;
    }
    return vec_.size();
  }

  T& operator[](std::size_t index) { return vec_[index]; }

  const T& operator[](std::size_t index) const { return vec_[index]; }
};

template <typename T, std::size_t N>
struct std::hash<unordered_array<T, N>> {
  std::size_t operator()(const unordered_array<T, N>& arr) const {
    unordered_array<T, N> sorted_arr = arr;
    std::sort(sorted_arr.begin(), sorted_arr.end());
    std::size_t hash_value = 0;
    for (const auto& element : sorted_arr) {
      hash_value ^= std::hash<T>()(element) + 0x9e3779b9 + (hash_value << 6) + (hash_value >> 2);
    }
    return hash_value;
  }
};

template <typename T, std::size_t N>
struct std::equal_to<unordered_array<T, N>> {
  bool operator()(const unordered_array<T, N>& arr1, const unordered_array<T, N>& arr2) const {
    unordered_array<T, N> sorted_arr1 = arr1;
    unordered_array<T, N> sorted_arr2 = arr2;
    std::sort(sorted_arr1.begin(), sorted_arr1.end());
    std::sort(sorted_arr2.begin(), sorted_arr2.end());
    return sorted_arr1 == sorted_arr2;
  }
};

// NOLINTEND

#endif  // SUBROSA_DG_BASIC_DATA_TYPE_CPP_
