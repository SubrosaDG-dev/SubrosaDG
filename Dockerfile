FROM fedora:40

RUN dnf update -y && \
dnf install -y dnf-plugins-core && \
dnf config-manager --add-repo https://developer.download.nvidia.com/compute/cuda/repos/fedora39/x86_64/cuda-fedora39.repo && \
dnf install -y git wget tar zip patchelf cmake vcpkg ccache ninja-build llvm-devel clang-devel gdb lldb lld clang-tools-extra iwyu gcovr python3-numpy cuda-toolkit && \
dnf clean all

ENV http_proxy="http://172.17.0.1:7890" \
https_proxy="http://172.17.0.1:7890" \
all_proxy="socks5://172.17.0.1:7890" \
VCPKG_FORCE_SYSTEM_BINARIES=1 \
VCPKG_ROOT="/root/vcpkg"

RUN git clone --depth 1 https://github.com/microsoft/vcpkg /root/vcpkg && \
/root/vcpkg/bootstrap-vcpkg.sh && \
/root/vcpkg/vcpkg install dbg-macro fmt magic-enum zlib cuda-api-wrappers gtest
