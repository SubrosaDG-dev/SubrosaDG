/**
 * @file BasicDataType.hpp
 * @brief The head file of SubrosaDG basic data type.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2022-11-02
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2023 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_BASIC_DATA_TYPE_HPP_
#define SUBROSA_DG_BASIC_DATA_TYPE_HPP_

// IWYU pragma: begin_keep

#include <dbg.h>

#include <iostream>

// IWYU pragma: end_keep

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace SubrosaDG {

using Usize = std::size_t;
using Isize = std::ptrdiff_t;

#ifdef SUBROSA_DG_SINGLE_PRECISION
using Real = float;
#else
using Real = double;
#endif

}  // namespace SubrosaDG

// NOLINTBEGIN

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
    auto it = map_.find(value);
    if (it != map_.end()) {
      return it->second;
    }
    return static_cast<std::size_t>(-1);
  }

  T& operator[](std::size_t index) { return vec_[index]; }

  const T& operator[](std::size_t index) const { return vec_[index]; }
};

namespace std {

template <typename T, std::size_t N>
struct hash<unordered_array<T, N>> {
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
struct equal_to<unordered_array<T, N>> {
  bool operator()(const unordered_array<T, N>& arr1, const unordered_array<T, N>& arr2) const {
    unordered_array<T, N> sorted_arr1 = arr1;
    unordered_array<T, N> sorted_arr2 = arr2;
    std::sort(sorted_arr1.begin(), sorted_arr1.end());
    std::sort(sorted_arr2.begin(), sorted_arr2.end());
    return sorted_arr1 == sorted_arr2;
  }
};

}  // namespace std

// NOLINTEND

#endif  // SUBROSA_DG_BASIC_DATA_TYPE_HPP_
