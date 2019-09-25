-include Makefile.local

# Default compilation flags, these can be overridden in Makefile.local
# or in environment like: CXXFLAGS=... make
CXX ?= g++
CXXFLAGS ?= -Og -g3 -ggdb -Wall -Wextra -Wno-sign-compare
DESTDIR ?=
prefix ?= /usr/local

# Try to get opencv path from pkg-config
CXXFLAGS += $(shell pkg-config --cflags-only-I opencv)
LDFLAGS += $(shell pkg-config --libs-only-L opencv)

# Required compilation options
CXXFLAGS += --std=c++14
LDFLAGS += -lpthread -lm
LDFLAGS += -lopencv_video -lopencv_imgcodecs -lopencv_photo -lopencv_imgproc -lopencv_core

VERSION = $(shell git describe)
CXXFLAGS += -DGIT_VERSION=\"$(shell git describe --always --dirty 2>/dev/null)\"

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
	install -D build/focus-stack $(DESTDIR)$(prefix)/bin/focus-stack
	mkdir -p $(DESTDIR)$(prefix)/share/man/man1/
	gzip -c docs/focus-stack.1 > $(DESTDIR)$(prefix)/share/man/man1/focus-stack.1.gz

builddeb:
	rm -rf DEBUILD
	mkdir -p DEBUILD
	git archive --prefix=focus-stack.orig/ --format=tar.gz HEAD > DEBUILD/focus-stack_$(VERSION).orig.tar.gz
	(cd DEBUILD && tar xzf focus-stack_$(VERSION).orig.tar.gz && cp -pr focus-stack.orig focus-stack)
	(cd DEBUILD/focus-stack && debuild)


-include $(DEPS)
-include $(TESTDEPS)

build/focus-stack: src/main.cc $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.cc
	$(CXX) $(CXXFLAGS) -MMD -c -o $@ $<

build/unittests: src/gtest_main.cc $(OBJS) $(TESTOBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lgtest $(LDFLAGS)
