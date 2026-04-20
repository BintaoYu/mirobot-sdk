cmake_minimum_required(VERSION 3.16)
project(SteelApp LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(STEEL_APP_STAGED_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(STEEL_APP_STAGED_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(STEEL_APP_SUPPORT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/support")

function(require_staged_file file_path)
    if (NOT EXISTS "${file_path}")
        message(FATAL_ERROR
            "Missing staged Steel file: ${file_path}\n"
            "Build the repository root first so the main build can copy headers into app/include and shared libraries into app/lib.")
    endif()
endfunction()

require_staged_file("${STEEL_APP_STAGED_INCLUDE_DIR}/comm_interface.h")
require_staged_file("${STEEL_APP_STAGED_INCLUDE_DIR}/serial_comm.h")
require_staged_file("${STEEL_APP_STAGED_INCLUDE_DIR}/tcp_comm.h")
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
