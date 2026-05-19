vcpkg_download_distfile(ARCHIVE
    URLS "https://gmsh.info/src/gmsh-${VERSION}-source.tgz"
    FILENAME "gmsh-${VERSION}-source.tgz"
    SHA512 f757688ed08b0c37ad3ebcf98b7661c385a434f83672fcad9c7f406afecc00fb1df6ef955a7ac76e54662ef95bcf2ca8a5d133c02603122ba5507f2d5359674e
)
vcpkg_extract_source_archive(
    SOURCE_PATH
    ARCHIVE "${ARCHIVE}"
    PATCHES
        installdirs.diff
        linking-and-naming.diff
)

string(COMPARE EQUAL "${VCPKG_LIBRARY_LINKAGE}" "static" BUILD_LIB)
string(COMPARE EQUAL "${VCPKG_LIBRARY_LINKAGE}" "dynamic" BUILD_SHARED)
string(COMPARE EQUAL "${VCPKG_CRT_LINKAGE}" "static" STATIC_RUNTIME)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    DISABLE_PARALLEL_CONFIGURE
    OPTIONS
        -DENABLE_BUILD_LIB=${BUILD_LIB}
        -DENABLE_BUILD_SHARED=${BUILD_SHARED}
        -DENABLE_BUILD_DYNAMIC=OFF # Needs gfortran
        -DENABLE_MSVC_STATIC_RUNTIME=${STATIC_RUNTIME}
        -DGMSH_PACKAGER=vcpkg
        -DGMSH_RELEASE=ON
        -DENABLE_PACKAGE_STRIP=ON
        -DENABLE_SYSTEM_CONTRIB=OFF
        # Manually enable some features
        -DENABLE_MESH=ON
        -DENABLE_ALGLIB=ON
        -DENABLE_ANN=ON
        -DENABLE_EIGEN=ON
        -DENABLE_OPENMP=ON
        -DENABLE_OPTHOM=ON
        -DENABLE_TINYOBJLOADER=ON
        # Not implement
        -DENABLE_BLAS_LAPACK=OFF
        -DENABLE_CAIRO=OFF
        -DENABLE_PROFILE=OFF
        -DENABLE_CGNS=OFF
        -DENABLE_CGNS_CPEX0045=OFF
        -DENABLE_GRAPHICS=OFF # Requires mesh, post, plugins and onelab
        -DENABLE_GMP=OFF
        -DENABLE_PARSER=OFF
        -DENABLE_PLUGINS=OFF
        -DENABLE_POST=OFF
        -DENABLE_POPPLER=OFF
        -DENABLE_PRIVATE_API=OFF
        -DENABLE_PRO=OFF
        -DENABLE_QUADMESHINGTOOLS=OFF
        -DENABLE_TOUCHBAR=OFF
        -DENABLE_VISUDEV=OFF
        -DENABLE_WRAP_JAVA=OFF
        -DENABLE_WRAP_PYTHON=OFF
        # Optional features, disable by default
        -DENABLE_MPI=OFF
        -DENABLE_OCC=OFF
        -DENABLE_OCC_CAF=OFF
        -DENABLE_OCC_STATIC=OFF
        -DENABLE_OCC_TBB=OFF
        -DENABLE_ZIPPER=OFF
        # Requies dependencies which not included in vcpkg yet
        -DENABLE_3M=OFF
        -DENABLE_BAMG=OFF
        -DENABLE_BLOSSOM=OFF
        -DENABLE_DINTEGRATION=OFF
        -DENABLE_DOMHEX=OFF
        -DENABLE_FLTK=OFF # Needs executable fltk-config
        -DENABLE_GEOMETRYCENTRAL=OFF
        -DENABLE_GETDP=OFF
        -DENABLE_GMM=OFF
        -DENABLE_HXT=OFF
        -DENABLE_KBIPACK=OFF
        -DENABLE_MATHEX=OFF
        -DENABLE_MED=OFF
        -DENABLE_MESQUITE=OFF
        -DENABLE_METIS=OFF
        -DENABLE_MMG=OFF
        -DENABLE_MPEG_ENCODE=OFF
        -DENABLE_MUMPS=OFF
        -DENABLE_NETGEN=OFF
        -DENABLE_NII2MESH=OFF
        -DENABLE_NUMPY=OFF
        -DENABLE_PETSC4PY=OFF
        -DENABLE_ONELAB=OFF
        -DENABLE_ONELAB_METAMODEL=OFF
        -DENABLE_OPENACC=OFF
        -DENABLE_OSMESA=OFF
        -DENABLE_P4EST=OFF
        -DENABLE_PETSC=OFF
        -DENABLE_QUADTRI=OFF
        -DENABLE_REVOROPT=OFF
        -DENABLE_SLEPC=OFF
        -DENABLE_SOLVER=OFF
        -DENABLE_TCMALLOC=OFF
        -DENABLE_TINYXML2=OFF
        -DENABLE_UNTANGLE=OFF
        -DENABLE_VOROPP=OFF
        -DENABLE_WINSLOWUNTANGLER=OFF
        # experimental
        -DENABLE_BUILD_ANDROID=OFF
        -DENABLE_BUILD_IOS=OFF

        -DENABLE_OS_SPECIFIC_INSTALL=OFF # Needs system permission
        -DENABLE_RPATH=OFF # Should use dependencies in vcpkg
        -DENABLE_TESTS=OFF
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup()

vcpkg_copy_tools(TOOL_NAMES gmsh AUTO_CLEAN)

file(REMOVE_RECURSE
    "${CURRENT_PACKAGES_DIR}/debug/include"
    "${CURRENT_PACKAGES_DIR}/debug/share"
)

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.txt")
