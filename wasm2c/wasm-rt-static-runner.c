#if defined(_WIN32)
// Remove warnings for strcat, strcpy as they are safely used here
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wasm-rt.h"

#if defined(_WIN32)
#define LINETERM "\r\n"
#else
#define LINETERM "\n"
#endif

void wasi_rt_sys_init();
wasm2c_sandbox_funcs_t get_wasm2c_sandbox_info();
void w2c__start(void* sbx);

int main(int argc, char const* argv[]) {
  char const* wasm_module_name = "";

  if (argc >= 3) {
    wasm_module_name = argv[1];
  }

  wasm_rt_sys_init();

  wasm2c_sandbox_funcs_t sandbox_info = get_wasm2c_sandbox_info();

  const uint32_t dont_override_heap_size = 0;
  void* sandbox = sandbox_info.create_wasm2c_sandbox(dont_override_heap_size);
  if (!sandbox) {
    printf("Error: Could not create sandbox" LINETERM);
    exit(1);
  }

  w2c__start(sandbox);

  sandbox_info.destroy_wasm2c_sandbox(sandbox);

  return 0;
}
