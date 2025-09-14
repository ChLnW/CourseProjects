#pragma once
#include "compiler.h"
#include "dram_info.h"
#include <vector>

/** The number of measurement rounds to detect a bank conflict. */
#define BANK_CONFLICT_ROUNDS 10

// /**
//  * Stores a set of (bank-)conflicting addresses.
//  */
// struct conflict_set {
//   /** The conflicting addresses. */
//   void *addresses[1000];
//   /** The length of the addresses array. */
//   int len;
// };
// typedef struct conflict_set conflict_set_t;

/* A4 TASK 1 */
unsigned long time_addresses(char *a_star, char *a);

/* A4 TASK 2 */
/** Returns true if a1 and a2 are different rows of the same bank. */
// idk how to check they are different rows, atm if by chance these two are on
// the same row they won't cause a conflict
static inline int is_bank_conflict(unsigned long threshold, char *a1,
                                   char *a2) {
  const int round = 5;
  unsigned long sum = 0,
                max = 0;
  for (int i=0; i<round; ++i) {
    unsigned long t = time_addresses(a1, a2);
    sum += t;
    if (t > max)
      max = t;
  }
  sum = (sum - max) / (round - 1);
  return sum > threshold;
}

/* A4 TASK 2 */
std::vector<std::vector<char *>> build_conflict_sets(void *mem,
                                                     unsigned long threshold,
                                                     size_t max_sets,
                                                     size_t max_per_set);

char *pick_random_address(void *buffer);
