// Based on https://web.archive.org/web/20191012035921/http://nadeausoftware.com/articles/2012/01/c_c_tip_how_use_compiler_predefined_macros_detect_operating_system#BSD
// Check for any posix or unix OS
#if !defined(_WIN32) && (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))

#include "wasm-rt-os.h"

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include <unistd.h>
#include <sys/mman.h>

size_t os_getpagesize() {
    return getpagesize();
}

void * os_mmap(void *hint, size_t size, int prot, int flags)
{
    int map_prot = PROT_NONE;
    int map_flags = MAP_ANONYMOUS | MAP_PRIVATE;
    uint64_t request_size, page_size;
    uint8_t *addr;
    uint32_t i;

    page_size = (uint64_t)os_getpagesize();
    request_size = (size + page_size - 1) & ~(page_size - 1);

    if ((size_t)request_size < size)
        /* integer overflow */
        return NULL;

    if (request_size > 16 * (uint64_t)UINT32_MAX)
        /* At most 16 G is allowed */
        return NULL;

    if (prot & MMAP_PROT_READ)
        map_prot |= PROT_READ;

    if (prot & MMAP_PROT_WRITE)
        map_prot |= PROT_WRITE;

    if (prot & MMAP_PROT_EXEC)
        map_prot |= PROT_EXEC;

#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
#ifndef __APPLE__
    if (flags & MMAP_MAP_32BIT)
        map_flags |= MAP_32BIT;
#endif
#endif

    if (flags & MMAP_MAP_FIXED)
        map_flags |= MAP_FIXED;

    /* try 5 times */
    for (i = 0; i < 5; i ++) {
        addr = mmap(hint, request_size, map_prot, map_flags, -1, 0);
        if (addr != MAP_FAILED)
            break;
    }

    if (addr == MAP_FAILED)
        return NULL;

    return addr;
}

void os_munmap(void *addr, size_t size)
{
    uint64_t page_size = (uint64_t)os_getpagesize();
    uint64_t request_size = (size + page_size - 1) & ~(page_size - 1);

    if (addr) {
        if (munmap(addr, request_size)) {
            printf("os_munmap error addr:%p, size:0x%" PRIx64 ", errno:%d\n",
                      addr, request_size, errno);
        }
    }
}

int os_mprotect(void *addr, size_t size, int prot)
{
    int map_prot = PROT_NONE;
    uint64_t page_size = (uint64_t)os_getpagesize();
    uint64_t request_size = (size + page_size - 1) & ~(page_size - 1);

    if (!addr)
        return 0;

    if (prot & MMAP_PROT_READ)
        map_prot |= PROT_READ;

    if (prot & MMAP_PROT_WRITE)
        map_prot |= PROT_WRITE;

    if (prot & MMAP_PROT_EXEC)
        map_prot |= PROT_EXEC;

    return mprotect(addr, request_size, map_prot);
}

#endif