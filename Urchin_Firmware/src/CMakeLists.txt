# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

# Local-only hardware bench-test builds, isolated from the normal firmware
# sources and from each other (they'd otherwise collide over duplicate
# app_main()/global definitions). Which one gets built is chosen by an
# environment variable that scripts/select_sources.py sets automatically
# per PlatformIO environment -- nobody needs to set these by hand.
if(DEFINED ENV{DBOX_ECHO_ONLY})
    FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/DBoxEcho/*.*)
elseif(DEFINED ENV{HERKULEX_TEST_ONLY})
    FILE(GLOB_RECURSE app_sources
        ${CMAKE_SOURCE_DIR}/src/Herkulex/*.*
        ${CMAKE_SOURCE_DIR}/src/BenchShared/*.*
        ${CMAKE_SOURCE_DIR}/src/HerkulexTest/*.*)
elseif(DEFINED ENV{DBOX_MOTOR_ONLY})
    FILE(GLOB_RECURSE app_sources
        ${CMAKE_SOURCE_DIR}/src/Herkulex/*.*
        ${CMAKE_SOURCE_DIR}/src/BenchShared/*.*
        ${CMAKE_SOURCE_DIR}/src/DBoxMotor/*.*)
    list(APPEND app_sources ${CMAKE_SOURCE_DIR}/src/DBoxEcho/Esp32UartPort.cpp)
else()
    foreach(bench_dir DBoxEcho HerkulexTest DBoxMotor BenchShared)
        FILE(GLOB_RECURSE bench_sources ${CMAKE_SOURCE_DIR}/src/${bench_dir}/*.*)
        if(bench_sources)
            list(REMOVE_ITEM app_sources ${bench_sources})
        endif()
    endforeach()
endif()

idf_component_register(SRCS ${app_sources})
