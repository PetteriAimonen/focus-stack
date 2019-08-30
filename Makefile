CXX ?= g++
CXXFLAGS ?= -O2 -g3 -ggdb -Wall -Wextra -Wno-sign-compare
CXXFLAGS += --std=c++14

CXXSRCS += focusstack.cc main.cc

OBJS = $(CXXSRCS:%.cc=build/%.o)
DEPS := $(OBJS:%.o=%.d)

all: build build/focus-stack

build:
	mkdir -p build

clean:
	rm -rf build

-include $(DEPS)

build/focus-stack: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

build/%.o: src/%.cc
	$(CXX) $(CXXFLAGS) -MMD -c -o $@ $<
