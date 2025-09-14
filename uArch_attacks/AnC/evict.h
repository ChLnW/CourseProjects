#pragma once
/* NOTE: This is just boilerplate code. Customize for your own needs. */

#include <stdint.h>

#define CACHE_LINE_SIZE 64

#define PAGE_SIZE (1<<12)
#define PAGE_SIZE_1G (((size_t) 1)<<30)

#define PML1_SIZE (4*1536*PAGE_SIZE)
#define PML2_SIZE ((size_t)128*1024*1024)

/* initialize your eviction buffer(s) */
extern int init_evict_buf();
extern void evict_cache(int set);
extern void evict_tlb();
/* extern void evict_both(); */
extern unsigned long reload_t(void *addr);
extern void cacheFlush(char *pages, void *target, uint64_t bufferSize, uint64_t intervalSize, int mask);