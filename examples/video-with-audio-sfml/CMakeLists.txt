cmake_minimum_required(VERSION 3.10)

project(video-with-audio-sfml)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find FFMPEG packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavcodec
    libavformat
    libavutil
    libswscale
    libswresample
)

# Find SFML packages
find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

include_directories("${PROJECT_SOURCE_DIR}/../../")  # framer.hpp
add_executable(video-with-audio-sfml "video-with-audio-sfml.cc")
target_link_libraries(video-with-audio-sfml
    PRIVATE
    PkgConfig::FFMPEG
    sfml-graphics
    sfml-window
    sfml-system
    sfml-audio
)