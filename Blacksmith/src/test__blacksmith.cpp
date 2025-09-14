#include "asm.h"
#include "blacksmith.h"
#include "compiler.h"
#include "dram_info.h"
#include <err.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>
static blacksmith_t *bs;

static bs_pattern_t best_ptrn;

static void *mem = _ptr(0x100000000000); /* hammer area */
static char hostname[64];

/* args */
static int run_dry = 0;
static int run_evaluation = 0;
static int run_sweep = 0;

#define ms_to_minutes(val) (val * 1000 * 60)
static inline uint64_t get_ms() {
  static struct timeval tp;
  gettimeofday(&tp, 0);
  return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

static void print_pattern(bs_pattern_t *bs_ptrn) {
  if (run_dry) {
    printf("%u, %u, ", bs_ptrn->bank, bs_ptrn->base_period);
  } else {
    printf("%u, %u, %u, %u, ", bs_ptrn->nr_bitflips, bs_ptrn->bank,
           bs->act_tREFI, bs_ptrn->base_period);
  }
  printf("\npattern length: %d\n", bs_ptrn->pattern_len);
  // for (int i = 0; i < bs_ptrn->pattern_len; ++i) {
  //   printf("%06lx ", _ul(bs_ptrn->pattern[i]) - _ul(mem));
  // }
  for (int i = 0; i < bs_ptrn->pattern_len; i+=bs_ptrn->base_period) {
      for (int j=0; j<bs_ptrn->base_period; ++j) {
          printf("%08lx ", _ul(bs_ptrn->pattern[i+j]) - _ul(mem));
      }
      printf("\n");
  }
  // printf("\n");
  // for (int i = 0; i < bs_ptrn->pattern_len; i+=bs_ptrn->base_period) {
  //     for (int j=0; j<bs_ptrn->base_period; ++j) {
  //         printf("%08lx ", _ul(bs_ptrn->pattern[i+j]) - _ul(mem));
  //     }
  //     printf("\n");
  // }
  printf("\n");
}

static void do_evaluation() {
  memset(&best_ptrn, 0, sizeof(bs_pattern_t));
  uint64_t t0 = get_ms();
  uint64_t bf_total = 0;
  uint64_t neffective = 0;
  int pi = 0; // total patterns
  // 15 minutes i think?
  for (; get_ms() - t0 < 5*60000; ++pi) {
    bs_pattern_t *bs_ptrn = blacksmith_next();
    if (!bs_ptrn)
      continue;
    print_pattern(bs_ptrn);

    // if (bs_ptrn->nr_bitflips > 0) {
    neffective += 1;
    // }
    bf_total += bs_ptrn->nr_bitflips;
    if (bs_ptrn->nr_bitflips > best_ptrn.nr_bitflips) {
      memcpy(&best_ptrn, bs_ptrn, sizeof(bs_pattern_t));
    }
  }

  printf("---EVALUATION SUMMARY\n");
  printf("hostname, #patterns, #effective, #bitflips\n");
  printf("%s, %d, %lu, %lu\n", hostname, pi, neffective, bf_total);

  if (run_sweep) {
    uint64_t best_nbitflips = 0;
    uint64_t locations = 0;
    /* A4 TASK 8. Sweep the best pattern */
    // TODO TODO TODO TODO TODO TODO TODO
    printf("---SWEEP SUMMARY\n");
    print_pattern(&best_ptrn);
    printf("hostname, #displacements, #bitflips\n");
    printf("%s, %lu, %lu\n", hostname, locations, best_nbitflips);
  }
}

int main(int argc, char *argv[]) {
  gethostname(hostname, 64);
  srand(getpid());

  int opt = -1;
  while ((opt = getopt(argc, argv, "desh")) != -1) {
    switch (opt) {
    case 'd': {
      /* A4 TASK 6. Generate patterns */
      run_dry = 1;
    } break;
    case 'e': {
      /* A4 TASK 7. Test patterns for 15 min */
      run_evaluation = 1;
    } break;
    case 's': {
      /* A4 TASK 8. Test patterns for 15 min and sweep best pattern */
      run_sweep = 1;
      run_evaluation = 1;
    } break;
    case 'h':
    default:
      fprintf(stderr, "Usage: %s [options]\n", argv[0]);
      fprintf(
          stderr,
          "Generates and tests 100 blacksmith-like patterns for bitflips.\n"
          "Options:\n"
          "  -d dry run: Print generated patterns without testing\n"
          "  -e run a 15 minute evaluation instead of only for 100 patterns\n"
          "  -s sweep best found pattern over &32MB of memory. Implies -e\n"
          "  -h this message\n");
      exit(0);
    }
  };
  dram_info_t dram = DEFAULT_DRAM_INFO;

  if (dram.nbank_sel_fn == 0) {
    // uninitialzed. run DRAMA and configure dram_info.h!
    fprintf(stderr, "dram_info uninitialized!\n");
    return 1;
  }
  mem = mmap(mem, GB(1), PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANON | MAP_HUGETLB | MAP_FIXED_NOREPLACE, -1, 0);
  if (mem == MAP_FAILED) {
    err(1, "mmap(mem)");
  }

  bs = init_blacksmith((unsigned char *)mem, GB(1), GB(1), &dram);

  if (run_dry) {
    printf("bank_idx, base_period, pattern\n");
  } else {
    printf("#bitflips, bank_idx, act_tREFi, base_period, pattern\n");
  }

  if (run_dry) {
    /* A4 TASK 6. Just generate and print patterns */
    // bs_pattern_t bs_ptrn;
    // for (int i = 0; i < 100; ++i) {
    //   new_bs_pattern(bs, &bs_ptrn);
    //   print_pattern(&bs_ptrn);
    // }
    return 0;
  }

  if (run_evaluation) {
    do_evaluation();
    return 0;
  }

  return 0;
}
