cmake_minimum_required(VERSION 3.20)
project(RocketForge VERSION 0.1.0 LANGUAGES CXX)

cmake_minimum_required(VERSION 3.20)

project(RocketForge VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    add_compile_options("$<$<COMPILE_LANGUAGE:C,CXX>:/utf-8>")
endif()


# Fetching
include(FetchContent)

## raylib
set(RAYLIB_VERSION 6.0)
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

## pybind11
set(PYBIND11_VERSION 3.0.1)
FetchContent_Declare(
    pybind11
    URL https://github.com/pybind/pybind11/archive/refs/tags/v${PYBIND11_VERSION}.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
)
FetchContent_MakeAvailable(pybind11)

## nlohmann_json
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# include
include_directories(
    simulation
    simulation/include/
)

# make
## python module
add_library(simulation STATIC
    simulation/source/dummy.cpp
)