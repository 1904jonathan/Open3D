include(ExternalProject)

if(MSVC)
    set(lib_name assimp-vc${MSVC_TOOLSET_VERSION}-mt)
else()
    set(lib_name assimp)
endif()

# IntelLLVM (SYCL) compiler defaults to fast math, causing NaN comparison code
# compilation error.
if(CMAKE_CXX_COMPILER_ID MATCHES "IntelLLVM")
    set(assimp_cmake_cxx_flags "${CMAKE_CXX_FLAGS} -ffp-contract=on -fno-fast-math")
else()
    set(assimp_cmake_cxx_flags "${CMAKE_CXX_FLAGS}")
endif()

ExternalProject_Add(
    ext_assimp
    PREFIX assimp
    GIT_REPOSITORY https://github.com/assimp/assimp
    GIT_TAG 9a5d6282322de3085474ca8f9f4a4a8f84756394
    DOWNLOAD_DIR "${OPEN3D_THIRD_PARTY_DOWNLOAD_DIR}/assimp"
    UPDATE_COMMAND ""
    CMAKE_ARGS
        ${ExternalProject_CMAKE_ARGS_hidden}
        -DCMAKE_CXX_FLAGS:STRING=${assimp_cmake_cxx_flags}
        -DBUILD_SHARED_LIBS=OFF
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DASSIMP_NO_EXPORT=ON
        -DASSIMP_BUILD_ASSIMP_TOOLS=OFF
        -DASSIMP_BUILD_TESTS=OFF
        -DASSIMP_INSTALL_PDB=OFF
        -DASSIMP_BUILD_ZLIB=ON
        -DHUNTER_ENABLED=OFF # Renamed to "ASSIMP_HUNTER_ENABLED" in newer assimp.
        -DCMAKE_DEBUG_POSTFIX=
    BUILD_BYPRODUCTS
        <INSTALL_DIR>/${Open3D_INSTALL_LIB_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}${lib_name}${CMAKE_STATIC_LIBRARY_SUFFIX}
        <INSTALL_DIR>/${Open3D_INSTALL_LIB_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}IrrXML${CMAKE_STATIC_LIBRARY_SUFFIX}
)

ExternalProject_Get_Property(ext_assimp INSTALL_DIR)
set(ASSIMP_INCLUDE_DIR ${INSTALL_DIR}/include/)
set(ASSIMP_LIB_DIR ${INSTALL_DIR}/${Open3D_INSTALL_LIB_DIR})
set(ASSIMP_LIBRARIES ${lib_name})
