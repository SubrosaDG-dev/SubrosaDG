vcpkg_buildpath_length_warning(37)

block(SCOPE_FOR VARIABLES PROPAGATE SOURCE_PATH)
set(VCPKG_BUILD_TYPE release) # header-only

set(VCPKG_USE_HEAD_VERSION ON)

vcpkg_from_gitlab(
    GITLAB_URL https://gitlab.com
    OUT_SOURCE_PATH SOURCE_PATH
    REPO libeigen/eigen
    HEAD_REF master
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DEIGEN_BUILD_TESTING=OFF
        -DEIGEN_BUILD_BLAS=OFF
        -DEIGEN_BUILD_LAPACK=OFF
        -DEIGEN_BUILD_DOC=OFF
        -DEIGEN_BUILD_PKGCONFIG=ON
        "-DCMAKEPACKAGE_INSTALL_DIR=${CURRENT_PACKAGES_DIR}/share/eigen3"
        "-DPKGCONFIG_INSTALL_DIR=${CURRENT_PACKAGES_DIR}/lib/pkgconfig"
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup()
endblock()

if(NOT VCPKG_BUILD_TYPE)
    file(INSTALL "${CURRENT_PACKAGES_DIR}/lib/pkgconfig/eigen3.pc" DESTINATION "${CURRENT_PACKAGES_DIR}/debug/lib/pkgconfig")
endif()
vcpkg_fixup_pkgconfig()

file(GLOB INCLUDES "${CURRENT_PACKAGES_DIR}/include/eigen3/*")
# Copy the eigen header files to conventional location for user-wide MSBuild integration
file(COPY ${INCLUDES} DESTINATION "${CURRENT_PACKAGES_DIR}/include")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/COPYING.README")
