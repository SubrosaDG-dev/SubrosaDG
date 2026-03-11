/**
 * @file BasicDataType.cpp
 * @brief The head file of SubrosaDG basic data type.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2022-11-02
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_BASIC_DATA_TYPE_CPP_
#define SUBROSA_DG_BASIC_DATA_TYPE_CPP_

#include <dbg-macro/dbg.h>

#include <array>
#include <cstddef>
#include <functional>
#include <iostream>  // IWYU pragma: keep

namespace SubrosaDG {

using Usize = unsigned int;
using Isize = int;

#ifdef SUBROSA_DG_SINGLE_PRECISION
using Real = float;
#else
using Real = double;
#endif

inline namespace Literals {

consteval Real operator""_r(long double x) { return static_cast<Real>(x); }

}  // namespace Literals

}  // namespace SubrosaDG

// NOLINTBEGIN(readability-identifier-naming)

template <typename T, std::size_t N>
struct unordered_array : std::array<T, N> {};

template <typename T, std::size_t N>
struct std::hash<unordered_array<T, N>> {
  std::size_t operator()(const unordered_array<T, N>& arr) const {
    unordered_array<T, N> sorted_arr = arr;
    std::ranges::sort(sorted_arr);
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
    std::ranges::sort(sorted_arr1);
    std::ranges::sort(sorted_arr2);
    return sorted_arr1 == sorted_arr2;
  }
};

// NOLINTEND(readability-identifier-naming)

#endif  // SUBROSA_DG_BASIC_DATA_TYPE_CPP_
