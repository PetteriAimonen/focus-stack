
# Compilation flags
CXX ?= g++
CXXFLAGS ?= -Og -g3 -ggdb -Wall -Wextra -Wno-sign-compare
CXXFLAGS += --std=c++14
LDFLAGS += -lpthread -lm
LDFLAGS += -lopencv_video -lopencv_imgcodecs -lopencv_photo -lopencv_imgproc -lopencv_core

# List of source code files
CXXSRCS += focusstack.cc main.cc worker.cc options.cc
CXXSRCS += task_align.cc task_grayscale.cc task_loadimg.cc
CXXSRCS += task_merge.cc task_reassign.cc task_saveimg.cc
CXXSRCS += task_wavelet.cc

# Generate list of object file and dependency file names
OBJS = $(CXXSRCS:%.cc=build/%.o)
DEPS := $(OBJS:%.o=%.d)

all: build build/focus-stack

build:
	mkdir -p build

clean:
	rm -rf build

-include $(DEPS)

build/focus-stack: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.cc
	$(CXX) $(CXXFLAGS) -MMD -c -o $@ $<
