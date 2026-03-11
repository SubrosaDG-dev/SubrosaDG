/**
 * @file Transformation.cpp
 * @brief The source file for SubrosaDG class Transformation.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2025-02-22
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2026 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_TRANSFORMATION_CPP_
#define SUBROSA_DG_TRANSFORMATION_CPP_

#include <oneapi/tbb.h>

#include <Eigen/Core>
#include <Utils/BasicDataType.cpp>
#include <Utils/MatrixDevice.cpp>

namespace SubrosaDG::Utils {

template <typename Scalar, int Rows, int Cols>
  requires(!Device::HasScalar<Scalar>)
inline void transferToDevice(const Eigen::Matrix<Scalar, Rows, Cols>& matrix_host,
                             Device::Matrix<Scalar, Rows, Cols>& matrix_device) {
  queue.memcpy(matrix_device.data(), matrix_host.data(), Rows * Cols * sizeof(Scalar)).wait();
}

template <typename Scalar, int Rows, int Cols>
  requires(!Device::HasScalar<Scalar>)
inline void transferToDevice(const Eigen::Array<Scalar, Eigen::Dynamic, 1>& array_host,
                             Device::Array<Scalar, Device::Dynamic, 1>& array_device) {
  const auto size = static_cast<std::size_t>(array_host.size());
  queue.memcpy(array_device.data(), array_host.data(), size * sizeof(Scalar)).wait();
}

template <typename Scalar, int Rows, int Cols>
  requires(!Device::HasScalar<Scalar>)
inline void transferToDevice(const Eigen::Array<Eigen::Matrix<Scalar, Rows, Cols>, Eigen::Dynamic, 1>& array_host,
                             Device::Array<Device::Matrix<Scalar, Rows, Cols>, Device::Dynamic, 1>& array_device) {
  const auto size = static_cast<std::size_t>(array_host.size());
  Scalar* array_span_host = sycl::malloc_host<Scalar>(size * Rows * Cols, queue);
  tbb::parallel_for(
      tbb::blocked_range<Isize>(0, static_cast<Isize>(size)), [&](const tbb::blocked_range<Isize>& range) -> void {
        for (Isize i = range.begin(); i != range.end(); i++) {
          Eigen::Map<Eigen::Matrix<Scalar, Rows, Cols>, Eigen::Unaligned, Eigen::Stride<Eigen::Dynamic, Eigen::Dynamic>>
              array_span_map(array_span_host + i, Eigen::Stride<Eigen::Dynamic, Eigen::Dynamic>(
                                                      static_cast<Isize>(size) * Rows, static_cast<Isize>(size)));
          array_span_map = array_host(i);
        }
      });
  queue.memcpy(array_device.data(), array_span_host, size * Rows * Cols * sizeof(Scalar)).wait();
  sycl::free(array_span_host, queue);
}

template <typename Scalar, int Rows, int Cols>
  requires(!Device::HasScalar<Scalar>)
inline void transferToHost(const Device::Matrix<Scalar, Rows, Cols>& matrix_device,
                           Eigen::Matrix<Scalar, Rows, Cols>& matrix_host) {
  queue.memcpy(matrix_host.data(), matrix_device.data(), Rows * Cols * sizeof(Scalar)).wait();
}

template <typename Scalar, int Rows, int Cols>
  requires(!Device::HasScalar<Scalar>)
inline void transferToHost(const Device::Array<Scalar, Device::Dynamic, 1>& array_device,
                           Eigen::Array<Scalar, Eigen::Dynamic, 1>& array_host) {
  const auto size = static_cast<std::size_t>(array_host.size());
  queue.memcpy(array_host.data(), array_device.data(), size * sizeof(Scalar)).wait();
}

template <typename Scalar, int Rows, int Cols>
  requires(!Device::HasScalar<Scalar>)
inline void transferToHost(const Device::Array<Device::Matrix<Scalar, Rows, Cols>, Device::Dynamic, 1>& array_device,
                           Eigen::Array<Eigen::Matrix<Scalar, Rows, Cols>, Eigen::Dynamic, 1>& array_host) {
  const auto size = static_cast<std::size_t>(array_host.size());
  Scalar* array_span_host = sycl::malloc_host<Scalar>(size * Rows * Cols, queue);
  queue.memcpy(array_span_host, array_device.data(), size * Rows * Cols * sizeof(Scalar)).wait();
  tbb::parallel_for(
      tbb::blocked_range<Isize>(0, static_cast<Isize>(size)), [&](const tbb::blocked_range<Isize>& range) -> void {
        for (Isize i = range.begin(); i != range.end(); i++) {
          Eigen::Map<Eigen::Matrix<Scalar, Rows, Cols>, Eigen::Unaligned, Eigen::Stride<Eigen::Dynamic, Eigen::Dynamic>>
              array_span_map(array_span_host + i, Eigen::Stride<Eigen::Dynamic, Eigen::Dynamic>(
                                                      static_cast<Isize>(size) * Rows, static_cast<Isize>(size)));
          array_host(i) = array_span_map;
        }
      });
  sycl::free(array_span_host, queue);
}

template <typename Scalar, int Rows, int Cols>
  requires(!Device::HasScalar<Scalar>)
inline void checkDeviceData(const Eigen::Matrix<Scalar, Rows, Cols>& matrix_host,
                            const Device::Matrix<Scalar, Rows, Cols>& matrix_device,
                            const std::string_view variable_name, const Real tol = 1.0e-10_r) {
  Eigen::Matrix<Scalar, Rows, Cols> matrix_copy;
  queue.memcpy(matrix_copy.data(), matrix_device.data(), Rows * Cols * sizeof(Scalar)).wait();
  if (!matrix_host.isApprox(matrix_copy, tol)) {
    std::cerr << std::format("Device matrix {} does not match host matrix within tolerance!", variable_name) << '\n';
  }
  std::cout << std::format("Device matrix {} checkDeviceData passed!", variable_name) << '\n';
}

template <typename Scalar, int Rows, int Cols>
  requires(!Device::HasScalar<Scalar>)
inline void checkDeviceData(const Eigen::Array<Scalar, Eigen::Dynamic, 1>& array_host,
                            const Device::Array<Scalar, Device::Dynamic, 1>& array_device,
                            const std::string_view variable_name, const Real tol = 1.0e-10_r) {
  const auto size = static_cast<std::size_t>(array_host.size());
  Eigen::Array<Scalar, Eigen::Dynamic, 1> array_copy(size);
  queue.memcpy(array_copy.data(), array_device.data(), size * sizeof(Scalar)).wait();
  if (!array_host.isApprox(array_copy, tol)) {
    std::cerr << std::format("Device scalar array {} does not match host scalar array within tolerance!", variable_name)
              << '\n';
  }
  std::cout << std::format("Device scalar array {} checkDeviceData passed!", variable_name) << '\n';
}

template <typename Scalar, int Rows, int Cols>
  requires(!Device::HasScalar<Scalar>)
inline void checkDeviceData(const Eigen::Array<Eigen::Matrix<Scalar, Rows, Cols>, Eigen::Dynamic, 1>& array_host,
                            const Device::Array<Device::Matrix<Scalar, Rows, Cols>, Device::Dynamic, 1>& array_device,
                            const std::string_view variable_name, const Real tol = 1.0e-10_r) {
  const auto size = static_cast<std::size_t>(array_host.size());
  Scalar* array_span_host = sycl::malloc_host<Scalar>(size * Rows * Cols, queue);
  queue.memcpy(array_span_host, array_device.data(), size * Rows * Cols * sizeof(Scalar)).wait();
  for (Isize i = 0; i < array_host.size(); i++) {
    Eigen::Map<Eigen::Matrix<Scalar, Rows, Cols>, Eigen::Unaligned, Eigen::Stride<Eigen::Dynamic, Eigen::Dynamic>>
        array_span_map(array_span_host + i,
                       Eigen::Stride<Eigen::Dynamic, Eigen::Dynamic>(array_host.size() * Rows, array_host.size()));
    if (!array_host(i).isApprox(array_span_map, tol)) {
      std::cerr << std::format("Device matrix array {} element {} does not match host array element within tolerance!",
                               variable_name, i)
                << '\n';
      break;
    }
  }
  sycl::free(array_span_host, queue);
  std::cout << std::format("Device matrix array {} checkDeviceData passed!", variable_name) << '\n';
}

}  // namespace SubrosaDG::Utils

#endif  // SUBROSA_DG_TRANSFORMATION_CPP_
