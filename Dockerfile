FROM fedora:rawhide

RUN dnf update -y && \
dnf install -y git cmake clang ccache gdb lldb lld ninja-build clang-tools-extra iwyu libcxx-devel eigen3-devel cgnslib-devel python3-numpy zip patchelf && \
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
git clone --depth 1 --branch "gmsh_4_12_0" https://gitlab.onelab.info/gmsh/gmsh.git && \
cd gmsh && \
mkdir build && \
cd build && \
cmake -DCMAKE_CXX_FLAGS="-stdlib=libc++" -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld -stdlib=libc++" -DCMAKE_BUILD_TYPE=Release -DDEFAULT=NO -DENABLE_SYSTEM_CONTRIB=YES -DENABLE_BUILD_SHARED=YES -DENABLE_BUILD_DYNAMIC=YES -DENABLE_OS_SPECIFIC_INSTALL=YES -DENABLE_PARSER=YES -DENABLE_BLOSSOM=YES -DENABLE_OPTHOM=YES -DENABLE_CGNS=YES -DENABLE_NETGEN=YES -DENABLE_EIGEN=YES -DENABLE_OPENMP=YES -DENABLE_MESH=YES .. && \
ninja && \
ninja install && \
cd /root && \
rm -rf gmsh && \
patchelf --set-rpath /usr/local/lib64 /usr/local/bin/gmsh && \
patchelf --set-rpath /usr/local/lib64 /usr/local/lib64/libgmsh.so
ENV GMSH_ROOT=/usr/local
