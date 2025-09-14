/* NOTE: This is just boilerplate code. Customize for your own needs. */
#include "evict.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  if (init_evict_buf() != 0) {
    /* exit(1); */
  }

  uint8_t *p;
  if (posix_memalign((void **)&p, PAGE_SIZE, PAGE_SIZE)) {
    printf("allocation failed\n");
    exit(1);
  }

  const int round = 10;
  uint8_t *adr, *adr2;
  for (int i = 0; i < 64; ++i) {
    // evict_cache(i);
    unsigned long time1 = 0;
    unsigned long sum1 = 0, max1 = 0;

    unsigned long time2 = 0;
    unsigned long sum2 = 0, max2 = 0;

    adr = &p[i * 64];
    *adr = i + 1;
    adr2 = &p[((i + 13) % 64) * 64];
    *adr2 = i + 1;

    for (int k = 0; k < round; ++k) {
      reload_t(adr);         // bring to l1 cache and tlb
      time1 = reload_t(adr); // l1 cache hit & tlb hit
      evict_cache(i);
      // access some other cache line to bring page translation back
      reload_t(adr2);
      time2 = reload_t(adr); // l1 cache (should) hit but tlb miss

      sum1 += time1;
      if (time1 > max1)
        max1 = time1;

      sum2 += time2;
      if (time2 > max2)
        max2 = time2;
    }
    /* Please keep: our grading script will look for lines that match this.  */
    printf("pgoff=0x%03x hit=%u miss=%u\n", i * 64,
           (unsigned int)(sum1 - max1) / (round - 1),
           (unsigned int)(sum2 - max2) / (round - 1));
  }
  return 0;
}
