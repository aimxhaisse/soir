# NEON

BUILD_DIR	:= build
BIN_DIR		:= bin
BINARY		:= $(BIN_DIR)/neon

VENV_DIR	:= .venv

DEPS_DIR 	:= deps
DEPS_GRPC 	:= $(DEPS_DIR)/grpc
DEPS_EFSW	:= $(DEPS_DIR)/efsw
DEPS_PYBIND	:= $(DEPS_DIR)/pybind11
DEPS_HTTPLIB	:= $(DEPS_DIR)/httplib
DEPS_LIBREMIDI	:= $(DEPS_DIR)/libremidi
DEPS_AUDIOFILE	:= $(DEPS_DIR)/audiofile
DEPS_RAPIDJSON	:= $(DEPS_DIR)/rapidjson

.PHONY: all deps clean full-clean $(BINARY) www

# Commands

all: $(BINARY)

deps: $(DEPS_EFSW) $(DEPS_PYBIND) $(DEPS_HTTPLIB) $(DEPS_LIBREMIDI) $(DEPS_AUDIOFILE) $(DEPS_GRPC) $(DEPS_RAPIDJSON)

clean:
	rm -f $(BINARY)
	cd $(BUILD_DIR) && make clean

full-clean: clean
	rm -rf $(DEPS_EFSW) $(DEPS_PYBIND) $(DEPS_HTTPLIB) $(DEPS_LIBREMIDI) $(DEPS_AUDIOFILE) $(DEPS_GRPC) $(DEPS_RAPIDJSON)

test: all
	./$(BUILD_DIR)/src/utils/neon_utils_test
	./$(BUILD_DIR)/src/core/neon_core_test

# Build

$(BUILD_DIR):
	mkdir -p $@

$(BIN_DIR):
	mkdir -p $@

$(BINARY): deps $(BUILD_DIR) $(BIN_DIR) $(VENV_DIR)
	cd $(BUILD_DIR) && \
	cmake -DCMAKE_BUILD_TYPE=Debug \
		-Dprotobuf_ABSL_PROVIDER=package		\
		-DgRPC_INSTALL=ON				\
		-DABSL_ENABLE_INSTALL=ON			\
		-DABSL_PROPAGATE_CXX_STD=ON  			\
		-DABSL_BUILD_TESTING=ON 			\
	        -DABSL_BUILD_TEST_HELPERS=ON			\
	        -DABSL_USE_EXTERNAL_GOOGLETEST=ON  		\
		-DBUILD_SHARED_LIBS=OFF				\
		-DPYTHON_EXECUTABLE=$(VENV_DIR)/bin/python	\
		-DPYBIND11_FINDPYTHON=ON			\
		.. && 						\
	cmake --build . -j 16 					\
	      --target neon neon_agent neon_core neon_utils neon_utils_test neon_core_test && \
	cp neon ../$(BINARY)


# Virtualenv

$(VENV_DIR):
	poetry install

# Deps

$(DEPS_GRPC):
	git clone https://github.com/grpc/grpc $@ && \
	cd $@ && \
	git checkout v1.62.1 && \
	git submodule update --init --recursive

$(DEPS_EFSW):
	git clone https://github.com/SpartanJ/efsw.git $@ && \
	cd $@ && \
	git checkout 1.3.1

$(DEPS_PYBIND):
	git clone https://github.com/pybind/pybind11.git $@ && \
	cd $@ && \
	git checkout v2.12.0

$(DEPS_HTTPLIB):
	git clone https://github.com/yhirose/cpp-httplib.git $@ && \
	cd $@ && \
	git checkout v0.14.3

$(DEPS_LIBREMIDI):
	git clone https://github.com/jcelerier/libremidi.git $@ && \
	cd $@ && \
	git checkout v4.5.0

$(DEPS_AUDIOFILE):
	git clone https://github.com/adamstark/AudioFile.git $@ && \
	cd $@ && \
	git checkout 1.1.1

$(DEPS_RAPIDJSON):
	git clone https://github.com/Tencent/rapidjson.git $@ && \
	cd $@ && \
	git checkout v1.1.0
