# MAETHSTRO L I V E

# Variables

DEPS_DIR 	:= deps
DEPS_ABSEIL	:= $(DEPS_DIR)/abseil

BIN_DIR 	:= bin
BINARY 		:= $(BIN_DIR)/maethstro

SRCS_LIVE 	:= $(filter-out $(wildcard src/*test.cc src/*/*test.cc), $(wildcard src/*.cc src/*/*.cc))
OBJS_LIVE 	:= $(SRCS_LIVE:.cc=.o)
DEPS_LIVE 	:= $(OBJS_LIVE:.o=.d)

CXXFLAGS 	:= -O3 -Wall -Wno-unused-local-typedef -Wno-deprecated-declarations -std=c++20 -I.

.PHONY: all deps clean

# Commands

all: $(BINARY)

deps: $(DEPS_ABSEIL)

clean:
	rm -rf $(OBJS_LIVE) $(DEPS_LIVE) $(BINARY)

full-clean: clean
	rm -f $(DEPS_ABSEIL)

# Dependencies

$(DEPS_ABSEIL):
	git clone https://github.com/abseil/abseil-cpp.git $@ && \
	cd $@ && \
	mkdir build && \
	cd build && \
	cmake -DABSL_BUILD_TESTING=ON -DABSL_USE_GOOGLETEST_HEAD=ON -DCMAKE_CXX_STANDARD=20 .. && \
	cmake --build . --target all

# Build

$(BINARY): deps $(OBJS_LIVE)
	clang++ $(OBJS_LIVE) -o $@

%.o: %.cc
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@
