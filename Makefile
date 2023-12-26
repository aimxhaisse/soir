# MAETHSTRO L I V E

DEPS_DIR 	:= deps
DEPS_ABSEIL	:= $(DEPS_DIR)/abseil

BIN_DIR 	:= bin
BINARY 		:= $(BIN_DIR)/maethstro

.PHONY: all deps clean

# Commands

all: $(BINARY)

deps: $(DEPS_ABSEIL)

clean:
	rm -rf $(DEPS_ABSEIL)

# Dependencies

$(DEPS_ABSEIL):
	git clone https://github.com/abseil/abseil-cpp.git $@ && \
	cd $@ && \
	mkdir build && \
	cd build && \
	cmake -DABSL_BUILD_TESTING=ON -DABSL_USE_GOOGLETEST_HEAD=ON -DCMAKE_CXX_STANDARD=20 .. && \
	cmake --build . --target all

# Build

$(BINARY): deps
