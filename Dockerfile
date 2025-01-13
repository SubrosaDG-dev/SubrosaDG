FROM fedora:39

RUN groupadd --gid 1000 user && \
useradd -s /bin/bash --uid 1000 --gid 1000 -m user && \
echo user ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/user && \
chmod 0440 /etc/sudoers.d/user

RUN dnf update -y && \
dnf install -y dnf-plugins-core git wget tar zip cmake vcpkg ccache ninja-build && \
dnf clean all

RUN echo -e "[oneAPI]\nname=Intel® oneAPI repository\nbaseurl=https://yum.repos.intel.com/oneapi\nenabled=1\n\
gpgcheck=1\nrepo_gpgcheck=1\ngpgkey=https://yum.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB" | \
tee /etc/yum.repos.d/oneAPI.repo && \
dnf install -y intel-oneapi-compiler-dpcpp-cpp intel-oneapi-tbb-devel

RUN dnf install -y intel-compute-runtime oneapi-level-zero

RUN dnf config-manager --add-repo https://developer.download.nvidia.com/compute/cuda/repos/fedora39/x86_64/cuda-fedora39.repo && \
dnf install -y cuda-runtime

RUN dnf install rocm-smi rocm-core rocm-runtime rocm-hip rocm-device-libs

USER user

ENV http_proxy="http://172.17.0.1:7890" \
https_proxy="http://172.17.0.1:7890" \
all_proxy="socks5://172.17.0.1:7890" \
VCPKG_FORCE_SYSTEM_BINARIES=1 \
VCPKG_ROOT="/home/user/vcpkg"

RUN git clone --depth 1 https://github.com/microsoft/vcpkg /home/user/vcpkg && \
/home/user/vcpkg/bootstrap-vcpkg.sh && \
/home/user/vcpkg/vcpkg install dbg-macro fmt magic-enum zlib
