FROM fedora:41

RUN echo -e "[oneAPI]\nname=Intel® oneAPI repository\nbaseurl=https://yum.repos.intel.com/oneapi\nenabled=1\n\
gpgcheck=1\nrepo_gpgcheck=1\ngpgkey=https://yum.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB" | \
tee /etc/yum.repos.d/oneAPI.repo && \
dnf update -y && \
dnf install -y git wget tar zip cmake vcpkg ccache ninja-build intel-oneapi-compiler-dpcpp-cpp && \
dnf clean all

ENV http_proxy="http://172.17.0.1:7890" \
https_proxy="http://172.17.0.1:7890" \
all_proxy="socks5://172.17.0.1:7890" \
VCPKG_FORCE_SYSTEM_BINARIES=1 \
VCPKG_ROOT="/home/user/vcpkg"

RUN groupadd --gid 1000 user && \
useradd -s /bin/bash --uid 1000 --gid 1000 -m user && \
echo user ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/user && \
chmod 0440 /etc/sudoers.d/user && \
git clone --depth 1 https://github.com/microsoft/vcpkg /home/user/vcpkg && \
/home/user/vcpkg/bootstrap-vcpkg.sh && \
/home/user/vcpkg/vcpkg install dbg-macro fmt magic-enum zlib

USER user
