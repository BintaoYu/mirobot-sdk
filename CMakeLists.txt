cmake_minimum_required(VERSION 3.16)
project(Steel LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BUILD_SHARED_LIBS ON)

if (WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

include(CTest)

add_subdirectory(common)
add_subdirectory(libs)
if (BUILD_TESTING)
    add_subdirectory(tests)
endif()

option(STEEL_STAGE_APP_SDK "Copy shared libraries and public headers into app after the main build." ON)

install(TARGETS common motor mirobot
    RUNTIME DESTINATION lib
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/common/include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

install(FILES
    ${CMAKE_SOURCE_DIR}/libs/motor/motor.h
    ${CMAKE_SOURCE_DIR}/libs/mirobot/mirobot.h
    DESTINATION include
)

if (STEEL_STAGE_APP_SDK)
    set(STEEL_APP_STAGE_DIR ${CMAKE_SOURCE_DIR}/app)
    set(STEEL_APP_STAGE_INCLUDE_DIR ${STEEL_APP_STAGE_DIR}/include)
    set(STEEL_APP_STAGE_LIB_DIR ${STEEL_APP_STAGE_DIR}/lib)
    set(STEEL_STAGE_CONFIG_ARGS)

    if (CMAKE_CONFIGURATION_TYPES)
        list(APPEND STEEL_STAGE_CONFIG_ARGS --config $<CONFIG>)
    endif()

    add_custom_target(stage_app_sdk ALL
        COMMAND ${CMAKE_COMMAND} -E rm -rf ${STEEL_APP_STAGE_INCLUDE_DIR}
        COMMAND ${CMAKE_COMMAND} -E rm -rf ${STEEL_APP_STAGE_LIB_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${STEEL_APP_STAGE_DIR}
        COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR} --prefix ${STEEL_APP_STAGE_DIR} ${STEEL_STAGE_CONFIG_ARGS}
        COMMENT "Staging Steel shared libraries and public headers into app"
        VERBATIM
    )

    add_dependencies(stage_app_sdk common motor mirobot)
endif()
