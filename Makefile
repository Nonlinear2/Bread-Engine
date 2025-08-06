VERSION := $(shell cat version.txt)
DEFAULT_NAME := bread_engine_$(VERSION).exe
EXE ?= $(DEFAULT_NAME)
CXX ?= clang++

BUILD_DIR := makefile-build

.PHONY: all clean

bread_engine: $(EXE)

uci_search: $(BUILD_DIR)/Makefile
	$(MAKE) -C $(BUILD_DIR) CXX=$(CXX) uci_search

search_position: $(BUILD_DIR)/Makefile
	$(MAKE) -C $(BUILD_DIR) CXX=$(CXX) search_position

data_gen: $(BUILD_DIR)/Makefile
	$(MAKE) -C $(BUILD_DIR) CXX=$(CXX) data_gen

all: bread_engine uci_search search_position data_gen

$(EXE): $(BUILD_DIR)/$(DEFAULT_NAME)
	cp $(BUILD_DIR)/$(DEFAULT_NAME) $(EXE)

$(BUILD_DIR)/$(DEFAULT_NAME): $(BUILD_DIR)/Makefile
	$(MAKE) -C $(BUILD_DIR) CXX=$(CXX) bread_engine

$(BUILD_DIR)/Makefile: CMakeLists.txt
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake -G "Unix Makefiles" -DCMAKE_CXX_COMPILER=$(CXX) -DCMAKE_BUILD_TYPE=Release ..

clean:
	rm -rf $(BUILD_DIR)