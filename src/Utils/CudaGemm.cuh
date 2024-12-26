
/**
 * @file CudaGemm.cuh
 * @brief The head file for SubrosaDG CudaGemm.
 *
 * @author Yufei.Liu, Calm.Liu@outlook.com | Chenyu.Bao, bcynuaa@163.com
 * @date 2023-07-04
 *
 * @version 0.1.0
 * @copyright Copyright (c) 2022 - 2024 by SubrosaDG developers. All rights reserved.
 * SubrosaDG is free software and is distributed under the MIT license.
 */

#ifndef SUBROSA_DG_CUDA_GEMM_CUH_
#define SUBROSA_DG_CUDA_GEMM_CUH_

#include <Eigen/Core>
#include <cuda/api.hpp>

// using real = float;
using real = double;

constexpr int kN = 32;
constexpr int kBatchSize = 10000;

constexpr int kThreadsPerBlock = 32;

struct Matrix {
  cuda::memory::device::unique_span<real> d_a_{cuda::memory::device::make_unique_span<real>(kBatchSize * 4 * kN)};
  cuda::memory::device::unique_span<real> d_b_{cuda::memory::device::make_unique_span<real>(kN * kN)};
  cuda::memory::device::unique_span<real> d_c_{cuda::memory::device::make_unique_span<real>(kBatchSize * 4 * kN)};

  Matrix();
};

float cudaComputation(Matrix& matrix);

#endif  // SUBROSA_DG_CUDA_GEMM_CUH_
