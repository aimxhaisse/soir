# MAETHSTRO L I V E
#
# A sad Makefile that can't actually compile: Abseil does not support
# simple unix linking without being a nightmare. It instead mimmics
# what we'd expect from a stupid simple Makefile, calling cmake when
# needed.

BUILD_DIR	:= build
BIN_DIR		:= bin
BINARY		:= $(BIN_DIR)/maethstro

DEPS_DIR 	:= deps
DEPS_ABSEIL	:= $(DEPS_DIR)/abseil

.PHONY: all deps clean full-clean $(BINARY)

# Commands

all: $(BINARY)

deps: $(DEPS_ABSEIL)

clean:
	rm -rf $(BINARY)

full-clean: clean
	rm -rf $(DEPS_ABSEIL) $(BUILD_DIR)

# Build

$(BUILD_DIR):
	mkdir -p $@

$(BINARY): deps $(BUILD_DIR)
	cd $(BUILD_DIR) && \
	cmake -DABSL_PROPAGATE_CXX_STD=ON .. && \
	cmake --build . --target maethstro && \
	cp maethstro ../$(BINARY)

# Deps

$(DEPS_ABSEIL):
	git clone https://github.com/abseil/abseil-cpp.git $@ && \
	cd $@ && \
	mkdir build && \
	cd build && \
	cmake -DABSL_BUILD_TESTING=ON -DABSL_USE_GOOGLETEST_HEAD=ON -DCMAKE_CXX_STANDARD=20 -DABSL_PROPAGATE_CXX_STD=ON .. && \
	cmake --build . --target all
