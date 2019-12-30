SHELL:=/bin/bash

.PHONY: libs
libs:
	apt-get install -y libbz2-dev liblzma-dev libz-dev
	apt-get install -y libsfml-dev

.PHONY: example
example:
	#update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++-7 40
	#update-alternatives --install /usr/bin/cc cc /usr/bin/clang-7 40
	#update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-7 40
	#update-alternatives --install /usr/bin/clang clang /usr/bin/clang-7 40
	#update-alternatives --install /usr/bin/ld ld /usr/bin/ld.lld-7 40
	mkdir -p build && cd build && cmake .. && make -j $$(nproc) && cp -prv example ../

.PHONY: clean
clean:
	rm -rf build test.m3u8* example
