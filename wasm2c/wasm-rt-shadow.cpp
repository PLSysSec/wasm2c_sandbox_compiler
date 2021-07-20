// Runtime functions for shadow memory

#include "wasm-rt.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>

enum class MEMORY_STATE: uint8_t {
  UNINIT = 0,
  ALLOCED = 1,
  INITIALIZED = 2,
};

struct wasm2c_shadow_memory_t {
  MEMORY_STATE* data;
  std::map<uint32_t, uint32_t> allocation_sizes;
};

static inline wasm2c_shadow_memory_t* construct_shadow_memory() {
  wasm2c_shadow_memory_t* ret = new wasm2c_shadow_memory_t;
  // allocate 8gb and one page as this should be sufficient for all cases
  ret->data = (MEMORY_STATE*) calloc(0x200010000ull, sizeof(MEMORY_STATE));
  return ret;
}

void destroy_shadow_memory(wasm2c_shadow_memory_t* ret) {
  free(ret->data);
  delete ret;
}

void wasm2c_shadow_memory_create(wasm_rt_memory_t* mem) {
  *(void**) &mem->data = construct_shadow_memory();
}


static inline wasm2c_shadow_memory_t* from_mem(wasm_rt_memory_t* mem) {
  return (wasm2c_shadow_memory_t*) mem->data;
}

void wasm2c_shadow_memory_destroy(wasm_rt_memory_t* mem) {
  free(from_mem(mem));
}

static inline void memory_state_check(wasm_rt_memory_t* mem, uint32_t ptr, uint32_t ptr_size, MEMORY_STATE state, const char* func_name) {
  MEMORY_STATE* heap_state = from_mem(mem)->data;

  for (uint32_t i = 0; i < ptr_size; i++) {
    uint64_t index = ptr;
    index += i;
    MEMORY_STATE curr_state = heap_state[index];
    if (!(curr_state == state)) {
      printf("WASM Shadow memory address sanitizer failed: Incorrect state! "
        "(Func: %s, Index: %" PRIu64 ", Expected: %d, Found: %d)!\n",
        func_name, index, (int) state, (int) curr_state);
      fflush(stdout);
#ifndef WASM_CHECK_SHADOW_MEMORY_NO_ABORT_ON_FAIL
      abort();
#endif
    }
  }
}

static inline void memory_state_not_check(wasm_rt_memory_t* mem, uint32_t ptr, uint32_t ptr_size, MEMORY_STATE state, const char* func_name) {
  MEMORY_STATE* heap_state = from_mem(mem)->data;

  for (uint32_t i = 0; i < ptr_size; i++) {
    uint64_t index = ptr;
    index += i;
    MEMORY_STATE curr_state = heap_state[index];
    if (!(curr_state != state)) {
      printf("WASM Shadow memory address sanitizer failed: Incorrect state! "
        "(Func: %s, Index: %" PRIu64 ", Expected not state: %d)!\n",
        func_name, index, (int) state);
      fflush(stdout);
#ifndef WASM_CHECK_SHADOW_MEMORY_NO_ABORT_ON_FAIL
      abort();
#endif
    }
  }
}

static inline void memory_state_transform(wasm_rt_memory_t* mem, uint32_t ptr, uint32_t ptr_size, MEMORY_STATE from_state, MEMORY_STATE to_state) {
  MEMORY_STATE* heap_state = from_mem(mem)->data;

  for (uint32_t i = 0; i < ptr_size; i++) {
    uint64_t index = ptr;
    index += i;

    if (heap_state[index] == from_state) {
      heap_state[index] = to_state;
    }
  }
}

static inline void memory_state_transform_set(wasm_rt_memory_t* mem, uint32_t ptr, uint32_t ptr_size, MEMORY_STATE state) {
  MEMORY_STATE* heap_state = from_mem(mem)->data;

  for (uint32_t i = 0; i < ptr_size; i++) {
    uint64_t index = ptr;
    index += i;
    heap_state[index] = state;
  }
}

void wasm2c_shadow_memory_check_load(wasm_rt_memory_t* mem, const char* func_name, uint32_t ptr, uint32_t ptr_size) {
  if (strcmp(func_name, "w2c_dlmalloc") == 0 ||
    strcmp(func_name, "w2c_dlfree") == 0
  ) {
    // these functions actually look at uninit memory
    return;
  }
#ifdef WASM_CHECK_SHADOW_MEMORY_UNINIT_READ
  memory_state_check(mem, ptr, ptr_size, MEMORY_STATE::INITIALIZED, func_name);
#else
  memory_state_not_check(mem, ptr, ptr_size, MEMORY_STATE::UNINIT, func_name);
#endif
}

void wasm2c_shadow_memory_check_store(wasm_rt_memory_t* mem, const char* func_name, uint32_t ptr, uint32_t ptr_size) {
  if (strcmp(func_name, "w2c_dlmalloc") == 0 ||
    strcmp(func_name, "w2c_dlfree") == 0
  ) {
    // these functions actually look at uninit memory
    return;
  }

  memory_state_not_check(mem, ptr, ptr_size, MEMORY_STATE::UNINIT, func_name);
  memory_state_transform(mem, ptr, ptr_size, MEMORY_STATE::ALLOCED, MEMORY_STATE::INITIALIZED);
}

void wasm2c_shadow_memory_check_global_reserve(wasm_rt_memory_t* mem, uint32_t ptr, uint32_t ptr_size) {
  memory_state_transform(mem, ptr, ptr_size, MEMORY_STATE::UNINIT, MEMORY_STATE::ALLOCED);
}

void wasm2c_shadow_memory_malloc(wasm_rt_memory_t* mem, uint32_t ptr, uint32_t ptr_size) {
  memory_state_check(mem, ptr, ptr_size, MEMORY_STATE::UNINIT, "MALLOC");
  memory_state_transform(mem, ptr, ptr_size, MEMORY_STATE::UNINIT, MEMORY_STATE::ALLOCED);
  std::map<uint32_t, uint32_t>* allocation_sizes = &(from_mem(mem)->allocation_sizes);
  (*allocation_sizes)[ptr] = ptr_size;
}

void wasm2c_shadow_memory_free(wasm_rt_memory_t* mem, uint32_t ptr) {
  std::map<uint32_t, uint32_t>* allocation_sizes = &(from_mem(mem)->allocation_sizes);
  auto it = allocation_sizes->find(ptr);
  if (it == allocation_sizes->end()) {
    printf("WASM Shadow memory address sanitizer failed: Incorrect free!\n");
    fflush(stdout);
#ifndef WASM_CHECK_SHADOW_MEMORY_NO_ABORT_ON_FAIL
    abort();
#endif
  } else {
    uint32_t ptr_size = it->second;
    memory_state_not_check(mem, ptr, ptr_size, MEMORY_STATE::UNINIT, "FREE");
    allocation_sizes->erase(it);

    memory_state_transform_set(mem, ptr, ptr_size, MEMORY_STATE::UNINIT);
  }
}

void wasm2c_shadow_memory_print_allocations(wasm_rt_memory_t* mem)
{
    std::map<uint32_t, uint32_t>* allocation_sizes = &(from_mem(mem)->allocation_sizes);

    puts("{ ");
    for (auto i = allocation_sizes->begin(); i != allocation_sizes->end(); ++i)
    {
      printf("%" PRIu32 ": %" PRIu32 "\n", i->first, i->second);
    }
    puts(" }");
}
