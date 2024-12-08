# SOIR

TEST_FILTER 	?= "*"

BUILD_DIR	:= build
BIN_DIR		:= bin
BINARY		:= $(BIN_DIR)/soir

VENV_DIR	:= .venv

.PHONY: all deps clean full-clean $(BINARY) www docs

# Commands

all: $(BINARY)

clean:
	rm -f $(BINARY)
	cd $(BUILD_DIR) && make clean

docs:
	$(BINARY) --config etc/mkdocs.yaml --mode script --script scripts/mk-docs.py

push: docs
	rsync -av www/site/* sbrk.org:services/soir.sbrk.org/data

serve: docs
	cd www/site && python -m http.server 4096

test: all
	./$(BUILD_DIR)/src/utils/soir_utils_test --gtest_filter=$(TEST_FILTER)
	./$(BUILD_DIR)/src/core/soir_core_test --gtest_filter=$(TEST_FILTER)

# Build

$(BUILD_DIR):
	mkdir -p $@

$(BIN_DIR):
	mkdir -p $@

$(BINARY): $(BUILD_DIR) $(BIN_DIR) $(VENV_DIR)
	cd $(BUILD_DIR) && \
	cmake -DCMAKE_BUILD_TYPE=Release 			\
		.. && 						\
	cmake --build . -j 16 					\
	      --target 	soir					\
	cp soir ../$(BINARY)


# Virtualenv

$(VENV_DIR):
	poetry install
