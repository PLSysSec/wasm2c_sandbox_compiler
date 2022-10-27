MKFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
MKFILE_DIR := $(dir $(MKFILE_PATH))
WABT_SRC_ROOT=$(shell realpath $(MKFILE_DIR)/../../..)
WASI_SDK_ROOT=/opt/bin/wasi-sdk

WABT_BIN_CHOICE=build_debug
WABT_BIN_TYPE=Debug
WASM_SECURITY_FLAGS=

WABT_BIN_ROOT=$(WABT_SRC_ROOT)/$(WABT_BIN_CHOICE)

WASI_CLANG=$(WASI_SDK_ROOT)/bin/clang
WASI_SYSROOT=$(WASI_SDK_ROOT)/share/wasi-sysroot

#CFLAGS for compiling files to play nice with wasm2c and hfi emulation
WASM_CFLAGS=-Wl,--export-all -Wl,--global-base=150000 -Wl,-z,stack-size=1048576 -Wl,--growable-table

WASM2C=$(WABT_BIN_ROOT)/wasm2c

WASM2C_RUNTIME_PATH=$(WABT_SRC_ROOT)/wasm2c/
WASM2C_RUNTIME_FILES=$(addprefix $(WASM2C_RUNTIME_PATH), wasm-rt-impl.c wasm-rt-os-unix.c wasm-rt-wasi.c wasm-rt-static-runner.c)
WASM2C_SHADOW_RUNTIME_FILES=$(addprefix $(WASM2C_RUNTIME_PATH), wasm-rt-shadow.cpp)

UVWASI_PATH=$(WABT_SRC_ROOT)/third_party/uvwasi

INCS=-I$(WASM2C_RUNTIME_PATH)
DEPS=

LIBS=-lpthread -lrt

all: wasmrt hello

$(WABT_BIN_ROOT):
	cmake -S $(WABT_SRC_ROOT) -B $(WABT_BIN_ROOT) -DCMAKE_BUILD_TYPE=$(WABT_BIN_TYPE)

wasmrt: $(WABT_BIN_ROOT)
	cd $(WABT_SRC_ROOT) && make update-wasm2c
	cd $(WABT_BIN_ROOT) && make -j$(shell nproc)

clean:
	rm -rf hello.wasm hello.wasm.c hello.wasm.h hello *.o

hello.wasm: hello.c
	$(WASI_CLANG) --sysroot $(WASI_SYSROOT) $(WASM_CFLAGS) hello.c -o hello.wasm

hello.wasm.c: hello.wasm $(WASM2C)
	$(WASM2C) hello.wasm -o hello.wasm.c

hello: hello.wasm.c $(WASM2C_RUNTIME_FILES)
	$(CXX) -g -c $(WASM_SECURITY_FLAGS) $(WASM2C_SHADOW_RUNTIME_FILES) $(INCS)
	$(CC)  -g -c $(WASM_SECURITY_FLAGS) hello.wasm.c $(WASM2C_RUNTIME_FILES) $(INCS)
	$(CXX) -g $(WASM_SECURITY_FLAGS) *.o -o hello $(DEPS) $(LIBS)

run: hello
	$(PWD)/hello

run-gdb: hello
	gdb --args $(PWD)/hello

run-rr: hello
	rr record $(PWD)/hello
