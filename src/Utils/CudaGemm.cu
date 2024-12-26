#include <cuda_runtime.h>

#include <Eigen/Core>
#include <cuda/api.hpp>

#include "Utils/CudaGemm.cuh"

__global__ void gemm(const real* d_a, const real* d_b, real* d_c) {
  int i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i < kBatchSize) {
    Eigen::Map<const Eigen::Matrix<real, 4, kN>> a(d_a + i * 4 * kN);
    Eigen::Map<const Eigen::Matrix<real, kN, kN>> b(d_b);
    Eigen::Map<Eigen::Matrix<real, 4, kN>> c(d_c + i * 4 * kN);
    a.row(0) * b.col(0);
  }
}

Matrix::Matrix() {
  Eigen::Array<Eigen::Matrix<real, 4, kN>, Eigen::Dynamic, 1> h_a(kBatchSize);
  Eigen::Matrix<real, kN, kN> h_b;

  for (int i = 0; i < kBatchSize; i++) {
    h_a[i].setRandom();
  }
  h_b.setRandom();

  cuda::memory::copy(d_a, h_a.data(), kBatchSize * 4 * kN * sizeof(real));
  cuda::memory::copy(d_b, h_b.data(), kN * kN * sizeof(real));
}

float cudaComputation(Matrix& matrix) {
  cuda::device_t device = cuda::device::current::get();

  cuda::launch_configuration_t launch_config =
      cuda::launch_config_builder().overall_size(kBatchSize).block_size(kThreadsPerBlock).build();

  std::pair<cuda::event_t, cuda::event_t> events = std::make_pair(device.create_event(), device.create_event());
  cuda::stream_t stream = device.default_stream();

  cuda::launch(cudaEigenComputation, launch_config, d_a.data(), d_b.data(), d_c.data());

  stream.enqueue.event(events.first);

  for (int i = 0; i < 5; i++) {
    cuda::launch(cudaEigenComputation, launch_config, d_a.data(), d_b.data(), d_c.data());
  }

  stream.enqueue.event(events.second);
  stream.synchronize();

  return cuda::event::time_elapsed_between(events).count();
}
