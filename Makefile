# Soir.
#
# Good old makefile, for now.

CXXFLAGS = -g -Wall -std=c++17 $(shell freetype-config --cflags) -Isrc
LDLIBS = -lglog -lgflags -lyaml-cpp -lsfml-graphics -lsfml-window -lsfml-system -lrtmidi

PRGM := bin/soir
TEST := bin/soir_test
CXX  := clang++
FMT  := clang-format

SRCS := $(filter-out $(wildcard src/*_test.cc src/*/*_test.cc), $(wildcard src/*.cc src/*/*.cc))
OBJS := $(SRCS:.cc=.o)
DEPS := $(OBJS:.o=.d)

SRCS_TEST := $(filter-out src/main.cc, $(wildcard src/*.cc src/*/*.cc))
OBJS_TEST := $(SRCS_TEST:.cc=.o)
DEPS_TEST := $(OBJS_TEST:.o=.d)

.PHONY: all clean re test fmt

all: $(PRGM)

fmt:
	$(FMT) -i -style Chromium $(SRCS)

clean:
	rm -rf $(OBJS) $(DEPS) $(PRGM)
	rm -rf $(OBJS_TEST) $(DEPS_TEST) $(TEST)

re: clean all

$(PRGM): $(OBJS)
	$(CXX) $(OBJS) $(LDLIBS) -o $@

%.o: %.cc
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(TEST): $(OBJS_TEST)
	$(CXX) $(OBJS_TEST) $(LDLIBS) -lgtest -o $@

test: $(TEST)
	$(TEST)

-include $(DEPS)
-include $(DEPS_TEST)
