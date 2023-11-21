/**
 * @file BasicDataType.hpp
 * @brief The head file of SubroseDG basic data type.
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

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <memory>

namespace std {

template <typename T, std::size_t N>
struct hash<std::array<T, N>> {
  std::size_t operator()(const std::array<T, N>& arr) const {
    std::array<T, N> sorted_arr = arr;
    std::sort(sorted_arr.begin(), sorted_arr.end());
    std::size_t hash_value = 0;
    for (const auto& element : sorted_arr) {
      hash_value ^= std::hash<T>()(element) + 0x9e3779b9 + (hash_value << 6) + (hash_value >> 2);
    }
    return hash_value;
  }
};

}  // namespace std

namespace SubrosaDG {

using Usize = std::size_t;
using Isize = std::ptrdiff_t;

#ifdef SUBROSA_DG_SINGLE_PRECISION
using Real = float;
#else
using Real = double;
#endif

}  // namespace SubrosaDG

template <typename T, std::size_t N>
struct UnorderedArray : std::array<T, N> {};

namespace std {

template <typename T, std::size_t N>
struct hash<UnorderedArray<T, N>> {
  std::size_t operator()(const UnorderedArray<T, N>& arr) const {
    UnorderedArray<T, N> sorted_arr = arr;
    std::sort(sorted_arr.begin(), sorted_arr.end());
    std::size_t hash_value = 0;
    for (const auto& element : sorted_arr) {
      hash_value ^= std::hash<T>()(element) + 0x9e3779b9 + (hash_value << 6) + (hash_value >> 2);
    }
    return hash_value;
  }
};

template <typename T, std::size_t N>
struct equal_to<UnorderedArray<T, N>> {
  bool operator()(const UnorderedArray<T, N>& arr1, const UnorderedArray<T, N>& arr2) const {
    UnorderedArray<T, N> sorted_arr1 = arr1;
    UnorderedArray<T, N> sorted_arr2 = arr2;
    std::sort(sorted_arr1.begin(), sorted_arr1.end());
    std::sort(sorted_arr2.begin(), sorted_arr2.end());
    return sorted_arr1 == sorted_arr2;
  }
};

}  // namespace std

#endif  // SUBROSA_DG_BASIC_DATA_TYPE_HPP_
