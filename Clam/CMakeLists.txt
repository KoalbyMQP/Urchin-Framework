cmake_minimum_required(VERSION 4.1)
project(Clam)

set(CMAKE_CXX_STANDARD 20)

#add_executable(Clam src/main.cpp)
include(FetchContent)

FetchContent_Declare(
        pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11.git
        GIT_TAG v2.13.6
)

FetchContent_MakeAvailable(pybind11)

pybind11_add_module(Clam src/import.cpp
        src/ValList.cpp)

set_target_properties(Clam PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Tests/Python
)