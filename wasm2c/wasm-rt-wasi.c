// This file contains modified versions of the emscripten runtime (as well as the wasi support it includes ) adapted for
//    - library sandboxing --- these apis have to support multiple wasm2c sandboxed components and cannot use global variables.
//                             Instead they operate on the sbx context specified as the first parameter to the API
//    - compatibility --- APIs like setjmp/longjmp are implemented in a very limited way upstream. Expanding this.
//    - security --- APIs like args_get, sys_open, fd_write, sys_read seemed to have security bugs upstream.
//                   Additionally, we don't want to allow any file system access. Restrict opens to the null file (/dev/null on unix, nul on windows)

/////////////////////////////////////////////////////////////
////////// File: Missing stuff from emscripten
/////////////////////////////////////////////////////////////
#include <stdint.h>
#include "wasm-rt-os.h"

typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint64_t u64;
typedef int64_t s64;
typedef float f32;
typedef double f64;

#ifndef WASM_RT_ADD_PREFIX
#define WASM_RT_ADD_PREFIX
#endif

/////////////////////////////////////////////////////////////
////////// File: Modified emscripten/tools/wasm2c/base.c
/////////////////////////////////////////////////////////////

/*
 * Base of all support for wasm2c code.
 */

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ssize_t detection: usually stdint provides it, but not on windows apparently
#ifdef _WIN32
#ifdef _MSC_VER
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#else // _MSC_VER
#ifdef _WIN64
typedef signed long long ssize_t;
#else // _WIN64
typedef signed long ssize_t;
#endif // _WIN64
#endif // _MSC_VER
#endif // _WIN32

#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#ifndef _WIN32
#include <unistd.h>
#else
#include <io.h>
#endif

#include "wasm-rt.h"
#include "wasm-rt-impl.h"

void wasm_rt_sys_init() {
  os_init();
}

VmCtx* wasm_rt_init_wasi(wasm_rt_memory_t* mem, wasm2c_rt_init_data *init_data) {
  return veriwasi_init(mem->data, mem->size, init_data->homedir, init_data->args, init_data-> argc, init_data->env, init_data->envc, init_data->log_path, init_data->netlist);
}

void wasm_rt_cleanup_wasi(VmCtx* ctx) {
  // os_clock_cleanup(&(wasi_data->clock_data));
   veriwasi_cleanup(ctx);
}
