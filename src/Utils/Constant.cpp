/**
 * @file Constant.cpp
 * @brief The constants head file to define some constants.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-04-10
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CONSTANT_CPP_
#define SUBROSA_DG_CONSTANT_CPP_

#include <limits>
#include <numbers>
#include <sycl/sycl.hpp>

#include "Utils/BasicDataType.cpp"

namespace SubrosaDG {

inline constexpr Real kPi{std::numbers::pi_v<Real>};
inline constexpr Real kEuler{std::numbers::e_v<Real>};

inline constexpr std::streamsize kRealSize{sizeof(Real)};

inline constexpr Real kRealMin{std::numeric_limits<Real>::min()};
inline constexpr Real kRealMax{std::numeric_limits<Real>::max()};
inline constexpr Real kRealEpsilon{std::numeric_limits<Real>::epsilon()};
inline constexpr int kRealSignificantDigits{std::numeric_limits<Real>::digits10};

#if !defined(SUBROSA_DG_GPU) || defined(SUBROSA_DG_DEVELOP)
const sycl::device kDevice = sycl::device(sycl::cpu_selector_v);
#else   // !defined(SUBROSA_DG_GPU) || defined(SUBROSA_DG_DEVELOP)
const sycl::device kDevice = sycl::device(sycl::gpu_selector_v);
#endif  // !defined(SUBROSA_DG_GPU) || defined(SUBROSA_DG_DEVELOP)

sycl::queue queue(kDevice);

inline constexpr int kLocalSize{32};

inline int getGroupSize(const int batch_size) { return (batch_size + kLocalSize - 1) / kLocalSize; }

inline int getGlobalSize(const int batch_size) { return getGroupSize(batch_size) * kLocalSize; }

inline sycl::nd_range<1> getNdRange(const int batch_size) {
  return {sycl::range<1>(static_cast<std::size_t>(getGlobalSize(batch_size))), sycl::range<1>(kLocalSize)};
}

inline namespace Literals {

constexpr Real operator""_deg(long double x) { return static_cast<Real>(x) * kPi / 180.0_r; }

}  // namespace Literals

}  // namespace SubrosaDG

#endif  // SUBROSA_DG_CONSTANT_CPP_
