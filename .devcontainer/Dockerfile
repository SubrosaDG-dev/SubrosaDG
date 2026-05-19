FROM fedora:43

RUN groupadd --gid 1000 user && \
useradd -s /bin/bash --uid 1000 --gid 1000 -m user && \
echo user ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/user && \
chmod 0440 /etc/sudoers.d/user

RUN dnf update -y && \
dnf install -y dnf-plugins-core git wget tar zip cmake vcpkg ccache ninja-build patchelf hwloc && \
dnf clean all

RUN echo -e "[oneAPI]\nname=Intel® oneAPI repository\nbaseurl=https://yum.repos.intel.com/oneapi\nenabled=1\n\
gpgcheck=1\nrepo_gpgcheck=1\ngpgkey=https://yum.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB" | \
tee /etc/yum.repos.d/oneAPI.repo && \
dnf install -y intel-cpp-essentials && \
dnf clean all

RUN dnf install -y intel-compute-runtime oneapi-level-zero && \
dnf clean all

RUN dnf config-manager addrepo --from-repofile https://developer.download.nvidia.com/compute/cuda/repos/fedora41/x86_64/cuda-fedora41.repo && \
dnf install -y cuda-toolkit && \
dnf clean all

RUN dnf install -y rocm && \
dnf clean all

ENV http_proxy="http://172.17.0.1:7890" \
https_proxy="http://172.17.0.1:7890" \
all_proxy="socks5://172.17.0.1:7890" \
VCPKG_FORCE_SYSTEM_BINARIES=1 \
VCPKG_ROOT="/home/user/vcpkg"

# Deprecated: oneAPI DPC++ for NVIDIA and AMD GPUs installation via Codeplay
# RUN curl -L "https://developer.codeplay.com/api/v1/products/download?product=oneapi&variant=nvidia&filters[]=linux" -o "oneapi-nvidia.sh" && \
# chmod +x ./oneapi-nvidia.sh && \
# ./oneapi-nvidia.sh -y && \
# curl -L "https://developer.codeplay.com/api/v1/products/download?product=oneapi&variant=amd&filters[]=linux" -o "oneapi-amd.sh" && \
# chmod +x ./oneapi-amd.sh && \
# ./oneapi-amd.sh -y && \
# rm ./oneapi-nvidia.sh ./oneapi-amd.sh && \

RUN git clone https://github.com/intel/llvm.git /tmp/llvm -b sycl-rel-7_0 --depth=1 && \
cd /tmp/llvm && \
cmake -S unified-runtime -B build_ur \
    -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ \
    -DUR_BUILD_TESTS=OFF \
    -DUR_ENABLE_TRACING=ON \
    -DUR_BUILD_ADAPTER_CUDA=ON \
    -DUR_BUILD_ADAPTER_HIP=ON \
    -DUR_HIP_INCLUDE_DIR=/usr/include/hip \
    -DUR_HIP_HSA_INCLUDE_DIR=/usr/include/hsa \
    -DUR_HIP_LIB_DIR=/usr/lib64 \
    -DUR_BUILD_ADAPTER_OPENCL=ON \
    -DCMAKE_BUILD_TYPE=Release && \
cmake --build build_ur -j$(nproc) && \
cp -r build_ur/lib/* /opt/intel/oneapi/compiler/latest/lib/ && \
rm -rf /tmp/llvm

USER user

RUN git clone --depth 1 https://github.com/microsoft/vcpkg /home/user/vcpkg && \
/home/user/vcpkg/bootstrap-vcpkg.sh && \
/home/user/vcpkg/vcpkg install dbg-macro fmt magic-enum zlib
