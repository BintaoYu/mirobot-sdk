# clang-cl-msvc-x64-linux-toolchain.cmake
#
# Cross-compiles Windows x64 binaries with the MSVC ABI from a Linux host.
# Prerequisites:
# 1. LLVM for Linux with clang-cl/lld-link/llvm-lib/llvm-rc/llvm-mt under
#    .toolchains/llvm/LLVM-*-Linux-X64
# 2. xwin splat output under .toolchains/xwin/splat
#
# Use with a host generator such as Ninja:
#   cmake -S . -B build-msvc-cross -G Ninja \
#     -DCMAKE_TOOLCHAIN_FILE=cmake/clang-cl-msvc-x64-linux-toolchain.cmake

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_SYSTEM_VERSION 10.0)

get_filename_component(STEEL_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

file(GLOB STEEL_LLVM_CANDIDATES LIST_DIRECTORIES true
    "${STEEL_REPO_ROOT}/.toolchains/llvm/LLVM-*-Linux-X64")
list(SORT STEEL_LLVM_CANDIDATES ORDER DESCENDING)
list(LENGTH STEEL_LLVM_CANDIDATES STEEL_LLVM_CANDIDATE_COUNT)
if (STEEL_LLVM_CANDIDATE_COUNT EQUAL 0)
    message(FATAL_ERROR
        "Missing local LLVM toolchain under ${STEEL_REPO_ROOT}/.toolchains/llvm.\n"
        "Run scripts/setup_windows_msvc_cross_toolchain.sh first.")
endif()

list(GET STEEL_LLVM_CANDIDATES 0 STEEL_LOCAL_LLVM_ROOT)
set(STEEL_LOCAL_LLVM_BIN_DIR "${STEEL_LOCAL_LLVM_ROOT}/bin")
set(STEEL_LOCAL_XWIN_ROOT "${STEEL_REPO_ROOT}/.toolchains/xwin/splat")

if (NOT EXISTS "${STEEL_LOCAL_LLVM_BIN_DIR}/clang-cl")
    message(FATAL_ERROR "Missing clang-cl in ${STEEL_LOCAL_LLVM_BIN_DIR}")
endif()
if (NOT EXISTS "${STEEL_LOCAL_LLVM_BIN_DIR}/lld-link")
    message(FATAL_ERROR "Missing lld-link in ${STEEL_LOCAL_LLVM_BIN_DIR}")
endif()
if (NOT EXISTS "${STEEL_LOCAL_LLVM_BIN_DIR}/llvm-lib")
    message(FATAL_ERROR "Missing llvm-lib in ${STEEL_LOCAL_LLVM_BIN_DIR}")
endif()
if (NOT EXISTS "${STEEL_LOCAL_XWIN_ROOT}/crt/include")
    message(FATAL_ERROR
        "Missing xwin sysroot under ${STEEL_LOCAL_XWIN_ROOT}.\n"
        "Run scripts/setup_windows_msvc_cross_toolchain.sh first.")
endif()

set(ENV{PATH} "${STEEL_LOCAL_LLVM_BIN_DIR}:$ENV{PATH}")

set(CMAKE_C_COMPILER "${STEEL_LOCAL_LLVM_BIN_DIR}/clang-cl")
set(CMAKE_CXX_COMPILER "${STEEL_LOCAL_LLVM_BIN_DIR}/clang-cl")
set(CMAKE_LINKER "${STEEL_LOCAL_LLVM_BIN_DIR}/lld-link")
set(CMAKE_AR "${STEEL_LOCAL_LLVM_BIN_DIR}/llvm-lib")
set(CMAKE_RC_COMPILER "${STEEL_LOCAL_LLVM_BIN_DIR}/llvm-rc")
set(CMAKE_MT "${STEEL_LOCAL_LLVM_BIN_DIR}/llvm-mt")

set(CMAKE_C_COMPILER_TARGET x86_64-pc-windows-msvc)
set(CMAKE_CXX_COMPILER_TARGET x86_64-pc-windows-msvc)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

if (NOT CMAKE_CONFIGURATION_TYPES AND NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_FIND_ROOT_PATH "${STEEL_LOCAL_XWIN_ROOT}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(STEEL_XWIN_INCLUDE_FLAGS
    "/imsvc${STEEL_LOCAL_XWIN_ROOT}/crt/include"
    "/imsvc${STEEL_LOCAL_XWIN_ROOT}/sdk/include/ucrt"
    "/imsvc${STEEL_LOCAL_XWIN_ROOT}/sdk/include/um"
    "/imsvc${STEEL_LOCAL_XWIN_ROOT}/sdk/include/shared"
    "/imsvc${STEEL_LOCAL_XWIN_ROOT}/sdk/include/winrt"
    "/imsvc${STEEL_LOCAL_XWIN_ROOT}/sdk/include/cppwinrt"
)

set(STEEL_XWIN_LINK_FLAGS
    "/libpath:${STEEL_LOCAL_XWIN_ROOT}/crt/lib/x86_64"
    "/libpath:${STEEL_LOCAL_XWIN_ROOT}/sdk/lib/ucrt/x86_64"
    "/libpath:${STEEL_LOCAL_XWIN_ROOT}/sdk/lib/um/x86_64"
)

string(JOIN " " STEEL_CLANG_CL_COMMON_FLAGS
    "-Wno-unused-command-line-argument"
    "-fuse-ld=lld-link"
    ${STEEL_XWIN_INCLUDE_FLAGS}
)
string(JOIN " " STEEL_CLANG_CL_LINK_FLAGS ${STEEL_XWIN_LINK_FLAGS})

set(CMAKE_C_FLAGS_INIT "${STEEL_CLANG_CL_COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${STEEL_CLANG_CL_COMMON_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${STEEL_CLANG_CL_LINK_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${STEEL_CLANG_CL_LINK_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "${STEEL_CLANG_CL_LINK_FLAGS}")
