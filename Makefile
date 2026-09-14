VERSION := $(shell cat version.txt)

ifeq ($(OS),Windows_NT)
    SUFFIX := .exe
else
    SUFFIX :=
endif

DEFAULT_NAME := bread_engine_$(VERSION)$(SUFFIX)
EXE ?= $(DEFAULT_NAME)
CXX := clang++
CXXFLAGS ?=
ARCH ?= native

BUILD_DIR := makefile-build

.PHONY: all clean

bread_engine: $(EXE)

native: ARCH := native
# microarchitecture level corresponding to avx2
avx2:   ARCH := x86-64-v3
avx512: ARCH := x86-64-v4

native avx2 avx512: bread_engine

uci_search: $(BUILD_DIR)/Makefile
	$(MAKE) -C $(BUILD_DIR) CXX=$(CXX) uci_search

search_position: $(BUILD_DIR)/Makefile
	$(MAKE) -C $(BUILD_DIR) CXX=$(CXX) search_position

all: bread_engine uci_search search_position

$(EXE): $(BUILD_DIR)/$(DEFAULT_NAME)
	cp $(BUILD_DIR)/$(DEFAULT_NAME) $(EXE)

$(BUILD_DIR)/$(DEFAULT_NAME): $(BUILD_DIR)/Makefile
	$(MAKE) -C $(BUILD_DIR) CXX=$(CXX) bread_engine

$(BUILD_DIR)/Makefile: CMakeLists.txt
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake -G "Unix Makefiles" \
		-DCMAKE_CXX_COMPILER=$(CXX) \
		-DCMAKE_CXX_FLAGS="$(CXXFLAGS)" \
		-DCMAKE_BUILD_TYPE=Release .. \
		-Dbread_ARCH=${ARCH}

clean:
	rm -rf $(BUILD_DIR)