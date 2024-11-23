cmake_minimum_required(VERSION 3.10)

project(video-without-audio)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavcodec
    libavformat
    libavutil
    libswscale
    libswresample
)

include_directories("${PROJECT_SOURCE_DIR}/../../")  # framer.hpp
add_executable(video-without-audio "video-without-audio.cc")
target_link_libraries(video-without-audio
    PRIVATE
    PkgConfig::FFMPEG
)