# -------------------------
# Setup
# -------------------------

cmake_minimum_required(VERSION 4.1)
project(D_BOX LANGUAGES CXX )

#used for Cyclic redundancy check sum
#find_package(Boost REQUIRED)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

FetchContent_Declare(
        pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11.git
        GIT_TAG v2.13.6
)
FetchContent_MakeAvailable(pybind11)

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
)

FetchContent_MakeAvailable(googletest)

# -------------------------
# C++ library
# -------------------------

add_library(dbox SHARED
        src/import.cpp
)

target_include_directories(dbox PUBLIC include)

# -------------------------
# PyBind
# -------------------------

pybind11_add_module(dbox_py DBOX_bindings.cpp)



target_link_libraries(dbox_py PRIVATE dbox)

set_target_properties(dbox_py PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/python
)


# -------------------------
# Unit tests
# -------------------------

option(BUILD_TESTS "Build unit tests" ON)

if(BUILD_TESTS)

    enable_testing()

    add_executable(dbox_tests
            tests/test_crc.cpp
            tests/test_api.cpp
    )

    target_link_libraries(dbox_tests
            PRIVATE
            dbox
            GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(dbox_tests)

endif()

if(BUILD_TESTS)

    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    add_test(
            NAME python_tests
            COMMAND Python3::Interpreter -m pytest ${CMAKE_SOURCE_DIR}/tests/Python/
    )

endif()