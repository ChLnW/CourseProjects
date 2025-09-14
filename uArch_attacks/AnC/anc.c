/* NOTE: This is just boilerplate code. Customize for your own needs. */
#include "compiler.h"
#include "evict.h"
#include <err.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

/* Have a look at what these flags do in the manual. You can use whatever flags
 * you like though. */
#define MMAP_FLAGS MAP_POPULATE | MAP_ANON | MAP_FIXED_NOREPLACE | MAP_PRIVATE
#define PROT_RW PROT_READ | PROT_WRITE

#define TARGET_SIZE (9*PAGE_SIZE_1G)

uint64_t findPML1(uint8_t *target_address);
uint64_t findPML2(uint8_t *target_address);
uint64_t findPML(uint8_t *target_address);

static void *target_address;

void write_csv_line(FILE *file, const int pml, const unsigned long *values,
                    int size) {

  if (!file) {
    exit(1);
  }

  fprintf(file, "pml%d,", pml);

  for (int i = 0; i < size; ++i) {
    fprintf(file, "%lu", values[i]);
    if (i < size - 1) {
      fprintf(file, ",");
    }
  }

  fprintf(file, "\n");
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("usage: %s 0xADDRESS\n", argv[0]);
    return 1;
  }
  target_address = _ptr(strtoul(argv[1], 0, 0));
  if (mmap(target_address, TARGET_SIZE, PROT_RW, MMAP_FLAGS, -1, 0) ==
      MAP_FAILED) {
    err(2, "mmap(0x%012lx)", _ul(target_address));
  }
  printf("target: %p\n", target_address);
  if (init_evict_buf() != 0) {
    printf("eviction buffer allocation failed");
    exit(1);
  }

  // const int round = 10;
  // unsigned long times[64], maxs[64];

  // unsigned long time1 = 0;
  // uint8_t *adr;
  // FILE *file = fopen("pml.csv", "w");

  // for (int pml = 1; pml <= 2; pml++) {
  //   int pml_offset = 1 << (3 + 9 * pml); // 4KB 2MB 1GB 512GB
  //   for (int offset = 0; offset <= 0; ++offset) {
  //     for (int set = 0; set < 64; ++set) {
  //       times[set] = 0;
  //       maxs[set] = 0;
  //     }
  //     adr = (uint8_t *)target_address;
  //     // moving 32KB for PML1 staircase
  //     adr = &adr[offset * 8 * pml_offset];
  //     for (int i = 0; i < round; ++i) {
  //       for (int set = 0; set < 64; ++set) {
  //         asm __volatile__("mov (%0), %%rax    \n\t" : : "c"(adr) : "%rax");
  //         int idx = ((11 + 2 * i) + (13 + 2 * i) * set) % 64;
  //         // cacheFlush(NULL, (void *)(uint64_t)(idx * CACHE_LINE_SIZE), PML1_SIZE,
  //         //            PAGE_SIZE, 0);
  //         cacheFlush(NULL, (void *)(uint64_t)(idx * CACHE_LINE_SIZE), PML2_SIZE,
  //                    PAGE_SIZE, 0);
  //         time1 = reload_t(adr);
  //         times[idx] += time1;
  //         if (time1 > maxs[idx])
  //           maxs[idx] = time1;
  //       }
  //     }

  //     // printf("off=%d", offset);
  //     // for (int i = 0; i < 10; ++i) {
  //     //   printf("\t%d", i);
  //     // }
  //     // for (int i = 0; i < 64; ++i) {
  //     //   if (i % 10 == 0) {
  //     //     printf("\n%d", i / 10);
  //     //   }
  //     //   times[i] = (times[i] - maxs[i]) / (round - 1);
  //     //   printf("\t%ld", times[i]);
  //     // }
  //     // printf("\n\n");

  //     for (int i = 0; i < 64; ++i) {
  //       times[i] = (times[i] - maxs[i]) / (round - 1);
  //     }
  //     write_csv_line(file, pml, times, 64);
  //   }
  // }

  // fclose(file);
  // unsigned long result;

  // unsigned long mask = (1 << 6) - 1;
  // unsigned long a = result >> 15;

  // for (int i = 0; i < 4; ++i) {
  //   printf("%ld:%ld|", mask & a, times[mask & a]);
  //   a >>= 9;
  // }
  // printf("\n%lx\n", a);

  // unsigned long sum1 = 0;
  // for (int i = 0; i < 64; ++i) {
  //   sum1 += times[i];
  // }
  // printf("avg: %ld\n", sum1 / 64);

  unsigned long mask = (1 << 6) - 1, mask2 = (1 << 3) - 1;

  unsigned long a = ((uint64_t)target_address) >> 12;
  for (int i = 0; i < 4; ++i) {
    printf("%lx-", a & mask2);
    a >>= 3;
    // printf("->%lx", a);
    printf("%ld|", mask & a);
    a >>= 6;
  }
  printf("\n");

  /* Note: The grading script will look for the last line printed by your
   * program that matches this. */
  // result = findPML1(target_address) | findPML2(target_address);
  // printf("PAGE=0x%012lx\n", result);

  a = findPML(target_address);
  printf("PAGE=0x%012lx\n", a);

  // int res = system("python3 plot.py");
  // if (res) {
  //   printf("Failed to graph\n");
  //   return 1;
  // }

  return 0;
}

uint64_t findPML1(uint8_t *target_address) {
  const int round = 10, convSize = 8;
  unsigned long time1 = 0;
  unsigned long times[64], maxs[64];
  uint8_t *adr;

  unsigned long convolution[64];
  unsigned long threshold = 0;

  for (int set = 0; set < 64; ++set) {
    convolution[set] = 0;
  }

  for (int offset = 0; offset < convSize; ++offset) {
    for (int set = 0; set < 64; ++set) {
      times[set] = 0;
      maxs[set] = 0;
    }
    // moving 32KB for PML1 staircase
    adr = &target_address[(offset * 8) * PAGE_SIZE];
    for (int i = 0; i < round; ++i) {
      for (int set = 0; set < 64; ++set) {
        asm __volatile__("mov (%0), %%rax    \n\t" : : "c"(adr) : "%rax");
        int idx = ((11 + 2 * i) + (13 + 2 * i) * set) % 64;
        cacheFlush(NULL, (void *)(uint64_t)(idx * CACHE_LINE_SIZE), PML1_SIZE,
                   PAGE_SIZE, 0);
        // cacheFlush(NULL, (void *)(uint64_t)(idx * CACHE_LINE_SIZE),
        // PML2_SIZE,
        //             PAGE_SIZE, 0);
        time1 = reload_t(adr);
        times[idx] += time1;
        if (time1 > maxs[idx])
          maxs[idx] = time1;
      }
    }
    for (int i = 0; i < 64; ++i) {
      times[i] = (times[i] - maxs[i]) / (round - 1);
      convolution[(64 + i - offset) % 64] += times[i];
      threshold += times[i];
    }
  }

  threshold /= (64 * convSize);

  unsigned long max = 0, argmax = 0;

  for (int set = 0; set < 64; ++set) {
    if (convolution[set] > max) {
      max = convolution[set];
      argmax = set;
    }
  }

  // binary search
  unsigned int idx = 0;
  for (unsigned int offset = 4; offset > 0; offset /= 2) {
    adr = &target_address[(idx + offset) * PAGE_SIZE];
    time1 = 0;
    max = 0;
    for (int i = 0; i < round; ++i) {
      asm __volatile__("mov (%0), %%rax    \n\t" : : "c"(adr) : "%rax");
      cacheFlush(NULL, (void *)(uint64_t)(argmax * CACHE_LINE_SIZE), PML1_SIZE,
                 PAGE_SIZE, 0);
      // cacheFlush(NULL, (void *)(uint64_t)(idx * CACHE_LINE_SIZE), PML2_SIZE,
      //             PAGE_SIZE, 0);
      uint64_t tmp = reload_t(adr);
      time1 += tmp;
      if (tmp > max)
        max = tmp;
    }
    time1 = (time1 - max) / (round - 1);
    if (time1 > threshold) {
      idx += offset;
    }
  }

  idx = 7 - idx;
  return (argmax << 15) | (idx << 12);
}

uint64_t findPML2(uint8_t *target_address) {
  const int round = 10, convSize = 8;
  unsigned long time1 = 0;
  unsigned long times[64], maxs[64];
  uint8_t *adr;

  unsigned long convolution[64];
  unsigned long threshold = 0;

  for (int set = 0; set < 64; ++set) {
    convolution[set] = 0;
  }

  for (int offset = 0; offset < convSize; ++offset) {
    for (int set = 0; set < 64; ++set) {
      times[set] = 0;
      maxs[set] = 0;
    }
    // moving 2MB for PML2 staircase
    adr = &target_address[(offset * 8) * (PAGE_SIZE << 9)];
    for (int i = 0; i < round; ++i) {
      for (int set = 0; set < 64; ++set) {
        asm __volatile__("mov (%0), %%rax    \n\t" : : "c"(adr) : "%rax");
        int idx = ((11 + 2 * i) + (13 + 2 * i) * set) % 64;
        cacheFlush(NULL, (void *)(uint64_t)(idx * CACHE_LINE_SIZE), PML2_SIZE,
                   PAGE_SIZE, 0);
        time1 = reload_t(adr);
        times[idx] += time1;
        if (time1 > maxs[idx])
          maxs[idx] = time1;
      }
    }
    for (int i = 0; i < 64; ++i) {
      times[i] = (times[i] - maxs[i]) / (round - 1);
      convolution[(64 + i - offset) % 64] += times[i];
      threshold += times[i];
    }
  }

  threshold /= (64 * convSize);

  unsigned long max = 0, argmax = 0;

  for (int set = 0; set < 64; ++set) {
    if (convolution[set] > max) {
      max = convolution[set];
      argmax = set;
    }
  }

  // binary search
  unsigned int idx = 0;
  for (unsigned int offset = 4; offset > 0; offset /= 2) {
    adr = &target_address[(idx + offset) * (PAGE_SIZE << 9)];
    time1 = 0;
    max = 0;
    for (int i = 0; i < round; ++i) {
      asm __volatile__("mov (%0), %%rax    \n\t" : : "c"(adr) : "%rax");
      cacheFlush(NULL, (void *)(uint64_t)(argmax * CACHE_LINE_SIZE), PML2_SIZE,
                 PAGE_SIZE, 0);
      uint64_t tmp = reload_t(adr);
      time1 += tmp;
      if (tmp > max)
        max = tmp;
    }
    time1 = (time1 - max) / (round - 1);
    // printf("time: %ld\n", time1);
    if (time1 > threshold) {
      idx += offset;
    }
  }

  idx = 7 - idx;
  // printf("threshold: %ld\nargmax: %ld\nidx: %d\n", threshold, argmax, idx);
  return (argmax << 24) | (idx << 21);
}

uint64_t findPML3(uint8_t *target_address) {
  const int round = 10, convSize = 2;
  unsigned long time1 = 0;
  unsigned long times[64], maxs[64];
  uint8_t *adr;

  unsigned long convolution[64];
  unsigned long threshold = 0;

  for (int set = 0; set < 64; ++set) {
    convolution[set] = 0;
  }

  for (int offset = 0; offset < convSize; ++offset) {
    for (int set = 0; set < 64; ++set) {
      times[set] = 0;
      maxs[set] = 0;
    }
    // moving 8GB for PML2 staircase
    adr = &target_address[(offset * 8) * PAGE_SIZE_1G];
    for (int i = 0; i < round; ++i) {
      for (int set = 0; set < 64; ++set) {
        asm __volatile__("mov (%0), %%rax    \n\t" : : "c"(adr) : "%rax");
        int idx = ((11 + 2 * i) + (13 + 2 * i) * set) % 64;
        cacheFlush(NULL, (void *)(uint64_t)(idx * CACHE_LINE_SIZE), PML2_SIZE,
                   PAGE_SIZE, 0);
        time1 = reload_t(adr);
        times[idx] += time1;
        if (time1 > maxs[idx])
          maxs[idx] = time1;
      }
    }
    for (int i = 0; i < 64; ++i) {
      times[i] = (times[i] - maxs[i]) / (round - 1);
      convolution[(64 + i - offset) % 64] += times[i];
      threshold += times[i];
    }
  }

  threshold /= (64 * convSize);

  unsigned long max = 0, argmax = 0;

  for (int set = 0; set < 64; ++set) {
    if (convolution[set] > max) {
      max = convolution[set];
      argmax = set;
    }
  }

  // binary search
  unsigned int idx = 0;
  for (unsigned int offset = 4; offset > 0; offset /= 2) {
    adr = &target_address[(idx + offset) * PAGE_SIZE_1G];
    time1 = 0;
    max = 0;
    for (int i = 0; i < round; ++i) {
      asm __volatile__("mov (%0), %%rax    \n\t" : : "c"(adr) : "%rax");
      cacheFlush(NULL, (void *)(uint64_t)(argmax * CACHE_LINE_SIZE), PML2_SIZE,
                 PAGE_SIZE, 0);
      uint64_t tmp = reload_t(adr);
      time1 += tmp;
      if (tmp > max)
        max = tmp;
    }
    time1 = (time1 - max) / (round - 1);
    // printf("time: %ld\n", time1);
    if (time1 > threshold) {
      idx += offset;
    }
  }

  idx = 7 - idx;
  // printf("threshold: %ld\nargmax: %ld\nidx: %d\n", threshold, argmax, idx);
  return (argmax << 33) | (idx << 30);
}

uint64_t findPML(uint8_t *target_address) {
  uint64_t evictSizes[] = {PML1_SIZE, PML2_SIZE};
  const int round = 10, convSize = 8;
  unsigned long time1 = 0;
  unsigned long times[64], maxs[64];
  uint8_t *adr;

  unsigned long convolution[64];
  unsigned long threshold = 0;

  uint64_t result = 0;
  for (int pmlevel = 1; pmlevel <= 2; ++pmlevel) {
    uint64_t stepSize = 1 << (3 + 9 * pmlevel);

    for (int set = 0; set < 64; ++set) {
      convolution[set] = 0;
    }

    threshold = 0;

    for (int offset = 0; offset < convSize; ++offset) {
      for (int set = 0; set < 64; ++set) {
        times[set] = 0;
        maxs[set] = 0;
      }
      adr = &target_address[(offset * 8) * stepSize];
      for (int i = 0; i < round; ++i) {
        for (int set = 0; set < 64; ++set) {
          asm __volatile__("mov (%0), %%rax    \n\t" : : "c"(adr) : "%rax");
          int idx = ((11 + 2 * i) + (13 + 2 * i) * set) % 64;
          cacheFlush(NULL, (void *)(uint64_t)(idx * CACHE_LINE_SIZE),
                     evictSizes[pmlevel - 1], PAGE_SIZE, 0);
          time1 = reload_t(adr);
          times[idx] += time1;
          if (time1 > maxs[idx])
            maxs[idx] = time1;
        }
      }
      for (int i = 0; i < 64; ++i) {
        times[i] = (times[i] - maxs[i]) / (round - 1);
        convolution[(64 + i - offset) % 64] += times[i];
        threshold += times[i];
      }
    }

    threshold /= (64 * convSize);

    unsigned long max = 0, argmax = 0;

    for (int set = 0; set < 64; ++set) {
      if (convolution[set] > max) {
        max = convolution[set];
        argmax = set;
      }
    }

    // binary search
    unsigned int idx = 0;
    for (unsigned int offset = 4; offset > 0; offset /= 2) {
      adr = &target_address[(idx + offset) * stepSize];
      time1 = 0;
      max = 0;
      for (int i = 0; i < round; ++i) {
        asm __volatile__("mov (%0), %%rax    \n\t" : : "c"(adr) : "%rax");
        cacheFlush(NULL, (void *)(uint64_t)(argmax * CACHE_LINE_SIZE),
                   evictSizes[pmlevel - 1], PAGE_SIZE, 0);
        uint64_t tmp = reload_t(adr);
        time1 += tmp;
        if (tmp > max)
          max = tmp;
      }
      time1 = (time1 - max) / (round - 1);
      // printf("time: %ld\n", time1);
      if (time1 > threshold) {
        idx += offset;
      }
    }

    idx = 7 - idx;

    result = result | ((idx | (argmax << 3)) << (3 + 9 * pmlevel));
  }
  // printf("threshold: %ld\nargmax: %ld\nidx: %d\n", threshold, argmax, idx);
  return result|findPML3(target_address);
}
