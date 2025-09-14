/* NOTE: This is just boilerplate code. Customize for your own needs. */
#include "evict.h"
#include "compiler.h"
#include <stdint.h>
#include <x86intrin.h>
/** touching entries here to evict caches or TLBs */
__used static uint8_t *evict_buf;

int init_evict_buf() {
  if (posix_memalign((void **)&evict_buf, PML2_SIZE/2, PAGE_SIZE_1G*8)) {
    return -1;
  }
  // for (int64_t i = 0; i < EVICT_SIZE; i += PAGE_SIZE) {
  //   *(uint64_t *)&evict_buf[i] =
  //       i / PAGE_SIZE + 1; // linux allocates pages on write
  // }
  for (size_t j=0; j<PML2_SIZE; j+=PAGE_SIZE) {
    *(uint64_t *) &(evict_buf[j]) = j/PAGE_SIZE + 1; // linux allocates pages on write
  }
  for (size_t j=0; j<PML2_SIZE; j+=PAGE_SIZE) {
    *(uint64_t *) &(evict_buf[7*PAGE_SIZE_1G+j]) = j/PAGE_SIZE + 2; // linux allocates pages on write
  }
  return 0;
}

void cacheFlush(char *pages, void *target, uint64_t bufferSize,
                uint64_t intervalSize, int mask) {
  int offset = mask ? (uint64_t)target & mask : (uint64_t)target;
  if (pages == NULL)
    pages = (char *)evict_buf;
  char *adrs;

  if (bufferSize > PML2_SIZE)
    bufferSize = PML2_SIZE;

  for (size_t j=0; j<bufferSize; j+=intervalSize) {
    adrs = &(pages[j+offset]);
    asm __volatile__(
        "mov (%0), %%rax    \n\t" 
        :
        : "c" (adrs) 
        : "%rax");
  }
  for (size_t j=bufferSize-intervalSize; j<bufferSize; j-=intervalSize) {
    adrs = &(pages[j+offset]);
    asm __volatile__(
        "mov (%0), %%rax    \n\t" 
        :
        : "c" (adrs) 
        : "%rax");
  }
  for (size_t j=0; j<bufferSize; j+=intervalSize) {
    adrs = &(pages[j+offset]);
    asm __volatile__(
        "mov (%0), %%rax    \n\t" 
        :
        : "c" (adrs) 
        : "%rax");
  }
  if (bufferSize < PML2_SIZE)
    return;
  for (size_t j=0; j<bufferSize; j+=intervalSize) {
    adrs = &(pages[7*PAGE_SIZE_1G+j+offset]);
    asm __volatile__(
        "mov (%0), %%rax    \n\t" 
        :
        : "c" (adrs) 
        : "%rax");
  }
  for (size_t j=bufferSize-intervalSize; j<bufferSize; j-=intervalSize) {
    adrs = &(pages[7*PAGE_SIZE_1G+j+offset]);
    asm __volatile__(
        "mov (%0), %%rax    \n\t" 
        :
        : "c" (adrs) 
        : "%rax");
  }
  for (size_t j=0; j<bufferSize; j+=intervalSize) {
    adrs = &(pages[7*PAGE_SIZE_1G+j+offset]);
    asm __volatile__(
        "mov (%0), %%rax    \n\t" 
        :
        : "c" (adrs) 
        : "%rax");
  }

  // for (uint64_t i = 0; i < bufferSize; i += intervalSize) {
  //   adrs = &pages[(i) + offset];
  //   asm __volatile__("mov (%0), %%rax    \n\t" : : "c"(adrs) : "%rax");
  // }
  // for (uint64_t i = bufferSize - intervalSize; i >= bufferSize; i -= intervalSize) {
  //   adrs = &pages[(i) + offset];
  //   asm __volatile__("mov (%0), %%rax    \n\t" : : "c"(adrs) : "%rax");
  // }
  // for (uint64_t i = 0; i < bufferSize; i += intervalSize) {
  //   adrs = &pages[(i) + offset];
  //   asm __volatile__("mov (%0), %%rax    \n\t" : : "c"(adrs) : "%rax");
  // }
}

void evict_cache(int set) {
  int offset = set * CACHE_LINE_SIZE;
  cacheFlush((char *)evict_buf, (void *)(uint64_t)offset, PML2_SIZE, PAGE_SIZE,
             0);
}

void evict_tlb() {
  cacheFlush((char *)evict_buf, (void *)0, PML2_SIZE, PAGE_SIZE, 0);
}

// changling probe
unsigned long reload_t(void *adrs) {
  volatile unsigned long low1, low2, high1, high2, time;

  asm __volatile__("mfence              \n\t"
                   "lfence              \n\t"
                   "rdtsc               \n\t" // read pre-access time
                   "mov %%rax, %0       \n\t" // store time lower bits
                   "mov %%rdx, %1       \n\t" // store time higher bits
                   "lfence              \n\t"
                   "mov (%4), %%rax     \n\t" // memory access
                   "lfence              \n\t" // wait for memory access
                   "rdtsc               \n\t" // read post-access time
                   "mov %%rax, %2       \n\t" // store time lower bits
                   "mov %%rdx, %3       \n\t" // store time higher bits
                   : "=r"(low1), "=r"(high1), "=r"(low2), "=r"(high2)
                   : "c"(adrs) // rcx for adrs
                   : "%rax", "%rbx", "%rdx");

  time = ((high2 << 32) ^ low2) - ((high1 << 32) ^ low1);
  return time;
}

// unsigned int reload_t(void *addr) {
//   unsigned int cpuid;
//   unsigned long start_t, end_t;

//   _mm_mfence();
//   _mm_lfence();
//   start_t = __rdtscp(&cpuid);
//   _mm_lfence();
//   *(volatile unsigned char *)addr;
//   _mm_lfence();
//   end_t = __rdtscp(&cpuid);
//   _mm_lfence();

//   return end_t - start_t;
// }
