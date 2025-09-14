#include "bs_pattern.h"
#include "blacksmith.h"
#include <assert.h>
#include <cstring>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "drama.h"

#define PATTERN_THRESHOLD 380


/* A4 TASK 6 */
/**
 * Pick a victim row from a random memory block.
 * Make sure there are rows around it so we can hammer it double-sided.
 * Keep away from the block boundaries to avoid bitflips in sensitive memory
 * through "1-sided" hammering.
 */
/* static int pick_victim_row(..) {} */

/* A4 TASK 6 */
/** Get an aggressor at a given row within a given bank.  */
// row_idx is presumed to have shifted bits to the right place
long pick_aggressor(blacksmith_t *bs, int bank_idx, int row_idx) {
  // (void)bs;
  // (void)bank_idx;
  // (void)row_idx;
  dram_info_t *dram = bs->dram;
  const unsigned long row_mask = dram->row_mask;
  // unsigned long res = dram->row_mask;
  // if (!row_mask)
  //   return 0;
  // for (unsigned long m = 1<<12; ; m <<= 1) {
  //   if (res & m) {
  //     res = m;
  //     break;
  //   }
  // }
  // res *= row_idx;
  unsigned long res = row_idx;
  for (int i=0; i<dram->nbank_sel_fn; ++i) {
    // only 1 (higher) bit of f overlaps with row idx bits.
    unsigned long f = dram->bank_sel_fn[i];
    
    if (!(f & res) != !(bank_idx & (1<<i))) { // if upper bit not matching idx bit
      unsigned long x = f & row_mask;
      if (x) { // if f share bit with row bits, x sets the shared (upper) bit
        res |= x ^ f;
      } else { // no shared bit -> f & res == 0 -> bank_idx & (1<<i) (i.e. bank bit) == 1
        x = 1;
        while (!(f & 1)) {
          x <<= 1;
          f >>= 1;
        }
        res |= x;
      }
    }
  }
  return res | (unsigned long) bs->mem;
}

/* A4 TASK 6 */
/** Generate a new blacksmith pattern. */
// #define PATTERN_MAX_SCALAR 8
int new_bs_pattern(blacksmith_t *bs, bs_pattern_t *bs_ptrn) {
  const int pattern_max_scalar = 8;
  // (void)bs;
  // zap previous pattern.
  memset(bs_ptrn, 0, sizeof(bs_pattern_t));
  
  // assumes row bits not overlaping with least significant 12 bits
  int row_mask_offset = 12;
  unsigned long row_mask = (bs->dram->row_mask & (bs->mem_sz - 1)) >> row_mask_offset;
  while (!(row_mask & 1)) {
    ++row_mask_offset;
    row_mask >>= 1;
  }

  assert(!((row_mask << row_mask_offset) & ((unsigned long) bs->mem)));
  
  int n = bs->act_tREFI + 10, // some redundency for REFI sync
      m = (rand() % pattern_max_scalar) + 1;
  // non-inclusive max
  int max_inv_freq = 1;
  int min_step = m;
  while (!(min_step & 1)) {
    max_inv_freq <<= 1;
    min_step >>= 1;
  }
  bs_ptrn->base_period = n;
  bs_ptrn->pattern_len = n * m;
  bs_ptrn->bank = rand() % (1 << bs->dram->nbank_sel_fn);
  char **t = (char **) bs_ptrn->pattern;
  
  for (int col=0, amp; (n-col) > 1; col+=amp) {
    // gen amplitude
    amp = (rand() % ((n-col)/2)) + 1;
    amp *= 2;
    // initialize frequency
    // OPTION 2: less high frequency
    // int inv_freq = 1;
    for (int s=0; s<min_step; ++s) {
      // initialize frequency
      // OPTION 1: more high frequency
      int inv_freq = 1;
      for (int row=s; row<m; row+=min_step) {
        if (t[row*n+col])
          continue;
        // gen victim row idx
        int victim;
        do {
          victim = rand() & row_mask;
        // edge rows cannot be 2-sided hammered
        // TODO: detect interference of victims
        } while (victim == 0 || (unsigned long) victim == row_mask); 
        // unsigned long a1 = victim - 1,
        //               a2 = victim + 1;
        unsigned long a1 = pick_aggressor(bs, bs_ptrn->bank, (victim - 1)<<row_mask_offset),
                      a2 = pick_aggressor(bs, bs_ptrn->bank, (victim + 1)<<row_mask_offset);
        // if (!is_bank_conflict(PATTERN_THRESHOLD, (char*)a1, (char*)a2)) {
        //   printf("%lx & %lx not conflicting\n", a1, a2);
        // }
        // frequency only decreases 
        for (int r = rand(); (inv_freq < max_inv_freq) && (r & 1); r >>= 1) {
          inv_freq <<= 1;
        }
        for (int i = row; i<m; i+= inv_freq*min_step) {
          for (int j=0; j<amp; j+=2) {
            t[i*n + col + j    ] = (char *) a1;
            t[i*n + col + j + 1] = (char *) a2;
          }
        }
      }
    }
  }
  // fill left-over col
  if (n&1) {
    for (int i=n, len=bs_ptrn->pattern_len; i<=len; i+=n) {
      char *ti = t[rand() % len];
      if (ti==t[i-1] || ti==t[i%len] || ti==t[(i-2+2*len)%len]) {
        ti = (char *) pick_aggressor(bs, bs_ptrn->bank, (rand() & row_mask)<<row_mask_offset);
      }
      t[i - 1] = ti;
    }
  }

  // for (int i=0; i<bs_ptrn->pattern_len; ++i) {
  //   if (((unsigned long) bs_ptrn->pattern[i]) - (unsigned long) (bs->mem) >= 1<<30) {
  //     printf("OUT OF RANGE!!!!!!!!!!!!\n");
  //   }
  // }
  return 0;
}
