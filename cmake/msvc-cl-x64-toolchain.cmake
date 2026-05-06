# msvc-cl-x64-toolchain.cmake
#
# Usage:
# 1. Open an MSVC x64 developer shell, or run vcvars64.bat first.
# 2. Configure with a generator that can drive cl.exe, for example Ninja:
#    cmake -S . -B build-msvc -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/msvc-cl-x64-toolchain.cmake

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

find_program(MSVC_CL_COMPILER
    NAMES cl
    REQUIRED)

find_program(MSVC_LINKER
    NAMES link
    REQUIRED)

find_program(MSVC_LIB_TOOL
    NAMES lib)

find_program(MSVC_RC_COMPILER
    NAMES rc)

find_program(MSVC_MT_TOOL
    NAMES mt)

set(CMAKE_C_COMPILER "${MSVC_CL_COMPILER}")
set(CMAKE_CXX_COMPILER "${MSVC_CL_COMPILER}")
set(CMAKE_LINKER "${MSVC_LINKER}")

if (MSVC_LIB_TOOL)
    set(CMAKE_AR "${MSVC_LIB_TOOL}")
endif()

if (MSVC_RC_COMPILER)
    set(CMAKE_RC_COMPILER "${MSVC_RC_COMPILER}")
endif()

if (MSVC_MT_TOOL)
    set(CMAKE_MT "${MSVC_MT_TOOL}")
endif()
