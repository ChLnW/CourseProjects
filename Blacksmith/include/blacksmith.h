#pragma once
#include "bs_pattern.h"
#include <assert.h>

// Size of a DRAM row (for x8 DRAM devices) in KB.
#define DRAM_PAGE_SIZE 8192
#define DATA_PATTERN_LEN DRAM_PAGE_SIZE

struct blacksmith {
  /* Memory range used for hammering. */
  // was void*
  unsigned char *mem;
  /* Contig. memory block size: 1GB with superpages (A4), smaller otherwise
   * (A5). */
  long mem_block_sz;
  /* Total memory size: 1GB for A4, 256MB for A5 TASK 1. */
  long mem_sz;
  /* To be determined in A4 TASK 8. */
  int act_tREFI;
  /* DRAM addressing information. */
  dram_info_t *dram;
  /* The data pattern to be used to fill the to be hammered memory area. */
  uint8_t __attribute__((aligned(0x1000))) data_pattern[DATA_PATTERN_LEN];
};

typedef struct blacksmith blacksmith_t;

/* A4 TASK 6 */
blacksmith_t *init_blacksmith(unsigned char *mem, long mem_sz, long mem_block_sz,
                              dram_info_t *dram);

/* A4 TASK 6 */
bs_pattern_t *blacksmith_next();

/* A4 TASK 7 */
void hammer(bs_pattern_t *bs_ptrn);

/* A5 TASK 2 */
int bitflip_exploitable(bitflip_t *bf);
