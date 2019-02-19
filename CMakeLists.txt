cmake_minimum_required(VERSION 3.0.0)

project(frame_example VERSION 0.1 LANGUAGES CXX)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Required on Ubuntu 18.04
set(PKG_CONFIG_EXECUTABLE "/usr/bin/pkg-config")

find_package(SFML 2.4 COMPONENTS graphics window system)
#find_package(FFMPEG COMPONENTS swscale avdevice avformat avcodec avutil avfilter swresample)
find_package(Threads)

message(STATUS ${SFML_LIBRARIES})
message(STATUS ${FFMPEG_LIBRARIES})

if (NOT DEBUG)
    set (COMPILE_FLAGS "-O3 -Wall -funsigned-char -std=c++17 -fno-omit-frame-pointer -pedantic -mssse3 -Wl,--strip-all -flto")
    #set (COMPILE_FLAGS "-O3 -funsigned-char -std=c++17 -fno-omit-frame-pointer -mssse3 -Wl,--strip-all -flto")
else()
    set (COMPILE_FLAGS "-O0 -g -fno-inline -Wall  -funsigned-char -std=c++17 -fno-omit-frame-pointer -pedantic -mssse3 ")
endif()

include_directories(${PROJECT_SOURCE_DIR}/libs/include/)

add_definitions(${COMPILE_FLAGS})

file(GLOB_RECURSE SOURCES "main.cpp")
add_executable(example ${SOURCES})
target_link_libraries(example ${FFMPEG_LIBRARIES})
target_link_libraries(example ${SFML_LIBRARIES})

# apt install libsfml-dev
# too old stuff on Ubuntu 18.04:
# sudo apt install -y libswscale-dev libavdevice-dev libavformat-dev libavcodec-dev libavutil-dev libavfilter-dev
#sudo apt remove -y libswscale-dev libavdevice-dev libavformat-dev libavcodec-dev libavutil-dev libavfilter-dev

# or do something statically like this..
#target_link_libraries(example "/home/trigen/projects/frame2/ffmpeg/libswscale/libswscale.a")
#target_link_libraries(example "/home/trigen/projects/frame2/ffmpeg/libavdevice/libavdevice.a")
#target_link_libraries(example "/home/trigen/projects/frame2/ffmpeg/libavformat/libavformat.a")
#target_link_libraries(example "/home/trigen/projects/frame2/ffmpeg/libavcodec/libavcodec.a")
#target_link_libraries(example "/home/trigen/projects/frame2/ffmpeg/libavutil/libavutil.a")
#target_link_libraries(example "/home/trigen/projects/frame2/ffmpeg/libavfilter/libavfilter.a")
#target_link_libraries(example "/home/trigen/projects/frame2/ffmpeg/libswresample/libswresample.a")
target_link_libraries(example "/home/trigen/projects/frame2/libs/lib/libswscale.a")
target_link_libraries(example "/home/trigen/projects/frame2/libs/lib/libavdevice.a")
target_link_libraries(example "/home/trigen/projects/frame2/libs/lib/libavformat.a")
target_link_libraries(example "/home/trigen/projects/frame2/libs/lib/libavcodec.a")
target_link_libraries(example "/home/trigen/projects/frame2/libs/lib/libavutil.a")
target_link_libraries(example "/home/trigen/projects/frame2/libs/lib/libavfilter.a")
target_link_libraries(example "/home/trigen/projects/frame2/libs/lib/libswresample.a")
target_link_libraries(example "/usr/lib/x86_64-linux-gnu/libz.a")
target_link_libraries(example "/home/trigen/projects/frame2/libs/lib/libx264.a")
target_link_libraries(example -ldl)
target_link_libraries(example "/usr/lib/x86_64-linux-gnu/libdl.a")
target_link_libraries(example "/usr/lib/x86_64-linux-gnu/liblzma.a")
target_link_libraries(example "/usr/lib/x86_64-linux-gnu/libbz2.a")

find_library(LIBZ NAMES libz.a PATHS /usr/lib /usr/local/lib /opt/local/lib /usr/lib/x86_64-linux-gnu)
#find_library(LIBX264 NAMES libx264.a PATHS /usr/lib /usr/local/lib /opt/local/lib /usr/lib/x86_64-linux-gnu)
find_library(LIBDLSTATIC NAMES libdl.a PATHS /usr/lib /usr/local/lib /opt/local/lib /usr/lib/x86_64-linux-gnu)
find_library(LIBLZMA NAMES liblzma.a PATHS /usr/lib /usr/local/lib /opt/local/lib /usr/lib/x86_64-linux-gnu)
find_library(LIBBZ2 NAMES libbz2.a PATHS /usr/lib /usr/local/lib /opt/local/lib /usr/lib/x86_64-linux-gnu)
find_library(LIBDLDYNAMIC NAMES dl PATHS /usr/lib /usr/local/lib /opt/local/lib /usr/lib/x86_64-linux-gnu)

# ??
#find_library(LIBX11 NAMES X11 PATHS /usr/lib /usr/local/lib /opt/local/lib /usr/lib/x86_64-linux-gnu)
#find_library(LIBXCB NAMES xcb PATHS /usr/lib /usr/local/lib /opt/local/lib /usr/lib/x86_64-linux-gnu)
#find_library(LIBVA NAMES vax PATHS /usr/lib /usr/local/lib /opt/local/lib /usr/lib/x86_64-linux-gnu)

target_link_libraries(example ${LIBZ})
#target_link_libraries(example ${LIBX264})
target_link_libraries(example ${LIBDLDYNAMIC})
#target_link_libraries(example ${LIBDLSTATIC})
target_link_libraries(example ${LIBLZMA})
target_link_libraries(example ${LIBBZ2})

target_link_libraries(example ${LIBX11})
target_link_libraries(example ${LIBXCB})
#target_link_libraries(example ${LIBVA})


#
## test example:
#target_link_libraries(example "/usr/lib/x86_64-linux-gnu/libpostproc.a")


target_link_libraries(example ${CMAKE_THREAD_LIBS_INIT})
