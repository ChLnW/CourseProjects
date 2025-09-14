#include "blacksmith.h"
#include "asm.h"
#include "bs_pattern.h"
#include "compiler.h"
#include "dram_info.h"
#include <cstring>
#include <err.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "drama.h"
/**
 * Blacksmith state struct.
 */
static blacksmith_t bs;

/**
 * Blacksmith pattern currently being tested.
 */
static bs_pattern_t cur_pattern;

#define HAMMER_ACTS 3000000
/* A4 TASK 7 hammer */
/* TODO: A4 TASK 8 hammer synchronized with REF */
void hammer(bs_pattern_t *ptrn) {
  size_t round = HAMMER_ACTS / ptrn->pattern_len + 1;
  unsigned long t, t1;
  mfence();
  for (size_t i = 0; i < round; i++) {
    for (int j = 0; j < ptrn->pattern_len; j+=ptrn->base_period) {
      t = rdtscp();
      for (int k = 0; k < ptrn->base_period; k+=2) {
        mfence();
        *(volatile char*)(ptrn->pattern[j+k]);
        *(volatile char*)(ptrn->pattern[j+k+1]);
        t1 = rdtscp();
        clflushopt(ptrn->pattern[j+k]);
        clflushopt(ptrn->pattern[j+k+1]);
        // if REF detected, skip to next iteration
        if (t1 - t > 1000)
          break;
        t = t1;
      }
    }
  }
}

/**
 * Randomly initializes the data pattern used by Blacksmith.
 *
 * @throws perror if opening /dev/urandom fails
 * @throws err if reading from /dev/urandom fails
 * @throws exit if opening /dev/urandom fails
 */
static void init_datapattern() {
  assert(bs.mem_sz % DATA_PATTERN_LEN == 0);
  int fd = open("/dev/urandom", O_RDONLY);
  if (fd < 0) {
    perror("/dev/urandom");
    exit(1);
  }
  if (read(fd, bs.data_pattern, DATA_PATTERN_LEN) != DATA_PATTERN_LEN) {
    err(1, "/dev/urand: read()");
  }
  close(fd);
}

/**
 * Resets the data pattern in the given memory.
 *
 * @param memory Pointer to the memory to reset.
 */
static void reset_datapattern(uint8_t *memory) {
  if (*(uint64_t *)&bs.data_pattern[0] == 0) {
    fprintf(stderr, "warn: data_pattern seems to be uninitialized\n");
  }
  for (int i = 0; i < _int(bs.mem_sz / DATA_PATTERN_LEN); ++i) {
    memcpy(&memory[i * DATA_PATTERN_LEN], bs.data_pattern, DATA_PATTERN_LEN);
  }
}

#define PATTERN_THRESHOLD 400
int find_REFI(char *mem, dram_info_t *dram, long block_mask) {
  unsigned long row_mask = dram->row_mask & block_mask;
  unsigned long f = 0;
  // find a f to modify row idx while staying in same bank 
  for (int i=0; i<dram->nbank_sel_fn; ++i) {
    f = dram->bank_sel_fn[i];
    if (f & row_mask) 
      break;
  }
  if (!f) {
    printf("cannot find a proper f\n");
    exit(1);
  }
  char *a1 = mem,
       *a2 = (char *) (f | (unsigned long) mem);
  if (!is_bank_conflict(PATTERN_THRESHOLD, (char*)a1, (char*)a2)) {
    printf("%p & %p not conflicting\n", a1, a2);
    exit(1);
  }
  // printf("find refi:\n");
  unsigned long t, t1;
  unsigned lasti = 0, refi = 0;
  mfence();
  t = rdtscp();
  for (unsigned i=0; i<300; ++i) {
    mfence();
    ACCESS_ONCE(*a1);
    ACCESS_ONCE(*a2);
    t1 = rdtscp();
    clflushopt(a1);
    clflushopt(a2);
    
    if (t1 - t > 1000) {
      // printf("%3d, %4lu\n", 2*i, t);
      lasti = refi;
      refi = i;
    }
    t = t1;
  }
  // refi = (refi / r) * 2;
  refi = 2 * (refi - lasti);
  printf("\nREFI: %u\n", refi);
  return refi;
}

/**
 * Initializes Blacksmith with hammerable memory `mem` of `mem_sz` bytes of
 * memory, made up of blocks of size `mem_block_sz` bytes.
 */
blacksmith_t *init_blacksmith(uint8_t *mem, long mem_sz, long mem_block_sz,
                              dram_info_t *dram) {
  blacksmith_t *instance = &bs;
  instance->mem = mem;
  instance->mem_sz = mem_sz;
  instance->mem_block_sz = mem_block_sz;
  instance->dram = dram;
  init_datapattern();
  /* A4 TASK 8: detect act_tREFI */
  instance->act_tREFI = find_REFI((char*) mem, dram, mem_sz-1);
  reset_datapattern(mem);
  return instance;
}

/**
 * Compares a memory region with a data pattern.
 *
 * @param mem The memory region to compare.
 * @return 1 if the memory region mismatches the data pattern, 0 otherwise.
 */
static int memcmp_datapattern(char *mem) {
  for (int i = 0; i < _int(bs.mem_sz / DATA_PATTERN_LEN); ++i) {
    if (memcmp((void *)(&mem[i * DATA_PATTERN_LEN]), bs.data_pattern,
               DATA_PATTERN_LEN)) {
      return 1;
    }
  }
  return 0;
}

/** Looks for bitflips and populates bs->bitflips with found flips */
static unsigned find_bitflips(unsigned char *memory, bitflip_t *bf_buf,
                              int buf_sz) {
  // The number of bitflip pointers.
  int nbf_ptrs = 0;

  // If there are bit flips, iterate over the memory, count and store all bit
  // flips in bit_flip_ptrs_out/bit_flip_masks_out.
  for (unsigned i = 0; i < bs.mem_sz; i += 8) {
    uint64_t ptrn_value = *(uint64_t *)&bs.data_pattern[i % DATA_PATTERN_LEN];
    uint64_t mask = *(uint64_t *)&memory[i] ^ ptrn_value;
    if (!mask) // no bitflips found
      continue;

    bitflip_t *bf = &bf_buf[nbf_ptrs++];
    bf->ptr = _ul(&memory[i]);
    bf->mask = mask;

    if (nbf_ptrs >= (int)buf_sz) {
      fprintf(stderr,
              "warn: Too many bitflips, I cannot keep track of more than %d.\n",
              buf_sz);
      break;
    }
  }
  return nbf_ptrs;
}

/**
 * Generate, hammer, detect bitflips, and return a new pattern. Reset
 * datapattern if needed
 */
bs_pattern_t *blacksmith_next() {
  // Generate a new pattern.
  new_bs_pattern(&bs, &cur_pattern);
  // Hammer the pattern.
  hammer(&cur_pattern);
  // Quick check to see if any bitflips occurred.
  if (!memcmp_datapattern((char *) bs.mem))
    return 0;
  // Extensive check to find location of bit flips.
  int nbf_ptrs =
      find_bitflips(bs.mem, cur_pattern.bitflips, ARR_SZ(cur_pattern.bitflips));
  if (!nbf_ptrs)
    return 0;
  
  // Update the number of bitflips that this pattern triggered.
  cur_pattern.nr_bitflip_ptrs = nbf_ptrs;
  for (int i = 0; i < nbf_ptrs; i++) {
    uint64_t mask = cur_pattern.bitflips[i].mask;
    while (mask > 0) {
      cur_pattern.nr_bitflips += (mask & 1);
      mask >>= 1;
    }
  }

  // Resets the memory to its original value according to the data pattern.
  reset_datapattern(bs.mem);

  return &cur_pattern;
}

/**
 * A5 TASK 2. Check if a bitflip is exploitable.
 */
/* int bitflip_exploitable(bitflip_t *bf) { } */
