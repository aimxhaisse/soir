# Soir.
#
# Good old makefile, for now.

CXXFLAGS = -g -Wall -std=c++14 $(shell freetype-config --cflags) -Isrc
LDLIBS = -lglog -lgflags -lyaml-cpp -lsfml-graphics -lsfml-window -lsfml-system

PRGM := soir
CXX  := clang++
FMT  := clang-format
SRCS := $(wildcard src/*.cc src/*/*.cc)
OBJS := $(SRCS:.cc=.o)
DEPS := $(OBJS:.o=.d)

.PHONY: all clean

all: $(PRGM)

re: clean all

fmt:
	$(FMT) -i -style Chromium $(SRCS)

$(PRGM): $(OBJS)
	$(CXX) $(OBJS) $(LDLIBS) -o $@

%.o: %.cc
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

clean:
	rm -rf $(OBJS) $(DEPS) $(PRGM)

-include $(DEPS)
