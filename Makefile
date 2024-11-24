SHELL:=/bin/bash

.PHONY: format
format:
	cmake --build build --target clangformat

.PHONY: clean
clean:
	rm -rfv examples/*/*.mp4
	rm -rfv examples/*/*.ts
	rm -rfv examples/*/*.m3u8
	rm -rfv examples/*/CMakeCache.txt
	rm -rfv examples/*/CMakeFiles
	rm -rfv examples/*/cmake-build-debug
	rm -rfv examples/*/.idea
	rm -rfv examples/*/Makefile
	rm -rfv examples/*/cmake_install.cmake
	rm -rfv examples/hello-world/hello-world
	rm -rfv examples/statically-link/hello-world
	rm -rfv examples/video-hls-stream-realtime/video-hls-stream-realtime
	rm -rfv examples/video-hls-stream/video-hls-stream
	rm -rfv examples/video-with-audio-sfml/video-with-sfml
	rm -rfv examples/video-with-audio/video-with-audio
	rm -rfv examples/video-with-sfml/video-with-sfml
	rm -rfv examples/video-without-audio/video-without-audio

.PHONY: smoke-test
smoke-test:
	make clean
	./smoke-test.sh
