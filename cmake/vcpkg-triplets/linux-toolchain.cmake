set(CMAKE_C_COMPILER /usr/bin/clang)
set(CMAKE_CXX_COMPILER /usr/bin/clang++)
set(CMAKE_CUDA_COMPILER "/usr/local/cuda/bin/nvcc")
set(CMAKE_CUDA_FLAGS "-allow-unsupported-compiler -ccbin /usr/bin/clang")

set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld")
