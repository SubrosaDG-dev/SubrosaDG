FROM fedora:rawhide

RUN dnf update -y && \
    dnf install -y git cmake vcpkg ccache ninja-build clang-devel llvm-devel gdb lldb lld clang-tools-extra iwyu gcovr python3-numpy wget tar zip patchelf && \
    dnf clean all

ENV CC=/usr/bin/clang
ENV CXX=/usr/bin/clang++
ENV CMAKE_GENERATOR=Ninja

ENV VCPKG_FORCE_SYSTEM_BINARIES=1
RUN cd /root && \
    git clone --depth 1 https://github.com/microsoft/vcpkg && \
    cd vcpkg && \
    ./bootstrap-vcpkg.sh
ENV VCPKG_ROOT=/root/vcpkg


RUN cd /root && \
    wget https://gmsh.info/bin/Linux/gmsh-git-Linux64-sdk.tgz \
    && tar -xzf gmsh-git-Linux64-sdk.tgz
ENV GMSH_ROOT=/root/gmsh-git-Linux64-sdk
