cmake_minimum_required(VERSION 3.10)

project(hello-world-static)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(hello-world-static "hello-world-static.cc")

target_link_libraries(hello-world-static "${PROJECT_SOURCE_DIR}/ffmpeg/libswscale/libswscale.a")
target_link_libraries(hello-world-static "${PROJECT_SOURCE_DIR}/ffmpeg/libavdevice/libavdevice.a")
target_link_libraries(hello-world-static "${PROJECT_SOURCE_DIR}/ffmpeg/libavformat/libavformat.a")
target_link_libraries(hello-world-static "${PROJECT_SOURCE_DIR}/ffmpeg/libavcodec/libavcodec.a")
target_link_libraries(hello-world-static "${PROJECT_SOURCE_DIR}/ffmpeg/libavutil/libavutil.a")
target_link_libraries(hello-world-static "${PROJECT_SOURCE_DIR}/ffmpeg/libavfilter/libavfilter.a")
target_link_libraries(hello-world-static "${PROJECT_SOURCE_DIR}/ffmpeg/libswresample/libswresample.a")

target_link_libraries(hello-world-static "/usr/lib/x86_64-linux-gnu/libz.a")
target_link_libraries(hello-world-static "${PROJECT_SOURCE_DIR}/x264/libx264.a")
target_link_libraries(hello-world-static -ldl)
target_link_libraries(hello-world-static "/usr/lib/x86_64-linux-gnu/libdl.a")
target_link_libraries(hello-world-static "/usr/lib/x86_64-linux-gnu/liblzma.a")
target_link_libraries(hello-world-static "/usr/lib/x86_64-linux-gnu/libbz2.a")

include_directories("${PROJECT_SOURCE_DIR}/../../")  # framer.hpp
include_directories("${PROJECT_SOURCE_DIR}/ffmpeg")