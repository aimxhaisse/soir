# NEON

TEST_FILTER 	?= "*"

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
DEPS_SDL	:= $(DEPS_DIR)/sdl

FASTBUILD	:= /fast

.PHONY: all deps clean full-clean $(BINARY) www docs

# Commands

all: $(BINARY)

deps: $(DEPS_EFSW) $(DEPS_PYBIND) $(DEPS_HTTPLIB) $(DEPS_LIBREMIDI) $(DEPS_AUDIOFILE) $(DEPS_GRPC) $(DEPS_RAPIDJSON) $(DEPS_SDL)

clean:
	rm -f $(BINARY)
	cd $(BUILD_DIR) && make clean

docs:
	$(BINARY) --config etc/mkdocs.yaml --mode script --script scripts/mk-docs.py

push: docs
	scp -r www/site/* sbrk.org:services/neon.sbrk.org/data

serve: docs
	cd www/site && python -m http.server 4096

full-clean: clean
	rm -rf $(DEPS_EFSW) $(DEPS_PYBIND) $(DEPS_HTTPLIB) $(DEPS_LIBREMIDI) $(DEPS_AUDIOFILE) $(DEPS_GRPC) $(DEPS_RAPIDJSON) $(DEPS_SDL)

test: all
	./$(BUILD_DIR)/src/utils/neon_utils_test --gtest_filter=$(TEST_FILTER)
	./$(BUILD_DIR)/src/core/neon_core_test --gtest_filter=$(TEST_FILTER)

# Build

$(BUILD_DIR):
	mkdir -p $@

$(BIN_DIR):
	mkdir -p $@

$(BINARY): deps $(BUILD_DIR) $(BIN_DIR) $(VENV_DIR)
	cd $(BUILD_DIR) && \
	cmake -DCMAKE_BUILD_TYPE=Release \
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
	      --target 	neon$(FASTBUILD)			\
			neon_agent$(FASTBUILD)			\
			neon_core$(FASTBUILD)			\
			neon_utils$(FASTBUILD) 			\
			neon_utils_test$(FASTBUILD) 		\
			neon_core_test$(FASTBUILD) &&		\
	cp neon ../$(BINARY)


# Virtualenv

$(VENV_DIR):
	poetry install

# Deps

$(DEPS_GRPC):
	git clone https://github.com/grpc/grpc $@ && \
	cd $@ && \
	git checkout v1.60.0 && \
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

$(DEPS_SDL):
	git clone https://github.com/libsdl-org/SDL.git $@ && \
	cd $@ && \
	git checkout release-2.30.7
