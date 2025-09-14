
#pragma once

// start measure.
static inline __attribute__((always_inline)) uint64_t rdtsc(void) {
    uint64_t lo, hi;
    __asm__ __volatile__ ("CPUID\n\t"
            "RDTSC\n\t" : "=d" (hi), "=a" (lo)::
            "%rbx", "%rcx");
    return (hi << 32) | lo;
}

// stop meassure.
static inline __attribute__((always_inline)) uint64_t rdtscp(void) {
    uint64_t lo, hi;
    __asm__ __volatile__("RDTSCP\n\t"
            "movq %%rdx, %0\n\t"
            "movq %%rax, %1\n\t"
            "CPUID\n\t": "=r" (hi), "=r" (lo):: "%rax",
            "%rbx", "%rcx", "%rdx");
    return (hi << 32) | lo;
}

static inline __attribute__((always_inline)) void reload_range(void* base, long stride, int n, uint64_t *results) {
    __asm__ volatile("mfence\n"); // all memory operations done.
    for (int k = 0; k < n; ++k) {
        size_t c = (k*17+64)&(n-1); // c=1,0,3,2
        // if (n <= 16) {
        //     u64 c = (k*7+15)&(n-1); // c=1,0,3,2 works for 16 entries Intel only
        // }
        unsigned volatile char *p = (uint8_t *)base + (stride * c);
        uint64_t t0 = rdtsc(); // Huh? why not rdtscp?
        *(volatile unsigned char *)p;
        uint64_t dt = rdtscp() - t0;
        //if (dt < 200) results[c]++;
        results[c] = dt;
    }
}

static inline __attribute__((always_inline)) void flush_range(void* start, long stride, int n) {
    __asm__("mfence");
    for (int k = 0; k < n; ++k) {
        volatile void *p = (uint8_t *)start + k * stride;
        __asm__ volatile("clflushopt (%0)\n"::"r"(p));
        __asm__ volatile("clflushopt (%0)\n"::"r"(p));
    }
    __asm__("lfence");
}
