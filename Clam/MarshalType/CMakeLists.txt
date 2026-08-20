cmake_minimum_required(VERSION 3.20)
project(MarshalType VERSION 0.1.0 LANGUAGES CXX)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)


set(CMAKE_POSITION_INDEPENDENT_CODE ON)

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
# Paths
# --------------------------------------------------
set(PROJECT_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(PYTHON_TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/test/Python")
set(PYTHON_MODULE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/python")

# --------------------------------------------------
# C++ CORE LIBRARY
# --------------------------------------------------
add_library(marshaltype
        src/ValList.cpp
        src/Item.cpp
)

add_library(MarshalType::marshaltype ALIAS marshaltype)

target_include_directories(marshaltype PUBLIC
        $<BUILD_INTERFACE:${PROJECT_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(marshaltype PUBLIC cxx_std_20)

# --------------------------------------------------
# PYTHON BINDINGS
# --------------------------------------------------
pybind11_add_module(marshaltype_py
        src/MarshalType_bindings.cpp
)

target_link_libraries(marshaltype_py PRIVATE marshaltype)

target_include_directories(marshaltype_py PRIVATE
        "${PROJECT_INCLUDE_DIR}"
)

set_target_properties(marshaltype_py PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${PYTHON_MODULE_OUTPUT_DIR}"
        LIBRARY_OUTPUT_DIRECTORY_DEBUG "${PYTHON_MODULE_OUTPUT_DIR}"
        LIBRARY_OUTPUT_DIRECTORY_RELEASE "${PYTHON_MODULE_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY "${PYTHON_MODULE_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PYTHON_MODULE_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PYTHON_MODULE_OUTPUT_DIR}"
)

# --------------------------------------------------
# INSTALL: C++ library + headers + CMake package export
# --------------------------------------------------

install(TARGETS marshaltype
        EXPORT MarshalTypeTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY "${PROJECT_INCLUDE_DIR}/"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT MarshalTypeTargets
        FILE MarshalTypeTargets.cmake
        NAMESPACE MarshalType::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MarshalType
)

configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/MarshalTypeConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/MarshalTypeConfig.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MarshalType
)

write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/MarshalTypeConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)

install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/MarshalTypeConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/MarshalTypeConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MarshalType
)

# --------------------------------------------------
# INSTALL: Python module
#--------------------------------------------------
execute_process(
        COMMAND "${Python3_EXECUTABLE}" -c
        "import sysconfig; print(sysconfig.get_path('platlib'))"
        OUTPUT_VARIABLE PYTHON_SITE_PACKAGES
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
install(TARGETS marshaltype_py
        LIBRARY DESTINATION "${PYTHON_SITE_PACKAGES}"
        RUNTIME DESTINATION "${PYTHON_SITE_PACKAGES}"
)
# --------------------------------------------------
# BUILD-ONLY UMBRELLA
# --------------------------------------------------
add_custom_target(build_all
        DEPENDS marshaltype marshaltype_py
)

# --------------------------------------------------
# TESTS
# --------------------------------------------------
if(BUILD_TESTING)
    # GoogleTest
    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.15.2
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    include(GoogleTest)

    # C++ tests
    add_executable(MarshalType_tests
            test/C++/test_ValList.cpp
            test/C++/test_item.cpp
    )

    target_link_libraries(MarshalType_tests PRIVATE
            marshaltype
            GTest::gtest_main
    )

    # Make sure the C++ test binary is part of build_all,
    # but do not run test discovery during build.
    add_dependencies(build_all MarshalType_tests)

    gtest_discover_tests(MarshalType_tests
            DISCOVERY_MODE PRE_TEST
    )

    # Python tests
    add_test(
            NAME python_tests
            COMMAND "${Python3_EXECUTABLE}" -m pytest -vv -s "${PYTHON_TEST_DIR}"
    )

    set_tests_properties(python_tests PROPERTIES
            ENVIRONMENT_MODIFICATION
            "PYTHONPATH=path_list_prepend:${PYTHON_MODULE_OUTPUT_DIR}"
    )

    # TEST-ONLY UMBRELLA
    add_custom_target(test_all
            COMMAND "${CMAKE_CTEST_COMMAND}" --output-on-failure -V
            DEPENDS build_all
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
            VERBATIM
    )
endif()