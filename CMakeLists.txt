cmake_minimum_required(VERSION 3.10)

project(frame_example)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CLANGFORMAT_EXECUTABLE clang-format-15)
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include(ClangFormat)

set(PKG_CONFIG_EXECUTABLE "/usr/bin/pkg-config")

# find_package(Threads)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
        libavcodec
        libavformat
        libavutil
        libswscale
        libswresample
)

message(STATUS ${SFML_LIBRARIES})

if (NOT DEBUG)
    set (COMPILE_FLAGS "-O3 -Wall -funsigned-char -std=c++17 -fno-omit-frame-pointer -pedantic -mssse3 -Wl,--strip-all -flto")
else()
    set (COMPILE_FLAGS "-O0 -g -fno-inline -Wall  -funsigned-char -std=c++17 -fno-omit-frame-pointer -pedantic -mssse3 ")
endif()

include_directories("${PROJECT_SOURCE_DIR}/")  # framer.hpp

add_definitions(${COMPILE_FLAGS})

file(GLOB_RECURSE SOURCES "examples/hello-world/hello-world.cc" "framer.hpp")

add_executable(frame_example ${SOURCES})

target_link_libraries(frame_example
        PRIVATE
        PkgConfig::FFMPEG
)

file(GLOB_RECURSE EXAMPLE_SOURCES "**.cc")

clangformat_setup(${SOURCES} "framer.hpp" ${EXAMPLE_SOURCES})