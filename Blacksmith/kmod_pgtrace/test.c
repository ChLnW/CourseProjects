#include "./pgtrace_ioc.h"
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>

// we borrow from  linux/mmzone.h
enum migratetype {
    MIGRATE_UNMOVABLE,
    MIGRATE_MOVABLE,
    MIGRATE_RECLAIMABLE,
    MIGRATE_PCPTYPES, /* the number of types on the pcp lists */
    MIGRATE_HIGHATOMIC = MIGRATE_PCPTYPES,
    MIGRATE_ISOLATE, /* can't allocate from here */
    MIGRATE_TYPES
};

static int fd_pgtrace;
static pgtrace_msg_t msg;

static char *mt_name(int mt) {
    if (mt >= MIGRATE_TYPES) {
        return NULL;
    }

    switch (mt) {
    case MIGRATE_UNMOVABLE:
        return "UNMOVABLE";
    case MIGRATE_MOVABLE:
        return "MOVABLE";
    case MIGRATE_RECLAIMABLE:
        return "RECLAIMABLE";
    case MIGRATE_HIGHATOMIC:
        return "HIGHATOMIC";
    case MIGRATE_ISOLATE:
        return "ISOLATE";
    default:
        return NULL;
    };
}

/** New instance, unsets currently traced page */
static inline void pgtrace_init() {
    fd_pgtrace = open("/proc/" PROC_PGTRACE, O_RDONLY);
    if (fd_pgtrace <= 0) {
        err(fd_pgtrace, "/proc/" PROC_PGTRACE " missing");
    }
}

/** Attach the page to be traced. Trace continues after page is reclaimed. */
static inline void pgtrace_attach(void *va) {
    msg.va = va;
    if (ioctl(fd_pgtrace, PGTRACE_IOC_ATTACH, &msg)) {
        err(1, "ioctl");
    }
}

/** Get the current Migratetype associated with the current page */
static inline int pgtrace_mt() {
    // Don't care about msg.
    int res;
    if ((res = ioctl(fd_pgtrace, PGTRACE_IOC_MT, &msg)) < 0) {
        err(1, "ioctl");
    }

    return res;
}

/** Check if 8 bytes at pgoff in assigned page are equal to val*/
static inline int pgtrace_equals(long val, long pgoff) {
    msg.val = val;
    msg.pgoff = pgoff;
    int res;
    if ((res = ioctl(fd_pgtrace, PGTRACE_IOC_EQ, &msg)) < 0) {
        err(1, "ioctl");
    }
    return res;
}

__attribute__((aligned(0x1000))) static unsigned char data[0x100000];

int main(void) {
    pgtrace_init();

    void *ptr = &data[0x50000];
    data[0x50000] = 0x12;
    printf(" ptr %p\n", ptr);
    printf("*ptr %lx\n", *(long *) ptr);
    pgtrace_attach(ptr);
    int mt = pgtrace_mt();
    printf("mt: %s\n", mt_name(mt));
    printf("eq: %d\n", pgtrace_equals(0x12, 0));
    return 0;
}
