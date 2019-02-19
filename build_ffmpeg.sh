set -o errexit
set -o pipefail
set -o nounset
set -o xtrace

# NOTE use update-alternatives to configure for example clang++-7, etc.

typeset ARCH=
typeset UBUNTU15=
typeset CENTOS7=

apt update
apt install -y lsb-release || true

mkdir -p /usr/local/src/framer/libs

if [[ $(lsb_release -a | grep -i Ubuntu) ]]; then
    UBUNTU15=true
    ARCH=UBUNTU15
#elif [[ $CENTOS7 == true ]]; then
else
    CENTOS7=true
    ARCH=CENTOS7
fi

echo preparing $ARCH

if [[ $UBUNTU15 == true ]]; then
	if [[ $(lsb_release -a | grep -i xenial ) ]]; then
		(wget https://www.nasm.us/pub/nasm/releasebuilds/2.13.01/nasm-2.13.01.tar.gz
		 tar xzvf nasm-2.13.01.tar.gz 
		 cd nasm-2.13.01
		 ./configure --prefix=/usr
		 make
		 sudo make install
		 # export PATH=/opt/nasm/bin/:$PATH
		 cd ..)
	else
		sudo apt-get install -y yasm
		sudo apt-get install -y nasm # apparently x264 switched to this
	fi
else
    yum install -y autoconf automake cmake freetype-devel gcc gcc-c++ git libtool make mercurial nasm pkgconfig zlib-devel
    git clone --depth 1 git://github.com/yasm/yasm.git && \
        cd yasm && \
        autoreconf -fiv && \
        CXX=$(which c++) ./configure && \
        make && \
        make install && \
        make distclean && cd ..
fi

# create temporary folder for building x264 and ffmpeg
mkdir -p tmp
cd tmp

# clone x264, build, install
if ! [[ -d x264 ]]; then
    git clone git://git.videolan.org/x264.git
fi
cd x264
git checkout 2451a7282463f68e532f2eee090a70ab139bb3e7 # parent of 71ed44c7312438fac7c5c5301e45522e57127db4, which is where they dropped support for:
#libavcodec/libx264.c:282:9: error: 'x264_bit_depth' undeclared (first use in this function); did you mean 'x264_picture_t'?
#     if (x264_bit_depth > 8)
#                  ^~~~~~~~~~~~~~

./configure --enable-static --enable-shared
make -j 8 && sudo make install

# we also want it in our libs dir.
./configure --prefix=/usr/local/src/framer/libs --enable-static --enable-shared
make -j 8 && sudo make install
cd ../

# clone ffmpeg, build, install
if ! [[ -d ffmpeg ]]; then
    git clone git://source.ffmpeg.org/ffmpeg.git
fi
cd ffmpeg
#git checkout 71052d85c16bd65fa1e3e01d9040f9a3925efd7a # or my muxing code won't work, they've modified the example since fba1592f35501bff0f28d7885f4128dfc7b82777
git checkout n3.1
gx=$(which c++)
#./configure --cxx=$gx --enable-shared --disable-swresample --enable-libx264 --enable-gpl
./configure --prefix=/usr/local/src/framer/libs --cxx=$gx --enable-shared --enable-libx264 --enable-gpl
make -j 4
sudo make install
cd ../
#END

cd ../ # leave "tmp"

