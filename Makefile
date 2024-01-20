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
DEPS_PROTOBUF	:= $(DEPS_DIR)/protobuf
DEPS_EFSW	:= $(DEPS_DIR)/efsw
DEPS_PYBIND	:= $(DEPS_DIR)/pybind11
DEPS_HTTPLIB	:= $(DEPS_DIR)/httplib
DEPS_LIBREMIDI	:= $(DEPS_DIR)/libremidi
DEPS_AUDIOFILE	:= $(DEPS_DIR)/audiofile

.PHONY: all deps clean full-clean $(BINARY)

# Commands

all: $(BINARY)

deps: $(DEPS_ABSEIL) $(DEPS_PROTOBUF) $(DEPS_EFSW) $(DEPS_PYBIND) $(DEPS_HTTPLIB) $(DEPS_LIBREMIDI) $(DEPS_AUDIOFILE)

clean:
	rm -rf $(BINARY)
	cd $(BUILD_DIR) && make clean

full-clean: clean
	rm -rf $(DEPS_ABSEIL) $(DEPS_PROTOBUF) $(DEPS_EFSW) $(DEPS_PYBIND) $(DEPS_HTTPLIB) $(DEPS_LIBREMIDI) $(DEPS_AUDIOFILE)

test: all
	./$(BUILD_DIR)/src/common/maethstro_common_test
	./$(BUILD_DIR)/src/matin/maethstro_matin_test
	./$(BUILD_DIR)/src/midi/maethstro_midi_test
	./$(BUILD_DIR)/src/soir/maethstro_soir_test

# Build

$(BUILD_DIR):
	mkdir -p $@

$(BINARY): deps $(BUILD_DIR)
	cd $(BUILD_DIR) && \
	cmake \
		-Dprotobuf_ABSL_PROVIDER=package	\
		-DABSL_PROPAGATE_CXX_STD=ON  		\
		-DABSL_BUILD_TESTING=ON 		\
	        -DABSL_BUILD_TEST_HELPERS=ON		\
	        -DABSL_USE_EXTERNAL_GOOGLETEST=ON  	\
		-DBUILD_SHARED_LIBS=OFF			\
		.. && \
	cmake --build . 				\
		-j 16 	 				\
		--target maethstro maethstro_matin_test maethstro_midi_test maethstro_soir_test maethstro_common_test && \
	cp maethstro ../$(BINARY)

# Deps

$(DEPS_ABSEIL):
	git clone https://github.com/abseil/abseil-cpp.git $@

$(DEPS_PROTOBUF):
	git clone https://github.com/protocolbuffers/protobuf.git $@ && \
	cd $@ && \
	git checkout v25.1 && \
	git submodule update --init --recursive

$(DEPS_EFSW):
	git clone https://github.com/SpartanJ/efsw.git $@ && \
	cd $@ && \
	git checkout 1.3.1

$(DEPS_PYBIND):
	git clone https://github.com/pybind/pybind11.git $@ && \
	cd $@ && \
	git checkout v2.11.1

$(DEPS_HTTPLIB):
	git clone https://github.com/yhirose/cpp-httplib.git $@ && \
	cd $@ && \
	git checkout v0.14.3

$(DEPS_LIBREMIDI):
	git clone https://github.com/jcelerier/libremidi.git $@ && \
	cd $@ && \
	git checkout v4.2.4

$(DEPS_AUDIOFILE):
	git clone https://github.com/adamstark/AudioFile.git $@ && \
	cd $@ && \
	git checkout 1.1.1
