#define PROC_PGTRACE "pgtrace"

enum PG_TRACE_IOC {
    PGTRACE_IOC_HELLO = 0x10000,
    /* Assign phys page to trace. */
    PGTRACE_IOC_ATTACH,
    /* Get migratetype of assigned page. */
    PGTRACE_IOC_MT,
    /* Check if 8 bytes in the page are equal to a given value. */
    PGTRACE_IOC_EQ
};

struct pgtrace_ioc_msg {
    union {
        /* attach */
        struct {
            void *va;
        };
        /* eq */
        struct {
            unsigned long val;
            short pgoff;
        };
    };
};

typedef struct pgtrace_ioc_msg pgtrace_msg_t;

#ifndef __KERNEL__

#endif
