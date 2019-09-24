-include Makefile.local

# Default compilation flags, these can be overridden in Makefile.local
# or in environment like: CXXFLAGS=... make
CXX ?= g++
CXXFLAGS ?= -Og -g3 -ggdb -Wall -Wextra -Wno-sign-compare

# Try to get opencv path from pkg-config
CXXFLAGS += $(shell pkg-config --cflags-only-I opencv)
LDFLAGS += $(shell pkg-config --libs-only-L opencv)

# Required compilation options
CXXFLAGS += --std=c++14
LDFLAGS += -lpthread -lm
LDFLAGS += -lopencv_video -lopencv_imgcodecs -lopencv_photo -lopencv_imgproc -lopencv_core

CXXFLAGS += -DGIT_VERSION=\"$(shell git describe --always --dirty)\"

# List of source code files
CXXSRCS += focusstack.cc worker.cc options.cc
CXXSRCS += task_align.cc task_grayscale.cc task_loadimg.cc
CXXSRCS += task_merge.cc task_reassign.cc task_saveimg.cc
CXXSRCS += task_wavelet.cc task_wavelet_opencl.cc task_denoise.cc

# Generate list of object file and dependency file names
OBJS = $(CXXSRCS:%.cc=build/%.o)
DEPS := $(OBJS:%.o=%.d)

# List of unit test files
TESTSRCS += task_grayscale_tests.cc
TESTSRCS += task_wavelet_tests.cc
TESTSRCS += task_wavelet_opencl_tests.cc

TESTOBJS = $(TESTSRCS:%.cc=build/%.o)
TESTDEPS := $(TESTOBJS:%.o=%.d)

all: build build/focus-stack

run_unittests: build/unittests
	build/unittests

build:
	mkdir -p build

clean:
	rm -rf build

install: all
	install build/focus-stack /usr/bin/
	gzip -c docs/focus-stack.1 > /usr/share/man/man1/focus-stack.1.gz

-include $(DEPS)
-include $(TESTDEPS)

build/focus-stack: src/main.cc $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.cc
	$(CXX) $(CXXFLAGS) -MMD -c -o $@ $<

build/unittests: src/gtest_main.cc $(OBJS) $(TESTOBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lgtest $(LDFLAGS)
