/* NOTE: This is just boilerplate code. Customize for your own needs. */
#include "evict.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
  if (init_evict_buf() != 0) {
    /* exit(1); */
  }
  // evict_tlb();

  uint64_t *p;
  if (posix_memalign((void **)&p, PAGE_SIZE, PAGE_SIZE)) {
    printf("allocation failed\n");
    exit(1);
  }

  unsigned long time1 = 0;
  unsigned long sum1 = 0, max1 = 0;

  unsigned long time2 = 0;
  unsigned long sum2 = 0, max2 = 0;

  const int round = 10;
  uint64_t *adr;
  for (int k = 0; k < round; ++k) {
    adr = &p[(11 + 19 * k) % 128];
    reload_t(adr);         // bring to l1 cache and tlb
    time1 = reload_t(adr); // l1 cache hit & tlb hit
    evict_tlb();
    time2 = reload_t(adr); // l1 cache (should) hit but tlb miss

    sum1 += time1;
    if (time1 > max1)
      max1 = time1;

    sum2 += time2;
    if (time2 > max2)
      max2 = time2;

    // printf("%ld -> %ld\n", time1, time2);
  }

  /* Please keep: our grading script will look for a line that matches this. */
  printf("cached=%u tlbmiss=%u\n", (unsigned int)(sum1 - max1) / (round - 1),
         (unsigned int)(sum2 - max2) / (round - 1));
}
