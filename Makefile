SHELL:=/bin/bash

.PHONY: format
format:
	cmake --build build --target clangformat