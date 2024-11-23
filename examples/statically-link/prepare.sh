#!/bin/bash

set -ex

rm -rfv x264 ffmpeg ffmpeg_out x264_out

git clone -b stable https://code.videolan.org/videolan/x264
git clone -b n7.1 https://github.com/FFmpeg/FFmpeg ffmpeg

apt-get update

sudo apt-get install -y yasm
sudo apt-get install -y nasm # apparently x264 switched to this

mkdir -p x264_out
mkdir -p ffmpeg_out

pushd x264
    ./configure --cxx=$(which c++) --enable-static --enable-shared --prefix=$(realpath $PWD/../x264_out)
    nice make -j 8 # $(nproc)
    make install

    ./configure --cxx=$(which c++) --enable-static --enable-shared
    nice make -j 8 # $(nproc)
    make install
popd

pushd ffmpeg
    ./configure --cxx=$(which c++) --enable-static --enable-shared --enable-libx264 --enable-gpl --enable-swresample --disable-libdrm --prefix=$(realpath $$PWD/../ffmpeg_out)
    nice make -j 8 # $(nproc)
    make install
popd