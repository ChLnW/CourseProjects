#include "drama.h"
#include "asm.h"
#include "compiler.h"
#include "dram_info.h"
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <unordered_set>

#define MAX_STEPS (1 << 17)

unsigned long time_addresses(char *a_star, char *a) {
  unsigned long t0 = rdtscp();
  lfence();
  /* We check if the average access time of BANK_CONFLICT_ROUNDS rounds exceeds
   * threshold.  */
  for (int i = 0; i < BANK_CONFLICT_ROUNDS; i++) {
    clflushopt(a_star);
    clflushopt(a);
    mfence();
    ACCESS_ONCE(*a_star);
    ACCESS_ONCE(*a);
  }
  return ((rdtscp() - t0) / BANK_CONFLICT_ROUNDS);
}

char *pick_random_address(void *buffer) {
  // row bits are upper bits (>13),
  size_t offset = rand() % (1 << 30);

  return (((char *)buffer) + offset);
}

// find an address conflicting with a0
inline char *find_conflict(void *mem, unsigned long threshold, char *a0) {
  while (1) {
    char *a = pick_random_address(mem);
    if (is_bank_conflict(threshold, a0, a)) {
      return a;
    }
  }
}

std::vector<char *> build_new_conflict_set(void *mem, unsigned long threshold,
                                           char *a0) {
  char *a1 = find_conflict(mem, threshold, a0);
  std::vector<char *> res = {a0, a1};
  return res;
}

std::vector<std::vector<char *>> build_conflict_sets(void *mem,
                                                     unsigned long threshold,
                                                     size_t max_sets,
                                                     size_t max_per_set) {
  std::vector<std::vector<char *>> conflict_sets;
  // std::unordered_set<char *> chosen_addresses;
  // std::ignore = max_sets;
  // Init base address and first set
  // char *a_star_0 = pick_random_address(mem);
  // chosen_addresses.insert(a_star_0);

  // find the first conflicting set
  // conflict_sets.push_back(build_new_conflict_set(mem, threshold, a_star_0));

  for (int i = 0; i < MAX_STEPS; i++) {
    bool newSet = true;
    char *candidate = pick_random_address(mem);

    // Check if the candidate conflicts with any existing conflict set
    for (auto &set : conflict_sets) {
      unsigned int nConflict = 0;

      for (size_t j = 0; j < set.size(); ++j) {
        if (is_bank_conflict(threshold, set[j], candidate)) {
          ++nConflict;
          if (nConflict <= j || set.size() >= max_per_set) {
            break;
          }
        } 
        else if (j > 1) {
          break;
        }
      }

      if (nConflict == set.size()) {
        // if (set.size() < max_per_set)
        set.push_back(candidate);
      }

      if (nConflict > 0) {
        newSet = false;
        break;
      }
    }

    // if no match was found create a new conflict set
    if (newSet && conflict_sets.size() < max_sets) {
      conflict_sets.push_back(build_new_conflict_set(mem, threshold, candidate));
      ++i;
    }
  }

  // for (auto &set: conflict_sets) {
  //   printf("%lu,", set.size());
  // }
  // printf("\ntotal: %ld\n", conflict_sets.size());

  if ((conflict_sets.size()&1) && (conflict_sets.size()>1) && (*(conflict_sets.end()-1)).size()==2)
    conflict_sets.pop_back();

  return conflict_sets;
}
