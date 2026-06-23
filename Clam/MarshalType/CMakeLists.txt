cmake_minimum_required(VERSION 3.20)
project(MarshalType LANGUAGES CXX)

option(BUILD_TESTING "Build tests" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
include(CTest)

# --------------------------------------------------
# Python + pybind11
# --------------------------------------------------
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

set(PYBIND11_FINDPYTHON ON)

FetchContent_Declare(
        pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11.git
        GIT_TAG v2.13.6
)
FetchContent_MakeAvailable(pybind11)

# --------------------------------------------------
# GoogleTest
# --------------------------------------------------
find_package(GTest REQUIRED)

# --------------------------------------------------
# Paths
# --------------------------------------------------
set(PROJECT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(PYTHON_TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test/Python)
set(PYTHON_MODULE_OUTPUT_DIR ${CMAKE_BINARY_DIR}/python)

# -------------------------
# C++ CORE LIBRARY
# -------------------------
add_library(marshaltype STATIC
        src/ValList.cpp
)

target_include_directories(marshaltype PUBLIC
        ${PROJECT_INCLUDE_DIR}
)

# -------------------------
# PYTHON BINDINGS
# -------------------------
pybind11_add_module(marshaltype_py
        src/MarshalType_bindings.cpp
)

target_link_libraries(marshaltype_py PRIVATE marshaltype)

target_include_directories(marshaltype_py PRIVATE
        ${PROJECT_INCLUDE_DIR}
)

set_target_properties(marshaltype_py PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${PYTHON_MODULE_OUTPUT_DIR}
        LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PYTHON_MODULE_OUTPUT_DIR}
        LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PYTHON_MODULE_OUTPUT_DIR}
        RUNTIME_OUTPUT_DIRECTORY ${PYTHON_MODULE_OUTPUT_DIR}
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PYTHON_MODULE_OUTPUT_DIR}
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PYTHON_MODULE_OUTPUT_DIR}
)

# --------------------------------------------------
# TESTS
# --------------------------------------------------
if(BUILD_TESTING)
    enable_testing()

    # -------------------------
    # GTEST (C++)
    # -------------------------
    add_executable(MarshalType_tests
            test/C++/test_item.cpp.disabled
            test/C++/test_ValList.cpp
    )

    target_link_libraries(MarshalType_tests
            PRIVATE
            marshaltype
            GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(MarshalType_tests)

    # -------------------------
    # PYTEST (Python)
    # -------------------------
    add_test(
            NAME python_tests
            COMMAND ${Python3_EXECUTABLE}
            -m pytest
            -vv
            -s
            ${PYTHON_TEST_DIR}
    )

    set_tests_properties(python_tests PROPERTIES
            ENVIRONMENT
            "PYTHONPATH=${PYTHON_MODULE_OUTPUT_DIR};$ENV{PYTHONPATH};PATH=${PYTHON_MODULE_OUTPUT_DIR};$ENV{PATH}"
    )

    # --------------------------------------------------
    # SINGLE ENTRY POINTS
    # --------------------------------------------------

    # Build everything (compile only)
    add_custom_target(build_all
            DEPENDS
            marshaltype
            marshaltype_py
            MarshalType_tests
    )

    # Run everything (tests)
    add_custom_target(test_all
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
            DEPENDS build_all
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()