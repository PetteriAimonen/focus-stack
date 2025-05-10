-include Makefile.local

# Default compilation flags, these can be overridden in Makefile.local
# or in environment like: CXXFLAGS=... make
CXX ?= g++
CXXFLAGS ?= -O2 -g3 -ggdb -Wall -Wextra -Wno-sign-compare
DESTDIR ?=
prefix ?= /usr/local

# Try to get opencv path from pkg-config
CXXFLAGS += $(shell pkg-config --silence-errors --cflags-only-I opencv4 || pkg-config --cflags-only-I opencv)
LDFLAGS += $(shell pkg-config --silence-errors --libs-only-L opencv4 || pkg-config --silence-errors --cflags-only-I opencv)

# Required compilation options
CXXFLAGS += --std=c++14
LDFLAGS += -lpthread -lm
LDFLAGS += -lopencv_video -lopencv_imgcodecs -lopencv_photo -lopencv_imgproc -lopencv_core

VERSION = $(shell git describe --always)
CXXFLAGS += -DGIT_VERSION=\"$(shell git describe --always --dirty 2>/dev/null)\"

# List of source code files
CXXSRCS += focusstack.cc worker.cc options.cc logger.cc
CXXSRCS += radialfilter.cc histogrampercentile.cc
CXXSRCS += task_3dpreview.cc
CXXSRCS += task_align.cc task_background_removal.cc task_denoise.cc
CXXSRCS += task_depthmap.cc task_depthmap_inpaint.cc task_focusmeasure.cc
CXXSRCS += task_grayscale.cc task_loadimg.cc
CXXSRCS += task_merge.cc task_reassign.cc task_saveimg.cc
CXXSRCS += task_wavelet.cc task_wavelet_opencl.cc

# Generate list of object file and dependency file names
OBJS = $(CXXSRCS:%.cc=build/%.o)
DEPS := $(OBJS:%.o=%.d)

# List of unit test files
TESTSRCS += task_grayscale_tests.cc
TESTSRCS += task_wavelet_tests.cc
TESTSRCS += task_wavelet_opencl_tests.cc
TESTSRCS += radialfilter_tests.cc

TESTOBJS = $(TESTSRCS:%.cc=build/%.o)
TESTDEPS := $(TESTOBJS:%.o=%.d)

$(shell mkdir -p build)

all: build/focus-stack
	which ronn && make update_docs || true

update_docs: docs/focus-stack.1 docs/focus-stack.html

run_unittests: build/unittests
	build/unittests

run_tests: build/focus-stack
	build/focus-stack --align-keep-size --output=build/pcb.jpg examples/pcb/pcb*.jpg
	idiff -fail 0.1 -failpercent 1 -warnpercent 100 build/pcb.jpg examples/pcb/expected.jpg

clean:
	rm -rf build
	mkdir -p build

install: all
	mkdir -p "$(DESTDIR)$(prefix)/bin/"
	install build/focus-stack "$(DESTDIR)$(prefix)/bin/focus-stack"
	mkdir -p "$(DESTDIR)$(prefix)/share/man/man1/"
	gzip -c docs/focus-stack.1 > "$(DESTDIR)$(prefix)/share/man/man1/focus-stack.1.gz"

make_debuild:
	rm -rf DEBUILD
	mkdir -p DEBUILD
	git archive --prefix=focus-stack.orig/ --format=tar.gz HEAD > DEBUILD/focus-stack_$(VERSION).orig.tar.gz
	(cd DEBUILD && tar xzf focus-stack_$(VERSION).orig.tar.gz && cp -pr focus-stack.orig focus-stack)

builddeb: make_debuild
	(cd DEBUILD/focus-stack && dch -v $(VERSION) "Git build" && debuild -uc -us -b)

builddeb_signed: make_debuild
	(cd DEBUILD/focus-stack && debuild -S)

docs/focus-stack.1: docs/focus-stack.md
	ronn --pipe --roff $< > $@

docs/focus-stack.html: docs/focus-stack.md
	ronn --pipe --html $< > $@

-include $(DEPS)
-include $(TESTDEPS)

build/focus-stack: src/main.cc $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.cc
	$(CXX) $(CXXFLAGS) -MMD -c -o $@ $<

build/unittests: src/gtest_main.cc $(OBJS) $(TESTOBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lgtest $(LDFLAGS)

# Mac OS X application bundle
build/focus-stack.app: build/focus-stack packaging/macosx/focus-stack-gui.scpt packaging/macosx/Info.plist
	rm -rf "$@"
	osacompile -o "$@" packaging/macosx/focus-stack-gui.scpt
	mv "$@/Contents/MacOS/droplet" "$@/Contents/MacOS/focus-stack-gui"
	mv "$@/Contents/Resources/droplet.rsrc" "$@/Contents/Resources/focus-stack-gui.rsrc"
	sed "s/VERSION/$(VERSION)/g" <"packaging/macosx/Info.plist" >"$@/Contents/Info.plist"
	cp "packaging/macosx/PkgInfo" "$@/Contents"
	cp "build/focus-stack" "$@/Contents/MacOS"
	dylibbundler -x "$@/Contents/MacOS/focus-stack" -d "$@/Contents/libs" -od -b

distrib/focus-stack_MacOSX.zip: build/focus-stack.app
	rm -rf distrib
	mkdir -p distrib
	mkdir distrib/focus-stack
	cp -pr build/focus-stack.app distrib/focus-stack
	cp -pr examples distrib/focus-stack
	cp README.md distrib/focus-stack/README.txt
	cp LICENSE.md distrib/focus-stack/LICENSE.txt
	cp docs/focus-stack.html distrib/focus-stack
	cd distrib; zip -r focus-stack_MacOSX.zip focus-stack

# Linux AppImage
build/linuxdeploy:
	wget -O $@ https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
	chmod +x $@

build/focus-stack.AppImage: build/focus-stack build/linuxdeploy
	mkdir -p build/AppDir
	mkdir -p build/AppDir/usr/share/applications/
	mkdir -p build/AppDir/usr/share/icons/hicolor/scalable/apps/
	prefix=/usr DESTDIR=build/AppDir make install
	cp packaging/focus-stack.desktop build/AppDir/usr/share/applications/
	touch build/AppDir/usr/share/icons/hicolor/scalable/apps/focus-stack.svg
	VERSION=$(VERSION) build/linuxdeploy --appdir build/AppDir --output appimage
	mv *.AppImage $@
