# Log Sift convenience targets
#
# make            Configure and build
# make run        Build and launch Log Sift
# make test       Build and run tests
# make clean      Remove the build directory
# make rebuild    Clean, configure, and build again

BUILD_DIR ?= build
CMAKE ?= cmake
CTEST ?= ctest
GENERATOR ?= Ninja
BUILD_TYPE ?= Release

.PHONY: all configure build run test clean rebuild

all: build

configure:
	$(CMAKE) -S . -B "$(BUILD_DIR)" -G "$(GENERATOR)" -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

build: configure
	$(CMAKE) --build "$(BUILD_DIR)"

ifeq ($(OS),Windows_NT)
run: build
	"$(BUILD_DIR)/logsift.exe"
else
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
run: build
	open "$(BUILD_DIR)/Log Sift.app"
else
run: build
	"$(BUILD_DIR)/logsift"
endif
endif

test: build
	$(CTEST) --test-dir "$(BUILD_DIR)" --output-on-failure

clean:
	$(CMAKE) -E remove_directory "$(BUILD_DIR)"

rebuild: clean build
