#pragma once
#include "compiler.h"
#include "drama.h"
#include "err.h"
#include <sys/mman.h>
#include <unistd.h>

/* Normal physical memory addresses usually start at 2^32. Lower physical addresses are
 * primarily for devices that can only handle 32 bit physical addresses (see
 * DMA32_RESERVE). */
#define PHYS_START 0x100000000UL

/**
 * This is the largest block that buddy produces, i.e., a block of order 10.
 * This number may be useful for A5 TASK 1.
 */
#define BLOCK_SIZE_MAX MB(4)

/*
 * 2-3 GB may be primarily used for DMA32 memory, which resides at pa < PHYS_START The
 * page allocator will only use this range if the normal memory range >= PHYS_START is
 * already occupied. This may assist you with A5 TASK 1 and A5 TASK 7. You may have a look
 * at /sys/devices/system/memory/memoryX/valid_zones to get an idea.
 */
#define DMA32_RESERVE GB(3)

/* A5 TASK 1 */
void *allocate_contiguous_blocks(void *addr, long size);
