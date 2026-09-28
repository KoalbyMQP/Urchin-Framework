# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

# Local-only, not committed: isolates the hardware-verification builds
# (env:DBoxEcho, env:HerkulexTest) from the normal firmware sources, and
# from each other, so they don't collide over duplicate app_main()
# definitions.
if(DEFINED ENV{DBOX_ECHO_ONLY})
    FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/DBoxEcho/*.*)
elseif(DEFINED ENV{HERKULEX_TEST_ONLY})
    FILE(GLOB_RECURSE app_sources
        ${CMAKE_SOURCE_DIR}/src/Herkulex/*.*
        ${CMAKE_SOURCE_DIR}/src/HerkulexTest/*.*)
else()
    FILE(GLOB_RECURSE dbox_echo_sources ${CMAKE_SOURCE_DIR}/src/DBoxEcho/*.*)
    if(dbox_echo_sources)
        list(REMOVE_ITEM app_sources ${dbox_echo_sources})
    endif()
    FILE(GLOB_RECURSE herkulex_test_sources ${CMAKE_SOURCE_DIR}/src/HerkulexTest/*.*)
    if(herkulex_test_sources)
        list(REMOVE_ITEM app_sources ${herkulex_test_sources})
    endif()
endif()

idf_component_register(SRCS ${app_sources})
