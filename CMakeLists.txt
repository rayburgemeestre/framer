cmake_minimum_required(VERSION 3.0.0)

project(frame_example VERSION 0.2 LANGUAGES CXX)

set(CLANGFORMAT_EXECUTABLE clang-format-15)
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include(ClangFormat)

set(PKG_CONFIG_EXECUTABLE "/usr/bin/pkg-config")

find_package(Threads)

message(STATUS ${SFML_LIBRARIES})

if (NOT DEBUG)
    set (COMPILE_FLAGS "-O3 -Wall -funsigned-char -std=c++17 -fno-omit-frame-pointer -pedantic -mssse3 -Wl,--strip-all -flto")
else()
    set (COMPILE_FLAGS "-O0 -g -fno-inline -Wall  -funsigned-char -std=c++17 -fno-omit-frame-pointer -pedantic -mssse3 ")
endif()

include_directories(/opt/cppse/build/ffmpeg/include/)

add_definitions(${COMPILE_FLAGS})

file(GLOB_RECURSE SOURCES "main.cpp")
add_executable(example ${SOURCES})

target_link_libraries(example "/opt/cppse/build/ffmpeg/lib/libswscale.a")
target_link_libraries(example "/opt/cppse/build/ffmpeg/lib/libavdevice.a")
target_link_libraries(example "/opt/cppse/build/ffmpeg/lib/libavformat.a")
target_link_libraries(example "/opt/cppse/build/ffmpeg/lib/libavcodec.a")
target_link_libraries(example "/opt/cppse/build/ffmpeg/lib/libavutil.a")
target_link_libraries(example "/opt/cppse/build/ffmpeg/lib/libavfilter.a")
target_link_libraries(example "/opt/cppse/build/ffmpeg/lib/libswresample.a")
target_link_libraries(example "/usr/lib/x86_64-linux-gnu/libz.a")
target_link_libraries(example "/opt/cppse/build/x264/lib/libx264.a")
target_link_libraries(example -ldl)
target_link_libraries(example "/usr/lib/x86_64-linux-gnu/libdl.a")
target_link_libraries(example "/usr/lib/x86_64-linux-gnu/liblzma.a")
target_link_libraries(example "/usr/lib/x86_64-linux-gnu/libbz2.a")
target_link_libraries(example -ldrm)

find_library(LIBZ NAMES libz.a PATHS /usr/lib /usr/local/lib /opt/local/lib /usr/lib/x86_64-linux-gnu)
find_library(LIBDLSTATIC NAMES libdl.a PATHS /usr/lib /usr/local/lib /opt/local/lib /usr/lib/x86_64-linux-gnu)
find_library(LIBLZMA NAMES liblzma.a PATHS /usr/lib /usr/local/lib /opt/local/lib /usr/lib/x86_64-linux-gnu)
find_library(LIBBZ2 NAMES libbz2.a PATHS /usr/lib /usr/local/lib /opt/local/lib /usr/lib/x86_64-linux-gnu)
find_library(LIBDLDYNAMIC NAMES dl PATHS /usr/lib /usr/local/lib /opt/local/lib /usr/lib/x86_64-linux-gnu)

target_link_libraries(example ${LIBZ})
target_link_libraries(example ${LIBDLDYNAMIC})
target_link_libraries(example ${LIBLZMA})
target_link_libraries(example ${LIBBZ2})
target_link_libraries(example ${LIBX11})
target_link_libraries(example ${LIBXCB})
target_link_libraries(example ${CMAKE_THREAD_LIBS_INIT})

file(GLOB_RECURSE EXAMPLE_SOURCES "**.cc")
clangformat_setup(${SOURCES} "framer.hpp" ${EXAMPLE_SOURCES})