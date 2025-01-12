#ifndef sys_h
#define sys_h
#include <stdbool.h>

#ifdef WIN32
#include "mem.h"
#include "windows_specific.h"

static inline
bool sys_init(void) {
	return !EnableLargePagePrivilege();
}

static inline
void* sys_alloc(size_t const size) {
	size_t const min_size  = GetLargePageMinimum();
	void* buffer = NULL;
	if ((buffer = VirtualAlloc(NULL, round_to_alignment(size, min_size), MEM_LARGE_PAGES|MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE)))
		return buffer;
	if ((buffer = VirtualAlloc(NULL, round_to_alignment(size, min_size), MEM_COMMIT, PAGE_READWRITE)))
		return buffer;
	return NULL;
}

#else

static inline
bool sys_init(void) { return false; }

static inline
void* sys_alloc(size_t const size) {
	return malloc(size);
}

#endif

#endif