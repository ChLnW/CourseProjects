#pragma once
#include "dram_info.h"
#include <stdint.h>

/* Define an upper limit for the pattern length. */
#define PATTERN_LIMIT 4096

/* We assume you will not get >256 bitflips per pattern, but we could be wrong.. */
#define BITFLIP_LIMIT 256

/**
 * Describes a bitflip in memory.
 */
struct bitflip {
    /* Pointer to 8-byte aligned 8 bytes, where there was one or more bit flips. */
    uint64_t ptr;
    /* Bit mask of where there were bitflips in *ptr. */
    uint64_t mask;
};

typedef struct bitflip bitflip_t;

/**
 * Describes a Blacksmith pattern.
 */
struct bs_pattern {
    /* The base period of the pattern. */
    int base_period;
    /* The targeted bank. */
    int bank;
    /* The pattern's length. */
    int pattern_len;
    /* The actual access pattern. */
    void *pattern[PATTERN_LIMIT];
    /* The number of bitflip pointers: one bitflip_t ptr references 64 bit, with 1 or more bit flips. */
    int nr_bitflip_ptrs;
    /* The total number of bitflips. */
    int nr_bitflips;
    /* The bitflips. */
    bitflip_t bitflips[BITFLIP_LIMIT];
};

typedef struct bs_pattern bs_pattern_t;

// Forward declaration.
typedef struct blacksmith blacksmith_t;

int new_bs_pattern(blacksmith_t *bs, bs_pattern_t *bs_ptrn);

// long pick_aggressor(blacksmith_t *bs, int bank_idx, int row_idx);
