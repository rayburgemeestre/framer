cmake_minimum_required(VERSION 3.10)

project(video-with-audio-sdl)

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

# Add SDL2 packages
find_package(SDL2 REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)

include_directories("${PROJECT_SOURCE_DIR}/../../")  # framer.hpp
include_directories(${SDL2_INCLUDE_DIRS})

add_executable(video-with-audio-sdl "video-with-audio-sdl.cc")
target_link_libraries(video-with-audio-sdl
    PRIVATE
    PkgConfig::FFMPEG
    SDL2::SDL2
    SDL2::SDL2main
)
