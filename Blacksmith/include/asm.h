#define clflushopt(addr) asm volatile("clflushopt (%0)" ::"r"(addr) : "memory")

#define mfence() asm volatile("mfence" ::: "memory")
#define lfence() asm volatile("lfence" ::: "memory")
#define sfence() asm volatile("sfence" ::: "memory")

static inline __attribute__((always_inline)) unsigned long rdtscp() {
    unsigned long lo, hi;
    asm volatile("rdtscp\n" : "=a"(lo), "=d"(hi)::"rcx");
    return (hi << 32UL) | lo;
}
