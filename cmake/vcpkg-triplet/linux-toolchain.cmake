set(CMAKE_C_COMPILER /usr/bin/clang)
set(CMAKE_CXX_COMPILER /usr/bin/clang++)

if (SUBROSA_DG_LIBCXX)
    set(CMAKE_CXX_FLAGS "-stdlib=libc++")
    set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld -stdlib=libc++")
else()
    set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld")
endif()
