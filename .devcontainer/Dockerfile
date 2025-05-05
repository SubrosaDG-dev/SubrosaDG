FROM fedora:41

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
dnf install -y intel-cpp-essentials && \
dnf clean all

RUN dnf install -y intel-compute-runtime oneapi-level-zero && \
dnf clean all

RUN dnf config-manager addrepo --from-repofile https://developer.download.nvidia.com/compute/cuda/repos/fedora41/x86_64/cuda-fedora41.repo && \
dnf install -y cuda-toolkit && \
dnf clean all

RUN dnf install -y rocm-smi rocm-core rocm-runtime rocm-hip rocm-device-libs && \
dnf clean all

ENV http_proxy="http://172.17.0.1:7897" \
https_proxy="http://172.17.0.1:7897" \
all_proxy="socks5://172.17.0.1:7897" \
VCPKG_FORCE_SYSTEM_BINARIES=1 \
VCPKG_ROOT="/home/user/vcpkg"

RUN curl -L "https://raw.githubusercontent.com/SubrosaDG-dev/SubrosaDG/refs/heads/master/utils/oneapi-gpu/oneapi-nvidia.sh" && \
chmod +x oneapi-nvidia.sh && \
./oneapi-nvidia.sh -y && \
curl -L "https://raw.githubusercontent.com/SubrosaDG-dev/SubrosaDG/refs/heads/master/utils/oneapi-gpu/oneapi-amd.sh" && \
chmod +x oneapi-amd.sh && \
./oneapi-amd.sh -y && \
rm oneapi-nvidia.sh oneapi-amd.sh

USER user

RUN git clone --depth 1 https://github.com/microsoft/vcpkg /home/user/vcpkg && \
/home/user/vcpkg/bootstrap-vcpkg.sh && \
/home/user/vcpkg/vcpkg install dbg-macro fmt magic-enum zlib
