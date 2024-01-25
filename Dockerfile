FROM fedora:rawhide

RUN dnf update -y && \
dnf install -y git cmake ccache ninja-build clang-devel llvm-devel gdb lldb lld libcxx-devel clang-tools-extra gcovr gperftools flamegraph python3-numpy eigen3-devel cgnslib-devel zip patchelf && \
dnf clean all

ENV CC=/usr/bin/clang
ENV CXX=/usr/bin/clang++
ENV CMAKE_GENERATOR=Ninja

RUN cd /root && \
git clone --depth 1 --branch "clang_17" https://github.com/include-what-you-use/include-what-you-use.git && \
cd include-what-you-use && \
mkdir build && \
cd build && \
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="/usr" .. && \
ninja install && \
cd /root && \
rm -rf include-what-you-use

ENV VCPKG_FORCE_SYSTEM_BINARIES=1
RUN cd /root && \
git clone --depth 1 https://github.com/microsoft/vcpkg && \
cd vcpkg && \
./bootstrap-vcpkg.sh
ENV VCPKG_ROOT=/root/vcpkg

RUN cd /root && \
git clone --depth 1 --branch "gmsh_4_12_2" https://gitlab.onelab.info/gmsh/gmsh.git && \
cd gmsh && \
sed -i 's|set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")|set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib64")|' CMakeLists.txt && \
mkdir build && \
cd build && \
cmake -DCMAKE_CXX_FLAGS="-stdlib=libc++" -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld -stdlib=libc++" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="/usr" -DDEFAULT=NO -DENABLE_SYSTEM_CONTRIB=YES -DENABLE_BUILD_SHARED=YES -DENABLE_BUILD_DYNAMIC=YES -DENABLE_OS_SPECIFIC_INSTALL=YES -DENABLE_ANN=YES -DENABLE_PARSER=YES -DENABLE_BLOSSOM=YES -DENABLE_OPTHOM=YES -DENABLE_CGNS=YES -DENABLE_NETGEN=YES -DENABLE_EIGEN=YES -DENABLE_OPENMP=YES -DENABLE_MESH=YES .. && \
ninja install && \
cd /root && \
rm -rf gmsh
ENV GMSH_ROOT=/usr
