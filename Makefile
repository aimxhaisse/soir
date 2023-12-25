# MAETHSTRO L I V E

DEPS 		:= deps

DEPS_GTEST_DIR  := $(DEPS)/gtest
DEPS_GTEST 	:= $(DEPS)/gtest/build/lib/libgtest.a

DEPS_CXXFLAGS	:= -I$(DEPS)/gtest/googletest/include
DEPS_LDFLAGS	:= -L$(DEPS)/gtest/build/lib

ALL_DEPS 	:= $(DEPS_GTEST)

CXXFLAGS = -O3 -Wall -Wno-unused-local-typedef -Wno-deprecated-declarations -std=c++17 $(shell freetype-config --cflags 2>/dev/null) -I. -I/usr/local/include/google/protobuf $(DEPS_CXXFLAGS)
LDLIBS = -lglog -lgflags -lgrpc++ -lprotobuf -lyaml-cpp -lpthread $(DEPS_LDFLAGS)

BINARY := build/maethstro
TEST   := build/maethstro_test
CXX    := clang++
FMT    := clang-format
PBUF   := protoc

SRCS_MAETHSTRO := $(filter-out $(wildcard src/main.cc src/*/*test.cc), $(wildcard src/main.cc src/*/*.cc))
OBJS_MAETHSTRO := $(SRCS_MAETHSTRO:.cc=.o)
DEPS_MAETHSTRO := $(OBJS_MAETHSTRO:.o=.d)

SRCS_TEST := $(filter-out server/main.cc, $(wildcard common/*/*.cc))
OBJS_TEST := $(SRCS_TEST:.cc=.o)
DEPS_TEST := $(OBJS_TEST:.o=.d)

SRCS_PB := $(wildcard proto/*.proto)
GENS_PB := $(SRCS_PB:.proto=.pb.cc)
OBJS_PB := $(SRCS_PB:.proto=.pb.o)
HEAD_PB := $(SRCS_PB:.proto=.pb.h) $(SRCS_PB:.proto=.pb.d)

SRCS_GRPC := $(wildcard proto/*.proto)
GENS_GRPC := $(SRCS_GRPC:.proto=.grpc.pb.cc)
OBJS_GRPC := $(SRCS_GRPC:.proto=.grpc.pb.o)
HEAD_GRPC := $(SRCS_GRPC:.proto=.grpc.pb.h) $(SRCS_GRPC:.proto=.grpc.pb.d)

GTEST_VERSION 	:= v1.10.x

.PHONY: all fmt clean test

help:
	@echo "Help for Maethstro:"
	@echo ""
	@echo "make             # this message"
	@echo "make all         # build everything"
	@echo "make fmt         # format code"
	@echo "make clean       # clean all build artifacts"
	@echo "make test        # run unit tests"
	@echo ""

all: $(MAETHSTRO) $(TEST)

fmt:
	$(FMT) -i -style Chromium $(SRCS_SERVER) $(SRCS_CLIENT) $(SRCS_COMMON)

clean:
	rm -rf $(OBJS_MAETHSTRO) $(DEPS_MAETHSTRO) $(MAETHSTRO)
	rm -rf $(OBJS_TEST) $(DEPS_TEST) $(TEST)
	rm -rf $(GENS_PB) $(OBJS_PB) $(HEAD_PB)
	rm -rf $(GENS_GRPC) $(OBJS_GRPC) $(HEAD_GRPC)

bin:
	mkdir -p bin

build:
	mkdir -p build

%.o: %.cc $(ALL_DEPS)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

%.grpc.pb.cc: %.proto
	$(PBUF) --grpc_out=. --plugin=protoc-gen-grpc=$(shell which grpc_cpp_plugin) $<

%.pb.cc: %.proto
	$(PBUF) --cpp_out=. $<

%.pb.o : %.pb.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(MAETHSTRO): build $(GENS_PB) $(GENS_GRPC) $(OBJS_MAETHSTRO) $(OBJS_PB) $(OBJS_GRPC)
	$(CXX) $(OBJS_MAETHSTRO) $(OBJS_PB) $(OBJS_GRPC) $(LDLIBS) -o $@

$(TEST): build $(GENS_PB) $(GENS_GRPC) $(OBJS_GRPC) $(OBJS_TEST) $(OBJS_PB)
	$(CXX) $(OBJS_TEST) $(OBJS_PB) $(OBJS_GRPC) $(LDLIBS) -lgtest -o $@

$(DEPS):
	mkdir -p $@

$(DEPS_GTEST_DIR): $(DEPS)
	git clone https://github.com/google/googletest.git $@
	touch $(DEPS)/*
	cd $(DEPS_GTEST_DIR) && git checkout origin/$(GTEST_VERSION) -b $(GTEST_VERSION)

$(DEPS_GTEST): $(DEPS_GTEST_DIR)
	cd $(DEPS_GTEST_DIR) && mkdir -p build && cd build && cmake .. && make
	touch $@

test: $(TEST)
	$(TEST)

-include $(DEPS_MAETHSTRO)
-include $(DEPS_TEST)
