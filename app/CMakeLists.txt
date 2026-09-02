cmake_minimum_required(VERSION 3.16)
project(SteelApp LANGUAGES CXX)

include(CTest)
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/steel_platform_tag.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(STEEL_APP_SDK_TAG "" CACHE STRING "Platform tag for the staged Steel SDK. Leave empty to use the current target platform.")
if (STEEL_APP_SDK_TAG STREQUAL "")
    steel_compute_platform_tag(STEEL_APP_SDK_TAG)
endif()

set(STEEL_APP_SDK_ROOT "" CACHE PATH "Root directory of the staged Steel SDK. Leave empty to use app/sdk/<tag>.")
if (STEEL_APP_SDK_ROOT STREQUAL "")
    set(STEEL_APP_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/sdk/${STEEL_APP_SDK_TAG}")
endif()

set(STEEL_APP_STAGED_INCLUDE_DIR "${STEEL_APP_SDK_ROOT}/include")
set(STEEL_APP_STAGED_LIB_DIR "${STEEL_APP_SDK_ROOT}/lib")
set(STEEL_APP_SUPPORT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/support")

function(require_staged_file file_path)
    if (NOT EXISTS "${file_path}")
        message(FATAL_ERROR
            "Missing staged Steel file: ${file_path}\n"
            "Build the repository root first so the main build can stage headers and shared libraries into app/sdk/<platform-tag>.")
    endif()
endfunction()

require_staged_file("${STEEL_APP_STAGED_INCLUDE_DIR}/comm_interface.h")
require_staged_file("${STEEL_APP_STAGED_INCLUDE_DIR}/device_session.h")
require_staged_file("${STEEL_APP_STAGED_INCLUDE_DIR}/io_adapter.h")
require_staged_file("${STEEL_APP_STAGED_INCLUDE_DIR}/serial_comm.h")
require_staged_file("${STEEL_APP_STAGED_INCLUDE_DIR}/tcp_comm.h")
require_staged_file("${STEEL_APP_STAGED_INCLUDE_DIR}/tcp_server_comm.h")
require_staged_file("${STEEL_APP_STAGED_INCLUDE_DIR}/udp_comm.h")
require_staged_file("${STEEL_APP_STAGED_INCLUDE_DIR}/motor.h")
require_staged_file("${STEEL_APP_STAGED_INCLUDE_DIR}/mirobot.h")

function(import_steel_library imported_target base_name)
    if (WIN32)
        set(runtime_library "${STEEL_APP_STAGED_LIB_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}${base_name}${CMAKE_SHARED_LIBRARY_SUFFIX}")
        set(import_library "${STEEL_APP_STAGED_LIB_DIR}/${CMAKE_IMPORT_LIBRARY_PREFIX}${base_name}${CMAKE_IMPORT_LIBRARY_SUFFIX}")

        require_staged_file("${runtime_library}")
        require_staged_file("${import_library}")

        add_library(${imported_target} SHARED IMPORTED GLOBAL)
        set_target_properties(${imported_target} PROPERTIES
            IMPORTED_LOCATION "${runtime_library}"
            IMPORTED_IMPLIB "${import_library}"
            INTERFACE_INCLUDE_DIRECTORIES "${STEEL_APP_STAGED_INCLUDE_DIR}"
        )
    else()
        set(shared_library "${STEEL_APP_STAGED_LIB_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}${base_name}${CMAKE_SHARED_LIBRARY_SUFFIX}")
        require_staged_file("${shared_library}")

        add_library(${imported_target} SHARED IMPORTED GLOBAL)
        set_target_properties(${imported_target} PROPERTIES
            IMPORTED_LOCATION "${shared_library}"
            INTERFACE_INCLUDE_DIRECTORIES "${STEEL_APP_STAGED_INCLUDE_DIR}"
        )
    endif()
endfunction()

import_steel_library(steel_sdk_common common)
import_steel_library(steel_sdk_motor motor)
import_steel_library(steel_sdk_mirobot mirobot)

target_link_libraries(steel_sdk_motor INTERFACE steel_sdk_common)
target_link_libraries(steel_sdk_mirobot INTERFACE steel_sdk_common)

function(configure_sdk_consumer target_name)
    target_include_directories(${target_name} PRIVATE ${STEEL_APP_SUPPORT_DIR})
    target_compile_features(${target_name} PRIVATE cxx_std_17)
    target_link_libraries(${target_name} PRIVATE steel_sdk_common steel_sdk_motor steel_sdk_mirobot)

    if (UNIX)
        set_target_properties(${target_name} PROPERTIES BUILD_RPATH "${STEEL_APP_STAGED_LIB_DIR}")
    endif()

    if (WIN32)
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:steel_sdk_common>"
                "$<TARGET_FILE:steel_sdk_motor>"
                "$<TARGET_FILE:steel_sdk_mirobot>"
                "$<TARGET_FILE_DIR:${target_name}>"
        )
    endif()
endfunction()

add_subdirectory(example)
if (BUILD_TESTING)
    add_subdirectory(test)
endif()
