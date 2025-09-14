#include "compiler.h"

int init_evict_ptr();
void *get_evict_ptr();
void evict_tlb_4k();

/**
 * For flushing the translation lookaside buffer (TLB).
 */
static inline void evict_tlb() {
    evict_tlb_4k();
}
