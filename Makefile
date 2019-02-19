SHELL:=/bin/bash

.PHONY: libs
libs:
	bash build_shell.sh "bash build_ffmpeg.sh"

.PHONY: example
example:
	mkdir -p build && cd build && cmake .. && make -j $$(nproc) && cp -prv example ../

.PHONY: clean
clean:
	rm -rf build test.m3u8* example
