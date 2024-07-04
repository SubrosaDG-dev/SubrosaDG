#include <cuda_runtime.h>
#include <cutlass/cutlass.h>
#include <cutlass/gemm/device/gemm_batched.h>

#include <Eigen/Core>
#include <cuda/api.hpp>

#include "Utils/CudaGemm.cuh"

cuda::device_t device = cuda::device::current::get();

cutlass::gemm::device::GemmBatched<real, cutlass::layout::ColumnMajor, real, cutlass::layout::ColumnMajor, real,
                                   cutlass::layout::ColumnMajor>
    gemm;

Matrix::Matrix() {
  Eigen::Matrix<real, kN, Eigen::Dynamic> h_a(kN, kBatchSize * kN);
  Eigen::Matrix<real, kN, kN> h_b;

  h_a.setRandom();
  h_b.setRandom();

  cuda::memory::copy(d_a_, h_a.data(), kBatchSize * kN * kN * sizeof(real));
  cuda::memory::copy(d_b_, h_b.data(), kN * kN * sizeof(real));
}

float cudaComputation(Matrix& matrix) {
  std::pair<cuda::event_t, cuda::event_t> events = std::make_pair(device.create_event(), device.create_event());
  cuda::stream_t stream = device.default_stream();

  stream.enqueue.event(events.first);
  gemm({{kN, kN, kN},
        {matrix.d_a_.data(), kN},
        kN * kN,
        {matrix.d_b_.data(), kN},
        0,
        {matrix.d_c_.data(), kN},
        kN * kN,
        {matrix.d_c_.data(), kN},
        kN * kN,
        {1.0, 0.0},
        kBatchSize});
  stream.enqueue.event(events.second);
  stream.synchronize();

  return cuda::event::time_elapsed_between(events).count();
}
